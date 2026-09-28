#include "core/Heightmap.hpp"
#include "core/MapColorizer.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

using tf::ColorizeSettings;
using tf::Heightmap;
using tf::Rgba8;

TEST(MapColorizer, WaterIsBlueAndLandIsNot) {
    const Rgba8 water = tf::terrainColor(0.2f, 0.5f);
    const Rgba8 land = tf::terrainColor(0.6f, 0.5f);
    EXPECT_GT(water.b, water.r);
    EXPECT_GT(land.g, land.b);
}

TEST(MapColorizer, DeepWaterIsDarkerThanShallowWater) {
    const Rgba8 shallow = tf::terrainColor(0.49f, 0.5f);
    const Rgba8 deep = tf::terrainColor(0.0f, 0.5f);
    EXPECT_LT(deep.r + deep.g + deep.b, shallow.r + shallow.g + shallow.b);
}

TEST(MapColorizer, RejectsBufferOfWrongSize) {
    const Heightmap map(4, 4);
    std::vector<Rgba8> pixels(15);
    EXPECT_THROW(tf::colorize(map, ColorizeSettings{}, pixels), std::invalid_argument);
}

TEST(MapColorizer, WritesOnePixelPerCell) {
    Heightmap map(2, 1);
    map.at(0, 0) = 0.1f;  // water
    map.at(1, 0) = 0.9f;  // land
    ColorizeSettings settings;
    settings.seaLevel = 0.5f;
    settings.hillshade = false;
    settings.coastline = false;

    std::vector<Rgba8> pixels(2);
    tf::colorize(map, settings, pixels);
    EXPECT_EQ(pixels[0], tf::terrainColor(0.1f, 0.5f));
    EXPECT_EQ(pixels[1], tf::terrainColor(0.9f, 0.5f));
}
