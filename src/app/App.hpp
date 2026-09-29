#pragma once

#include "app/MapRenderer.hpp"
#include "app/Window.hpp"
#include "core/AppSettings.hpp"
#include "core/Brush.hpp"
#include "core/Heightmap.hpp"
#include "core/Localization.hpp"
#include "core/MapColorizer.hpp"
#include "core/TerrainGenerator.hpp"

#include <raylib.h>

#include <filesystem>
#include <string>

namespace tf {

// The editor: owns the map, reacts to input and draws the map and the UI every frame.
// App.cpp: main loop, input and actions. AppUi.cpp: menus, panels and windows.
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
    void setLanguage(Language language);  // switches the UI and saves the choice

    // Drawing (AppUi.cpp). The panel functions return the edge that borders the map area.
    void drawBrushCursor() const;
    void drawUi();
    float drawMainMenu();                // returns the menu bar height
    float drawStatusBar();               // returns its top edge
    float drawToolbar(float top);        // returns its right edge
    float drawPropertiesPanel(float top);  // returns its left edge
    void drawGeneratorWindow();
    void drawControlsWindow();

    [[nodiscard]] Vector2 mouseCell() const;  // mouse position in map cell coordinates

    // Members are constructed top to bottom and destroyed bottom to top.
    // The window (and its OpenGL context) must be created first and destroyed last,
    // because MapRenderer owns a GPU texture.
    Window m_window;
    ImGuiLayer m_imgui;
    Heightmap m_map;
    MapRenderer m_renderer;

    std::filesystem::path m_settingsPath;  // terraforge.ini next to the executable
    AppSettings m_settings;
    Localizer m_text;  // UI text in the active language: m_text(TextId::MenuFile)

    Camera2D m_camera{};
    GeneratorSettings m_generator;
    BrushSettings m_brush;
    ColorizeSettings m_colors;

    bool m_needsRecolor = true;     // the map changed and the texture must be updated
    bool m_hasManualEdits = false;  // the user painted since the last generation
    bool m_fitPending = true;       // fit the map once the UI has been laid out
    bool m_quitRequested = false;   // File > Exit
    bool m_showGenerator = false;
    bool m_showControls = false;
    bool m_showImGuiDemo = false;
    Rectangle m_mapArea{};  // screen area not covered by the UI, where the map is shown
    std::string m_statusMessage;
};

}  // namespace tf
