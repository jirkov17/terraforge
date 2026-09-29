#pragma once

#include <array>

namespace tf {

class Heightmap;

// Ready-made shapes of the world. A shape mask says where land should be (1) and where
// the sea should be (0); the terrain generator adds mountains and a ragged coast on top.
enum class ShapePreset { Continent, Archipelago, TwoContinents, InlandSea, Ocean };

inline constexpr std::array kShapePresets{ShapePreset::Continent, ShapePreset::Archipelago,
                                          ShapePreset::TwoContinents, ShapePreset::InlandSea,
                                          ShapePreset::Ocean};

// Fills the mask with the preset shape, values in [0, 1]. The seed bends the outlines a little,
// so every world looks different; the same seed always gives the same mask.
// Ocean is all zeros: an empty canvas for drawing a continent by hand.
void makeShapeMask(Heightmap& mask, ShapePreset preset, int seed);

}  // namespace tf
