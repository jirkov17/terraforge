#include "core/Geography.hpp"

#include "core/Heightmap.hpp"

namespace tf {

Geography analyzeGeography(const Heightmap& map, const GeographySettings& settings) {
    HydrologySettings hydrology;
    hydrology.seaLevel = settings.seaLevel;
    hydrology.riverThreshold = settings.riverThreshold;

    ClimateSettings climate;
    climate.seaLevel = settings.seaLevel;
    climate.northTemperature = settings.northTemperature;
    climate.southTemperature = settings.southTemperature;

    Geography geography;
    geography.hydrology = computeHydrology(map, hydrology);
    geography.climate = computeClimate(map, geography.hydrology, climate);

    MountainPassSettings passes;
    passes.seaLevel = settings.seaLevel;
    geography.passes = findMountainPasses(map, passes);
    return geography;
}

}  // namespace tf
