#include "core/TerrainGenerator.hpp"

#include "core/Heightmap.hpp"

#include <FastNoiseLite.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tf {

namespace {

constexpr int kShapeBlurDivisor = 64;  // blur radius = map size / 64 (8 cells for 512)

bool sameSize(const Heightmap& a, const Heightmap& b) noexcept {
    return a.width() == b.width() && a.height() == b.height();
}

}  // namespace

void generateNoise(Heightmap& noise, const GeneratorSettings& settings) {
    // Fractal Brownian motion: several layers of OpenSimplex noise, each finer and weaker.
    FastNoiseLite fbm(settings.seed);
    fbm.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fbm.SetFractalType(FastNoiseLite::FractalType_FBm);
    fbm.SetFractalOctaves(std::max(1, settings.octaves));
    fbm.SetFractalLacunarity(settings.lacunarity);
    fbm.SetFractalGain(settings.gain);
    fbm.SetFrequency(settings.frequency);

    const float scale = 1.0f / static_cast<float>(std::max(noise.width(), noise.height()));
    for (int y = 0; y < noise.height(); ++y) {
        for (int x = 0; x < noise.width(); ++x) {
            // Same look at any map resolution: sample in [0, 1] map coordinates.
            noise.at(x, y) =
                fbm.GetNoise(static_cast<float>(x) * scale, static_cast<float>(y) * scale);
        }
    }
    noise.normalize();
}

void combineTerrain(Heightmap& map, const Heightmap& noise, const Heightmap& shape,
                    float shapeStrength) {
    if (!sameSize(map, noise) || !sameSize(map, shape)) {
        throw std::invalid_argument("combineTerrain: all grids must have the same size");
    }
    Heightmap blurred = shape;
    blurField(blurred, std::max(1, std::max(map.width(), map.height()) / kShapeBlurDivisor));

    const float strength = std::clamp(shapeStrength, 0.0f, 1.0f);
    const auto noiseValues = noise.values();
    const auto shapeValues = blurred.values();
    const auto mapValues = map.values();
    for (std::size_t i = 0; i < mapValues.size(); ++i) {
        // No normalize() afterwards: stretching would turn a painted empty ocean back into land.
        // lerp of two values in [0, 1] already stays in [0, 1].
        mapValues[i] = std::lerp(noiseValues[i], shapeValues[i], strength);
    }
}

void generateTerrain(Heightmap& map, const GeneratorSettings& settings) {
    Heightmap noise(map.width(), map.height());
    Heightmap shape(map.width(), map.height());
    generateNoise(noise, settings);
    makeShapeMask(shape, settings.shape, settings.seed);
    combineTerrain(map, noise, shape, settings.shapeStrength);
}

}  // namespace tf
