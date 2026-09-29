#include "app/App.hpp"

#include "app/BrushTools.hpp"

#include <imgui.h>
#include <raymath.h>
#include <rlImGui.h>

#include "core/Erosion.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>

namespace tf {

namespace {

constexpr int kMapSize = 512;
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;

constexpr float kMargin = 10.0f;
constexpr float kMinZoom = 0.25f;
constexpr float kMaxZoom = 32.0f;
// Land / Sea turn sea into land (mask 0 -> 1) in about a third of a second at the brush center.
constexpr float kShapeBrushStrength = 3.0f;

constexpr Color kBackgroundColor{28, 32, 38, 255};
constexpr const char* kSettingsFile = "terraforge.ini";

bool isShiftDown() {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

bool isControlDown() {
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}

}  // namespace

App::App()
    : m_window(kWindowWidth, kWindowHeight, "Terraforge"),
      m_map(kMapSize, kMapSize),
      m_noise(kMapSize, kMapSize),
      m_shapeMask(kMapSize, kMapSize),
      m_renderer(kMapSize, kMapSize),
      m_settingsPath(std::filesystem::path(GetApplicationDirectory()) / kSettingsFile),
      m_settings(loadSettings(m_settingsPath)),
      m_text(m_settings.language),
      m_mapArea{0.0f, 0.0f, static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)} {
    if (!m_imgui.hasCyrillicFont() && m_text.language() == Language::Russian) {
        // Without the font Russian text would be all "???": stay usable in English.
        m_text.setLanguage(Language::English);
    }
    regenerate();
}

void App::run() {
    while (!WindowShouldClose() && !m_quitRequested) {
        handleInput(GetFrameTime());

        if (m_needsRecolor) {
            m_renderer.update(m_map, m_colors);
            m_needsRecolor = false;
        }

        BeginDrawing();
        ClearBackground(kBackgroundColor);

        BeginMode2D(m_camera);
        m_renderer.draw();
        drawBrushCursor();
        EndMode2D();

        rlImGuiBegin();
        drawUi();  // also updates m_mapArea
        rlImGuiEnd();

        EndDrawing();

        // The free area is known only after the UI has been drawn once.
        if (m_fitPending) {
            fitMapToScreen();
            m_fitPending = false;
        }
    }
}

// ---------------------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------------------

void App::handleInput(float dt) {
    // ImGui tells us whether the mouse/keyboard are busy with the UI (e.g. dragging a slider).
    const ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        handleCameraInput();
        paint(dt);
    }
    if (!io.WantCaptureKeyboard) {
        handleShortcuts();
    }
}

void App::handleCameraInput() {
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        if (isControlDown()) {
            // Ctrl + wheel: brush size.
            m_brush.radius = std::clamp(m_brush.radius * std::exp(0.1f * wheel), kMinBrushRadius,
                                        kMaxBrushRadius);
        } else {
            // Wheel: zoom towards the point under the cursor.
            const Vector2 mouse = GetMousePosition();
            m_camera.target = GetScreenToWorld2D(mouse, m_camera);
            m_camera.offset = mouse;
            m_camera.zoom = std::clamp(m_camera.zoom * std::exp(0.15f * wheel), kMinZoom, kMaxZoom);
        }
    }

    // Right or middle mouse button: drag the map.
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        m_camera.target -= GetMouseDelta() / m_camera.zoom;
    }
}

void App::handleShortcuts() {
    for (std::size_t i = 0; i < kTools.size(); ++i) {
        if (IsKeyPressed(KEY_ONE + static_cast<int>(i))) {
            m_tool = kTools[i];
        }
    }
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        m_brush.radius = std::max(kMinBrushRadius, m_brush.radius / 1.25f);
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        m_brush.radius = std::min(kMaxBrushRadius, m_brush.radius * 1.25f);
    }
    if (IsKeyPressed(KEY_F)) {
        fitMapToScreen();
    }
}

