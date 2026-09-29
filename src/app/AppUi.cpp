// The editor UI: main menu, toolbar, properties panel, status bar and floating windows.
// These are App member functions like the ones in App.cpp, only kept in their own file.

#include "app/App.hpp"

#include "app/BrushTools.hpp"

#include <imgui.h>
#include <rlImGui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <format>
#include <initializer_list>
#include <string>

namespace tf {

namespace {

constexpr float kMargin = 8.0f;             // gap between the UI panels and the screen edges
constexpr float kPropertiesWidth = 280.0f;  // at 100% display scaling
constexpr float kGeneratorWidth = 320.0f;
constexpr float kToolIconScale = 1.5f;      // toolbar icons relative to the text size

constexpr ImGuiWindowFlags kFixedPanelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                              ImGuiWindowFlags_NoCollapse |
                                              ImGuiWindowFlags_NoSavedSettings |
                                              ImGuiWindowFlags_AlwaysAutoResize;

constexpr const char* kMissingFontWarning =
    "Font assets/fonts/NotoSans-Regular.ttf not found: Russian is unavailable";

// Display scaling set in Windows (1.0 = 100%, 1.5 = 150%). The UI font is scaled the same way.
float uiScale() {
    return std::max(1.0f, GetWindowScaleDPI().x);
}

// ImGui identifies windows, menus and headers by their label. With "text###id" only the part
// after ### is the identity, so after a language switch windows keep their position and
// headers stay open or closed.
std::string withId(const char* text, const char* id) {
    return std::format("{}###{}", text, id);
}

std::string withIcon(const char* icon, const char* text) {
    return std::format("{} {}", icon, text);
}

ImVec4 toImVec4(Color color) {
    return {static_cast<float>(color.r) / 255.0f, static_cast<float>(color.g) / 255.0f,
            static_cast<float>(color.b) / 255.0f, static_cast<float>(color.a) / 255.0f};
}

// Label above a full-width widget: works for long Russian labels in a narrow panel,
// where ImGui's default "widget, then label on the right" would cut the text.
void labelAbove(const char* text) {
    ImGui::TextUnformatted(text);
    ImGui::SetNextItemWidth(-FLT_MIN);
}

}  // namespace

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
    const float top = drawMainMenu();
    const float bottom = drawStatusBar();
    const float left = drawToolbar(top);
    const float right = drawPropertiesPanel(top);

    if (m_showGenerator) {
        drawGeneratorWindow();
    }
    if (m_showControls) {
        drawControlsWindow();
    }
    if (m_showImGuiDemo) {
        ImGui::ShowDemoWindow(&m_showImGuiDemo);
    }

    m_mapArea = {left, top, std::max(1.0f, right - left), std::max(1.0f, bottom - top)};
}

