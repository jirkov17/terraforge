#pragma once

#include <cstdint>
#include <span>

namespace tf {

class Heightmap;

// 8-bit RGBA pixel. Same memory layout as raylib's Color, so a buffer of these
// can be uploaded to a GPU texture as is.
struct Rgba8 {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    friend bool operator==(const Rgba8&, const Rgba8&) = default;
};

struct ColorizeSettings {
    float seaLevel = 0.45f;
    bool hillshade = true;            // shade slopes as if lit from the north-west, like paper maps
    float hillshadeStrength = 0.85f;  // 0 = flat colors, 1 = full shading
    bool coastline = true;            // dark outline where water touches land
};

// Base color of one cell without shading. Colors depend on the depth below or
// the altitude above the sea level: shallow and deep water, beach, grass, forest, rock, snow.
[[nodiscard]] Rgba8 terrainColor(float height, float seaLevel) noexcept;

// Writes one pixel per heightmap cell, row by row. `pixels` must have width * height elements.
void colorize(const Heightmap& map, const ColorizeSettings& settings, std::span<Rgba8> pixels);

}  // namespace tf
