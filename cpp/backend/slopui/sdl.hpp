#pragma once
// SlopUI's SDL3/OpenGL backend (Java: java/gl): GlPainter (batched triangles, scissor clips; OpenGL 3.3 core
// or OpenGL ES 3.0), TtfFont (stb_truetype, baked per UI size), SdlHost (window, input, UI size, fullscreen).
#include <memory>
#include <string>
#include <vector>

#include "slopui/ui.hpp"

struct SDL_Window;
struct SDL_Cursor;
struct SDL_GLContextState;

namespace slopui {

struct GlApi;

/** The painter on OpenGL: triangles batched per texture, positions snapped to device pixels. */
class GlPainter : public Painter {
public:
    explicit GlPainter(bool es);
    ~GlPainter() override;
    void begin(int fbW, int fbH, float unit);
    void end();
    float unit() const override { return unit_; }
    void triangles(const Image* image, const float* xy, const float* uv, const argb* colours, int n) override;
    void pushClip(float x, float y, float w, float h) override;
    void popClip() override;
    void setOpacity(float a) override;
    float opacity() const override { return opacity_; }
    std::shared_ptr<Image> createImage(int w, int h, const argb* pixels, bool smooth, bool repeat) override;
    void clear(argb colour);

private:
    void flush();
    void scissor();
    struct Vertex {
        float x, y, u, v;
        unsigned char r, g, b, a;
    };
    std::vector<Vertex> verts_;
    unsigned program_ = 0, vao_ = 0, vbo_ = 0;
    int uScreen_ = -1;
    const Image* current_ = nullptr;
    std::shared_ptr<Image> white_;
    int fbW_ = 0, fbH_ = 0;
    float unit_ = 1, opacity_ = 1;
    std::vector<std::array<int, 4>> clips_;
};

/** A TrueType font (stb_truetype): 'size' UI units per em, baked again for each device-pixel size. */
class TtfFont : public Font {
public:
    /** The font file's bytes; throws std::runtime_error if it is not a font. */
    TtfFont(std::vector<unsigned char> data, float size);
    ~TtfFont() override;
    static std::shared_ptr<TtfFont> load(const std::string& path, float size);
    bool has(char32_t cp) const override;
    float advance(char32_t cp) override;
    float lineHeight() const override { return lineH_; }
    void drawGlyph(Painter& p, char32_t cp, float x, float y, argb colour) override;

private:
    void bake(Painter& p);
    struct Impl;
    std::unique_ptr<Impl> impl_;
    float size_, scaleEm_ = 1, ascent_ = 0, lineH_ = 0;
};

/**
 * An SDL3 window running a Ui: input to it in UI units, a frame each refresh (vsync), the UI size from the
 * window (Auto: one UI unit per 400th of the height, whole units) or fixed, fullscreen (desktop mode, no
 * decorations; the bar hides itself there).
 */
class SdlHost : public Host {
public:
    SdlHost(const std::string& title, int w, int h);
    ~SdlHost() override;
    void setUi(Ui& ui);
    /** One frame: events, size, timers, drawing; false when the window closes. */
    bool frame(const std::function<void()>& beforeDraw = {});
    void setUiScale(int s) { uiScale_ = s; }
    int uiScale() const { return uiScale_; }
    void setSize(int w, int h);
    bool fullscreen() const { return fullscreen_; }
    void setFullscreen(bool on);
    void requestClose() { closing_ = true; }
    void setClearColour(argb c) { clear_ = c; }
    void setTitle(const std::string& t);
    GlPainter& painter() { return *painter_; }

    std::string clipboard() override;
    void setClipboard(const std::string& text) override;
    void setCursor(int shape) override;

private:
    float ratio() const;
    SDL_Window* window_ = nullptr;
    SDL_GLContextState* gl_ = nullptr;
    std::unique_ptr<GlPainter> painter_;
    Ui* ui_ = nullptr;
    SDL_Cursor* cursors_[7] = {};
    int cursorShown_ = -1, uiScale_ = 0;
    bool fullscreen_ = false, closing_ = false;
    std::uint64_t last_ = 0;
    argb clear_ = 0xFF000000;
};

}  // namespace slopui
