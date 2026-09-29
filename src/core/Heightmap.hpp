#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace tf {

// A rectangular grid of terrain heights stored row by row (index = y * width + x).
//
// Heights are normalized to [0, 1]: 0 is the deepest ocean floor, 1 is the highest peak.
// Whether a cell is land or water is decided by the sea level, which is a view setting
// and is deliberately not stored here.
class Heightmap {
public:
    Heightmap(int width, int height, float initialHeight = 0.0f);

    [[nodiscard]] int width() const noexcept { return m_width; }
    [[nodiscard]] int height() const noexcept { return m_height; }
    [[nodiscard]] bool contains(int x, int y) const noexcept;

    // Fast access without bounds checks (asserts in Debug builds).
    [[nodiscard]] float at(int x, int y) const;
    [[nodiscard]] float& at(int x, int y);

    // Safe access: coordinates outside the map are clamped to the nearest edge cell.
    [[nodiscard]] float atClamped(int x, int y) const noexcept;

    void fill(float value);

    // Stretches all values so that the lowest becomes 0 and the highest becomes 1.
    void normalize();

    [[nodiscard]] std::span<const float> values() const noexcept { return m_values; }
    [[nodiscard]] std::span<float> values() noexcept { return m_values; }

private:
    [[nodiscard]] std::size_t indexOf(int x, int y) const noexcept;

    int m_width;
    int m_height;
    std::vector<float> m_values;
};

// Blurs the grid with three box blur passes (close to a Gaussian blur). Each pass uses a running
// sum, so the cost does not depend on the radius. Cells outside the map repeat the edge cells.
void blurField(Heightmap& field, int radius);

}  // namespace tf
