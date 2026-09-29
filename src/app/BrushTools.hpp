#pragma once

#include "core/Brush.hpp"
#include "core/Localization.hpp"

#include <raylib.h>
#include <rlImGui.h>  // Font Awesome icon codes (ICON_FA_...)

#include <array>

namespace tf {

// Everything the UI needs to know about the editor tools, shared by App.cpp and AppUi.cpp.
// `inline constexpr`: one object for the whole program even though several .cpp files include it.

inline constexpr float kMinBrushRadius = 1.0f;  // in cells
inline constexpr float kMaxBrushRadius = 128.0f;

// Editor tools. The first four sculpt the heights; Land and Sea paint the shape mask
// of the continent, and the terrain is rebuilt from it. The core only knows BrushTool,
// so the shape tools are mapped onto Raise and Lower applied to the mask.
enum class Tool { Raise, Lower, Smooth, Flatten, Land, Sea };

// In toolbar order; the hotkey of kTools[i] is the digit i + 1.
inline constexpr std::array kTools{Tool::Raise,   Tool::Lower, Tool::Smooth,
                                   Tool::Flatten, Tool::Land,  Tool::Sea};

constexpr bool isShapeTool(Tool tool) noexcept {
    return tool == Tool::Land || tool == Tool::Sea;
}

constexpr BrushTool brushToolFor(Tool tool) noexcept {
    switch (tool) {
        case Tool::Raise:
        case Tool::Land:
            return BrushTool::Raise;
        case Tool::Lower:
        case Tool::Sea:
            return BrushTool::Lower;
        case Tool::Smooth:
            return BrushTool::Smooth;
        case Tool::Flatten:
            return BrushTool::Flatten;
    }
    return BrushTool::Raise;
}

// Shift swaps each tool with its opposite.
constexpr Tool oppositeTool(Tool tool) noexcept {
    switch (tool) {
        case Tool::Raise:
            return Tool::Lower;
        case Tool::Lower:
            return Tool::Raise;
        case Tool::Land:
            return Tool::Sea;
        case Tool::Sea:
            return Tool::Land;
        case Tool::Smooth:
        case Tool::Flatten:
            return tool;
    }
    return tool;
}

// Color of the brush cursor and of the tool icon.
constexpr Color toolColor(Tool tool) noexcept {
    switch (tool) {
        case Tool::Raise:
            return Color{120, 230, 120, 230};
        case Tool::Lower:
            return Color{240, 120, 110, 230};
        case Tool::Smooth:
            return Color{120, 190, 250, 230};
        case Tool::Flatten:
            return Color{245, 210, 110, 230};
        case Tool::Land:
            return Color{214, 190, 130, 230};
        case Tool::Sea:
            return Color{90, 150, 235, 230};
    }
    return WHITE;
}

constexpr const char* toolIcon(Tool tool) noexcept {
    switch (tool) {
        case Tool::Raise:
            return ICON_FA_ARROW_UP;
        case Tool::Lower:
            return ICON_FA_ARROW_DOWN;
        case Tool::Smooth:
            return ICON_FA_DROPLET;
        case Tool::Flatten:
            return ICON_FA_RULER_HORIZONTAL;
        case Tool::Land:
            return ICON_FA_MOUNTAIN;
        case Tool::Sea:
            return ICON_FA_WATER;
    }
    return "?";
}

constexpr TextId toolName(Tool tool) noexcept {
    switch (tool) {
        case Tool::Raise:
            return TextId::ToolRaise;
        case Tool::Lower:
            return TextId::ToolLower;
        case Tool::Smooth:
            return TextId::ToolSmooth;
        case Tool::Flatten:
            return TextId::ToolFlatten;
        case Tool::Land:
            return TextId::ToolLand;
        case Tool::Sea:
            return TextId::ToolSea;
    }
    return TextId::ToolRaise;
}

}  // namespace tf
