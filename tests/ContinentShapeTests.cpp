#include "core/ContinentShape.hpp"
#include "core/Heightmap.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using tf::Heightmap;
using tf::ShapePreset;

namespace {

constexpr int kSize = 128;
constexpr int kSeed = 42;

Heightmap makeMask(ShapePreset preset, int seed = kSeed) {
    Heightmap mask(kSize, kSize);
    tf::makeShapeMask(mask, preset, seed);
    return mask;
}

float landFraction(const Heightmap& mask) {
    const auto land = std::ranges::count_if(mask.values(), [](float v) { return v > 0.5f; });
    return static_cast<float>(land) / static_cast<float>(mask.values().size());
}

}  // namespace

TEST(ContinentShape, EveryPresetStaysInZeroToOneAndIsDeterministic) {
    for (const ShapePreset preset : tf::kShapePresets) {
        const Heightmap a = makeMask(preset);
        const Heightmap b = makeMask(preset);
        EXPECT_TRUE(std::ranges::equal(a.values(), b.values()));
        const auto [lo, hi] = std::ranges::minmax(a.values());
        EXPECT_GE(lo, 0.0f);
        EXPECT_LE(hi, 1.0f);
    }
}

TEST(ContinentShape, OceanIsEmpty) {
    EXPECT_EQ(std::ranges::max(makeMask(ShapePreset::Ocean).values()), 0.0f);
}

TEST(ContinentShape, ContinentIsLandInTheCenterAndSeaAtTheEdges) {
    const Heightmap mask = makeMask(ShapePreset::Continent);
    EXPECT_GT(mask.at(kSize / 2, kSize / 2), 0.8f);
    EXPECT_LT(mask.at(0, 0), 0.1f);
}

TEST(ContinentShape, TwoContinentsHaveAStraitInTheMiddle) {
    const Heightmap mask = makeMask(ShapePreset::TwoContinents);
    const int middle = kSize / 2;
    const float strait = mask.at(middle, middle);
    EXPECT_LT(strait, mask.at(kSize / 4, middle));      // west continent
    EXPECT_LT(strait, mask.at(3 * kSize / 4, middle));  // east continent
    EXPECT_LT(strait, 0.5f);
}

TEST(ContinentShape, InlandSeaIsSurroundedByLand) {
    const Heightmap mask = makeMask(ShapePreset::InlandSea);
    const int middle = kSize / 2;
    EXPECT_LT(mask.at(middle, middle), 0.5f);             // the sea in the center
    EXPECT_GT(mask.at(middle, kSize / 8 + 4), 0.5f);      // land to the north of it
    EXPECT_GT(mask.at(middle, 7 * kSize / 8 - 4), 0.5f);  // land to the south of it
}

TEST(ContinentShape, ArchipelagoIsMostlySeaWithSomeLand) {
    const float land = landFraction(makeMask(ShapePreset::Archipelago));
    EXPECT_GT(land, 0.03f);
    EXPECT_LT(land, 0.5f);
}

TEST(ContinentShape, SeedBendsTheOutline) {
    const Heightmap a = makeMask(ShapePreset::Continent, 1);
    const Heightmap b = makeMask(ShapePreset::Continent, 2);
    EXPECT_FALSE(std::ranges::equal(a.values(), b.values()));
}
