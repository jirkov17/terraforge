#include "core/ContinentShape.hpp"

#include "core/Heightmap.hpp"

#include <FastNoiseLite.h>

#include <algorithm>

namespace tf {

namespace {

// Offsets the seeds of the helper noises, so they do not repeat the terrain noise.
constexpr int kWarpSeedOffset = 101;
constexpr int kIslandSeedOffset = 202;

constexpr float kWarpAmount = 0.12f;  // how far outlines are bent, in normalized coordinates

constexpr float smoothstep(float edge0, float edge1, float x) noexcept {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// 1 in the center of the map, 0 on its edges.
// "Square bump" from https://www.redblobgames.com/maps/terrain-from-noise/
constexpr float centerBump(float nx, float ny) noexcept {
    return (1.0f - nx * nx) * (1.0f - ny * ny);
}

// 1 in the center of the ellipse, falling to 0 at its border and outside.
constexpr float ellipse(float nx, float ny, float cx, float cy, float rx, float ry) noexcept {
    const float dx = (nx - cx) / rx;
    const float dy = (ny - cy) / ry;
    return std::clamp(1.0f - (dx * dx + dy * dy), 0.0f, 1.0f);
}

FastNoiseLite makeNoise(int seed, float frequency, int octaves) {
    FastNoiseLite noise(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(octaves);
    noise.SetFrequency(frequency);
    return noise;
}

}  // namespace

void makeShapeMask(Heightmap& mask, ShapePreset preset, int seed) {
    if (preset == ShapePreset::Ocean) {
        mask.fill(0.0f);
        return;
    }

    // Coordinates are normalized to [-1, 1] so the shapes do not depend on the map size.
    FastNoiseLite warp = makeNoise(seed + kWarpSeedOffset, 1.5f, 2);
    FastNoiseLite islands = makeNoise(seed + kIslandSeedOffset, 3.0f, 3);
    const float width = static_cast<float>(mask.width());
    const float height = static_cast<float>(mask.height());

    for (int y = 0; y < mask.height(); ++y) {
        for (int x = 0; x < mask.width(); ++x) {
            const float nx = 2.0f * (static_cast<float>(x) + 0.5f) / width - 1.0f;
            const float ny = 2.0f * (static_cast<float>(y) + 0.5f) / height - 1.0f;
            // Domain warping: sample the shape at a slightly shifted point, which turns
            // perfect ellipses into organic outlines.
            const float wx = nx + kWarpAmount * warp.GetNoise(nx, ny);
            const float wy = ny + kWarpAmount * warp.GetNoise(nx + 17.0f, ny - 9.0f);

            float value = 0.0f;
            switch (preset) {
                case ShapePreset::Continent:
                    value = centerBump(wx, wy);
                    break;
                case ShapePreset::Archipelago: {
                    // Islands where a medium-frequency noise peaks, fading out near the edges.
                    const float n = 0.5f * (islands.GetNoise(nx, ny) + 1.0f);
                    value =
                        smoothstep(0.52f, 0.62f, n) * smoothstep(0.05f, 0.4f, centerBump(nx, ny));
                    break;
                }
                case ShapePreset::TwoContinents: {
                    const float west = ellipse(wx, wy, -0.47f, 0.0f, 0.4f, 0.75f);
                    const float east = ellipse(wx, wy, 0.47f, 0.05f, 0.4f, 0.7f);
                    value = smoothstep(0.0f, 0.45f, std::max(west, east));
                    break;
                }
                case ShapePreset::InlandSea: {
                    const float land = smoothstep(0.1f, 0.45f, centerBump(wx, wy));
                    const float sea =
                        smoothstep(0.0f, 0.3f, ellipse(wx, wy, 0.0f, 0.0f, 0.5f, 0.42f));
                    value = land * (1.0f - sea);
                    break;
                }
                case ShapePreset::Ocean:
                    break;
            }
            mask.at(x, y) = std::clamp(value, 0.0f, 1.0f);
        }
    }
}

}  // namespace tf
