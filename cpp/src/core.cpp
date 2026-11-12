// SlopUI core: text helpers, shapes, themes (with a small JSON reader), the skin. Java: Shapes, Theme, Json,
// Skin, FallbackFont.
#include "slopui/core.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <variant>

namespace slopui {

std::u32string toU32(std::string_view s) {
    std::u32string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        char32_t cp;
        int n;
        if (c < 0x80) {
            cp = c;
            n = 1;
        } else if ((c >> 5) == 6) {
            cp = c & 0x1F;
            n = 2;
        } else if ((c >> 4) == 14) {
            cp = c & 0x0F;
            n = 3;
        } else if ((c >> 3) == 30) {
            cp = c & 0x07;
            n = 4;
        } else {
            out.push_back(U'?');
            i++;
            continue;
        }
        if (i + n > s.size()) {
            break;
        }
        for (int k = 1; k < n; k++) {
            cp = cp << 6 | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        }
        out.push_back(cp);
        i += n;
    }
    return out;
}

std::string toUtf8(std::u32string_view s) {
    std::string out;
    for (char32_t c : s) {
        if (c < 0x80) {
            out.push_back(char(c));
        } else if (c < 0x800) {
            out.push_back(char(0xC0 | c >> 6));
            out.push_back(char(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            out.push_back(char(0xE0 | c >> 12));
            out.push_back(char(0x80 | (c >> 6 & 0x3F)));
            out.push_back(char(0x80 | (c & 0x3F)));
        } else {
            out.push_back(char(0xF0 | c >> 18));
            out.push_back(char(0x80 | (c >> 12 & 0x3F)));
            out.push_back(char(0x80 | (c >> 6 & 0x3F)));
            out.push_back(char(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

float Font::width(std::u32string_view s) {
    float w = 0;
    for (char32_t c : s) {
        w += advance(c);
    }
    return w;
}

void Font::draw(Painter& p, std::u32string_view s, float x, float y, argb colour) {
    float px = x;
    for (char32_t c : s) {
        if (c != U' ') {
            drawGlyph(p, c, px, y, colour);
        }
        px += advance(c);
    }
}

void FallbackFont::drawGlyph(Painter& p, char32_t cp, float x, float y, argb c) {
    Font& f = of(cp);
    f.drawGlyph(p, cp, x, &f == primary_.get() ? y : y + (primary_->lineHeight() - f.lineHeight()) / 2, c);
}

// Shapes

namespace Shapes {

static const float NO_UV[64 * 6] = {};

void quad(Painter& p, float x, float y, float w, float h, argb c00, argb c10, argb c11, argb c01) {
    if (w <= 0 || h <= 0) {
        return;
    }
    float xy[] = {x, y, x + w, y, x + w, y + h, x, y, x + w, y + h, x, y + h};
    argb c[] = {c00, c10, c11, c00, c11, c01};
    p.triangles(nullptr, xy, NO_UV, c, 6);
}

void fill(Painter& p, float x, float y, float w, float h, argb c) { quad(p, x, y, w, h, c, c, c, c); }

void vgradient(Painter& p, float x, float y, float w, float h, argb top, argb bottom) {
    quad(p, x, y, w, h, top, top, bottom, bottom);
}

void hgradient(Painter& p, float x, float y, float w, float h, argb left, argb right) {
    quad(p, x, y, w, h, left, right, right, left);
}

void image(Painter& p, const Image& img, float sx, float sy, float sw, float sh, float x, float y, float w, float h,
           argb tint) {
    float xy[] = {x, y, x + w, y, x + w, y + h, x, y, x + w, y + h, x, y + h};
    float uv[] = {sx, sy, sx + sw, sy, sx + sw, sy + sh, sx, sy, sx + sw, sy + sh, sx, sy + sh};
    argb c[] = {tint, tint, tint, tint, tint, tint};
    p.triangles(&img, xy, uv, c, 6);
}

Poly cut(float x, float y, float w, float h, float tl, float tr, float br, float bl) {
    float m = std::min(w, h) / 2;
    tl = std::min(tl, m);
    tr = std::min(tr, m);
    br = std::min(br, m);
    bl = std::min(bl, m);
    float raw[] = {x + tl, y, x + w - tr, y, x + w, y + tr, x + w, y + h - br, x + w - br, y + h, x + bl, y + h,
                   x, y + h - bl, x, y + tl};
    Poly out;
    for (int i = 0; i < 8; i++) {
        float px = raw[i * 2], py = raw[i * 2 + 1];
        size_t n = out.size() / 2;
        if (n > 0 && std::abs(out[n * 2 - 2] - px) < 1e-4f && std::abs(out[n * 2 - 1] - py) < 1e-4f) {
            continue;
        }
        if (i == 7 && std::abs(out[0] - px) < 1e-4f && std::abs(out[1] - py) < 1e-4f) {
            continue;
        }
        out.push_back(px);
        out.push_back(py);
    }
    return out;
}

void poly(Painter& p, const Poly& pts, argb top, argb bottom, float y0, float h) {
    polyTextured(p, nullptr, pts, top, bottom, y0, h);
}

void polyTextured(Painter& p, const Image* tex, const Poly& pts, argb top, argb bottom, float y0, float h) {
    int n = int(pts.size() / 2);
    if (n < 3) {
        return;
    }
    float cx = 0, cy = 0;
    for (int i = 0; i < n; i++) {
        cx += pts[i * 2];
        cy += pts[i * 2 + 1];
    }
    cx /= n;
    cy /= n;
    std::vector<float> xy(n * 6), uv(n * 6);
    std::vector<argb> c(n * 3);
    float u = p.unit();
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float v[] = {cx, cy, pts[i * 2], pts[i * 2 + 1], pts[j * 2], pts[j * 2 + 1]};
        for (int k = 0; k < 3; k++) {
            xy[i * 6 + k * 2] = v[k * 2];
            xy[i * 6 + k * 2 + 1] = v[k * 2 + 1];
            uv[i * 6 + k * 2] = v[k * 2] * u;
            uv[i * 6 + k * 2 + 1] = v[k * 2 + 1] * u;
            c[i * 3 + k] = lerp(top, bottom, h <= 0 ? 0 : (v[k * 2 + 1] - y0) / h);
        }
    }
    p.triangles(tex, xy.data(), uv.data(), c.data(), n * 3);
}

static std::array<float, 2> normal(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f) {
        return {0, 0};
    }
    return {dy / len, -dx / len};
}

Poly offset(const Poly& pts, float d) {
    int n = int(pts.size() / 2);
    Poly out(n * 2);
    for (int i = 0; i < n; i++) {
        int a = (i + n - 1) % n, b = (i + 1) % n;
        auto n1 = normal(pts[a * 2], pts[a * 2 + 1], pts[i * 2], pts[i * 2 + 1]);
        auto n2 = normal(pts[i * 2], pts[i * 2 + 1], pts[b * 2], pts[b * 2 + 1]);
        float mx = n1[0] + n2[0], my = n1[1] + n2[1];
        float len = std::sqrt(mx * mx + my * my);
        if (len < 1e-5f) {
            mx = n2[0];
            my = n2[1];
            len = 1;
        }
        mx /= len;
        my /= len;
        float dot = mx * n2[0] + my * n2[1];
        float dist = d / std::max(0.2f, dot);
        out[i * 2] = pts[i * 2] + mx * dist;
        out[i * 2 + 1] = pts[i * 2 + 1] + my * dist;
    }
    return out;
}

void ring(Painter& p, const Poly& a, const Poly& b, argb ca, argb cb) {
    int n = int(a.size() / 2);
    std::vector<float> xy(n * 12), uv(n * 12);
    std::vector<argb> c(n * 6);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float v[] = {a[i * 2], a[i * 2 + 1], a[j * 2], a[j * 2 + 1], b[j * 2], b[j * 2 + 1],
                     a[i * 2], a[i * 2 + 1], b[j * 2], b[j * 2 + 1], b[i * 2], b[i * 2 + 1]};
        std::copy(v, v + 12, xy.begin() + i * 12);
        argb cc[] = {ca, ca, cb, ca, cb, cb};
        std::copy(cc, cc + 6, c.begin() + i * 6);
    }
    p.triangles(nullptr, xy.data(), uv.data(), c.data(), n * 6);
}

void outline(Painter& p, const Poly& pts, float t, argb c) { ring(p, pts, offset(pts, -t), c, c); }

void glow(Painter& p, const Poly& pts, float size, argb c) { ring(p, pts, offset(pts, size), c, c & 0x00FFFFFF); }

void line(Painter& p, float x1, float y1, float x2, float y2, float t, argb c) {
    auto n = normal(x1, y1, x2, y2);
    float ox = n[0] * t / 2, oy = n[1] * t / 2;
    float xy[] = {x1 + ox, y1 + oy, x2 + ox, y2 + oy, x2 - ox, y2 - oy, x1 + ox, y1 + oy, x2 - ox, y2 - oy,
                  x1 - ox, y1 - oy};
    argb cc[] = {c, c, c, c, c, c};
    p.triangles(nullptr, xy, NO_UV, cc, 6);
}

argb lerp(argb a, argb b, float t) {
    if (t <= 0) {
        return a;
    }
    if (t >= 1) {
        return b;
    }
    argb r = 0;
    for (int s = 0; s < 32; s += 8) {
        int ca = a >> s & 0xFF, cb = b >> s & 0xFF;
        r |= argb(std::lround(ca + (cb - ca) * t)) << s;
    }
    return r;
}

argb alpha(argb c, float f) {
    argb a = argb(std::lround((c >> 24) * std::clamp(f, 0.0f, 1.0f)));
    return a << 24 | (c & 0xFFFFFF);
}

}  // namespace Shapes

// Themes

const std::array<const char*, Theme::COLOURS> Theme::colourNames = {
    "text", "textDim", "textDisabled", "textBright", "accent", "accentSoft", "panelTop", "panelBottom",
    "panelBorder", "panelLight", "titleTop", "titleBottom", "titleText", "barTop", "barBottom", "buttonTop",
    "buttonBottom", "hoverTop", "hoverBottom", "pressTop", "pressBottom", "buttonBorder", "buttonText", "fieldTop",
    "fieldBottom", "fieldBorder", "focus", "selection", "selectionText", "rowHover", "trackTop", "trackBottom",
    "fillTop", "fillBottom", "glow", "shadow", "warning", "tooltipTop", "tooltipBottom", "shade", "rowAlt", "texture"};
const std::array<const char*, Theme::SIZES> Theme::sizeNames = {
    "row", "pad", "gap", "cut", "border", "glowSize", "titleHeight", "barHeight", "scrollbar", "shadowSize",
    "textureStrength", "hoverTime", "pressTime", "openTime", "tooltipDelay", "maxWindow", "buttonHeight", "splitter"};

Theme::Theme() {
    c = {0xFFC8C8B8, 0xFF8A9078, 0xFF545C48, 0xFFF4F0D8, 0xFFE8D860, 0xFFB8C890, 0xF2242C1E, 0xF20C100A,
         0xFF5A6A48, 0x50B8C890, 0xFF3E4E2E, 0xFF1A2214, 0xFFF0E8B8, 0xF02C3624, 0xF0101410, 0xFF3A4630,
         0xFF1A2216, 0xFF5A6E40, 0xFF26321C, 0xFF10140C, 0xFF2C3822, 0xFF74885A, 0xFFE0E0C8, 0xFF080A06,
         0xFF141810, 0xFF4A5838, 0xFFE8D860, 0xFF586A2C, 0xFFFFFFE8, 0x28B8C890, 0xFF080A06, 0xFF1A2014,
         0xFFE0E880, 0xFF6A8434, 0x90D0E070, 0xA0000000, 0xFFFF7060, 0xF8303C26, 0xF8141A10, 0x80000000,
         0x10B8C890, 0xFFB8C890};
    s = {12, 6, 4, 4, 1, 5, 14, 12, 6, 8, 0.07f, 0.12f, 0.06f, 0.14f, 0.6f, 0.92f, 16, 5};
}

namespace {

/** A small JSON reader (themes): objects, arrays, strings, numbers, true/false/null, // comments. */
struct JsonValue;
using JsonObject = std::vector<std::pair<std::string, JsonValue>>;
using JsonArray = std::vector<JsonValue>;
struct JsonValue {
    std::variant<std::nullptr_t, bool, double, std::string, std::shared_ptr<JsonArray>, std::shared_ptr<JsonObject>> v;
};

struct JsonReader {
    std::string_view s;
    size_t i = 0;

    [[noreturn]] void error(const std::string& what) const {
        throw std::runtime_error("JSON: " + what + " at " + std::to_string(i));
    }
    void space() {
        while (i < s.size()) {
            char c = s[i];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                i++;
            } else if (c == '/' && i + 1 < s.size() && s[i + 1] == '/') {
                while (i < s.size() && s[i] != '\n') {
                    i++;
                }
            } else {
                break;
            }
        }
    }
    bool peek(char c) {
        if (i < s.size() && s[i] == c) {
            i++;
            return true;
        }
        return false;
    }
    void expect(char c) {
        if (!peek(c)) {
            error(std::string("'") + c + "' expected");
        }
    }
    std::string string() {
        expect('"');
        std::string b;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') {
                return b;
            }
            if (c == '\\') {
                char e = s[i++];
                if (e == 'n') {
                    b += '\n';
                } else if (e == 't') {
                    b += '\t';
                } else if (e == 'u') {
                    b += toUtf8(std::u32string(1, char32_t(std::stoi(std::string(s.substr(i, 4)), nullptr, 16))));
                    i += 4;
                } else {
                    b += e;
                }
            } else {
                b += c;
            }
        }
        error("unterminated string");
    }
    JsonValue value() {
        if (i >= s.size()) {
            error("end of text");
        }
        char c = s[i];
        if (c == '{') {
            i++;
            auto m = std::make_shared<JsonObject>();
            space();
            if (peek('}')) {
                return {m};
            }
            while (true) {
                space();
                std::string k = string();
                space();
                expect(':');
                space();
                m->emplace_back(k, value());
                space();
                if (peek('}')) {
                    return {m};
                }
                expect(',');
            }
        }
        if (c == '[') {
            i++;
            auto l = std::make_shared<JsonArray>();
            space();
            if (peek(']')) {
                return {l};
            }
            while (true) {
                space();
                l->push_back(value());
                space();
                if (peek(']')) {
                    return {l};
                }
                expect(',');
            }
        }
        if (c == '"') {
            return {string()};
        }
        if (s.substr(i, 4) == "true") {
            i += 4;
            return {true};
        }
        if (s.substr(i, 5) == "false") {
            i += 5;
            return {false};
        }
        if (s.substr(i, 4) == "null") {
            i += 4;
            return {nullptr};
        }
        size_t start = i;
        while (i < s.size() && std::string_view("+-0123456789.eE").find(s[i]) != std::string_view::npos) {
            i++;
        }
        if (start == i) {
            error(std::string("unexpected '") + c + "'");
        }
        return {std::stod(std::string(s.substr(start, i - start)))};
    }
};

template <size_t N>
int indexOf(const std::array<const char*, N>& names, const std::string& key) {
    for (size_t i = 0; i < N; i++) {
        if (key == names[i]) {
            return int(i);
        }
    }
    return -1;
}

}  // namespace

argb Theme::parseColour(std::string_view v) {
    std::string h(v.substr(v.starts_with('#') ? 1 : 0));
    unsigned long long x = std::stoull(h, nullptr, 16);
    return h.size() <= 6 ? argb(0xFF000000u | x) : argb(x);
}

Theme Theme::read(std::string_view json) const {
    Theme t = *this;
    t.texture.reset();
    t.textureOwner = nullptr;
    JsonReader r{json};
    r.space();
    JsonValue root = r.value();
    auto* obj = std::get_if<std::shared_ptr<JsonObject>>(&root.v);
    if (!obj) {
        throw std::runtime_error("JSON: an object expected");
    }
    for (auto& [k, v] : **obj) {
        if (k == "name") {
            if (auto* n = std::get_if<std::string>(&v.v)) {
                t.name = *n;
            }
        } else if (k == "colours" || k == "sizes") {
            auto* m = std::get_if<std::shared_ptr<JsonObject>>(&v.v);
            if (!m) {
                continue;
            }
            for (auto& [key, val] : **m) {
                if (k == "colours") {
                    int idx = indexOf(colourNames, key);
                    if (auto* str = std::get_if<std::string>(&val.v); idx >= 0 && str) {
                        t.c[idx] = parseColour(*str);
                    }
                } else {
                    int idx = indexOf(sizeNames, key);
                    if (auto* d = std::get_if<double>(&val.v); idx >= 0 && d) {
                        t.s[idx] = float(*d);
                    }
                }
            }
        }
    }
    return t;
}

std::string Theme::write() const {
    std::string b = "{\n  \"name\": \"" + name + "\",\n  \"colours\": {\n";
    char buf[64];
    for (size_t i = 0; i < c.size(); i++) {
        std::snprintf(buf, sizeof buf, "    \"%s\": \"#%08X\"%s\n", colourNames[i], c[i], i + 1 < c.size() ? "," : "");
        b += buf;
    }
    b += "  },\n  \"sizes\": {\n";
    for (size_t i = 0; i < s.size(); i++) {
        std::snprintf(buf, sizeof buf, "    \"%s\": %g%s\n", sizeNames[i], double(s[i]), i + 1 < s.size() ? "," : "");
        b += buf;
    }
    return b + "  }\n}\n";
}

// Skin

namespace Skin {

const Image& texture(Painter& p, Theme& t) {
    if (!t.texture || t.textureOwner != &p) {
        const int n = 128;
        std::vector<argb> px(n * n);
        std::uint64_t seed = 0x5EED1977ULL;
        std::vector<float> row(n);
        auto next = [&] {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            return float(seed >> 40 & 0xFFFF) / 65535.0f;
        };
        for (int y = 0; y < n; y++) {
            for (int x = 0; x < n; x++) {
                row[x] = next();
            }
            float base = next();
            for (int x = 0; x < n; x++) {
                float sum = 0;
                for (int k = -6; k <= 6; k++) {
                    sum += row[(x + k + n) % n];
                }
                float v = 0.55f * base + 0.25f * (sum / 13) + 0.2f * row[x];
                argb a = argb(std::clamp(int(std::lround(v * 255)), 0, 255));
                px[y * n + x] = a << 24 | 0xFFFFFF;
            }
        }
        t.texture = p.createImage(n, n, px.data(), true, true);
        t.textureOwner = &p;
    }
    return *t.texture;
}

void texture(Painter& p, Theme& t, const Shapes::Poly& pts, float y0, float h, float strength) {
    float a = t.s[Theme::TEXTURE_STRENGTH] * strength;
    if (a <= 0) {
        return;
    }
    argb tint = Shapes::alpha(t.c[Theme::TEXTURE], a);
    Shapes::polyTextured(p, &texture(p, t), pts, tint, tint, y0, h);
}

void panel(Painter& p, Theme& t, float x, float y, float w, float h, float cut, bool shadow) {
    auto pts = Shapes::cut(x, y, w, h, cut, 0, cut, 0);
    if (shadow) {
        float ss = t.s[Theme::SHADOW_SIZE];
        auto sh = Shapes::cut(x + 1, y + 2, w, h, cut, 0, cut, 0);
        Shapes::ring(p, sh, Shapes::offset(sh, ss), t.c[Theme::SHADOW], t.c[Theme::SHADOW] & 0xFFFFFF);
        Shapes::poly(p, sh, t.c[Theme::SHADOW], t.c[Theme::SHADOW], y, h);
    }
    Shapes::poly(p, pts, t.c[Theme::PANEL_TOP], t.c[Theme::PANEL_BOTTOM], y, h);
    texture(p, t, pts, y, h, 1);
    Shapes::vgradient(p, x + cut, y + 1, w - cut - 1, std::min(6.0f, h / 3), t.c[Theme::PANEL_LIGHT], 0);
    Shapes::outline(p, pts, t.s[Theme::BORDER], t.c[Theme::PANEL_BORDER]);
}

void button(Painter& p, Theme& t, float x, float y, float w, float h, float hover, float press, bool enabled,
            bool accent, bool focus) {
    using namespace Shapes;
    float cutSize = std::min(t.s[Theme::CUT], h / 3);
    auto pts = Shapes::cut(x, y, w, h, cutSize, 0, cutSize, 0);
    argb top = lerp(t.c[Theme::BUTTON_TOP], t.c[Theme::HOVER_TOP], hover);
    argb bottom = lerp(t.c[Theme::BUTTON_BOTTOM], t.c[Theme::HOVER_BOTTOM], hover);
    top = lerp(top, t.c[Theme::PRESS_TOP], press);
    bottom = lerp(bottom, t.c[Theme::PRESS_BOTTOM], press);
    if (!enabled) {
        top = lerp(t.c[Theme::BUTTON_TOP], t.c[Theme::PANEL_BOTTOM], 0.5f);
        bottom = lerp(t.c[Theme::BUTTON_BOTTOM], t.c[Theme::PANEL_BOTTOM], 0.5f);
    }
    if (enabled && hover > 0) {
        glow(p, pts, t.s[Theme::GLOW_SIZE] * hover, alpha(t.c[Theme::GLOW], hover * (1 - press * 0.5f)));
    }
    poly(p, pts, top, bottom, y, h);
    texture(p, t, pts, y, h, 0.8f);
    argb light = alpha(0xFFFFFFFF, enabled ? 0.22f * (1 - press) + 0.06f : 0.06f);
    argb dark = alpha(0xFF000000, 0.35f + 0.3f * press);
    vgradient(p, x + cutSize, y + 1, w - cutSize - 1, std::max(1.0f, h * 0.45f), light, 0);
    if (press > 0) {
        vgradient(p, x + 1, y + 1, w - 2, std::max(1.0f, h * 0.3f), alpha(0xFF000000, 0.4f * press), 0);
    }
    fill(p, x + 1, y + h - 1, w - cutSize - 1, 1, dark);
    argb edge = !enabled ? t.c[Theme::FIELD_BORDER]
                : accent ? lerp(t.c[Theme::ACCENT], 0xFFFFFFFF, hover * 0.3f)
                         : lerp(t.c[Theme::BUTTON_BORDER], t.c[Theme::ACCENT_SOFT], hover);
    outline(p, pts, t.s[Theme::BORDER], edge);
    if (focus) {
        outline(p, offset(pts, 2), 1, alpha(t.c[Theme::FOCUS], 0.8f));
    }
}

void field(Painter& p, Theme& t, float x, float y, float w, float h, float focus, bool enabled) {
    using namespace Shapes;
    float c = std::min(t.s[Theme::CUT] * 0.5f, h / 3);
    auto pts = cut(x, y, w, h, 0, c, 0, c);
    poly(p, pts, t.c[Theme::FIELD_TOP], t.c[Theme::FIELD_BOTTOM], y, h);
    vgradient(p, x + 1, y + 1, w - 2 - c, std::min(4.0f, h / 2), 0x60000000, 0);
    argb edge = lerp(t.c[Theme::FIELD_BORDER], t.c[Theme::FOCUS], focus);
    if (!enabled) {
        edge = lerp(t.c[Theme::FIELD_BORDER], t.c[Theme::PANEL_BOTTOM], 0.5f);
    }
    if (focus > 0) {
        glow(p, pts, 3 * focus, alpha(t.c[Theme::FOCUS], 0.35f * focus));
    }
    outline(p, pts, t.s[Theme::BORDER], edge);
}

void chevron(Painter& p, float cx, float cy, float s, int dir, argb c) {
    float h = s / 2;
    switch (dir) {
        case 0:
            Shapes::line(p, cx - h, cy - h / 2, cx, cy + h / 2, 1.2f, c);
            Shapes::line(p, cx, cy + h / 2, cx + h, cy - h / 2, 1.2f, c);
            break;
        case 1:
            Shapes::line(p, cx - h / 2, cy - h, cx + h / 2, cy, 1.2f, c);
            Shapes::line(p, cx + h / 2, cy, cx - h / 2, cy + h, 1.2f, c);
            break;
        case 2:
            Shapes::line(p, cx - h, cy + h / 2, cx, cy - h / 2, 1.2f, c);
            Shapes::line(p, cx, cy - h / 2, cx + h, cy + h / 2, 1.2f, c);
            break;
        default:
            Shapes::line(p, cx + h / 2, cy - h, cx - h / 2, cy, 1.2f, c);
            Shapes::line(p, cx - h / 2, cy, cx + h / 2, cy + h, 1.2f, c);
    }
}

void tick(Painter& p, float x, float y, float s, float amount, argb c) {
    float ax = x + s * 0.2f, ay = y + s * 0.52f, bx = x + s * 0.42f, by = y + s * 0.74f, cx = x + s * 0.82f,
          cy = y + s * 0.26f;
    float first = std::min(1.0f, amount * 2.5f), second = std::clamp((amount - 0.4f) / 0.6f, 0.0f, 1.0f);
    if (first > 0) {
        Shapes::line(p, ax, ay, ax + (bx - ax) * first, ay + (by - ay) * first, 1.6f, c);
    }
    if (second > 0) {
        Shapes::line(p, bx, by, bx + (cx - bx) * second, by + (cy - by) * second, 1.6f, c);
    }
}

std::u32string fit(Font& f, std::u32string_view text, float width) {
    if (f.width(text) <= width) {
        return std::u32string(text);
    }
    float dw = f.width(U"...");
    size_t end = text.size();
    while (end > 0 && f.width(text.substr(0, end)) + dw > width) {
        end--;
    }
    return std::u32string(text.substr(0, end)) + U"...";
}

}  // namespace Skin

}  // namespace slopui
