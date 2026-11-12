#pragma once
// SlopUI core: drawing (Painter, Image, Font), shapes, themes and the skin. The same classes as the Java
// version (java/core, package slopui); keep the two in step.
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace slopui {

using argb = std::uint32_t;

/** An image the backend made (Painter::createImage); size in pixels. */
class Image {
public:
    virtual ~Image() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

/**
 * What the toolkit draws with: triangles, optionally textured, in UI units (the backend multiplies by unit()
 * and snaps to device pixels). The one drawing primitive; Shapes builds the rest.
 */
class Painter {
public:
    virtual ~Painter() = default;
    /** Device pixels per UI unit. */
    virtual float unit() const = 0;
    /** n vertices (a multiple of 3): xy in UI units, uv in image pixels (ignored without an image), a colour each. */
    virtual void triangles(const Image* image, const float* xy, const float* uv, const argb* colours, int n) = 0;
    virtual void pushClip(float x, float y, float w, float h) = 0;
    virtual void popClip() = 0;
    /** Every colour after this has its alpha multiplied by a (0..1). */
    virtual void setOpacity(float a) = 0;
    virtual float opacity() const = 0;
    virtual std::shared_ptr<Image> createImage(int w, int h, const argb* pixels, bool smooth, bool repeat) = 0;
};

/** What the toolkit asks of the window it runs in. */
class Host {
public:
    virtual ~Host() = default;
    virtual std::string clipboard() = 0;
    virtual void setClipboard(const std::string& text) = 0;
    /** The pointer's shape: one of Cursor. */
    virtual void setCursor(int shape) = 0;
};

namespace Cursor {
inline constexpr int ARROW = 0, TEXT = 1, HAND = 2, RESIZE_H = 3, RESIZE_V = 4, MOVE = 5;
}

/** Key codes and modifiers (GLFW's values, as the Java version; a host maps its own to these). */
namespace Keys {
inline constexpr int SPACE = 32, A = 65, C = 67, V = 86, X = 88, Z = 90;
inline constexpr int ESCAPE = 256, ENTER = 257, TAB = 258, BACKSPACE = 259, INSERT = 260, DELETE_ = 261, RIGHT = 262,
                     LEFT = 263, DOWN = 264, UP = 265, PAGE_UP = 266, PAGE_DOWN = 267, HOME = 268, END = 269, F1 = 290,
                     F10 = 299, F11 = 300, KP_ENTER = 335, LEFT_ALT = 342, RIGHT_ALT = 346;
inline constexpr int SHIFT = 1, CTRL = 2, ALT = 4, SUPER = 8;
inline bool isEnter(int key) { return key == ENTER || key == KP_ENTER; }
}  // namespace Keys

/** UTF-8 to code points and back. */
std::u32string toU32(std::string_view s);
std::string toUtf8(std::u32string_view s);

/** Text in UI units (TrueType fonts from the backend; several as one: FallbackFont). */
class Font {
public:
    virtual ~Font() = default;
    virtual bool has(char32_t cp) const = 0;
    /** The pen's advance after the code point, UI units. */
    virtual float advance(char32_t cp) = 0;
    /** Line height, UI units (text is drawn with its top at y). */
    virtual float lineHeight() const = 0;
    virtual void drawGlyph(Painter& p, char32_t cp, float x, float y, argb colour) = 0;

    float width(std::u32string_view s);
    float width(std::string_view s) { return width(toU32(s)); }
    void draw(Painter& p, std::u32string_view s, float x, float y, argb colour);
    void draw(Painter& p, std::string_view s, float x, float y, argb colour) { draw(p, toU32(s), x, y, colour); }
};

/** Two fonts as one: the primary's glyphs, the fallback's for the code points the primary lacks. */
class FallbackFont : public Font {
public:
    FallbackFont(std::shared_ptr<Font> primary, std::shared_ptr<Font> fallback)
        : primary_(std::move(primary)), fallback_(std::move(fallback)) {}
    bool has(char32_t cp) const override { return primary_->has(cp) || fallback_->has(cp); }
    float advance(char32_t cp) override { return of(cp).advance(cp); }
    float lineHeight() const override { return primary_->lineHeight(); }
    void drawGlyph(Painter& p, char32_t cp, float x, float y, argb c) override;

private:
    Font& of(char32_t cp) const { return primary_->has(cp) || !fallback_->has(cp) ? *primary_ : *fallback_; }
    std::shared_ptr<Font> primary_, fallback_;
};

/** Shapes from Painter::triangles. Polygons: x0, y0, x1, y1... clockwise on the screen (y down). */
namespace Shapes {
using Poly = std::vector<float>;
void quad(Painter& p, float x, float y, float w, float h, argb c00, argb c10, argb c11, argb c01);
void fill(Painter& p, float x, float y, float w, float h, argb c);
void vgradient(Painter& p, float x, float y, float w, float h, argb top, argb bottom);
void hgradient(Painter& p, float x, float y, float w, float h, argb left, argb right);
void image(Painter& p, const Image& img, float sx, float sy, float sw, float sh, float x, float y, float w, float h,
           argb tint);
/** A rectangle with its corners cut: top-left, top-right, bottom-right, bottom-left. */
Poly cut(float x, float y, float w, float h, float tl, float tr, float br, float bl);
void poly(Painter& p, const Poly& pts, argb top, argb bottom, float y0, float h);
void polyTextured(Painter& p, const Image* tex, const Poly& pts, argb top, argb bottom, float y0, float h);
Poly offset(const Poly& pts, float d);
void ring(Painter& p, const Poly& a, const Poly& b, argb ca, argb cb);
void outline(Painter& p, const Poly& pts, float t, argb c);
void glow(Painter& p, const Poly& pts, float size, argb c);
void line(Painter& p, float x1, float y1, float x2, float y2, float t, argb c);
argb lerp(argb a, argb b, float t);
argb alpha(argb c, float f);
}  // namespace Shapes

/**
 * Colours, sizes and fonts the widgets are drawn with, indexed by the constants below (theme.c[TEXT],
 * theme.s[ROW]); a theme file (JSON) sets them by name (the Java version's format).
 */
class Theme {
public:
    enum Colour {
        TEXT, TEXT_DIM, TEXT_DISABLED, TEXT_BRIGHT, ACCENT, ACCENT_SOFT, PANEL_TOP, PANEL_BOTTOM, PANEL_BORDER,
        PANEL_LIGHT, TITLE_TOP, TITLE_BOTTOM, TITLE_TEXT, BAR_TOP, BAR_BOTTOM, BUTTON_TOP, BUTTON_BOTTOM, HOVER_TOP,
        HOVER_BOTTOM, PRESS_TOP, PRESS_BOTTOM, BUTTON_BORDER, BUTTON_TEXT, FIELD_TOP, FIELD_BOTTOM, FIELD_BORDER,
        FOCUS, SELECTION, SELECTION_TEXT, ROW_HOVER, TRACK_TOP, TRACK_BOTTOM, FILL_TOP, FILL_BOTTOM, GLOW, SHADOW,
        WARNING, TOOLTIP_TOP, TOOLTIP_BOTTOM, SHADE, ROW_ALT, TEXTURE, COLOURS
    };
    enum Size {
        ROW, PAD, GAP, CUT, BORDER, GLOW_SIZE, TITLE_HEIGHT, BAR_HEIGHT, SCROLLBAR, SHADOW_SIZE, TEXTURE_STRENGTH,
        HOVER_TIME, PRESS_TIME, OPEN_TIME, TOOLTIP_DELAY, MAX_WINDOW, BUTTON_HEIGHT, SPLITTER, SIZES
    };
    static const std::array<const char*, COLOURS> colourNames;
    static const std::array<const char*, SIZES> sizeNames;

    std::string name = "Game";
    std::array<argb, COLOURS> c{};
    std::array<float, SIZES> s{};
    std::shared_ptr<Font> font, titleFontOverride;
    /** The panel texture (Skin), made on first use per painter. */
    std::shared_ptr<Image> texture;
    const Painter* textureOwner = nullptr;

    /** The built-in game look. */
    Theme();
    Font& titleFont() const { return titleFontOverride ? *titleFontOverride : *font; }
    /** A theme file read over this theme's values (missing keys keep them); throws std::runtime_error. */
    Theme read(std::string_view json) const;
    std::string write() const;
    static argb parseColour(std::string_view v);
};

/** How the widgets look, from the theme: textured gradient panels with cut corners, bevelled buttons. */
namespace Skin {
const Image& texture(Painter& p, Theme& t);
void texture(Painter& p, Theme& t, const Shapes::Poly& pts, float y0, float h, float strength);
void panel(Painter& p, Theme& t, float x, float y, float w, float h, float cut, bool shadow);
void button(Painter& p, Theme& t, float x, float y, float w, float h, float hover, float press, bool enabled,
            bool accent, bool focus);
void field(Painter& p, Theme& t, float x, float y, float w, float h, float focus, bool enabled);
/** A chevron: down, right, up or left (0-3). */
void chevron(Painter& p, float cx, float cy, float s, int dir, argb c);
void tick(Painter& p, float x, float y, float s, float amount, argb c);
std::u32string fit(Font& f, std::u32string_view text, float width);
}  // namespace Skin

}  // namespace slopui
