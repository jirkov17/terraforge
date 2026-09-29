#pragma once

#include "core/ContinentShape.hpp"

namespace tf {

class Heightmap;

struct GeneratorSettings {
    int seed = 1337;
    float frequency = 3.0f;   // size of land masses: higher = more, smaller islands
    int octaves = 6;          // number of noise layers: more = finer detail
    float lacunarity = 2.0f;  // how much finer each next layer is
    float gain = 0.5f;        // how strong each next layer is ("roughness")
    ShapePreset shape = ShapePreset::Continent;
    // How closely the land follows the shape mask: 0 = pure noise, 1 = exactly the mask.
    // At 0.55 and sea level 0.45, mask 0 is always sea and mask 1 is always land.
    float shapeStrength = 0.55f;
};

// Terrain is built in two steps, so that redrawing the shape does not recompute the noise:
//   1. generateNoise: fractal noise, the slow part (about 50 ms for 512 x 512);
//   2. combineTerrain: noise + blurred shape mask, fast enough to run every frame.

// Fills `noise` with fractal Brownian motion normalized to exactly [0, 1].
void generateNoise(Heightmap& noise, const GeneratorSettings& settings);

// map = lerp(noise, blurred shape, shapeStrength), in [0, 1]. The blur turns a hand-drawn
// mask with sharp edges into land that rises gradually from the coast inland.
// All three grids must have the same size.
void combineTerrain(Heightmap& map, const Heightmap& noise, const Heightmap& shape,
                    float shapeStrength);

// Both steps at once with the preset from the settings. Deterministic: the same settings
// always produce the same map.
void generateTerrain(Heightmap& map, const GeneratorSettings& settings);

}  // namespace tf
