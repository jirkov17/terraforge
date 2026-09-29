# Third-party libraries are downloaded by CMake on the first configure (needs Git and internet).
# They land in build/_deps and are not part of the repository.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)  # show download progress: raylib is ~150 MB, the first configure takes a while

# raylib: window, input, 2D/3D rendering. https://www.raylib.com
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        6.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
    SYSTEM)

# Dear ImGui: immediate-mode UI (panels, sliders, buttons). https://github.com/ocornut/imgui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9b
    GIT_SHALLOW    TRUE
    SYSTEM)

# rlImGui: draws ImGui with raylib. Pinned to a commit: the project has no release for every fix.
FetchContent_Declare(rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG        1550009359ad975927f7f0e4a3f47e3f27123ea9
    SYSTEM)

# FastNoiseLite: header-only noise library for terrain generation. https://github.com/Auburn/FastNoiseLite
FetchContent_Declare(fastnoiselite
    GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
    GIT_TAG        v1.1.1
    GIT_SHALLOW    TRUE
    SYSTEM)

FetchContent_MakeAvailable(raylib imgui rlimgui fastnoiselite)

# ImGui and rlImGui ship without CMake support, so we build them into one static library.
add_library(imgui_raylib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp)
target_include_directories(imgui_raylib SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${rlimgui_SOURCE_DIR})
target_link_libraries(imgui_raylib PUBLIC raylib)
# rlImGui merges Font Awesome icons into the UI font; match them to our 16 px Noto Sans (default is 11).
target_compile_definitions(imgui_raylib PUBLIC FONT_AWESOME_ICON_SIZE=14)

add_library(fastnoiselite INTERFACE)
target_include_directories(fastnoiselite SYSTEM INTERFACE ${fastnoiselite_SOURCE_DIR}/Cpp)

if(TERRAFORGE_BUILD_TESTS)
    # GoogleTest: unit testing framework. https://github.com/google/googletest
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # MSVC: use the same runtime library as our code
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
        GIT_SHALLOW    TRUE
        SYSTEM)
    FetchContent_MakeAvailable(googletest)
endif()
