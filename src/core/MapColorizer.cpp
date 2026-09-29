#include "core/MapColorizer.hpp"

#include "core/Geography.hpp"
#include "core/Heightmap.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace tf {

namespace {

struct ColorStop {
    float at;  // depth or altitude, in height units
    Rgba8 color;
};

// Depth below the sea level -> water color.
constexpr std::array kWaterStops{
    ColorStop{0.00f, {128, 186, 204, 255}},  // shallow water near the shore
    ColorStop{0.06f, {74, 138, 180, 255}},   // open sea
    ColorStop{0.30f, {26, 60, 104, 255}},    // deep ocean
};

// Altitude above the sea level -> land color.
constexpr std::array kLandStops{
    ColorStop{0.000f, {226, 212, 162, 255}},  // beach
    ColorStop{0.012f, {156, 188, 110, 255}},  // meadows
    ColorStop{0.100f, {104, 156, 82, 255}},   // lowland forests
    ColorStop{0.200f, {80, 128, 72, 255}},    // highland forests
    ColorStop{0.290f, {152, 136, 100, 255}},  // hills
    ColorStop{0.400f, {128, 120, 114, 255}},  // mountains
    ColorStop{0.500f, {242, 242, 242, 255}},  // snow caps
};

constexpr Rgba8 kCoastlineColor{44, 62, 80, 255};

// Lake depth -> fresh water color: a little greener and lighter than the sea.
constexpr std::array kLakeStops{
    ColorStop{0.00f, {126, 182, 206, 255}},
    ColorStop{0.05f, {72, 132, 178, 255}},
};
constexpr Rgba8 kRiverColor{58, 116, 186, 255};

constexpr float kBeachWidth = 0.012f;  // altitude where the beach ends (the meadows stop above)

std::uint8_t toByte(float value) {
    // +0.5 rounds to the nearest integer (std::lround is noticeably slower in a hot loop).
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 255.0f) + 0.5f);
}

float channel(std::uint8_t value) {
    return static_cast<float>(value);
}

Rgba8 mix(Rgba8 a, Rgba8 b, float t) {
    return {toByte(std::lerp(channel(a.r), channel(b.r), t)),
            toByte(std::lerp(channel(a.g), channel(b.g), t)),
            toByte(std::lerp(channel(a.b), channel(b.b), t)), 255};
}

template <std::size_t N>
Rgba8 sampleGradient(const std::array<ColorStop, N>& stops, float value) {
    if (value <= stops.front().at) {
        return stops.front().color;
    }
    for (std::size_t i = 1; i < N; ++i) {
        if (value <= stops[i].at) {
            const float t = (value - stops[i - 1].at) / (stops[i].at - stops[i - 1].at);
            return mix(stops[i - 1].color, stops[i].color, t);
        }
    }
    return stops.back().color;
}

// Brightness multiplier for a slope with the given height gradient:
// 1 for flat ground, above 1 for slopes facing the light, below 1 for slopes facing away.
float hillshadeFactor(float dzdx, float dzdy, float strength) {
    // Direction to the light: north-west (top-left of the screen), 45 degrees above the ground.
    constexpr float kLightX = -0.5f;
    constexpr float kLightY = -0.5f;
    constexpr float kLightZ = 0.70710678f;

    // Surface normal of z = f(x, y) is (-dz/dx, -dz/dy, 1), normalized.
    const float length = std::sqrt(dzdx * dzdx + dzdy * dzdy + 1.0f);
    const float lighting = (-dzdx * kLightX - dzdy * kLightY + kLightZ) / length;
    const float factor = lighting / kLightZ;
    return std::clamp(std::lerp(1.0f, factor, strength), 0.35f, 1.35f);
}

Rgba8 brighten(Rgba8 color, float factor) {
    return {toByte(channel(color.r) * factor), toByte(channel(color.g) * factor),
            toByte(channel(color.b) * factor), 255};
}

