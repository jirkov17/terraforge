// Timings of the heavy algorithms on a full-size map. Disabled by default (ctest skips them).
// Run by hand in Release, from build/bin/Release:
//   terraforge_tests --gtest_also_run_disabled_tests --gtest_filter=Benchmark.*

#include "core/ContinentShape.hpp"
#include "core/Erosion.hpp"
#include "core/Geography.hpp"
#include "core/Heightmap.hpp"
#include "core/TerrainGenerator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>

using tf::GeneratorSettings;
using tf::Heightmap;

namespace {

constexpr int kMapSize = 512;  // same as the editor

// Runs `work` and prints how long it took.
template <typename Work>
void measure(const char* name, Work&& work) {
    const auto start = std::chrono::steady_clock::now();
    work();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    std::printf("  %-28s %8.2f ms\n", name,
                std::chrono::duration<double, std::milli>(elapsed).count());
}

}  // namespace

TEST(Benchmark, DISABLED_TerrainGeneration) {
    Heightmap noise(kMapSize, kMapSize);
    Heightmap shape(kMapSize, kMapSize);
    Heightmap map(kMapSize, kMapSize);
    const GeneratorSettings settings;

    measure("generateNoise", [&] { tf::generateNoise(noise, settings); });
    measure("makeShapeMask", [&] { tf::makeShapeMask(shape, settings.shape, settings.seed); });
    measure("combineTerrain (per frame)",
            [&] { tf::combineTerrain(map, noise, shape, settings.shapeStrength); });
}

TEST(Benchmark, DISABLED_Erosion) {
    Heightmap map(kMapSize, kMapSize);
    tf::generateTerrain(map, GeneratorSettings{});
    const tf::ErosionSettings settings;
    measure("erode (70k drops)", [&] { tf::erode(map, settings); });
}

TEST(Benchmark, DISABLED_Geography) {
    Heightmap map(kMapSize, kMapSize);
    tf::generateTerrain(map, GeneratorSettings{});
    tf::Geography geography;
    measure("analyzeGeography",
            [&] { geography = tf::analyzeGeography(map, tf::GeographySettings{}); });

    // What the defaults produce, to tune them without looking at the map.
    const tf::Hydrology& water = geography.hydrology;
    int land = 0;
    int lakes = 0;
    int rivers = 0;
    for (std::size_t i = 0; i < water.flow.size(); ++i) {
        const float h = map.values()[i];
        land += water.sea[i] ? 0 : 1;
        lakes += water.isLake(i, h) ? 1 : 0;
        rivers += water.isRiver(i, h) ? 1 : 0;
    }
    std::printf("  land %d cells, lakes %d, rivers %d, mountain passes %zu\n", land, lakes, rivers,
                geography.passes.size());

    // Share of each biome on land (Sea, Glacier, Tundra, ... in enum order), for the default
    // climate and for a hot south.
    const auto printBiomes = [&](const char* name, const tf::Geography& result) {
        std::array<int, 10> biomes{};
        for (const tf::Biome biome : result.climate.biome) {
            ++biomes[static_cast<std::size_t>(biome)];
        }
        std::printf("  %-8s", name);
        for (std::size_t b = 1; b < biomes.size(); ++b) {
            std::printf(" %5.1f", 100.0 * biomes[b] / std::max(1, land));
        }
        std::printf("\n");
    };
    std::printf("  %% land:  glac  tund  taig  frst  mead  step  dsrt  swmp  mnts\n");
    printBiomes("default", geography);
    tf::GeographySettings hotSouth;
    hotSouth.southTemperature = 1.0f;
    printBiomes("hot", tf::analyzeGeography(map, hotSouth));
}
