#pragma once

#include <cstdint>
#include <vector>

namespace tf {

class Heightmap;
struct Hydrology;

// Land cover of a cell, chosen from temperature and moisture like on a Whittaker diagram.
enum class Biome : std::uint8_t {
    Sea,
    Glacier,
    Tundra,
    Taiga,
    Forest,
    Meadow,
    Steppe,
    Desert,
    Swamp,
    Mountains,
};

struct ClimateSettings {
    float seaLevel = 0.45f;
    // Warmth at the top (north) and the bottom (south) edge of the map: 0 = eternal ice,
    // 1 = scorching heat. In between the temperature changes linearly with the latitude.
    float northTemperature = 0.25f;
    float southTemperature = 0.9f;
    float lapseRate = 0.5f;  // cooling per unit of altitude above the sea: cold mountain tops
    // Distance over which the moisture from water fades (by a factor e), as a fraction of the
    // map size.
    float moistureReach = 0.08f;
    float rainShadow = 1.2f;  // how much moisture winds lose when they climb mountains
};

// Temperature, moisture and biome of every cell, indexed like Heightmap values.
struct Climate {
    int width = 0;
    int height = 0;
    std::vector<float> temperature;  // 0 = frozen, 1 = hot
    std::vector<float> moisture;     // 0 = bone dry, 1 = soaking wet
    std::vector<Biome> biome;
};

// Temperature: latitude minus altitude. Moisture: closeness to the sea, lakes and rivers,
// reduced behind mountains for the westerly wind (rain shadow). Biome: from both.
[[nodiscard]] Climate computeClimate(const Heightmap& map, const Hydrology& water,
                                     const ClimateSettings& settings);

// The biome of one land cell. `altitude` is the height above the sea level, `slope` the
// height difference per cell.
[[nodiscard]] Biome classifyBiome(float temperature, float moisture, float altitude,
                                  float slope) noexcept;

}  // namespace tf
