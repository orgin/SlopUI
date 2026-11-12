// SlopUI TtfFont (Java: slopui.gl.TtfFont): stb_truetype, Latin to Latin Extended B, baked per device size.
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <unordered_map>

#include "slopui/sdl.hpp"
#include "stb/stb_truetype.h"

namespace slopui {

namespace {
constexpr int FIRST = 32, COUNT = 0x250 - 32;
}

struct TtfFont::Impl {
    std::vector<unsigned char> data;
    stbtt_fontinfo info{};
    std::unordered_map<char32_t, float> advances;
    float bakedUnit = -1;
    const Painter* owner = nullptr;
    std::shared_ptr<Image> atlas;
    std::vector<stbtt_bakedchar> baked;
};

TtfFont::TtfFont(std::vector<unsigned char> data, float size) : impl_(std::make_unique<Impl>()), size_(size) {
    impl_->data = std::move(data);
    if (!stbtt_InitFont(&impl_->info, impl_->data.data(), 0)) {
        throw std::runtime_error("not a TrueType font");
    }
    scaleEm_ = stbtt_ScaleForMappingEmToPixels(&impl_->info, size);
    int a, d, g;
    stbtt_GetFontVMetrics(&impl_->info, &a, &d, &g);
    ascent_ = a * scaleEm_;
    lineH_ = (a - d) * scaleEm_;
}

TtfFont::~TtfFont() = default;

std::shared_ptr<TtfFont> TtfFont::load(const std::string& path, float size) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot read " + path);
    }
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return std::make_shared<TtfFont>(std::move(bytes), size);
}

bool TtfFont::has(char32_t cp) const {
    return cp == U' ' || (cp >= FIRST && cp < char32_t(FIRST + COUNT) && stbtt_FindGlyphIndex(&impl_->info, int(cp)) != 0);
}

float TtfFont::advance(char32_t cp) {
    auto it = impl_->advances.find(cp);
    if (it != impl_->advances.end()) {
        return it->second;
    }
    int adv, lsb;
    stbtt_GetCodepointHMetrics(&impl_->info, int(cp), &adv, &lsb);
    float a = adv * scaleEm_;
    impl_->advances[cp] = a;
    return a;
}

void TtfFont::bake(Painter& p) {
    float unit = p.unit();
    float px = stbtt_ScaleForMappingEmToPixels(&impl_->info, size_ * unit) / stbtt_ScaleForPixelHeight(&impl_->info, 1);
    std::vector<stbtt_bakedchar> chars(COUNT);
    for (int side = 256; side <= 4096; side *= 2) {
        std::vector<unsigned char> bitmap(size_t(side) * side);
        if (stbtt_BakeFontBitmap(impl_->data.data(), 0, px, bitmap.data(), side, side, FIRST, COUNT, chars.data()) > 0) {
            std::vector<argb> pixels(bitmap.size());
            for (size_t i = 0; i < bitmap.size(); i++) {
                pixels[i] = argb(bitmap[i]) << 24 | 0xFFFFFF;
            }
            impl_->atlas = p.createImage(side, side, pixels.data(), false, false);
            impl_->baked = std::move(chars);
            impl_->bakedUnit = unit;
            impl_->owner = &p;
            return;
        }
    }
}

void TtfFont::drawGlyph(Painter& p, char32_t cp, float x, float y, argb colour) {
    if (impl_->bakedUnit != p.unit() || impl_->owner != &p) {
        bake(p);
    }
    if (impl_->baked.empty() || cp < FIRST || cp >= char32_t(FIRST + COUNT)) {
        return;
    }
    const stbtt_bakedchar& c = impl_->baked[cp - FIRST];
    float unit = p.unit();
    float bx = std::round(x * unit), by = std::round((y + ascent_) * unit);
    float x0 = bx + c.xoff, y0 = by + c.yoff;
    float gw = float(c.x1 - c.x0), gh = float(c.y1 - c.y0);
    if (gw <= 0 || gh <= 0) {
        return;
    }
    Shapes::image(p, *impl_->atlas, c.x0, c.y0, gw, gh, x0 / unit, y0 / unit, gw / unit, gh / unit, colour);
}

}  // namespace slopui
