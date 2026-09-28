cmake_minimum_required(VERSION 3.25)

project(Terraforge
    VERSION 0.1.0
    DESCRIPTION "Fantasy map editor: generate and sculpt terrain for your own world"
    LANGUAGES C CXX)  # C is needed to build raylib

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # compile_commands.json for clangd / clang-tidy

# Single-config generators (Ninja, Makefiles) build Debug unless told otherwise.
# Visual Studio is multi-config: pick the configuration with --config Debug|Release.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# All executables go to build/bin (Visual Studio adds a Debug/ or Release/ subfolder).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(TERRAFORGE_BUILD_TESTS "Build unit tests" ON)
option(TERRAFORGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Dependencies)

add_subdirectory(src)

if(TERRAFORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Open the app (not a library) by default when the generated .sln is opened in Visual Studio.
set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT terraforge)
