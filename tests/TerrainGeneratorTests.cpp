#include "core/Heightmap.hpp"
#include "core/TerrainGenerator.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using tf::GeneratorSettings;
using tf::Heightmap;

TEST(TerrainGenerator, SameSeedGivesSameMap) {
    Heightmap a(64, 64);
    Heightmap b(64, 64);
    tf::generateTerrain(a, GeneratorSettings{});
    tf::generateTerrain(b, GeneratorSettings{});
    EXPECT_TRUE(std::ranges::equal(a.values(), b.values()));
}

TEST(TerrainGenerator, DifferentSeedsGiveDifferentMaps) {
    Heightmap a(64, 64);
    Heightmap b(64, 64);
    GeneratorSettings settings;
    tf::generateTerrain(a, settings);
    settings.seed += 1;
    tf::generateTerrain(b, settings);
    EXPECT_FALSE(std::ranges::equal(a.values(), b.values()));
}

TEST(TerrainGenerator, HeightsCoverZeroToOne) {
    Heightmap map(64, 64);
    tf::generateTerrain(map, GeneratorSettings{});
    const auto [lo, hi] = std::ranges::minmax(map.values());
    EXPECT_FLOAT_EQ(lo, 0.0f);
    EXPECT_FLOAT_EQ(hi, 1.0f);
}

TEST(TerrainGenerator, IslandModeKeepsEdgesLowerThanCenter) {
    Heightmap map(64, 64);
    GeneratorSettings settings;
    settings.islandStrength = 1.0f;
    tf::generateTerrain(map, settings);
    EXPECT_LT(map.at(0, 0), map.at(32, 32));
    EXPECT_LT(map.at(63, 32), map.at(32, 32));
}
