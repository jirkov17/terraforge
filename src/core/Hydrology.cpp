#include "core/Hydrology.hpp"

#include "core/Heightmap.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <queue>
#include <utility>

namespace tf {

namespace {

// Every filled cell is at least this much higher than the cell it was reached from, so the
// filled surface has no perfectly flat areas and water always has somewhere to go.
constexpr float kEpsilon = 1e-5f;

struct Offset {
    int dx;
    int dy;
    float distance;
};

constexpr float kDiagonal = 1.41421356f;
constexpr std::array<Offset, 8> kNeighbours{{
    {-1, -1, kDiagonal},
    {0, -1, 1.0f},
    {1, -1, kDiagonal},
    {-1, 0, 1.0f},
    {1, 0, 1.0f},
    {-1, 1, kDiagonal},
    {0, 1, 1.0f},
    {1, 1, kDiagonal},
}};

constexpr std::size_t kNoReceiver = static_cast<std::size_t>(-1);

}  // namespace

Hydrology computeHydrology(const Heightmap& map, const HydrologySettings& settings) {
    const int width = map.width();
    const int height = map.height();
    const auto heights = map.values();
    const std::size_t count = heights.size();
    const auto index = [width](int x, int y) {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
               static_cast<std::size_t>(x);
    };

    Hydrology result;
    result.width = width;
    result.height = height;
    result.settings = settings;
    result.waterLevel.assign(heights.begin(), heights.end());
    result.flow.assign(count, 0.0f);
    result.sea.assign(count, false);

    // --- Priority-Flood -------------------------------------------------------------------
    // Min-heap of (water level, cell). std::priority_queue is a max-heap by default, so
    // std::greater turns it around; pairs compare by level first.
    using Entry = std::pair<float, std::size_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    std::vector<bool> closed(count, false);

    // Water can leave the land through the sea and through the map edges: start from there.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t i = index(x, y);
            const bool isSea = heights[i] < settings.seaLevel;
            const bool isEdge = x == 0 || y == 0 || x == width - 1 || y == height - 1;
            result.sea[i] = isSea;
            if (isSea || isEdge) {
                closed[i] = true;
                open.emplace(heights[i], i);
            }
        }
    }

    // Cells in the order they left the queue: non-decreasing water level.
    std::vector<std::size_t> order;
    order.reserve(count);
    while (!open.empty()) {
        const auto [level, i] = open.top();
        open.pop();
        order.push_back(i);

        const int x = static_cast<int>(i % static_cast<std::size_t>(width));
        const int y = static_cast<int>(i / static_cast<std::size_t>(width));
        for (const Offset& offset : kNeighbours) {
            const int nx = x + offset.dx;
            const int ny = y + offset.dy;
            if (!map.contains(nx, ny)) {
                continue;
            }
            const std::size_t n = index(nx, ny);
            if (closed[n]) {
                continue;
            }
            closed[n] = true;
            // A cell lower than the cell we came from is inside a pit: raise its water to the
            // pit's rim, where it would spill over.
            result.waterLevel[n] = std::max(heights[n], level + kEpsilon);
            open.emplace(result.waterLevel[n], n);
        }
    }

    // --- Flow accumulation ----------------------------------------------------------------
    // Rain: one unit on every land cell. Each cell passes everything it collected to its
    // steepest downhill neighbour (D8). Walking `order` backwards visits the highest cells
    // first, so every cell is complete before it is passed on: its receiver is strictly
    // lower and therefore left the queue earlier.
    for (std::size_t i = 0; i < count; ++i) {
        result.flow[i] = result.sea[i] ? 0.0f : 1.0f;
    }
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        const std::size_t i = *it;
        if (result.sea[i]) {
            continue;  // the sea is where rivers end
        }
        const int x = static_cast<int>(i % static_cast<std::size_t>(width));
        const int y = static_cast<int>(i / static_cast<std::size_t>(width));

        std::size_t receiver = kNoReceiver;
        float steepest = 0.0f;
        for (const Offset& offset : kNeighbours) {
            const int nx = x + offset.dx;
            const int ny = y + offset.dy;
            if (!map.contains(nx, ny)) {
                continue;
            }
            const std::size_t n = index(nx, ny);
            const float slope = (result.waterLevel[i] - result.waterLevel[n]) / offset.distance;
            if (slope > steepest) {
                steepest = slope;
                receiver = n;
            }
        }
        if (receiver != kNoReceiver) {
            result.flow[receiver] += result.flow[i];
        }
        // No receiver: a land cell on the map edge, the water leaves the map.
    }

    return result;
}

}  // namespace tf
