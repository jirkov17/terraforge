#include "core/Brush.hpp"

#include "core/Heightmap.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace tf {

namespace {

// Smooth and Flatten blend towards a target instead of adding a fixed amount.
// This constant turns "strength" into a blend rate that feels similar to Raise/Lower.
constexpr float kBlendRate = 12.0f;

float neighbourhoodAverage(const Heightmap& map, int x, int y) {
    float sum = 0.0f;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            sum += map.atClamped(x + dx, y + dy);
        }
    }
    return sum / 9.0f;
}

}  // namespace

float brushFalloff(float distance, float radius) noexcept {
    if (radius <= 0.0f || distance >= radius) {
        return 0.0f;
    }
    const float t = distance / radius;  // 0 in the center, 1 on the rim
    const float s = 1.0f - t * t;
    return s * s;  // smooth bump without a visible edge
}

bool applyBrush(Heightmap& map, const BrushSettings& brush, float centerX, float centerY,
                float dt) {
    if (brush.radius <= 0.0f || dt <= 0.0f) {
        return false;
    }

    // Only the cells inside the brush's bounding box can change.
    const int x0 = std::max(0, static_cast<int>(std::floor(centerX - brush.radius)));
    const int y0 = std::max(0, static_cast<int>(std::floor(centerY - brush.radius)));
    const int x1 = std::min(map.width() - 1, static_cast<int>(std::floor(centerX + brush.radius)));
    const int y1 = std::min(map.height() - 1, static_cast<int>(std::floor(centerY + brush.radius)));
    if (x0 > x1 || y0 > y1) {
        return false;  // the brush is completely outside the map
    }

    // Smoothing must read the heights from *before* this frame; otherwise already smoothed
    // cells would affect their neighbours and the result would depend on the loop order.
    std::optional<Heightmap> before;
    if (brush.tool == BrushTool::Smooth) {
        before = map;
    }

    bool touched = false;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - centerX;
            const float dy = static_cast<float>(y) + 0.5f - centerY;
            const float weight = brushFalloff(std::sqrt(dx * dx + dy * dy), brush.radius);
            if (weight <= 0.0f) {
                continue;
            }
            touched = true;

            float& h = map.at(x, y);
            // Exponential approach: frame-rate independent and never overshoots the target.
            const float blend = 1.0f - std::exp(-kBlendRate * brush.strength * weight * dt);

            switch (brush.tool) {
                case BrushTool::Raise:
                    h += brush.strength * weight * dt;
                    break;
                case BrushTool::Lower:
                    h -= brush.strength * weight * dt;
                    break;
                case BrushTool::Smooth:
                    h = std::lerp(h, neighbourhoodAverage(*before, x, y), blend);
                    break;
                case BrushTool::Flatten:
                    h = std::lerp(h, brush.targetHeight, blend);
                    break;
            }
            h = std::clamp(h, 0.0f, 1.0f);
        }
    }
    return touched;
}

std::string_view toString(BrushTool tool) noexcept {
    switch (tool) {
        case BrushTool::Raise:
            return "Raise";
        case BrushTool::Lower:
            return "Lower";
        case BrushTool::Smooth:
            return "Smooth";
        case BrushTool::Flatten:
            return "Flatten";
    }
    return "Unknown";
}

}  // namespace tf