// How strongly a river cell is painted: faint where it starts, solid four times downstream.
float riverOpacity(float flow, float threshold) {
    return std::clamp(0.45f + 0.25f * std::log2(flow / threshold), 0.45f, 1.0f);
}

bool touchesLand(const Heightmap& map, int x, int y, float seaLevel) {
    return map.atClamped(x - 1, y) >= seaLevel || map.atClamped(x + 1, y) >= seaLevel ||
           map.atClamped(x, y - 1) >= seaLevel || map.atClamped(x, y + 1) >= seaLevel;
}

}  // namespace

Rgba8 terrainColor(float height, float seaLevel) noexcept {
    if (height < seaLevel) {
        return sampleGradient(kWaterStops, seaLevel - height);
    }
    return sampleGradient(kLandStops, height - seaLevel);
}

Rgba8 biomeColor(Biome biome) noexcept {
    switch (biome) {
        case Biome::Sea:
            return kWaterStops[1].color;
        case Biome::Glacier:
            return {236, 240, 244, 255};
        case Biome::Tundra:
            return {178, 182, 156, 255};
        case Biome::Taiga:
            return {82, 118, 94, 255};
        case Biome::Forest:
            return {90, 140, 74, 255};
        case Biome::Meadow:
            return {158, 190, 108, 255};
        case Biome::Steppe:
            return {204, 194, 128, 255};
        case Biome::Desert:
            return {232, 210, 152, 255};
        case Biome::Swamp:
            return {102, 120, 82, 255};
        case Biome::Mountains:
            return {140, 128, 116, 255};
    }
    return {255, 0, 255, 255};  // magenta: a biome without a color is easy to spot
}

void colorize(const Heightmap& map, const ColorizeSettings& settings, std::span<Rgba8> pixels,
              const Geography* geography) {
    const int width = map.width();
    const int height = map.height();
    if (pixels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        throw std::invalid_argument("colorize: pixel buffer size must match the heightmap size");
    }

    // Heights are tiny compared to the map size (0..1 vs 512 cells), so slopes are
    // exaggerated for shading. Scaling with the map size keeps the look resolution-independent.
    const float reliefScale = 0.1f * static_cast<float>(std::max(width, height));
    const Hydrology* water =
        settings.rivers && geography != nullptr ? &geography->hydrology : nullptr;
    const Climate* climate =
        settings.mode == MapMode::Biomes && geography != nullptr ? &geography->climate : nullptr;

    std::size_t i = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x, ++i) {
            const float h = map.at(x, y);
            Rgba8 color = terrainColor(h, settings.seaLevel);
            // The biome map keeps the sea and the beaches of the relief map.
            if (climate != nullptr && h >= settings.seaLevel + kBeachWidth) {
                color = biomeColor(climate->biome[i]);
            }

            if (h < settings.seaLevel) {
                if (settings.coastline && touchesLand(map, x, y, settings.seaLevel)) {
                    color = kCoastlineColor;
                }
            } else if (settings.hillshade) {
                // Central differences: the slope along x and y at this cell.
                const float dzdx = 0.5f * (map.atClamped(x + 1, y) - map.atClamped(x - 1, y));
                const float dzdy = 0.5f * (map.atClamped(x, y + 1) - map.atClamped(x, y - 1));
                color = brighten(color, hillshadeFactor(dzdx * reliefScale, dzdy * reliefScale,
                                                        settings.hillshadeStrength));
            }

            // Fresh water on top of the land: flat lakes, rivers blended over the shaded ground.
            if (water != nullptr && h >= settings.seaLevel) {
                if (water->isLake(i, h)) {
                    color = sampleGradient(kLakeStops, water->lakeDepth(i, h));
                } else if (water->isRiver(i, h)) {
                    const float opacity =
                        riverOpacity(water->flow[i], water->settings.riverThreshold);
                    color = mix(color, kRiverColor, opacity);
                }
            }

            pixels[i] = color;
        }
    }
}

}  // namespace tf
