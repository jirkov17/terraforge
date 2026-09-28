#pragma once

#include <string_view>

namespace tf {

class Heightmap;

enum class BrushTool { Raise, Lower, Smooth, Flatten };

struct BrushSettings {
    BrushTool tool = BrushTool::Raise;
    float radius = 24.0f;       // in cells
    float strength = 0.35f;     // how fast the brush works (height units per second at the center)
    float targetHeight = 0.5f;  // Flatten pulls terrain towards this height
};

// Applies the brush for one frame, centered at (centerX, centerY) in cell coordinates.
// Cell (x, y) covers the square [x, x + 1) x [y, y + 1).
// dt is the frame time in seconds, so the result does not depend on FPS.
// Returns true if the brush touched at least one cell of the map.
bool applyBrush(Heightmap& map, const BrushSettings& brush, float centerX, float centerY, float dt);

// Brush weight at `distance` from the center: 1 in the center, smoothly falling to 0 at `radius`.
[[nodiscard]] float brushFalloff(float distance, float radius) noexcept;

[[nodiscard]] std::string_view toString(BrushTool tool) noexcept;

}  // namespace tf
