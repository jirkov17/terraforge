#include "core/TerrainGenerator.hpp"

#include "core/Heightmap.hpp"

#include <FastNoiseLite.h>

#include <algorithm>
#include <cmath>

namespace tf {

namespace {

// 0 in the center of the map, 1 on its edges.
// "Square bump" from https://www.redblobgames.com/maps/terrain-from-noise/
float edgeDistance(float nx, float ny) {
    return 1.0f - (1.0f - nx * nx) * (1.0f - ny * ny);
}

}  // namespace

void generateTerrain(Heightmap& map, const GeneratorSettings& settings) {
    // Fractal Brownian motion: several layers of OpenSimplex noise, each finer and weaker.
    FastNoiseLite noise(settings.seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(std::max(1, settings.octaves));
    noise.SetFractalLacunarity(settings.lacunarity);
    noise.SetFractalGain(settings.gain);
    noise.SetFrequency(settings.frequency);

    const float width = static_cast<float>(map.width());
    const float height = static_cast<float>(map.height());
    const float scale = 1.0f / std::max(width, height);  // same look at any map resolution

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const float fx = static_cast<float>(x);
            const float fy = static_cast<float>(y);

            // GetNoise returns roughly [-1, 1]; move it to [0, 1].
            const float elevation = 0.5f * (noise.GetNoise(fx * scale, fy * scale) + 1.0f);

            // Push the edges of the map down so that the land is surrounded by the sea.
            const float nx = 2.0f * (fx + 0.5f) / width - 1.0f;
            const float ny = 2.0f * (fy + 0.5f) / height - 1.0f;
            const float island = 1.0f - edgeDistance(nx, ny);

            map.at(x, y) = std::lerp(elevation, island, settings.islandStrength);
        }
    }

    map.normalize();
}

}  // namespace tf
