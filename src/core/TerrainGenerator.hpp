#pragma once

namespace tf {

class Heightmap;

struct GeneratorSettings {
    int seed = 1337;
    float frequency = 3.0f;        // size of land masses: higher = more, smaller islands
    int octaves = 6;               // number of noise layers: more = finer detail
    float lacunarity = 2.0f;       // how much finer each next layer is
    float gain = 0.5f;             // how strong each next layer is ("roughness")
    float islandStrength = 0.45f;  // 0 = land can touch the edges, 1 = ocean all around
};

// Fills the whole heightmap with procedurally generated terrain in [0, 1].
// Deterministic: the same settings always produce the same map.
void generateTerrain(Heightmap& map, const GeneratorSettings& settings);

}  // namespace tf
