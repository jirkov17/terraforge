#include "core/AppSettings.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>

using tf::AppSettings;
using tf::Language;

namespace {

AppSettings parse(const char* text) {
    std::istringstream input(text);
    return tf::parseSettings(input);
}

}  // namespace

TEST(AppSettings, DefaultsToEnglish) {
    EXPECT_EQ(AppSettings{}.language, Language::English);
}

TEST(AppSettings, WriteThenParseGivesTheSameSettings) {
    AppSettings settings;
    settings.language = Language::Russian;

    std::stringstream stream;
    tf::writeSettings(settings, stream);
    EXPECT_EQ(tf::parseSettings(stream), settings);
}

TEST(AppSettings, ReadsLanguage) {
    EXPECT_EQ(parse("language=ru\n").language, Language::Russian);
    EXPECT_EQ(parse("language=en\n").language, Language::English);
}

TEST(AppSettings, IgnoresSpacesCommentsAndWindowsLineEndings) {
    EXPECT_EQ(parse("# comment\n\n  language =  ru  \r\n").language, Language::Russian);
}

TEST(AppSettings, KeepsDefaultsForGarbageUnknownKeysAndInvalidValues) {
    const AppSettings settings = parse(
        "this is not a setting\n"
        "=ru\n"
        "theme=dark\n"
        "language=klingon\n");
    EXPECT_EQ(settings, AppSettings{});
}

TEST(AppSettings, EmptyInputGivesDefaults) {
    EXPECT_EQ(parse(""), AppSettings{});
}

TEST(AppSettings, MissingFileGivesDefaults) {
    const auto path = std::filesystem::temp_directory_path() / "terraforge_no_such_file.ini";
    std::filesystem::remove(path);
    EXPECT_EQ(tf::loadSettings(path), AppSettings{});
}

TEST(AppSettings, SaveThenLoadFileGivesTheSameSettings) {
    const auto path = std::filesystem::temp_directory_path() / "terraforge_settings_test.ini";
    AppSettings settings;
    settings.language = Language::Russian;

    ASSERT_TRUE(tf::saveSettings(settings, path));
    EXPECT_EQ(tf::loadSettings(path), settings);
    std::filesystem::remove(path);
}
