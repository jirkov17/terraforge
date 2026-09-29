#pragma once

namespace tf {

class Heightmap;

// Hydraulic erosion with simulated raindrops (Hans Theobald Beyer, "Implementation of a method
// for hydraulic erosion", 2015). Every drop starts at a random point, rolls downhill, picks up
// soil where it runs fast and drops it where it slows down. Many drops carve valleys and
// river beds and leave sediment fans at their mouths.
struct ErosionSettings {
    int droplets = 70'000;
    int seed = 1;            // the same seed gives the same result
    float seaLevel = 0.45f;  // a drop that reaches the sea drops all its sediment there
    int radius = 3;          // soil is taken from a circle of this many cells: wider valleys
    float inertia = 0.05f;   // 0 = drops always follow the slope, 1 = never turn
    float capacity = 4.0f;   // how much sediment fast, heavy drops can carry
    float minCapacity = 0.01f;
    float erodeSpeed = 0.3f;    // fraction of free capacity filled with soil per step
    float depositSpeed = 0.3f;  // fraction of extra sediment dropped per step
    float evaporation = 0.01f;  // fraction of water lost per step
    float gravity = 4.0f;
    int maxSteps = 30;  // lifetime of a drop
};

// Erodes the heightmap in place. Heights stay in [0, 1]; soil is only moved or lost
// (a drop that evaporates or leaves the map loses its sediment), never created.
void erode(Heightmap& map, const ErosionSettings& settings);

}  // namespace tf
