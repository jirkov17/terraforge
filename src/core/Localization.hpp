#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

namespace tf {

enum class Language { English, Russian };

// Every piece of translatable UI text. When you add an id, add its translations to the table
// in Localization.cpp at the same position: a compile-time check fails otherwise.
enum class TextId {
    // Main menu
    MenuFile,
    MenuGenerate,
    MenuExportPng,
    MenuExit,
    MenuView,
    MenuFitToWindow,
    MenuHillshade,
    MenuCoastline,
    MenuLanguage,
    MenuHelp,
    MenuControls,
    MenuImGuiDemo,

    // Brush tools
    ToolRaise,
    ToolLower,
    ToolSmooth,
    ToolFlatten,

    // Properties panel
    PanelProperties,
    SectionBrush,
    BrushRadius,
    BrushStrength,
    BrushTargetHeight,
    SectionMapView,
    SeaLevel,
    HillshadeStrength,

    // Terrain generation window
    WindowGenerator,
    Seed,
    RandomSeedTooltip,
    LandSize,
    LandSizeTooltip,
    Island,
    IslandTooltip,
    Advanced,
    Detail,
    DetailTooltip,
    Roughness,
    RoughnessTooltip,
    ManualEditsWarning,
    Generate,
    GenerateDiscardEdits,

    // Status bar
    StatusCell,
    StatusHeight,
    StatusWater,
    StatusLand,
    StatusOutsideMap,
    StatusZoom,

    // Messages. "{}" is replaced with a file name (std::vformat).
    ExportSaved,
    SaveFailed,
    ExportTooMany,

    // Controls help
    HelpPaint,
    HelpSwapRaiseLower,
    HelpPan,
    HelpZoom,
    HelpKeys,

    Count  // not a text: the number of ids
};

inline constexpr std::size_t kTextCount = static_cast<std::size_t>(TextId::Count);

// The language's own name ("English", "Русский"): shown untranslated in the language menu,
// so people can find their language whatever language is active now.
[[nodiscard]] std::string_view languageName(Language language) noexcept;

// Short code for settings files: "en", "ru". languageFromCode() returns nothing for unknown codes.
[[nodiscard]] std::string_view languageCode(Language language) noexcept;
[[nodiscard]] std::optional<Language> languageFromCode(std::string_view code) noexcept;

// UI text in the given language, UTF-8. Points to a string literal: null-terminated and valid
// for the whole program, so it can be passed straight to ImGui.
[[nodiscard]] const char* translate(TextId id, Language language) noexcept;

// The active UI language. Call it like a function: text(TextId::MenuFile).
class Localizer {
public:
    explicit Localizer(Language language = Language::English) noexcept : m_language(language) {}

    void setLanguage(Language language) noexcept { m_language = language; }
    [[nodiscard]] Language language() const noexcept { return m_language; }

    [[nodiscard]] const char* operator()(TextId id) const noexcept {
        return translate(id, m_language);
    }

private:
    Language m_language;
};

}  // namespace tf
