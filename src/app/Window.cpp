#include "app/Window.hpp"

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

#include <cmath>
#include <stdexcept>
#include <string>

namespace tf {

namespace {

constexpr float kUiFontSize = 16.0f;  // at 100% display scaling
constexpr const char* kUiFontFile = "assets/fonts/NotoSans-Regular.ttf";

// Adds the UI font to the ImGui font atlas. Returns false if the font file is missing.
// ImGui 1.92 rasterizes glyphs on demand, so Cyrillic works without listing glyph ranges.
bool addUiFont() {
    // Relative to the executable, not the working directory: the app may be started from anywhere.
    const std::string path = std::string(GetApplicationDirectory()) + kUiFontFile;
    if (!FileExists(path.c_str())) {
        return false;
    }
    // Same DPI handling as the default font in rlImGui.
    const float scale = GetWindowScaleDPI().y;
    ImFontConfig config;
    config.SizePixels = std::ceil(kUiFontSize * scale);
    config.ExtraSizeScale = scale;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), 0.0f, &config) != nullptr;
}

}  // namespace

Window::Window(int width, int height, const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    if (!IsWindowReady()) {
        throw std::runtime_error("Could not open a window (OpenGL 3.3 is required)");
    }
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);  // Esc must not close the editor and lose the user's work
}

Window::~Window() {
    CloseWindow();
}

ImGuiLayer::ImGuiLayer() {
    // rlImGuiSetup() split in two so we can add our own font in between.
    // Begin loads the built-in font only when no font callback is set, so we add ours after it
    // and make it the default. End merges the Font Awesome icons into the last added font.
    rlImGuiBeginInitImGui();
    m_hasCyrillicFont = addUiFont();
    if (m_hasCyrillicFont) {
        ImGuiIO& io = ImGui::GetIO();
        io.FontDefault = io.Fonts->Fonts.back();
    }
    ImGui::StyleColorsDark();
    rlImGuiEndInitImGui();
}

ImGuiLayer::~ImGuiLayer() {
    rlImGuiShutdown();
}

}  // namespace tf