float App::drawMainMenu() {
    if (!ImGui::BeginMainMenuBar()) {
        return 0.0f;
    }
    const float height = ImGui::GetWindowHeight();

    if (ImGui::BeginMenu(withId(m_text(TextId::MenuFile), "file").c_str())) {
        if (ImGui::MenuItem(
                withIcon(ICON_FA_WAND_MAGIC_SPARKLES, m_text(TextId::MenuGenerate)).c_str())) {
            m_showGenerator = true;
        }
        if (ImGui::MenuItem(withIcon(ICON_FA_IMAGE, m_text(TextId::MenuExportPng)).c_str())) {
            exportPng();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(m_text(TextId::MenuExit))) {
            m_quitRequested = true;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(withId(m_text(TextId::MenuView), "view").c_str())) {
        if (ImGui::MenuItem(withIcon(ICON_FA_EXPAND, m_text(TextId::MenuFitToWindow)).c_str(),
                            "F")) {
            fitMapToScreen();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(m_text(TextId::MenuHillshade), nullptr, &m_colors.hillshade)) {
            m_needsRecolor = true;
        }
        if (ImGui::MenuItem(m_text(TextId::MenuCoastline), nullptr, &m_colors.coastline)) {
            m_needsRecolor = true;
        }
        ImGui::EndMenu();
    }

    const std::string languageMenu =
        withId(withIcon(ICON_FA_LANGUAGE, m_text(TextId::MenuLanguage)).c_str(), "language");
    if (ImGui::BeginMenu(languageMenu.c_str())) {
        for (const Language language : {Language::English, Language::Russian}) {
            // Without the Cyrillic font Russian would be unreadable "???".
            const bool available = language == Language::English || m_imgui.hasCyrillicFont();
            // languageName() returns a view of a string literal, so data() is null-terminated.
            if (ImGui::MenuItem(languageName(language).data(), nullptr,
                                language == m_text.language(), available)) {
                setLanguage(language);
            }
            if (!available) {
                ImGui::SetItemTooltip("%s", kMissingFontWarning);
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(withId(m_text(TextId::MenuHelp), "help").c_str())) {
        if (ImGui::MenuItem(withIcon(ICON_FA_KEYBOARD, m_text(TextId::MenuControls)).c_str())) {
            m_showControls = true;
        }
#ifndef NDEBUG
        // Debug builds only: a catalog of every ImGui widget, handy while building the UI.
        ImGui::MenuItem(m_text(TextId::MenuImGuiDemo), nullptr, &m_showImGuiDemo);
#endif
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
    return height;
}

float App::drawStatusBar() {
    const ImGuiStyle& style = ImGui::GetStyle();
    const float height = ImGui::GetFrameHeight();  // the same height as the menu bar
    const float top = static_cast<float>(GetScreenHeight()) - height;

    ImGui::SetNextWindowPos({0.0f, top});
    ImGui::SetNextWindowSize({static_cast<float>(GetScreenWidth()), height});
    // Vertical padding = frame padding, so one line of text fills exactly the frame height.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {2.0f * style.FramePadding.x,
                                                      style.FramePadding.y});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    constexpr ImGuiWindowFlags kFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("###statusbar", nullptr, kFlags)) {
        const Vector2 cell = mouseCell();
        const int x = static_cast<int>(std::floor(cell.x));
        const int y = static_cast<int>(std::floor(cell.y));
        const bool hoveringMap = !ImGui::GetIO().WantCaptureMouse && m_map.contains(x, y);
        if (hoveringMap) {
            const float h = m_map.at(x, y);
            ImGui::Text("%s %d, %d   %s %.3f (%s)", m_text(TextId::StatusCell), x, y,
                        m_text(TextId::StatusHeight), static_cast<double>(h),
                        m_text(h < m_colors.seaLevel ? TextId::StatusWater : TextId::StatusLand));
        } else {
            ImGui::TextDisabled("%s", m_text(TextId::StatusOutsideMap));
        }

        ImGui::SameLine(0.0f, 4.0f * style.ItemSpacing.x);
        ImGui::Text("%s %.2fx   %d FPS", m_text(TextId::StatusZoom),
                    static_cast<double>(m_camera.zoom), GetFPS());

        if (!m_statusMessage.empty()) {
            ImGui::SameLine(0.0f, 4.0f * style.ItemSpacing.x);
            ImGui::TextUnformatted(m_statusMessage.c_str());
        }
        if (!m_imgui.hasCyrillicFont()) {
            ImGui::SameLine(0.0f, 4.0f * style.ItemSpacing.x);
            ImGui::TextColored({1.0f, 0.8f, 0.4f, 1.0f}, "%s", kMissingFontWarning);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
    return top;
}

float App::drawToolbar(float top) {
    ImGui::SetNextWindowPos({kMargin, top + kMargin});
    float right = 0.0f;
    if (ImGui::Begin("###toolbar", nullptr, kFixedPanelFlags | ImGuiWindowFlags_NoTitleBar)) {
        // ImGui 1.92 renders fonts at any size on demand, so the icons can simply be drawn larger.
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * kToolIconScale);
        const float size = 1.6f * ImGui::GetFrameHeight();
        for (std::size_t i = 0; i < kTools.size(); ++i) {
            const BrushTool tool = kTools[i];
            const bool active = m_brush.tool == tool;

            ImGui::PushID(static_cast<int>(i));
            int pushedColors = 1;
            ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(toolColor(tool)));
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button,
                                      ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                ++pushedColors;
            }
            if (ImGui::Button(toolIcon(tool), {size, size})) {
                m_brush.tool = tool;
            }
            ImGui::PopStyleColor(pushedColors);
            ImGui::SetItemTooltip("%s (%d)", m_text(toolName(tool)), static_cast<int>(i) + 1);
            ImGui::PopID();
        }
        ImGui::PopFont();
    }
    right = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
    ImGui::End();
    return right;
}

