#include "core/MountainPasses.hpp"

#include "core/Heightmap.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <utility>

namespace tf {

namespace {

// Disjoint sets ("union-find"): which component every cell belongs to.
// Path compression + union by size make every operation almost O(1).
class UnionFind {
public:
    explicit UnionFind(std::size_t count) : m_parent(count), m_size(count, 1) {
        std::iota(m_parent.begin(), m_parent.end(), std::size_t{0});  // every cell alone
    }

    std::size_t find(std::size_t i) {
        while (m_parent[i] != i) {
            m_parent[i] = m_parent[m_parent[i]];  // path halving: skip every other step
            i = m_parent[i];
        }
        return i;
    }

    // Joins two roots; the smaller tree goes under the bigger one to keep the trees flat.
    void unite(std::size_t a, std::size_t b) {
        if (m_size[a] < m_size[b]) {
            std::swap(a, b);
        }
        m_parent[b] = a;
        m_size[a] += m_size[b];
    }

    [[nodiscard]] std::size_t size(std::size_t root) const { return m_size[root]; }

private:
    std::vector<std::size_t> m_parent;
    std::vector<std::size_t> m_size;
};

bool tooClose(const MountainPass& a, const MountainPass& b, int spacing) {
    const int dx = a.x - b.x;
    const int dy = a.y - b.y;
    return dx * dx + dy * dy < spacing * spacing;
}

}  // namespace

std::vector<MountainPass> findMountainPasses(const Heightmap& map,
                                             const MountainPassSettings& settings) {
    const int width = map.width();
    const auto heights = map.values();
    const std::size_t count = heights.size();
    const auto minValley =
        static_cast<std::size_t>(settings.minValleyFraction * static_cast<float>(count));
    const float minPassHeight = settings.seaLevel + settings.minAltitude;

    // Cells from the lowest to the highest. A projection sorts indices by their height
    // without writing a comparison lambda.
    std::vector<std::size_t> order(count);
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::sort(order, {}, [&](std::size_t i) { return heights[i]; });

    UnionFind components(count);
    std::vector<bool> added(count, false);
    std::vector<MountainPass> passes;

    for (const std::size_t i : order) {
        const int x = static_cast<int>(i % static_cast<std::size_t>(width));
        const int y = static_cast<int>(i / static_cast<std::size_t>(width));

        // The distinct components around this cell (4 neighbours, so at most 4).
        std::array<std::size_t, 4> roots{};
        std::size_t rootCount = 0;
        constexpr std::array<std::array<int, 2>, 4> kSteps{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
        for (const auto& [dx, dy] : kSteps) {
            const int nx = x + dx;
            const int ny = y + dy;
            if (!map.contains(nx, ny)) {
                continue;
            }
            const std::size_t n = static_cast<std::size_t>(ny) * static_cast<std::size_t>(width) +
                                  static_cast<std::size_t>(nx);
            if (!added[n]) {
                continue;  // higher than this cell: not flooded yet
            }
            const std::size_t root = components.find(n);
            if (std::find(roots.begin(), roots.begin() + static_cast<std::ptrdiff_t>(rootCount),
                          root) == roots.begin() + static_cast<std::ptrdiff_t>(rootCount)) {
                roots[rootCount++] = root;
            }
        }

        // Two or more big valleys meet here: the lowest crossing between them.
        const auto bigValleys =
            std::count_if(roots.begin(), roots.begin() + static_cast<std::ptrdiff_t>(rootCount),
                          [&](std::size_t root) { return components.size(root) >= minValley; });
        if (bigValleys >= 2 && heights[i] >= minPassHeight) {
            const MountainPass pass{x, y, heights[i]};
            const bool duplicate = std::ranges::any_of(passes, [&](const MountainPass& other) {
                return tooClose(pass, other, settings.minSpacing);
            });
            if (!duplicate) {
                passes.push_back(pass);
            }
        }

        added[i] = true;
        std::size_t root = components.find(i);
        for (std::size_t k = 0; k < rootCount; ++k) {
            const std::size_t other = components.find(roots[k]);
            if (other != root) {
                components.unite(root, other);
                root = components.find(i);
            }
        }
    }
    return passes;
}

}  // namespace tf
