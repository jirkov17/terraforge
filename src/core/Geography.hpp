#pragma once

#include "core/Climate.hpp"
#include "core/Hydrology.hpp"

namespace tf {

class Heightmap;

struct GeographySettings {
    float seaLevel = 0.45f;
    float riverThreshold = 400.0f;   // see HydrologySettings
    float northTemperature = 0.25f;  // see ClimateSettings
    float southTemperature = 0.9f;
};

// Everything derived from the heights: never edited by hand, recomputed after the terrain
// changes (after a brush stroke, a generation, an erosion run or a sea level change).
struct Geography {
    Hydrology hydrology;  // lakes and rivers
    Climate climate;      // temperature, moisture and biomes; uses the rivers and lakes
};

[[nodiscard]] Geography analyzeGeography(const Heightmap& map, const GeographySettings& settings);

}  // namespace tf
