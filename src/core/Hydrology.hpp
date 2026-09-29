#pragma once

#include <cstddef>
#include <vector>

namespace tf {

class Heightmap;

struct HydrologySettings {
    float seaLevel = 0.45f;
    float minLakeDepth = 0.002f;    // shallower puddles are not shown as lakes
    float riverThreshold = 400.0f;  // cells draining through a point before it becomes a river
};

// Where water collects and how it flows over the terrain. All vectors are indexed like
// Heightmap values (index = y * width + x).
struct Hydrology {
    int width = 0;
    int height = 0;
    HydrologySettings settings;

    // Height of the water surface: the terrain with every pit filled up to its spill point.
    // Equal to the terrain height on slopes, higher than it in lakes.
    std::vector<float> waterLevel;
    // Number of land cells whose rain flows through this cell (the cell itself included).
    std::vector<float> flow;
    std::vector<bool> sea;  // below the sea level

    [[nodiscard]] float lakeDepth(std::size_t i, float terrainHeight) const noexcept {
        return waterLevel[i] - terrainHeight;
    }
    [[nodiscard]] bool isLake(std::size_t i, float terrainHeight) const noexcept {
        return !sea[i] && lakeDepth(i, terrainHeight) >= settings.minLakeDepth;
    }
    [[nodiscard]] bool isRiver(std::size_t i, float terrainHeight) const noexcept {
        return !sea[i] && !isLake(i, terrainHeight) && flow[i] >= settings.riverThreshold;
    }
};

// Fills pits with Priority-Flood (Barnes, Lehman, Mulla 2014) to find lakes, then lets every
// land cell send its rain to its steepest downhill neighbour to find rivers. O(n log n).
[[nodiscard]] Hydrology computeHydrology(const Heightmap& map, const HydrologySettings& settings);

}  // namespace tf
