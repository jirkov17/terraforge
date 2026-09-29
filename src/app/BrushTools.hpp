#pragma once

#include "core/Brush.hpp"
#include "core/Localization.hpp"

#include <raylib.h>
#include <rlImGui.h>  // Font Awesome icon codes (ICON_FA_...)

#include <array>

namespace tf {

// Everything the UI needs to know about the brush tools, shared by App.cpp and AppUi.cpp.
// `inline constexpr`: one object for the whole program even though several .cpp files include it.

inline constexpr float kMinBrushRadius = 1.0f;  // in cells
inline constexpr float kMaxBrushRadius = 128.0f;

// In toolbar order; the hotkey of kTools[i] is the digit i + 1.
inline constexpr std::array kTools{BrushTool::Raise, BrushTool::Lower, BrushTool::Smooth,
                                   BrushTool::Flatten};

// Color of the brush cursor and of the tool icon.
constexpr Color toolColor(BrushTool tool) noexcept {
    switch (tool) {
        case BrushTool::Raise:
            return Color{120, 230, 120, 230};
        case BrushTool::Lower:
            return Color{240, 120, 110, 230};
        case BrushTool::Smooth:
            return Color{120, 190, 250, 230};
        case BrushTool::Flatten:
            return Color{245, 210, 110, 230};
    }
    return WHITE;
}

constexpr const char* toolIcon(BrushTool tool) noexcept {
    switch (tool) {
        case BrushTool::Raise:
            return ICON_FA_ARROW_UP;
        case BrushTool::Lower:
            return ICON_FA_ARROW_DOWN;
        case BrushTool::Smooth:
            return ICON_FA_DROPLET;
        case BrushTool::Flatten:
            return ICON_FA_RULER_HORIZONTAL;
    }
    return "?";
}

constexpr TextId toolName(BrushTool tool) noexcept {
    switch (tool) {
        case BrushTool::Raise:
            return TextId::ToolRaise;
        case BrushTool::Lower:
            return TextId::ToolLower;
        case BrushTool::Smooth:
            return TextId::ToolSmooth;
        case BrushTool::Flatten:
            return TextId::ToolFlatten;
    }
    return TextId::ToolRaise;
}

}  // namespace tf
