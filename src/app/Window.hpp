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
class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;
};

}  // namespace tf
