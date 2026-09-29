#include "app/MapRenderer.hpp"

#include "core/Heightmap.hpp"

#include <stdexcept>

namespace tf {

// We upload our own pixel buffer to raylib, so both types must look the same in memory.
static_assert(sizeof(Rgba8) == sizeof(Color), "Rgba8 must match raylib's Color layout");

namespace {

// A raylib Image that points at our buffer (it does not own the memory, never unload it).
Image wrapPixels(std::vector<Rgba8>& pixels, int width, int height) {
    return Image{pixels.data(), width, height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
}

}  // namespace

MapRenderer::MapRenderer(int width, int height)
    : m_width(width),
      m_height(height),
      m_pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
    m_texture = LoadTextureFromImage(wrapPixels(m_pixels, width, height));
    if (!IsTextureValid(m_texture)) {
        throw std::runtime_error("Could not create the map texture");
    }
    SetTextureFilter(m_texture, TEXTURE_FILTER_BILINEAR);
}

MapRenderer::~MapRenderer() {
    UnloadTexture(m_texture);
}

void MapRenderer::update(const Heightmap& map, const ColorizeSettings& settings,
                         const Geography* geography) {
    if (map.width() != m_width || map.height() != m_height) {
        throw std::invalid_argument("MapRenderer: heightmap size does not match the texture");
    }
    colorize(map, settings, m_pixels, geography);
    UpdateTexture(m_texture, m_pixels.data());
}

void MapRenderer::draw() const {
    DrawTexture(m_texture, 0, 0, WHITE);
}

bool MapRenderer::exportPng(const std::string& path) const {
    // raylib's C API wants a non-const Image, but ExportImage only reads the pixels.
    auto& pixels = const_cast<std::vector<Rgba8>&>(m_pixels);
    return ExportImage(wrapPixels(pixels, m_width, m_height), path.c_str());
}

}  // namespace tf
