#include "core/Erosion.hpp"

#include "core/Heightmap.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace tf {

namespace {

struct HeightAndGradient {
    float height;
    float gradientX;
    float gradientY;
};

// Height and slope at a point between cell centers, interpolated from the four corners.
// Valid for 0 <= x < width - 1 and 0 <= y < height - 1.
HeightAndGradient sample(const Heightmap& map, float x, float y) {
    const int cx = static_cast<int>(x);
    const int cy = static_cast<int>(y);
    const float u = x - static_cast<float>(cx);
    const float v = y - static_cast<float>(cy);

    const float nw = map.at(cx, cy);
    const float ne = map.at(cx + 1, cy);
    const float sw = map.at(cx, cy + 1);
    const float se = map.at(cx + 1, cy + 1);

    return {
        .height = nw * (1 - u) * (1 - v) + ne * u * (1 - v) + sw * (1 - u) * v + se * u * v,
        .gradientX = (ne - nw) * (1 - v) + (se - sw) * v,
        .gradientY = (sw - nw) * (1 - u) + (se - ne) * u,
    };
}

// Adds `amount` of soil at a point, split between the four surrounding cells by distance.
void deposit(Heightmap& map, float x, float y, float amount) {
    const int cx = static_cast<int>(x);
    const int cy = static_cast<int>(y);
    const float u = x - static_cast<float>(cx);
    const float v = y - static_cast<float>(cy);
    map.at(cx, cy) += amount * (1 - u) * (1 - v);
    map.at(cx + 1, cy) += amount * u * (1 - v);
    map.at(cx, cy + 1) += amount * (1 - u) * v;
    map.at(cx + 1, cy + 1) += amount * u * v;
}

struct BrushCell {
    int dx;
    int dy;
    float weight;
};

// Cells within `radius` of the center, closer cells weigh more. Computed once per erode() call.
std::vector<BrushCell> makeErosionBrush(int radius) {
    std::vector<BrushCell> cells;
    const float r = static_cast<float>(radius);
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (distance < r) {
                cells.push_back({dx, dy, r - distance});
            }
        }
    }
    return cells;
}

// Takes up to `amount` of soil from around cell (cx, cy) and returns how much was taken.
// Weights are renormalized near the map edges, where part of the brush is outside.
float erodeAround(Heightmap& map, const std::vector<BrushCell>& brush, int cx, int cy,
                  float amount) {
    float totalWeight = 0.0f;
    for (const BrushCell& cell : brush) {
        if (map.contains(cx + cell.dx, cy + cell.dy)) {
            totalWeight += cell.weight;
        }
    }
    float taken = 0.0f;
    for (const BrushCell& cell : brush) {
        const int x = cx + cell.dx;
        const int y = cy + cell.dy;
        if (!map.contains(x, y)) {
            continue;
        }
        float& h = map.at(x, y);
        const float share = std::min(h, amount * cell.weight / totalWeight);  // never below 0
        h -= share;
        taken += share;
    }
    return taken;
}

// A float in [0, 1) from the raw 32-bit output of mt19937. std::uniform_real_distribution
// is implemented differently by each standard library, so the same seed would give different
// maps on Windows and Linux; mt19937 itself is fully specified by the standard.
float random01(std::mt19937& rng) {
    constexpr float kTwoPow24 = 16'777'216.0f;
    return static_cast<float>(rng() >> 8) / kTwoPow24;  // top 24 bits fit a float exactly
}

}  // namespace

void erode(Heightmap& map, const ErosionSettings& settings) {
    const int width = map.width();
    const int height = map.height();
    if (width < 2 || height < 2 || settings.droplets <= 0) {
        return;
    }
    const float maxX = static_cast<float>(width - 1);
    const float maxY = static_cast<float>(height - 1);

    std::mt19937 rng(static_cast<std::uint32_t>(settings.seed));
    const std::vector<BrushCell> brush = makeErosionBrush(std::max(1, settings.radius));

    for (int drop = 0; drop < settings.droplets; ++drop) {
        float x = random01(rng) * maxX;
        float y = random01(rng) * maxY;
        float dirX = 0.0f;
        float dirY = 0.0f;
        float speed = 1.0f;
        float water = 1.0f;
        float sediment = 0.0f;

        for (int step = 0; step < settings.maxSteps; ++step) {
            const HeightAndGradient here = sample(map, x, y);
            if (here.height < settings.seaLevel) {
                deposit(map, x, y, sediment);  // the river mouth: everything settles
                break;
            }

            // New direction: mostly downhill, partly the old direction (inertia).
            dirX = dirX * settings.inertia - here.gradientX * (1.0f - settings.inertia);
            dirY = dirY * settings.inertia - here.gradientY * (1.0f - settings.inertia);
            const float length = std::sqrt(dirX * dirX + dirY * dirY);
            if (length < 1e-9f) {
                break;  // perfectly flat ground: the drop does not move
            }
            dirX /= length;
            dirY /= length;

            const float oldX = x;
            const float oldY = y;
            x += dirX;  // one cell per step
            y += dirY;
            if (x < 0.0f || y < 0.0f || x >= maxX || y >= maxY) {
                break;  // left the map, its sediment is lost
            }

            const float deltaHeight = sample(map, x, y).height - here.height;
            // Fast, big drops going steeply downhill can carry more.
            const float capacity =
                std::max(-deltaHeight * speed * water * settings.capacity, settings.minCapacity);

            if (sediment > capacity || deltaHeight > 0.0f) {
                // Uphill: fill the pit behind the drop (at most up to the new height).
                // Otherwise: drop part of the sediment it cannot carry.
                const float amount = deltaHeight > 0.0f
                                         ? std::min(deltaHeight, sediment)
                                         : (sediment - capacity) * settings.depositSpeed;
                sediment -= amount;
                deposit(map, oldX, oldY, amount);
            } else {
                // Take soil, but never more than the height difference: digging deeper than the
                // next point would create a pit.
                const float amount =
                    std::min((capacity - sediment) * settings.erodeSpeed, -deltaHeight);
                sediment +=
                    erodeAround(map, brush, static_cast<int>(oldX), static_cast<int>(oldY), amount);
            }

            speed = std::sqrt(std::max(0.0f, speed * speed + deltaHeight * settings.gravity));
            water *= 1.0f - settings.evaporation;
        }
    }

    for (float& h : map.values()) {
        h = std::clamp(h, 0.0f, 1.0f);
    }
}

}  // namespace tf
