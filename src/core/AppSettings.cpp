#include "core/AppSettings.hpp"

#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

namespace tf {

namespace {

constexpr std::string_view kLanguageKey = "language";

std::string_view trim(std::string_view text) noexcept {
    constexpr std::string_view kSpaces = " \t\r";  // '\r': files saved with Windows line endings
    const std::size_t first = text.find_first_not_of(kSpaces);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kSpaces);
    return text.substr(first, last - first + 1);
}

}  // namespace

AppSettings parseSettings(std::istream& input) {
    AppSettings settings;
    std::string line;
    while (std::getline(input, line)) {
        const std::string_view text = trim(line);
        if (text.empty() || text.front() == '#') {
            continue;
        }
        const std::size_t equals = text.find('=');
        if (equals == std::string_view::npos) {
            continue;  // not a "key=value" line
        }
        const std::string_view key = trim(text.substr(0, equals));
        const std::string_view value = trim(text.substr(equals + 1));

        if (key == kLanguageKey) {
            if (const auto language = languageFromCode(value)) {
                settings.language = *language;
            }
        }
    }
    return settings;
}

void writeSettings(const AppSettings& settings, std::ostream& output) {
    output << "# Terraforge settings\n";
    output << kLanguageKey << '=' << languageCode(settings.language) << '\n';
}

AppSettings loadSettings(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }
    return parseSettings(file);
}

bool saveSettings(const AppSettings& settings, const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file) {
        return false;
    }
    writeSettings(settings, file);
    return static_cast<bool>(file);  // false if a write failed, e.g. the disk is full
}

}  // namespace tf
