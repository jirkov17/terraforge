#include "app/Window.hpp"

#include <raylib.h>
#include <rlImGui.h>

#include <stdexcept>

namespace tf {

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
    rlImGuiSetup(true);  // true = dark theme
}

ImGuiLayer::~ImGuiLayer() {
    rlImGuiShutdown();
}

}  // namespace tf
