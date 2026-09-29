add_executable(terraforge_tests
    AppSettingsTests.cpp
    Benchmarks.cpp
    BrushTests.cpp
    ClimateTests.cpp
    ContinentShapeTests.cpp
    ErosionTests.cpp
    HeightmapTests.cpp
    HydrologyTests.cpp
    LocalizationTests.cpp
    MapColorizerTests.cpp
    TerrainGeneratorTests.cpp)
target_link_libraries(terraforge_tests PRIVATE terraforge_core GTest::gtest_main)
terraforge_set_warnings(terraforge_tests)

include(GoogleTest)
# PRE_TEST: list the tests when ctest runs, not right after the build
# (more robust on Windows, where an antivirus scan can slow down the first launch).
gtest_discover_tests(terraforge_tests DISCOVERY_MODE PRE_TEST)
