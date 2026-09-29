#include "core/Heightmap.hpp"
#include "core/Hydrology.hpp"
#include "core/TerrainGenerator.hpp"

#include <gtest/gtest.h>

#include <cmath>

using tf::Heightmap;
using tf::Hydrology;
using tf::HydrologySettings;

namespace {

constexpr int kSize = 32;

std::size_t indexOf(const Heightmap& map, int x, int y) {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(map.width()) +
           static_cast<std::size_t>(x);
}

HydrologySettings landOnly() {
    HydrologySettings settings;
    settings.seaLevel = 0.0f;  // no sea: water can only leave through the map edges
    return settings;
}

// A plateau at 0.6 with a round pit (0.5) in the middle.
Heightmap makePlateauWithPit() {
    Heightmap map(kSize, kSize, 0.6f);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float dx = static_cast<float>(x - kSize / 2);
            const float dy = static_cast<float>(y - kSize / 2);
            if (dx * dx + dy * dy < 25.0f) {
                map.at(x, y) = 0.5f;
            }
        }
    }
    return map;
}

// Slopes down from west (1.0) to east (0.2); the sea level 0.3 floods the eastern strip.
Heightmap makeSlopeToTheSea() {
    Heightmap map(kSize, kSize);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            map.at(x, y) = 1.0f - 0.8f * static_cast<float>(x) / static_cast<float>(kSize - 1);
        }
    }
    return map;
}

}  // namespace

TEST(Hydrology, FillsAPitWithALake) {
    const Heightmap map = makePlateauWithPit();
    const Hydrology water = tf::computeHydrology(map, landOnly());

    const std::size_t center = indexOf(map, kSize / 2, kSize / 2);
    EXPECT_TRUE(water.isLake(center, map.at(kSize / 2, kSize / 2)));
    // The lake is filled up to the rim of the pit.
    EXPECT_NEAR(water.waterLevel[center], 0.6f, 0.01f);
    // The plateau around it is dry.
    EXPECT_FALSE(water.isLake(indexOf(map, 2, 2), map.at(2, 2)));
}

TEST(Hydrology, FlowGrowsDownhill) {
    const Heightmap map = makeSlopeToTheSea();
    HydrologySettings settings;
    settings.seaLevel = 0.3f;
    const Hydrology water = tf::computeHydrology(map, settings);

    const int row = kSize / 2;
    EXPECT_LT(water.flow[indexOf(map, 5, row)], water.flow[indexOf(map, 15, row)]);
    EXPECT_LT(water.flow[indexOf(map, 15, row)], water.flow[indexOf(map, 25, row)]);
}

TEST(Hydrology, TheSeaIsNeverARiverOrALake) {
    const Heightmap map = makeSlopeToTheSea();
    HydrologySettings settings;
    settings.seaLevel = 0.3f;
    settings.riverThreshold = 1.0f;  // every land cell would be a river
    const Hydrology water = tf::computeHydrology(map, settings);

    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const std::size_t i = indexOf(map, x, y);
            if (map.at(x, y) < settings.seaLevel) {
                EXPECT_TRUE(water.sea[i]);
                EXPECT_FALSE(water.isRiver(i, map.at(x, y)));
                EXPECT_FALSE(water.isLake(i, map.at(x, y)));
            } else {
                EXPECT_TRUE(water.isRiver(i, map.at(x, y)));
            }
        }
    }
}

TEST(Hydrology, EveryLandCellCanDrain) {
    // After filling, each inland cell has a strictly lower neighbour: no water gets stuck.
    Heightmap map(64, 64);
    tf::generateTerrain(map, tf::GeneratorSettings{});
    HydrologySettings settings;
    const Hydrology water = tf::computeHydrology(map, settings);

    for (int y = 1; y < 63; ++y) {
        for (int x = 1; x < 63; ++x) {
            const std::size_t i = indexOf(map, x, y);
            if (water.sea[i]) {
                continue;
            }
            bool hasLowerNeighbour = false;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if ((dx != 0 || dy != 0) &&
                        water.waterLevel[indexOf(map, x + dx, y + dy)] < water.waterLevel[i]) {
                        hasLowerNeighbour = true;
                    }
                }
            }
            EXPECT_TRUE(hasLowerNeighbour) << "stuck water at " << x << ", " << y;
        }
    }
}

TEST(Hydrology, WaterLevelIsNeverBelowTheTerrain) {
    Heightmap map(64, 64);
    tf::generateTerrain(map, tf::GeneratorSettings{});
    const Hydrology water = tf::computeHydrology(map, HydrologySettings{});
    for (std::size_t i = 0; i < water.waterLevel.size(); ++i) {
        EXPECT_GE(water.waterLevel[i], map.values()[i]);
    }
}
