#pragma once

#include "app/MapRenderer.hpp"
#include "app/Window.hpp"
#include "core/Brush.hpp"
#include "core/Heightmap.hpp"
#include "core/MapColorizer.hpp"
#include "core/TerrainGenerator.hpp"

#include <raylib.h>

#include <string>

namespace tf {

// The editor: owns the map, reacts to input and draws the map and the UI every frame.
class App {
public:
    App();
    void run();

private:
    // Input
    void handleInput(float dt);
    void handleCameraInput();
    void handleShortcuts();
    void paint(float dt);

    // Actions
    void regenerate();
    void fitMapToScreen();
    void exportPng();

    // Drawing
    void drawBrushCursor() const;
    void drawUi();
    void drawGeneratorSection();
    void drawBrushSection();
    void drawViewSection();
    void drawInfoSection();
    void drawHelpSection();

    [[nodiscard]] Vector2 mouseCell() const;  // mouse position in map cell coordinates

    // Members are constructed top to bottom and destroyed bottom to top.
    // The window (and its OpenGL context) must be created first and destroyed last,
    // because MapRenderer owns a GPU texture.
    Window m_window;
    ImGuiLayer m_imgui;
    Heightmap m_map;
    MapRenderer m_renderer;

    Camera2D m_camera{};
    GeneratorSettings m_generator;
    BrushSettings m_brush;
    ColorizeSettings m_colors;

    bool m_needsRecolor = true;     // the map changed and the texture must be updated
    bool m_hasManualEdits = false;  // the user painted since the last generation
    bool m_showImGuiDemo = false;
    float m_panelRight = 0.0f;  // right edge of the UI panel, in screen pixels
    std::string m_statusMessage;
};

}  // namespace tf
