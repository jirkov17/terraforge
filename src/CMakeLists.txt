# Core: terrain data and algorithms. Pure C++ without graphics, so it is easy to unit test.
add_library(terraforge_core STATIC
    core/AppSettings.cpp
    core/Brush.cpp
    core/Climate.cpp
    core/ContinentShape.cpp
    core/Erosion.cpp
    core/Geography.cpp
    core/Heightmap.cpp
    core/Hydrology.cpp
    core/Localization.cpp
    core/MapColorizer.cpp
    core/MountainPasses.cpp
    core/TerrainGenerator.cpp)
target_include_directories(terraforge_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(terraforge_core PRIVATE fastnoiselite)
terraforge_set_warnings(terraforge_core)

# App: window, rendering and UI on top of the core.
add_executable(terraforge
    app/App.cpp
    app/AppUi.cpp
    app/MapRenderer.cpp
    app/Window.cpp
    app/main.cpp)
target_link_libraries(terraforge PRIVATE terraforge_core imgui_raylib raylib)
terraforge_set_warnings(terraforge)

# Visual Studio: run the app from the folder with the .exe (exported PNGs land there).
set_target_properties(terraforge PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:terraforge>")

# Fonts and other runtime files: the app looks for them next to the .exe.
add_custom_command(TARGET terraforge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:terraforge>/assets
    COMMENT "Copying assets next to terraforge")
