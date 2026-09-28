#include "app/App.hpp"

#include <imgui.h>
#include <raymath.h>
#include <rlImGui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <format>

namespace tf {

namespace {

constexpr int kMapSize = 512;
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;

constexpr float kMargin = 10.0f;
constexpr float kPanelWidth = 340.0f;  // at 100% display scaling
constexpr float kMinZoom = 0.25f;
constexpr float kMaxZoom = 32.0f;
constexpr float kMinBrushRadius = 1.0f;
constexpr float kMaxBrushRadius = 128.0f;

constexpr Color kBackgroundColor{28, 32, 38, 255};

constexpr std::array kTools{BrushTool::Raise, BrushTool::Lower, BrushTool::Smooth,
                            BrushTool::Flatten};

Color toolColor(BrushTool tool) {
    switch (tool) {
        case BrushTool::Raise:
            return Color{120, 230, 120, 230};
        case BrushTool::Lower:
            return Color{240, 120, 110, 230};
        case BrushTool::Smooth:
            return Color{120, 190, 250, 230};
        case BrushTool::Flatten:
            return Color{245, 210, 110, 230};
    }
    return WHITE;
}

bool isShiftDown() {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

bool isControlDown() {
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}

// Display scaling set in Windows (1.0 = 100%, 1.5 = 150%). rlImGui scales its font the same way.
float uiScale() {
    return std::max(1.0f, GetWindowScaleDPI().x);
}

float panelWidth() {
    return kPanelWidth * uiScale();
}

}  // namespace

App::App()
    : m_window(kWindowWidth, kWindowHeight, "Terraforge"),
      m_map(kMapSize, kMapSize),
      m_renderer(kMapSize, kMapSize),
      m_panelRight(kMargin + panelWidth()) {
    regenerate();
    fitMapToScreen();
}

void App::run() {
    while (!WindowShouldClose()) {
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
        drawUi();
        rlImGuiEnd();

        EndDrawing();
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
            m_brush.tool = kTools[i];
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

    BrushSettings brush = m_brush;
    if (isShiftDown()) {  // Shift swaps Raise and Lower
        if (brush.tool == BrushTool::Raise) {
            brush.tool = BrushTool::Lower;
        } else if (brush.tool == BrushTool::Lower) {
            brush.tool = BrushTool::Raise;
        }
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
    generateTerrain(m_map, m_generator);
    m_needsRecolor = true;
    m_hasManualEdits = false;
}

void App::fitMapToScreen() {
    // Fit the whole map into the free area to the right of the UI panel.
    const float left = m_panelRight + kMargin;
    const float availableWidth =
        std::max(1.0f, static_cast<float>(GetScreenWidth()) - left - kMargin);
    const float availableHeight =
        std::max(1.0f, static_cast<float>(GetScreenHeight()) - 2.0f * kMargin);
    const float mapWidth = static_cast<float>(m_map.width());
    const float mapHeight = static_cast<float>(m_map.height());

    m_camera.zoom = std::clamp(std::min(availableWidth / mapWidth, availableHeight / mapHeight),
                               kMinZoom, kMaxZoom);
    m_camera.target = {mapWidth / 2.0f, mapHeight / 2.0f};  // look at the center of the map...
    m_camera.offset = {left + availableWidth / 2.0f,  // ...placed in the center of the free area
                       kMargin + availableHeight / 2.0f};
    m_camera.rotation = 0.0f;
}

void App::exportPng() {
    namespace fs = std::filesystem;
    for (int i = 1; i <= 999; ++i) {
        const fs::path path = std::format("terraforge_map_{:03}.png", i);
        if (fs::exists(path)) {
            continue;
        }
        m_statusMessage = m_renderer.exportPng(path.string())
                              ? std::format("Saved {} (in the working folder)", path.string())
                              : std::format("Could not save {}", path.string());
        return;
    }
    m_statusMessage = "Too many exported maps, clean up the folder";
}

// ---------------------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------------------

void App::drawBrushCursor() const {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    const Vector2 center = mouseCell();
    const float pixel = 1.0f / m_camera.zoom;  // one screen pixel in world units
    const Color color = toolColor(m_brush.tool);

    const float outer = m_brush.radius;
    const float inner = 0.5f * m_brush.radius;  // where the brush is at about half strength
    DrawRing(center, std::max(0.0f, outer - 1.5f * pixel), outer, 0.0f, 360.0f, 64, color);
    DrawRing(center, std::max(0.0f, inner - pixel), inner, 0.0f, 360.0f, 48, Fade(color, 0.35f));
    DrawCircleV(center, 2.0f * pixel, color);
}

void App::drawUi() {
    // Fixed width, height follows the content.
    ImGui::SetNextWindowPos({kMargin, kMargin}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({panelWidth(), 0.0f}, {panelWidth(), FLT_MAX});
    if (ImGui::Begin("Terraforge", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        drawGeneratorSection();
        drawBrushSection();
        drawViewSection();
        drawInfoSection();
        drawHelpSection();
    }
    m_panelRight = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
    ImGui::End();

    if (m_showImGuiDemo) {
        ImGui::ShowDemoWindow(&m_showImGuiDemo);
    }
}

void App::drawGeneratorSection() {
    if (!ImGui::CollapsingHeader("Terrain generation", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    // Regenerate when a slider is released, not on every frame of dragging it.
    bool settingsChanged = false;
    auto finished = [&settingsChanged] {
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            settingsChanged = true;
        }
    };

    if (ImGui::InputInt("Seed", &m_generator.seed)) {
        settingsChanged = true;
    }
    if (ImGui::Button("Random seed")) {
        m_generator.seed = GetRandomValue(1, 999'999);
        settingsChanged = true;
    }

    ImGui::SliderFloat("Land size", &m_generator.frequency, 0.5f, 10.0f, "%.2f");
    ImGui::SetItemTooltip("Noise frequency: higher = more and smaller islands");
    finished();
    ImGui::SliderInt("Detail", &m_generator.octaves, 1, 10);
    ImGui::SetItemTooltip("Number of noise octaves");
    finished();
    ImGui::SliderFloat("Roughness", &m_generator.gain, 0.2f, 0.8f, "%.2f");
    ImGui::SetItemTooltip("Noise gain: how strong the small details are");
    finished();
    ImGui::SliderFloat("Island", &m_generator.islandStrength, 0.0f, 1.0f, "%.2f");
    ImGui::SetItemTooltip("0 = land may touch the edges, 1 = ocean all around");
    finished();

    if (m_hasManualEdits) {
        ImGui::TextColored({1.0f, 0.8f, 0.4f, 1.0f}, "Map was edited by hand:");
        ImGui::TextColored({1.0f, 0.8f, 0.4f, 1.0f}, "auto-regeneration is paused.");
    }
    const bool generateClicked =
        ImGui::Button(m_hasManualEdits ? "Generate (discard edits)" : "Generate");

    if (generateClicked || (settingsChanged && !m_hasManualEdits)) {
        regenerate();
    }
}

void App::drawBrushSection() {
    if (!ImGui::CollapsingHeader("Brush", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    for (std::size_t i = 0; i < kTools.size(); ++i) {
        const std::string label = std::format("{} ({})", toString(kTools[i]), i + 1);
        if (ImGui::RadioButton(label.c_str(), m_brush.tool == kTools[i])) {
            m_brush.tool = kTools[i];
        }
        if (i % 2 == 0) {
            ImGui::SameLine(0.5f * panelWidth());  // two tools per row
        }
    }
    ImGui::SliderFloat("Radius", &m_brush.radius, kMinBrushRadius, kMaxBrushRadius, "%.0f cells");
    ImGui::SliderFloat("Strength", &m_brush.strength, 0.02f, 1.5f, "%.2f");
    if (m_brush.tool == BrushTool::Flatten) {
        ImGui::SliderFloat("Target height", &m_brush.targetHeight, 0.0f, 1.0f, "%.3f");
    }
}

void App::drawViewSection() {
    if (!ImGui::CollapsingHeader("Map view", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    if (ImGui::SliderFloat("Sea level", &m_colors.seaLevel, 0.0f, 1.0f, "%.3f")) {
        m_needsRecolor = true;
    }
    if (ImGui::Checkbox("Hillshade", &m_colors.hillshade)) {
        m_needsRecolor = true;
    }
    if (m_colors.hillshade) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##shade", &m_colors.hillshadeStrength, 0.0f, 1.0f, "%.2f")) {
            m_needsRecolor = true;
        }
    }
    if (ImGui::Checkbox("Coastline", &m_colors.coastline)) {
        m_needsRecolor = true;
    }
    if (ImGui::Button("Fit to window (F)")) {
        fitMapToScreen();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export PNG")) {
        exportPng();
    }
}

void App::drawInfoSection() {
    if (!ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    const Vector2 cell = mouseCell();
    const int x = static_cast<int>(std::floor(cell.x));
    const int y = static_cast<int>(std::floor(cell.y));
    if (m_map.contains(x, y)) {
        const float h = m_map.at(x, y);
        ImGui::Text("Cell %d, %d   height %.3f (%s)", x, y, static_cast<double>(h),
                    h < m_colors.seaLevel ? "water" : "land");
    } else {
        ImGui::TextDisabled("Cursor is outside the map");
    }
    ImGui::Text("Zoom %.2fx   %d FPS", static_cast<double>(m_camera.zoom), GetFPS());
    if (!m_statusMessage.empty()) {
        ImGui::TextWrapped("%s", m_statusMessage.c_str());
    }
}

void App::drawHelpSection() {
    if (!ImGui::CollapsingHeader("Controls")) {
        return;
    }
    ImGui::BulletText("LMB: paint with the brush");
    ImGui::BulletText("Shift + LMB: Raise <-> Lower");
    ImGui::BulletText("RMB / MMB drag: move the map");
    ImGui::BulletText("Wheel: zoom, Ctrl + wheel: brush size");
    ImGui::BulletText("1-4: tools, [ ]: brush size, F: fit");
    ImGui::Checkbox("Show ImGui demo (UI examples)", &m_showImGuiDemo);
}

}  // namespace tf
