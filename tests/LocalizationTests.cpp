#include "core/Localization.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>

using tf::Language;
using tf::Localizer;
using tf::TextId;

namespace {

constexpr std::array kLanguages{Language::English, Language::Russian};

}  // namespace

TEST(Localization, EveryTextIsNonEmptyInEveryLanguage) {
    for (const Language language : kLanguages) {
        for (std::size_t i = 0; i < tf::kTextCount; ++i) {
            const char* text = tf::translate(static_cast<TextId>(i), language);
            ASSERT_NE(text, nullptr);
            EXPECT_GT(std::strlen(text), 0u) << "TextId #" << i;
        }
    }
}

TEST(Localization, ReturnsTheRequestedLanguage) {
    EXPECT_STREQ(tf::translate(TextId::MenuFile, Language::English), "File");
    EXPECT_STREQ(tf::translate(TextId::MenuFile, Language::Russian), "Файл");
}

TEST(Localization, MessagesWithAFileNameFormatInEveryLanguage) {
    // std::vformat throws std::format_error if a translation breaks the "{}" placeholder.
    const std::string fileName = "map_001.png";
    for (const Language language : kLanguages) {
        for (const TextId id : {TextId::ExportSaved, TextId::SaveFailed}) {
            const std::string message =
                std::vformat(tf::translate(id, language), std::make_format_args(fileName));
            EXPECT_NE(message.find(fileName), std::string::npos) << message;
        }
    }
}

TEST(Localization, LanguageNamesAreNative) {
    EXPECT_EQ(tf::languageName(Language::English), "English");
    EXPECT_EQ(tf::languageName(Language::Russian), "Русский");
}

TEST(Localization, LanguageCodesRoundTrip) {
    for (const Language language : kLanguages) {
        EXPECT_EQ(tf::languageFromCode(tf::languageCode(language)), language);
    }
    EXPECT_EQ(tf::languageFromCode("de"), std::nullopt);
    EXPECT_EQ(tf::languageFromCode(""), std::nullopt);
}

TEST(Localizer, SwitchesLanguage) {
    Localizer text;
    EXPECT_EQ(text.language(), Language::English);
    EXPECT_STREQ(text(TextId::ToolRaise), "Raise");

    text.setLanguage(Language::Russian);
    EXPECT_EQ(text.language(), Language::Russian);
    EXPECT_STREQ(text(TextId::ToolRaise), "Поднять");
}
