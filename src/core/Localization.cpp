#include "core/Localization.hpp"

#include <array>
#include <cassert>
#include <span>

namespace tf {

namespace {

// Both languages live in one row, so a text cannot be added without its translation.
// Plain "..." literals are UTF-8 (MSVC builds with /utf-8, GCC and Clang default to it).
// u8"..." would be const char8_t[] in C++20, which ImGui does not accept.
struct Translation {
    TextId id;
    std::string_view english;
    std::string_view russian;
};

constexpr auto kTranslations = std::to_array<Translation>({
    // Main menu
    {TextId::MenuFile, "File", "Файл"},
    {TextId::MenuGenerate, "Generate terrain...", "Генерация рельефа..."},
    {TextId::MenuGeography, "Geography...", "География..."},
    {TextId::MenuExportPng, "Export PNG", "Экспорт в PNG"},
    {TextId::MenuExit, "Exit", "Выход"},
    {TextId::MenuView, "View", "Вид"},
    {TextId::MenuFitToWindow, "Fit map to window", "Вписать карту в окно"},
    {TextId::MenuHillshade, "Hillshade", "Отмывка рельефа"},
    {TextId::MenuCoastline, "Coastline", "Береговая линия"},
    {TextId::MenuRivers, "Rivers and lakes", "Реки и озёра"},
    {TextId::MenuPasses, "Mountain passes", "Горные перевалы"},
    {TextId::MenuReliefMap, "Relief map", "Карта рельефа"},
    {TextId::MenuBiomeMap, "Biome map", "Карта биомов"},
    {TextId::MenuLanguage, "Language", "Язык"},
    {TextId::MenuHelp, "Help", "Справка"},
    {TextId::MenuControls, "Controls", "Управление"},
    {TextId::MenuImGuiDemo, "ImGui demo", "Демо ImGui"},

    // Brush tools
    {TextId::ToolRaise, "Raise", "Поднять"},
    {TextId::ToolLower, "Lower", "Опустить"},
    {TextId::ToolSmooth, "Smooth", "Сгладить"},
    {TextId::ToolFlatten, "Flatten", "Выровнять"},
    {TextId::ToolLand, "Land", "Суша"},
    {TextId::ToolSea, "Sea", "Море"},
    {TextId::ShapeToolHint,
     "Draws the shape of the continent. Hand edits of the relief are replaced.",
     "Рисует форму континента. Ручные правки рельефа заменяются."},

    // Properties panel
    {TextId::PanelProperties, "Properties", "Свойства"},
    {TextId::SectionBrush, "Brush", "Кисть"},
    {TextId::BrushRadius, "Radius", "Радиус"},
    {TextId::BrushStrength, "Strength", "Сила"},
    {TextId::BrushTargetHeight, "Target height", "Целевая высота"},
    {TextId::SectionMapView, "Map view", "Вид карты"},
    {TextId::SeaLevel, "Sea level", "Уровень моря"},
    {TextId::HillshadeStrength, "Hillshade", "Отмывка"},

    // Terrain generation window
    {TextId::WindowGenerator, "Terrain generation", "Генерация рельефа"},
    {TextId::Seed, "Seed", "Сид"},
    {TextId::RandomSeedTooltip, "Random seed", "Случайный сид"},
    {TextId::LandSize, "Land size", "Размер суши"},
    {TextId::LandSizeTooltip, "Higher = more and smaller islands",
     "Больше = больше островов, но мельче"},
    {TextId::Shape, "Shape", "Форма"},
    {TextId::ShapePresetTooltip, "Choosing a shape replaces the one drawn by hand",
     "Выбор формы заменяет нарисованную вручную"},
    {TextId::ShapeContinent, "Continent", "Континент"},
    {TextId::ShapeArchipelago, "Archipelago", "Архипелаг"},
    {TextId::ShapeTwoContinents, "Two continents", "Два материка"},
    {TextId::ShapeInlandSea, "Inland sea", "Внутреннее море"},
    {TextId::ShapeOcean, "Empty ocean (draw your own)", "Пустой океан (нарисуй сам)"},
    {TextId::ShapeStrength, "Shape strength", "Сила формы"},
    {TextId::ShapeStrengthTooltip,
     "0 = only noise, 1 = exactly the shape. Coasts are ragged in between.",
     "0 = только шум, 1 = точно по форме. Между ними берега изрезанные."},
    {TextId::ShapePainted, "The shape was drawn by hand.", "Форма нарисована вручную."},
    {TextId::ResetShape, "Reset to the preset", "Вернуть заготовку"},
    {TextId::Advanced, "Advanced", "Дополнительно"},
    {TextId::Detail, "Detail", "Детализация"},
    {TextId::DetailTooltip, "Number of noise octaves", "Число октав шума"},
    {TextId::Roughness, "Roughness", "Шероховатость"},
    {TextId::RoughnessTooltip, "How strong the small details are",
     "Насколько заметны мелкие детали"},
    {TextId::ManualEditsWarning, "Map was edited by hand: auto-regeneration is paused.",
     "Карта изменена вручную: автогенерация на паузе."},
    {TextId::Generate, "Generate", "Сгенерировать"},
    {TextId::GenerateDiscardEdits, "Generate (discard edits)", "Сгенерировать (сбросить правки)"},

    // Geography window
    {TextId::WindowGeography, "Geography", "География"},
    {TextId::SectionErosion, "Erosion", "Эрозия"},
    {TextId::ErosionDroplets, "Raindrops", "Капли дождя"},
    {TextId::ErosionDropletsTooltip, "Each drop washes soil downhill. More drops = deeper valleys.",
     "Каждая капля смывает грунт вниз по склону. Больше капель — глубже долины."},
    {TextId::Erode, "Erode the terrain", "Размыть рельеф"},
    {TextId::RiverThreshold, "River threshold", "Порог рек"},
    {TextId::RiverThresholdTooltip,
     "How many cells must drain through a point for a river to appear there. Lower = more rivers.",
     "Сколько клеток должно стекать через точку, чтобы там появилась река. Меньше — больше рек."},
    {TextId::SectionClimate, "Climate", "Климат"},
    {TextId::NorthTemperature, "Warmth in the north", "Тепло на севере"},
    {TextId::SouthTemperature, "Warmth in the south", "Тепло на юге"},
    {TextId::TemperatureTooltip,
     "0 = eternal ice, 1 = scorching heat. Mountains are always colder.",
     "0 = вечные льды, 1 = зной. В горах всегда холоднее."},

    // Biomes
    {TextId::BiomeSea, "sea", "море"},
    {TextId::BiomeGlacier, "glacier", "ледник"},
    {TextId::BiomeTundra, "tundra", "тундра"},
    {TextId::BiomeTaiga, "taiga", "тайга"},
    {TextId::BiomeForest, "forest", "лес"},
    {TextId::BiomeMeadow, "meadows", "луга"},
    {TextId::BiomeSteppe, "steppe", "степь"},
    {TextId::BiomeDesert, "desert", "пустыня"},
    {TextId::BiomeSwamp, "swamp", "болото"},
    {TextId::BiomeMountains, "mountains", "горы"},

    // Status bar
    {TextId::StatusCell, "Cell", "Клетка"},
    {TextId::StatusHeight, "height", "высота"},
    {TextId::StatusWater, "water", "вода"},
    {TextId::StatusLand, "land", "суша"},
    {TextId::StatusLake, "lake", "озеро"},
    {TextId::StatusRiver, "river", "река"},
    {TextId::StatusPass, "mountain pass", "горный перевал"},
    {TextId::StatusOutsideMap, "Cursor is outside the map", "Курсор за пределами карты"},
    {TextId::StatusZoom, "Zoom", "Масштаб"},

    // Messages
    {TextId::ExportSaved, "Saved {} (in the working folder)", "Сохранено: {} (в рабочей папке)"},
    {TextId::SaveFailed, "Could not save {}", "Не удалось сохранить {}"},
    {TextId::ExportTooMany, "Too many exported maps, clean up the folder",
     "Слишком много экспортированных карт, почистите папку"},
    {TextId::ManualEditsReplaced, "Hand edits of the relief were replaced by the new shape",
     "Ручные правки рельефа заменены новой формой"},
    {TextId::ErosionDone, "Erosion: {} drops in {} ms", "Эрозия: {} капель за {} мс"},

    // Controls help
    {TextId::HelpPaint, "LMB: paint with the brush", "ЛКМ: рисовать кистью"},
    {TextId::HelpSwapRaiseLower, "Shift + LMB: Raise <-> Lower",
     "Shift + ЛКМ: поднять <-> опустить"},
    {TextId::HelpPan, "RMB / MMB drag: move the map", "ПКМ / СКМ: двигать карту"},
    {TextId::HelpZoom, "Wheel: zoom, Ctrl + wheel: brush size",
     "Колесо: масштаб, Ctrl + колесо: размер кисти"},
    {TextId::HelpKeys, "1-6: tools, [ ]: brush size, F: fit to window, M: relief / biomes",
     "1-6: инструменты, [ ]: размер кисти, F: вписать в окно, M: рельеф / биомы"},
});

// Checks at compile time that the table can be indexed directly by TextId.
consteval bool isValidTable(std::span<const Translation> table) {
    if (table.size() != kTextCount) {
        return false;  // a TextId has no row, or a row has no TextId
    }
    for (std::size_t i = 0; i < table.size(); ++i) {
        const Translation& row = table[i];
        if (static_cast<std::size_t>(row.id) != i) {
            return false;  // rows are out of enum order: lookup by index would return wrong text
        }
        if (row.english.empty() || row.russian.empty()) {
            return false;
        }
    }
    return true;
}

static_assert(isValidTable(kTranslations),
              "kTranslations must have one row per TextId, in enum order, with both texts");

}  // namespace

std::string_view languageName(Language language) noexcept {
    switch (language) {
        case Language::English:
            return "English";
        case Language::Russian:
            return "Русский";
    }
    return "?";
}

std::string_view languageCode(Language language) noexcept {
    switch (language) {
        case Language::English:
            return "en";
        case Language::Russian:
            return "ru";
    }
    return "en";
}

std::optional<Language> languageFromCode(std::string_view code) noexcept {
    for (const Language language : {Language::English, Language::Russian}) {
        if (code == languageCode(language)) {
            return language;
        }
    }
    return std::nullopt;
}

const char* translate(TextId id, Language language) noexcept {
    // Safe thanks to the static_assert above: row i describes TextId i.
    assert(id < TextId::Count);
    const Translation& row = kTranslations[static_cast<std::size_t>(id)];
    const std::string_view text = language == Language::Russian ? row.russian : row.english;
    return text.data();
}

}  // namespace tf
