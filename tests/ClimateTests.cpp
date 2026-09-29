#include "core/Climate.hpp"
#include "core/Heightmap.hpp"
#include "core/Hydrology.hpp"

#include <gtest/gtest.h>

using tf::Biome;
using tf::Climate;
using tf::ClimateSettings;
using tf::Heightmap;
using tf::HydrologySettings;

namespace {

// The wind dries out over about half of the map width, so the map must be wide enough for
// the rain shadow test to see a difference before the air is dry anyway.
constexpr int kSize = 128;
constexpr float kSeaLevel = 0.45f;
constexpr float kLowland = 0.5f;  // a little above the sea

std::size_t indexOf(int x, int y) {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(kSize) +
           static_cast<std::size_t>(x);
}

// Climate without rivers, so only the sea and the terrain matter.
Climate climateOf(const Heightmap& map, const ClimateSettings& settings = {}) {
    HydrologySettings water;
    water.seaLevel = settings.seaLevel;
    water.riverThreshold = 1e9f;
    return tf::computeClimate(map, tf::computeHydrology(map, water), settings);
}

// Sea in the westmost columns, flat lowland everywhere else.
Heightmap makeWestCoast(int seaColumns = 4) {
    Heightmap map(kSize, kSize, kLowland);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < seaColumns; ++x) {
            map.at(x, y) = 0.2f;
        }
    }
    return map;
}

}  // namespace

TEST(ClassifyBiome, FollowsTheWhittakerTable) {
    constexpr float kFlat = 0.0f;
    constexpr float kLow = 0.1f;
    EXPECT_EQ(tf::classifyBiome(0.05f, 0.5f, kLow, kFlat), Biome::Glacier);
    EXPECT_EQ(tf::classifyBiome(0.2f, 0.5f, kLow, kFlat), Biome::Tundra);
    EXPECT_EQ(tf::classifyBiome(0.35f, 0.6f, kLow, kFlat), Biome::Taiga);
    EXPECT_EQ(tf::classifyBiome(0.35f, 0.1f, kLow, kFlat), Biome::Steppe);
    EXPECT_EQ(tf::classifyBiome(0.55f, 0.3f, kLow, kFlat), Biome::Meadow);
    EXPECT_EQ(tf::classifyBiome(0.55f, 0.6f, kLow, kFlat), Biome::Forest);
    EXPECT_EQ(tf::classifyBiome(0.85f, 0.1f, kLow, kFlat), Biome::Desert);
    EXPECT_EQ(tf::classifyBiome(0.85f, 0.4f, kLow, kFlat), Biome::Steppe);
}

TEST(ClassifyBiome, HighGroundIsMountainsOrIce) {
    EXPECT_EQ(tf::classifyBiome(0.6f, 0.5f, 0.4f, 0.0f), Biome::Mountains);
    EXPECT_EQ(tf::classifyBiome(0.2f, 0.5f, 0.4f, 0.0f), Biome::Glacier);
}

TEST(ClassifyBiome, SwampsNeedWetLowFlatGround) {
    EXPECT_EQ(tf::classifyBiome(0.55f, 0.9f, 0.01f, 0.0f), Biome::Swamp);
    EXPECT_NE(tf::classifyBiome(0.55f, 0.9f, 0.01f, 0.05f), Biome::Swamp);  // too steep
    EXPECT_NE(tf::classifyBiome(0.55f, 0.9f, 0.2f, 0.0f), Biome::Swamp);    // too high
    EXPECT_NE(tf::classifyBiome(0.55f, 0.5f, 0.01f, 0.0f), Biome::Swamp);   // too dry
}

TEST(Climate, NorthIsColderThanSouth) {
    const Climate climate = climateOf(makeWestCoast());
    EXPECT_LT(climate.temperature[indexOf(30, 2)], climate.temperature[indexOf(30, 61)]);
}

TEST(Climate, MountainsAreColder) {
    Heightmap map = makeWestCoast();
    map.at(40, 32) = 0.9f;
    const Climate climate = climateOf(map);
    EXPECT_LT(climate.temperature[indexOf(40, 32)], climate.temperature[indexOf(20, 32)]);
}

TEST(Climate, CoastIsWetterThanTheInterior) {
    const Climate climate = climateOf(makeWestCoast());
    EXPECT_GT(climate.moisture[indexOf(6, 32)], climate.moisture[indexOf(60, 32)]);
}

TEST(Climate, MountainsCastARainShadow) {
    // The same coast with and without a north-south ridge: east of the ridge it is drier.
    const Heightmap flat = makeWestCoast();
    Heightmap ridge = makeWestCoast();
    for (int y = 0; y < kSize; ++y) {
        for (int x = 24; x < 30; ++x) {
            ridge.at(x, y) = 0.8f;
        }
    }
    const std::size_t behindTheRidge = indexOf(40, 32);
    EXPECT_LT(climateOf(ridge).moisture[behindTheRidge], climateOf(flat).moisture[behindTheRidge]);
}

TEST(Climate, SeaCellsAreSea) {
    const Climate climate = climateOf(makeWestCoast());
    EXPECT_EQ(climate.biome[indexOf(1, 10)], Biome::Sea);
    EXPECT_NE(climate.biome[indexOf(30, 10)], Biome::Sea);
}

TEST(Climate, WarmerSettingsGiveWarmerLand) {
    ClimateSettings cold;
    cold.northTemperature = 0.1f;
    cold.southTemperature = 0.1f;
    ClimateSettings hot = cold;
    hot.northTemperature = 0.9f;
    hot.southTemperature = 0.9f;
    const Heightmap map = makeWestCoast();
    EXPECT_EQ(climateOf(map, cold).biome[indexOf(30, 30)], Biome::Glacier);
    EXPECT_NE(climateOf(map, hot).biome[indexOf(30, 30)], Biome::Glacier);
}
