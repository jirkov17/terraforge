#include "core/Heightmap.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace tf {

Heightmap::Heightmap(int width, int height, float initialHeight)
    : m_width(width), m_height(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Heightmap size must be positive");
    }
    m_values.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
                    initialHeight);
}

bool Heightmap::contains(int x, int y) const noexcept {
    return x >= 0 && y >= 0 && x < m_width && y < m_height;
}

float Heightmap::at(int x, int y) const {
    assert(contains(x, y));
    return m_values[indexOf(x, y)];
}

float& Heightmap::at(int x, int y) {
    assert(contains(x, y));
    return m_values[indexOf(x, y)];
}

float Heightmap::atClamped(int x, int y) const noexcept {
    x = std::clamp(x, 0, m_width - 1);
    y = std::clamp(y, 0, m_height - 1);
    return m_values[indexOf(x, y)];
}

void Heightmap::fill(float value) {
    std::ranges::fill(m_values, value);
}

void Heightmap::normalize() {
    const auto [minIt, maxIt] = std::ranges::minmax_element(m_values);
    const float lo = *minIt;
    const float range = *maxIt - lo;
    if (range <= 0.0f) {
        fill(0.0f);  // a perfectly flat map: nothing to stretch
        return;
    }
    for (float& h : m_values) {
        h = (h - lo) / range;
    }
}

std::size_t Heightmap::indexOf(int x, int y) const noexcept {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) +
           static_cast<std::size_t>(x);
}

namespace {

// out[i] = average of in[i - radius .. i + radius], indices clamped to the line.
// The window sum is updated by adding the value that enters and removing the one that leaves.
void boxBlurLine(std::span<const float> in, std::span<float> out, int radius) {
    const int n = static_cast<int>(in.size());
    const auto sample = [&](int i) {
        return in[static_cast<std::size_t>(std::clamp(i, 0, n - 1))];
    };

    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        sum += sample(i);
    }
    const float scale = 1.0f / static_cast<float>(2 * radius + 1);
    for (int i = 0; i < n; ++i) {
        out[static_cast<std::size_t>(i)] = sum * scale;
        sum += sample(i + radius + 1) - sample(i - radius);
    }
}

}  // namespace

void blurField(Heightmap& field, int radius) {
    if (radius <= 0) {
        return;
    }
    const int width = field.width();
    const int height = field.height();
    const auto longest = static_cast<std::size_t>(std::max(width, height));
    std::vector<float> in(longest);
    std::vector<float> out(longest);
    const std::span<const float> inRow(in.data(), static_cast<std::size_t>(width));
    const std::span<float> outRow(out.data(), static_cast<std::size_t>(width));
    const std::span<const float> inColumn(in.data(), static_cast<std::size_t>(height));
    const std::span<float> outColumn(out.data(), static_cast<std::size_t>(height));

    // A blur along x and then along y equals a 2D box blur (the box filter is separable).
    for (int pass = 0; pass < 3; ++pass) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                in[static_cast<std::size_t>(x)] = field.at(x, y);
            }
            boxBlurLine(inRow, outRow, radius);
            for (int x = 0; x < width; ++x) {
                field.at(x, y) = out[static_cast<std::size_t>(x)];
            }
        }
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                in[static_cast<std::size_t>(y)] = field.at(x, y);
            }
            boxBlurLine(inColumn, outColumn, radius);
            for (int y = 0; y < height; ++y) {
                field.at(x, y) = out[static_cast<std::size_t>(y)];
            }
        }
    }
}

}  // namespace tf