float App::drawPropertiesPanel(float top) {
    const float width = kPropertiesWidth * uiScale();
    const float left = static_cast<float>(GetScreenWidth()) - width - kMargin;

    ImGui::SetNextWindowPos({left, top + kMargin});
    ImGui::SetNextWindowSizeConstraints({width, 0.0f}, {width, FLT_MAX});
    if (ImGui::Begin(withId(m_text(TextId::PanelProperties), "properties").c_str(), nullptr,
                     kFixedPanelFlags)) {
        const std::string brushHeader = std::format(
            "{}: {}###brush", m_text(TextId::SectionBrush), m_text(toolName(m_brush.tool)));
        if (ImGui::CollapsingHeader(brushHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            labelAbove(m_text(TextId::BrushRadius));
            ImGui::SliderFloat("##radius", &m_brush.radius, kMinBrushRadius, kMaxBrushRadius,
                               "%.0f");
            labelAbove(m_text(TextId::BrushStrength));
            ImGui::SliderFloat("##strength", &m_brush.strength, 0.02f, 1.5f, "%.2f");
            if (m_brush.tool == BrushTool::Flatten) {
                labelAbove(m_text(TextId::BrushTargetHeight));
                ImGui::SliderFloat("##target", &m_brush.targetHeight, 0.0f, 1.0f, "%.3f");
            }
        }

        if (ImGui::CollapsingHeader(withId(m_text(TextId::SectionMapView), "mapview").c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            labelAbove(m_text(TextId::SeaLevel));
            if (ImGui::SliderFloat("##sea", &m_colors.seaLevel, 0.0f, 1.0f, "%.3f")) {
                m_needsRecolor = true;
            }
            ImGui::BeginDisabled(!m_colors.hillshade);  // turned on and off in the View menu
            labelAbove(m_text(TextId::HillshadeStrength));
            if (ImGui::SliderFloat("##shade", &m_colors.hillshadeStrength, 0.0f, 1.0f, "%.2f")) {
                m_needsRecolor = true;
            }
            ImGui::EndDisabled();
        }

        ImGui::Spacing();
        if (ImGui::Button(
                withIcon(ICON_FA_WAND_MAGIC_SPARKLES, m_text(TextId::MenuGenerate)).c_str(),
                {-FLT_MIN, 0.0f})) {
            m_showGenerator = true;
        }
    }
    ImGui::End();
    return left;
}

void App::drawGeneratorWindow() {
    const ImVec2 screenCenter{0.5f * static_cast<float>(GetScreenWidth()),
                              0.5f * static_cast<float>(GetScreenHeight())};
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_FirstUseEver, {0.5f, 0.5f});
    const float width = kGeneratorWidth * uiScale();
    ImGui::SetNextWindowSizeConstraints({width, 0.0f}, {width, FLT_MAX});
    if (!ImGui::Begin(withId(m_text(TextId::WindowGenerator), "generator").c_str(),
                      &m_showGenerator,
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    // Regenerate when a slider is released, not on every frame of dragging it.
    bool settingsChanged = false;
    auto finished = [&settingsChanged] {
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            settingsChanged = true;
        }
    };

    // Seed field and a dice button that fills it with a random value.
    ImGui::TextUnformatted(m_text(TextId::Seed));
    const float diceWidth = ImGui::GetFrameHeight();
    ImGui::SetNextItemWidth(-(diceWidth + ImGui::GetStyle().ItemSpacing.x));
    if (ImGui::InputInt("##seed", &m_generator.seed)) {
        settingsChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_DICE, {diceWidth, 0.0f})) {
        m_generator.seed = GetRandomValue(1, 999'999);
        settingsChanged = true;
    }
    ImGui::SetItemTooltip("%s", m_text(TextId::RandomSeedTooltip));

    labelAbove(m_text(TextId::LandSize));
    ImGui::SliderFloat("##landsize", &m_generator.frequency, 0.5f, 10.0f, "%.2f");
    ImGui::SetItemTooltip("%s", m_text(TextId::LandSizeTooltip));
    finished();

    labelAbove(m_text(TextId::Island));
    ImGui::SliderFloat("##island", &m_generator.islandStrength, 0.0f, 1.0f, "%.2f");
    ImGui::SetItemTooltip("%s", m_text(TextId::IslandTooltip));
    finished();

    if (ImGui::TreeNode(withId(m_text(TextId::Advanced), "advanced").c_str())) {
        labelAbove(m_text(TextId::Detail));
        ImGui::SliderInt("##detail", &m_generator.octaves, 1, 10);
        ImGui::SetItemTooltip("%s", m_text(TextId::DetailTooltip));
        finished();

        labelAbove(m_text(TextId::Roughness));
        ImGui::SliderFloat("##roughness", &m_generator.gain, 0.2f, 0.8f, "%.2f");
        ImGui::SetItemTooltip("%s", m_text(TextId::RoughnessTooltip));
        finished();
        ImGui::TreePop();
    }

    ImGui::Spacing();
    if (m_hasManualEdits) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.8f, 0.4f, 1.0f});
        ImGui::TextWrapped("%s", m_text(TextId::ManualEditsWarning));
        ImGui::PopStyleColor();
    }
    const TextId generateLabel =
        m_hasManualEdits ? TextId::GenerateDiscardEdits : TextId::Generate;
    const bool generateClicked = ImGui::Button(m_text(generateLabel), {-FLT_MIN, 0.0f});

    if (generateClicked || (settingsChanged && !m_hasManualEdits)) {
        regenerate();
    }
    ImGui::End();
}

void App::drawControlsWindow() {
    ImGui::SetNextWindowPos({0.5f * static_cast<float>(GetScreenWidth()),
                             0.5f * static_cast<float>(GetScreenHeight())},
                            ImGuiCond_FirstUseEver, {0.5f, 0.5f});
    if (ImGui::Begin(withId(m_text(TextId::MenuControls), "controls").c_str(), &m_showControls,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        for (const TextId line : {TextId::HelpPaint, TextId::HelpSwapRaiseLower, TextId::HelpPan,
                                  TextId::HelpZoom, TextId::HelpKeys}) {
            ImGui::BulletText("%s", m_text(line));
        }
    }
    ImGui::End();
}

}  // namespace tf
