#pragma once

#include "core/MapColorizer.hpp"

#include <raylib.h>

#include <string>
#include <vector>

namespace tf {

class Heightmap;

// Turns a heightmap into a colored GPU texture and draws it.
// Owns the texture (RAII), so it must be created after the window and destroyed before it.
class MapRenderer {
public:
    MapRenderer(int width, int height);
    ~MapRenderer();

    MapRenderer(const MapRenderer&) = delete;
    MapRenderer& operator=(const MapRenderer&) = delete;

    // Recomputes the pixel colors on the CPU and uploads them to the texture.
    void update(const Heightmap& map, const ColorizeSettings& settings,
                const Geography* geography = nullptr);

    // Draws the map with its top-left corner at (0, 0): one cell = one world unit.
    void draw() const;

    // Saves the current image as a PNG file. Returns false on failure.
    [[nodiscard]] bool exportPng(const std::string& path) const;

private:
    int m_width;
    int m_height;
    std::vector<Rgba8> m_pixels;
    Texture2D m_texture{};
};

}  // namespace tf
