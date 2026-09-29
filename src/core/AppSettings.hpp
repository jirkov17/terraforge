#pragma once

#include "core/Localization.hpp"

#include <filesystem>
#include <iosfwd>

namespace tf {

// Preferences of the application itself, not of a world: they survive restarts and are shared
// by all worlds. Stored as a small "key=value" text file (terraforge.ini).
struct AppSettings {
    Language language = Language::English;

    bool operator==(const AppSettings&) const = default;
};

// Reads "key=value" lines. Empty lines and lines starting with '#' are skipped; unknown keys,
// malformed lines and invalid values are ignored, so a hand-edited file never stops the app
// from starting: the affected settings just keep their defaults.
[[nodiscard]] AppSettings parseSettings(std::istream& input);
void writeSettings(const AppSettings& settings, std::ostream& output);

// File versions of the above. A missing or unreadable file gives the defaults.
[[nodiscard]] AppSettings loadSettings(const std::filesystem::path& path);
bool saveSettings(const AppSettings& settings, const std::filesystem::path& path);

}  // namespace tf
