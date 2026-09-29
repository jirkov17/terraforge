// Timings of the heavy algorithms on a full-size map. Disabled by default (ctest skips them).
// Run by hand in Release, from build/bin/Release:
//   terraforge_tests --gtest_also_run_disabled_tests --gtest_filter=Benchmark.*

#include "core/ContinentShape.hpp"
#include "core/Heightmap.hpp"
#include "core/TerrainGenerator.hpp"

#include <gtest/gtest.h>

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
