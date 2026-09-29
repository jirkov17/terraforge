#include "core/Heightmap.hpp"
#include "core/MountainPasses.hpp"
#include "core/TerrainGenerator.hpp"

#include <gtest/gtest.h>

using tf::Heightmap;
using tf::MountainPass;
using tf::MountainPassSettings;

namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 32;
constexpr int kRidgeLeft = 30;
constexpr int kRidgeRight = 34;
constexpr int kNotchY = 16;

// Two valleys (west and east) divided by a north-south ridge with one notch in it.
Heightmap makeTwoValleys(float valley, float ridge, float notch) {
    Heightmap map(kWidth, kHeight, valley);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = kRidgeLeft; x <= kRidgeRight; ++x) {
            map.at(x, y) = y == kNotchY ? notch : ridge;
        }
    }
    return map;
}

MountainPassSettings noSea() {
    MountainPassSettings settings;
    settings.seaLevel = 0.0f;
    settings.minAltitude = 0.7f;         // passes must be at least 0.7 high
    settings.minValleyFraction = 0.05f;  // about 100 cells
    return settings;
}

}  // namespace

TEST(MountainPasses, FindsTheNotchBetweenTwoValleys) {
    const Heightmap map = makeTwoValleys(0.5f, 0.95f, 0.8f);
    const std::vector<MountainPass> passes = tf::findMountainPasses(map, noSea());

    ASSERT_EQ(passes.size(), 1u);
    EXPECT_EQ(passes[0].y, kNotchY);
    EXPECT_GE(passes[0].x, kRidgeLeft);
    EXPECT_LE(passes[0].x, kRidgeRight);
    EXPECT_FLOAT_EQ(passes[0].height, 0.8f);
}

TEST(MountainPasses, LowSaddlesAreNotMountainPasses) {
    // The same shape, but only hills: the notch is below the mountain threshold.
    const Heightmap map = makeTwoValleys(0.3f, 0.6f, 0.5f);
    EXPECT_TRUE(tf::findMountainPasses(map, noSea()).empty());
}

TEST(MountainPasses, SmallDipsOnTheRidgeAreIgnored) {
    Heightmap map = makeTwoValleys(0.5f, 0.95f, 0.8f);
    // A tiny hollow on the ridge top, far from the notch.
    map.at(32, 4) = 0.9f;
    map.at(32, 5) = 0.9f;
    const std::vector<MountainPass> passes = tf::findMountainPasses(map, noSea());
    ASSERT_EQ(passes.size(), 1u);
    EXPECT_EQ(passes[0].y, kNotchY);
}

TEST(MountainPasses, OneValleyHasNoPasses) {
    const Heightmap map(kWidth, kHeight, 0.8f);
    EXPECT_TRUE(tf::findMountainPasses(map, noSea()).empty());
}

TEST(MountainPasses, PassesOnARealMapAreHighAndApart) {
    Heightmap map(256, 256);
    tf::generateTerrain(map, tf::GeneratorSettings{});
    const MountainPassSettings settings;
    const std::vector<MountainPass> passes = tf::findMountainPasses(map, settings);

    EXPECT_FALSE(passes.empty()) << "the default settings find no passes on a default map";
    for (std::size_t a = 0; a < passes.size(); ++a) {
        EXPECT_GE(passes[a].height, settings.seaLevel + settings.minAltitude);
        EXPECT_FLOAT_EQ(passes[a].height, map.at(passes[a].x, passes[a].y));
        for (std::size_t b = a + 1; b < passes.size(); ++b) {
            const int dx = passes[a].x - passes[b].x;
            const int dy = passes[a].y - passes[b].y;
            EXPECT_GE(dx * dx + dy * dy, settings.minSpacing * settings.minSpacing);
        }
    }
}
