#include "core/Erosion.hpp"
#include "core/Heightmap.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numeric>

using tf::ErosionSettings;
using tf::Heightmap;

namespace {

constexpr int kSize = 64;

// A smooth mountain: 1 in the center, 0 at the corners.
Heightmap makeCone() {
    Heightmap map(kSize, kSize);
    const float center = 0.5f * static_cast<float>(kSize - 1);
    const float maxDistance = std::sqrt(2.0f) * center;
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float dx = static_cast<float>(x) - center;
            const float dy = static_cast<float>(y) - center;
            map.at(x, y) = 1.0f - std::sqrt(dx * dx + dy * dy) / maxDistance;
        }
    }
    return map;
}

ErosionSettings landOnly(int droplets = 5'000) {
    ErosionSettings settings;
    settings.droplets = droplets;
    settings.seaLevel = 0.0f;  // everything is land
    return settings;
}

double totalSoil(const Heightmap& map) {
    const auto values = map.values();
    return std::accumulate(values.begin(), values.end(), 0.0);
}

}  // namespace

TEST(Erosion, SameSeedGivesSameResult) {
    Heightmap a = makeCone();
    Heightmap b = makeCone();
    tf::erode(a, landOnly());
    tf::erode(b, landOnly());
    EXPECT_TRUE(std::ranges::equal(a.values(), b.values()));
}

TEST(Erosion, DifferentSeedsGiveDifferentResults) {
    Heightmap a = makeCone();
    Heightmap b = makeCone();
    ErosionSettings settings = landOnly();
    tf::erode(a, settings);
    settings.seed += 1;
    tf::erode(b, settings);
    EXPECT_FALSE(std::ranges::equal(a.values(), b.values()));
}

TEST(Erosion, HeightsStayInZeroToOne) {
    Heightmap map = makeCone();
    tf::erode(map, landOnly(20'000));
    const auto [lo, hi] = std::ranges::minmax(map.values());
    EXPECT_GE(lo, 0.0f);
    EXPECT_LE(hi, 1.0f);
}

TEST(Erosion, NeverCreatesSoil) {
    Heightmap map = makeCone();
    const double before = totalSoil(map);
    tf::erode(map, landOnly(20'000));
    EXPECT_LE(totalSoil(map), before + 1e-3);
}

TEST(Erosion, WashesSoilDownTheSlopes) {
    // Soil leaves the upper slopes and settles lower down or leaves the map.
    const Heightmap original = makeCone();
    Heightmap map = original;
    tf::erode(map, landOnly(20'000));

    double upperBefore = 0.0;
    double upperAfter = 0.0;
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            if (original.at(x, y) > 0.5f) {
                upperBefore += original.at(x, y);
                upperAfter += map.at(x, y);
            }
        }
    }
    EXPECT_LT(upperAfter, upperBefore);
}

TEST(Erosion, ZeroDropletsChangeNothing) {
    Heightmap map = makeCone();
    tf::erode(map, landOnly(0));
    EXPECT_TRUE(std::ranges::equal(map.values(), makeCone().values()));
}

TEST(Erosion, LeavesTheSeaAlone) {
    // Everything is below the sea level: drops land in the sea and stop at once.
    Heightmap map = makeCone();
    ErosionSettings settings = landOnly();
    settings.seaLevel = 1.1f;
    tf::erode(map, settings);
    EXPECT_TRUE(std::ranges::equal(map.values(), makeCone().values()));
}
