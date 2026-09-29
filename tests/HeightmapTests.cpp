#include "core/Heightmap.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using tf::Heightmap;

TEST(Heightmap, RejectsNonPositiveSize) {
    EXPECT_THROW(Heightmap(0, 10), std::invalid_argument);
    EXPECT_THROW(Heightmap(10, -1), std::invalid_argument);
}

TEST(Heightmap, StartsFilledWithInitialHeight) {
    const Heightmap map(4, 3, 0.25f);
    EXPECT_EQ(map.width(), 4);
    EXPECT_EQ(map.height(), 3);
    EXPECT_EQ(map.values().size(), 12u);
    for (float h : map.values()) {
        EXPECT_FLOAT_EQ(h, 0.25f);
    }
}

TEST(Heightmap, ContainsOnlyCellsInsideTheGrid) {
    const Heightmap map(4, 3);
    EXPECT_TRUE(map.contains(0, 0));
    EXPECT_TRUE(map.contains(3, 2));
    EXPECT_FALSE(map.contains(4, 0));
    EXPECT_FALSE(map.contains(0, 3));
    EXPECT_FALSE(map.contains(-1, 0));
}

TEST(Heightmap, StoresValuesRowByRow) {
    Heightmap map(4, 3);
    map.at(1, 2) = 0.7f;
    EXPECT_FLOAT_EQ(map.values()[2 * 4 + 1], 0.7f);
}

TEST(Heightmap, AtClampedReturnsNearestEdgeCell) {
    Heightmap map(4, 3);
    map.at(0, 0) = 0.1f;
    map.at(3, 2) = 0.9f;
    EXPECT_FLOAT_EQ(map.atClamped(-5, -5), 0.1f);
    EXPECT_FLOAT_EQ(map.atClamped(100, 100), 0.9f);
}

TEST(Heightmap, NormalizeStretchesToZeroOne) {
    Heightmap map(3, 1);
    map.at(0, 0) = 2.0f;
    map.at(1, 0) = 3.0f;
    map.at(2, 0) = 4.0f;
    map.normalize();
    EXPECT_FLOAT_EQ(map.at(0, 0), 0.0f);
    EXPECT_FLOAT_EQ(map.at(1, 0), 0.5f);
    EXPECT_FLOAT_EQ(map.at(2, 0), 1.0f);
}

TEST(Heightmap, NormalizeOfFlatMapDoesNotDivideByZero) {
    Heightmap map(3, 3, 0.4f);
    map.normalize();
    for (float h : map.values()) {
        EXPECT_FLOAT_EQ(h, 0.0f);
    }
}

TEST(BlurField, KeepsAFlatFieldFlat) {
    Heightmap map(16, 8, 0.3f);
    tf::blurField(map, 3);
    for (float h : map.values()) {
        EXPECT_NEAR(h, 0.3f, 1e-5f);
    }
}

TEST(BlurField, RadiusZeroChangesNothing) {
    Heightmap map(4, 4);
    map.at(1, 1) = 1.0f;
    tf::blurField(map, 0);
    EXPECT_FLOAT_EQ(map.at(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(map.at(2, 2), 0.0f);
}

TEST(BlurField, TurnsAStepIntoASmoothRamp) {
    // Left half 0, right half 1.
    Heightmap map(32, 4);
    for (int y = 0; y < 4; ++y) {
        for (int x = 16; x < 32; ++x) {
            map.at(x, y) = 1.0f;
        }
    }
    tf::blurField(map, 2);
    // Far from the step nothing changes, next to it the values climb monotonically.
    EXPECT_NEAR(map.at(0, 0), 0.0f, 1e-5f);
    EXPECT_NEAR(map.at(31, 0), 1.0f, 1e-5f);
    EXPECT_GT(map.at(15, 0), 0.0f);
    EXPECT_LT(map.at(16, 0), 1.0f);
    for (int x = 1; x < 32; ++x) {
        EXPECT_GE(map.at(x, 0), map.at(x - 1, 0) - 1e-6f);
    }
}
