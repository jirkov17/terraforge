#pragma once

namespace tf {

// RAII wrapper: opens the main window (and its OpenGL context) in the constructor
// and closes it in the destructor. Textures and other GPU resources may only exist
// while the window is open.
class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

// RAII wrapper: sets up Dear ImGui on top of raylib and shuts it down in the destructor.
// Loads the UI font from assets/fonts next to the executable; if it is missing, falls back
// to the built-in ImGui font, which has no Cyrillic letters.
class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    [[nodiscard]] bool hasCyrillicFont() const noexcept { return m_hasCyrillicFont; }

private:
    bool m_hasCyrillicFont = false;
};

}  // namespace tf
