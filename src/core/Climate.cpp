#include "core/Climate.hpp"

#include "core/Heightmap.hpp"
#include "core/Hydrology.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace tf {

namespace {

constexpr int kUnreached = -1;
constexpr float kFlatSlope = 0.004f;       // flatter ground can turn into a swamp
constexpr float kLakeEvaporation = 0.02f;  // humidity the wind picks up per lake cell crossed

// Distance in cells (8 neighbours) from every cell to the nearest cell where isSource(i) holds.
// Multi-source breadth-first search: all sources start at distance 0, and the first time the
// search reaches a cell is along a shortest path. O(n). A template instead of std::function:
// the predicate is a lambda, and the compiler can inline it into the loop.
template <typename IsSource>
std::vector<int> distanceFrom(const Heightmap& map, IsSource&& isSource) {
    const std::size_t count = map.values().size();
    const auto width = static_cast<std::size_t>(map.width());

    std::vector<int> distance(count, kUnreached);
    // A plain vector works as the FIFO queue: every cell is added once, `head` walks forward.
    std::vector<std::size_t> queue;
    queue.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        if (isSource(i)) {
            distance[i] = 0;
            queue.push_back(i);
        }
    }
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const std::size_t i = queue[head];
        const int x = static_cast<int>(i % width);
        const int y = static_cast<int>(i / width);
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = x + dx;
                const int ny = y + dy;
                if (!map.contains(nx, ny)) {
                    continue;
                }
                const std::size_t n =
                    static_cast<std::size_t>(ny) * width + static_cast<std::size_t>(nx);
                if (distance[n] == kUnreached) {
                    distance[n] = distance[i] + 1;
                    queue.push_back(n);
                }
            }
        }
    }
    return distance;
}

// Humidity of the westerly wind at every cell. Over the sea the air is full of water;
// over land it slowly dries out, and every climb forces rain out of it, so the land behind
// a mountain range stays dry (rain shadow). Lakes give a little water back.
std::vector<float> windHumidity(const Heightmap& map, const Hydrology& water, float rainShadow) {
    // Small bumps should not stop the wind, only mountain ranges: use a blurred terrain.
    Heightmap smooth = map;
    blurField(smooth, std::max(2, std::max(map.width(), map.height()) / 64));

    // Without any climb the air is dry after crossing half of the map.
    const float drying = 1.0f / (0.5f * static_cast<float>(map.width()));
    std::vector<float> humidity(map.values().size(), 0.0f);
    std::size_t i = 0;
    for (int y = 0; y < map.height(); ++y) {
        float air = 1.0f;
        for (int x = 0; x < map.width(); ++x, ++i) {
            if (water.sea[i]) {
                air = 1.0f;
            } else {
                const float climb =
                    x > 0 ? std::max(0.0f, smooth.at(x, y) - smooth.at(x - 1, y)) : 0.0f;
                air = std::max(0.0f, air - rainShadow * climb - drying);
                if (water.isLake(i, map.at(x, y))) {
                    air = std::min(1.0f, air + kLakeEvaporation);  // big lakes give more
                }
            }
            humidity[i] = air;
        }
    }
    return humidity;
}

}  // namespace

Biome classifyBiome(float temperature, float moisture, float altitude, float slope) noexcept {
    if (temperature < 0.12f) {
        return Biome::Glacier;
    }
    if (altitude > 0.34f) {
        return temperature < 0.3f ? Biome::Glacier : Biome::Mountains;
    }
    if (moisture > 0.72f && altitude < 0.05f && slope < kFlatSlope && temperature > 0.3f) {
        return Biome::Swamp;  // wet, low and flat: water has nowhere to go
    }
    // A simplified Whittaker diagram: temperature bands, split by moisture.
    if (temperature < 0.25f) {
        return Biome::Tundra;
    }
    if (temperature < 0.45f) {
        return moisture < 0.25f ? Biome::Steppe : Biome::Taiga;
    }
    if (temperature < 0.65f) {
        if (moisture < 0.3f) {
            return Biome::Steppe;
        }
        return moisture < 0.5f ? Biome::Meadow : Biome::Forest;
    }
    if (moisture < 0.38f) {
        return Biome::Desert;
    }
    return moisture < 0.55f ? Biome::Steppe : Biome::Forest;
}

Climate computeClimate(const Heightmap& map, const Hydrology& water,
                       const ClimateSettings& settings) {
    const int width = map.width();
    const int height = map.height();
    const std::size_t count = map.values().size();

    Climate climate;
    climate.width = width;
    climate.height = height;
    climate.temperature.resize(count);
    climate.moisture.resize(count);
    climate.biome.resize(count);

    const auto heights = map.values();
    const std::vector<int> toSea = distanceFrom(map, [&](std::size_t i) { return water.sea[i]; });
    const std::vector<int> toFreshWater = distanceFrom(map, [&](std::size_t i) {
        return water.isLake(i, heights[i]) || water.isRiver(i, heights[i]);
    });
    const std::vector<float> humidity = windHumidity(map, water, settings.rainShadow);
    const float reach =
        std::max(1.0f, settings.moistureReach * static_cast<float>(std::max(width, height)));
    // exp(-distance / reach): 1 next to the water, fading with distance, 0 if there is none.
    const auto closeness = [](int distance, float fadeDistance) {
        return distance == kUnreached ? 0.0f
                                      : std::exp(-static_cast<float>(distance) / fadeDistance);
    };

    std::size_t i = 0;
    for (int y = 0; y < height; ++y) {
        const float latitude =
            height > 1 ? static_cast<float>(y) / static_cast<float>(height - 1) : 0.5f;
        const float seaLevelWarmth =
            std::lerp(settings.northTemperature, settings.southTemperature, latitude);
        for (int x = 0; x < width; ++x, ++i) {
            const float h = map.at(x, y);
            const float altitude = h - settings.seaLevel;

            const float temperature = std::clamp(
                seaLevelWarmth - settings.lapseRate * std::max(0.0f, altitude), 0.0f, 1.0f);
            // The sea matters most; rivers and lakes only wet a narrow strip along them.
            const float moisture = std::clamp(0.3f * closeness(toSea[i], reach) +
                                                  0.15f * closeness(toFreshWater[i], 0.5f * reach) +
                                                  0.55f * humidity[i],
                                              0.0f, 1.0f);

            climate.temperature[i] = temperature;
            climate.moisture[i] = moisture;
            if (water.sea[i]) {
                climate.biome[i] = Biome::Sea;
            } else {
                const float dzdx = 0.5f * (map.atClamped(x + 1, y) - map.atClamped(x - 1, y));
                const float dzdy = 0.5f * (map.atClamped(x, y + 1) - map.atClamped(x, y - 1));
                const float slope = std::max(std::abs(dzdx), std::abs(dzdy));
                climate.biome[i] = classifyBiome(temperature, moisture, altitude, slope);
            }
        }
    }
    return climate;
}

}  // namespace tf
