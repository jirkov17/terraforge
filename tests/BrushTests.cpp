#include "core/Brush.hpp"
#include "core/Heightmap.hpp"

#include <gtest/gtest.h>

using tf::BrushSettings;
using tf::BrushTool;
using tf::Heightmap;

namespace {

constexpr float kDt = 1.0f / 60.0f;

BrushSettings makeBrush(BrushTool tool, float radius = 5.0f, float strength = 1.0f) {
    BrushSettings brush;
    brush.tool = tool;
    brush.radius = radius;
    brush.strength = strength;
    return brush;
}

}  // namespace

TEST(BrushFalloff, IsOneInTheCenterAndZeroOnTheRim) {
    EXPECT_FLOAT_EQ(tf::brushFalloff(0.0f, 10.0f), 1.0f);
    EXPECT_FLOAT_EQ(tf::brushFalloff(10.0f, 10.0f), 0.0f);
    EXPECT_FLOAT_EQ(tf::brushFalloff(15.0f, 10.0f), 0.0f);
}

TEST(BrushFalloff, DecreasesFromCenterToRim) {
    float previous = tf::brushFalloff(0.0f, 10.0f);
    for (float d = 1.0f; d <= 10.0f; d += 1.0f) {
        const float current = tf::brushFalloff(d, 10.0f);
        EXPECT_LT(current, previous) << "at distance " << d;
        previous = current;
    }
}

TEST(Brush, RaiseLiftsTheCenterMoreThanTheEdge) {
    Heightmap map(21, 21, 0.5f);
    ASSERT_TRUE(tf::applyBrush(map, makeBrush(BrushTool::Raise), 10.5f, 10.5f, kDt));

    EXPECT_GT(map.at(10, 10), 0.5f);
    EXPECT_GT(map.at(10, 10), map.at(13, 10));
    EXPECT_FLOAT_EQ(map.at(0, 0), 0.5f);  // outside the radius: untouched
}

TEST(Brush, LowerDigsTheTerrain) {
    Heightmap map(21, 21, 0.5f);
    tf::applyBrush(map, makeBrush(BrushTool::Lower), 10.5f, 10.5f, kDt);
    EXPECT_LT(map.at(10, 10), 0.5f);
}

TEST(Brush, HeightsStayWithinZeroOne) {
    Heightmap map(21, 21, 0.99f);
    for (int frame = 0; frame < 600; ++frame) {
        tf::applyBrush(map, makeBrush(BrushTool::Raise, 5.0f, 2.0f), 10.5f, 10.5f, kDt);
    }
    EXPECT_FLOAT_EQ(map.at(10, 10), 1.0f);
}

TEST(Brush, OutsideTheMapChangesNothing) {
    Heightmap map(10, 10, 0.5f);
    EXPECT_FALSE(tf::applyBrush(map, makeBrush(BrushTool::Raise), -50.0f, -50.0f, kDt));
    for (float h : map.values()) {
        EXPECT_FLOAT_EQ(h, 0.5f);
    }
}

TEST(Brush, FlattenMovesTowardsTargetHeight) {
    Heightmap map(21, 21, 0.8f);
    BrushSettings brush = makeBrush(BrushTool::Flatten);
    brush.targetHeight = 0.3f;
    for (int frame = 0; frame < 120; ++frame) {
        tf::applyBrush(map, brush, 10.5f, 10.5f, kDt);
    }
    EXPECT_NEAR(map.at(10, 10), 0.3f, 0.01f);
}

TEST(Brush, SmoothFlattensASpike) {
    Heightmap map(21, 21, 0.2f);
    map.at(10, 10) = 1.0f;
    tf::applyBrush(map, makeBrush(BrushTool::Smooth), 10.5f, 10.5f, kDt);
    EXPECT_LT(map.at(10, 10), 1.0f);
    EXPECT_GT(map.at(11, 10), 0.2f);  // part of the spike spread to the neighbours
}
