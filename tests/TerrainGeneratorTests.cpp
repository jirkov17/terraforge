#include "core/Heightmap.hpp"
#include "core/TerrainGenerator.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using tf::GeneratorSettings;
using tf::Heightmap;
using tf::ShapePreset;

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

TEST(TerrainGenerator, NoiseCoversZeroToOne) {
    Heightmap noise(64, 64);
    tf::generateNoise(noise, GeneratorSettings{});
    const auto [lo, hi] = std::ranges::minmax(noise.values());
    EXPECT_FLOAT_EQ(lo, 0.0f);
    EXPECT_FLOAT_EQ(hi, 1.0f);
}

TEST(TerrainGenerator, HeightsStayInZeroToOneAndUseMostOfTheRange) {
    Heightmap map(64, 64);
    tf::generateTerrain(map, GeneratorSettings{});
    const auto [lo, hi] = std::ranges::minmax(map.values());
    EXPECT_GE(lo, 0.0f);
    EXPECT_LE(hi, 1.0f);
    EXPECT_GT(hi - lo, 0.5f);
}

TEST(TerrainGenerator, ContinentShapeKeepsEdgesLowerThanCenter) {
    Heightmap map(64, 64);
    GeneratorSettings settings;
    settings.shapeStrength = 1.0f;
    tf::generateTerrain(map, settings);
    EXPECT_LT(map.at(0, 0), map.at(32, 32));
    EXPECT_LT(map.at(63, 32), map.at(32, 32));
}

TEST(TerrainGenerator, ShapeDecidesLandAndSea) {
    // Left half painted as land, right half as sea: with the default strength the left side
    // must be above the default sea level (0.45) and the right side below it.
    constexpr float kSeaLevel = 0.45f;
    Heightmap noise(128, 64);
    Heightmap shape(128, 64);
    Heightmap map(128, 64);
    tf::generateNoise(noise, GeneratorSettings{});
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            shape.at(x, y) = 1.0f;
        }
    }
    tf::combineTerrain(map, noise, shape, GeneratorSettings{}.shapeStrength);

    for (int y = 0; y < 64; ++y) {
        EXPECT_GT(map.at(10, y), kSeaLevel);   // deep inside the painted land
        EXPECT_LT(map.at(117, y), kSeaLevel);  // deep inside the painted sea
    }
}

TEST(TerrainGenerator, EmptyOceanHasNoLandAtDefaultStrength) {
    Heightmap map(64, 64);
    GeneratorSettings settings;
    settings.shape = ShapePreset::Ocean;
    tf::generateTerrain(map, settings);
    EXPECT_LT(std::ranges::max(map.values()), 0.45f + 1e-4f);
}

TEST(TerrainGenerator, CombineRejectsDifferentSizes) {
    Heightmap map(64, 64);
    Heightmap noise(64, 64);
    Heightmap shape(32, 32);
    EXPECT_THROW(tf::combineTerrain(map, noise, shape, 0.5f), std::invalid_argument);
}
