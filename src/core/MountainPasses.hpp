#pragma once

#include <vector>

namespace tf {

class Heightmap;

struct MountainPassSettings {
    float seaLevel = 0.45f;
    float minAltitude = 0.2f;  // a saddle this high above the sea counts as a mountain pass
    // Valleys smaller than this share of the map are ignored: a dip on a mountain top is not
    // a valley worth crossing into. Measured when the valleys meet, so it is the area below
    // the saddle: about 50 cells on a 512 x 512 map, which gives 5-10 passes per continent.
    float minValleyFraction = 0.0002f;
    int minSpacing = 12;  // passes closer than this many cells are merged into one
};

struct MountainPass {
    int x = 0;
    int y = 0;
    float height = 0.0f;
};

// Mountain passes: the lowest points where one can cross from one big valley into another.
// Cells are added from the lowest to the highest (like water rising), neighbouring cells are
// joined with union-find, and a cell that joins two big "lakes" is a saddle point.
[[nodiscard]] std::vector<MountainPass> findMountainPasses(const Heightmap& map,
                                                           const MountainPassSettings& settings);

}  // namespace tf