void App::paint(float dt) {
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        return;
    }
    const Vector2 cell = mouseCell();
    const Tool tool = isShiftDown() ? oppositeTool(m_tool) : m_tool;  // Shift: Raise <-> Lower

    BrushSettings brush = m_brush;
    brush.tool = brushToolFor(tool);

    if (isShapeTool(tool)) {
        // Land / Sea paint the shape mask, then the whole terrain is rebuilt from it.
        brush.strength = kShapeBrushStrength;
        if (applyBrush(m_shapeMask, brush, cell.x, cell.y, dt)) {
            if (m_hasManualEdits) {
                m_statusMessage = m_text(TextId::ManualEditsReplaced);
            }
            m_shapeIsPainted = true;
            rebuildTerrain();
        }
        return;
    }

    // Flatten levels the terrain to the height where the stroke started.
    if (brush.tool == BrushTool::Flatten && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const int x = static_cast<int>(std::floor(cell.x));
        const int y = static_cast<int>(std::floor(cell.y));
        if (m_map.contains(x, y)) {
            m_brush.targetHeight = m_map.at(x, y);
            brush.targetHeight = m_brush.targetHeight;
        }
    }

    if (applyBrush(m_map, brush, cell.x, cell.y, dt)) {
        m_needsRecolor = true;
        m_hasManualEdits = true;
    }
}

Vector2 App::mouseCell() const {
    // The map is drawn at (0, 0) with one cell per world unit, so world coordinates are cells.
    return GetScreenToWorld2D(GetMousePosition(), m_camera);
}

// ---------------------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------------------

void App::regenerate() {
    generateNoise(m_noise, m_generator);
    if (!m_shapeIsPainted) {
        // Preset outlines depend on the seed too; a hand-drawn shape is kept as it is.
        makeShapeMask(m_shapeMask, m_generator.shape, m_generator.seed);
    }
    rebuildTerrain();
}

void App::rebuildTerrain() {
    combineTerrain(m_map, m_noise, m_shapeMask, m_generator.shapeStrength);
    m_needsRecolor = true;
    m_hasManualEdits = false;
}

void App::applyShapePreset(ShapePreset preset) {
    m_generator.shape = preset;
    makeShapeMask(m_shapeMask, preset, m_generator.seed);
    m_shapeIsPainted = false;
    rebuildTerrain();
}

void App::runErosion() {
    ErosionSettings settings;
    settings.droplets = m_erosionDroplets;
    settings.seed = m_generator.seed + m_erosionRuns++;
    settings.seaLevel = m_colors.seaLevel;

    const auto start = std::chrono::steady_clock::now();
    erode(m_map, settings);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    m_needsRecolor = true;
    m_hasManualEdits = true;  // regenerating would throw the erosion away
    const long long milliseconds = elapsed.count();
    m_statusMessage = std::vformat(m_text(TextId::ErosionDone),
                                   std::make_format_args(m_erosionDroplets, milliseconds));
}

void App::fitMapToScreen() {
    // Fit the whole map into the screen area that the UI panels leave free.
    const float availableWidth = std::max(1.0f, m_mapArea.width - 2.0f * kMargin);
    const float availableHeight = std::max(1.0f, m_mapArea.height - 2.0f * kMargin);
    const float mapWidth = static_cast<float>(m_map.width());
    const float mapHeight = static_cast<float>(m_map.height());

    m_camera.zoom = std::clamp(std::min(availableWidth / mapWidth, availableHeight / mapHeight),
                               kMinZoom, kMaxZoom);
    m_camera.target = {mapWidth / 2.0f, mapHeight / 2.0f};    // look at the center of the map...
    m_camera.offset = {m_mapArea.x + m_mapArea.width / 2.0f,  // ...placed in the center of the area
                       m_mapArea.y + m_mapArea.height / 2.0f};
    m_camera.rotation = 0.0f;
}

void App::exportPng() {
    namespace fs = std::filesystem;
    for (int i = 1; i <= 999; ++i) {
        const fs::path path = std::format("terraforge_map_{:03}.png", i);
        if (fs::exists(path)) {
            continue;
        }
        const std::string name = path.string();
        // The message comes from the translation table, so the format string is known only at
        // run time: std::vformat instead of std::format (which checks it at compile time).
        const TextId message =
            m_renderer.exportPng(name) ? TextId::ExportSaved : TextId::SaveFailed;
        m_statusMessage = std::vformat(m_text(message), std::make_format_args(name));
        return;
    }
    m_statusMessage = m_text(TextId::ExportTooMany);
}

void App::setLanguage(Language language) {
    if (language == m_text.language()) {
        return;
    }
    m_text.setLanguage(language);
    m_settings.language = language;
    m_statusMessage.clear();  // it was written in the previous language
    if (!saveSettings(m_settings, m_settingsPath)) {
        const std::string name = m_settingsPath.string();
        m_statusMessage = std::vformat(m_text(TextId::SaveFailed), std::make_format_args(name));
    }
}

}  // namespace tf
