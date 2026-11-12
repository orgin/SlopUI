// SlopUI SdlHost (Java: slopui.gl.GlfwHost): an SDL3 window with an OpenGL 3.3 core (or ES 3.0) context.
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "slopui/sdl.hpp"

namespace slopui {

namespace {

/** SDL key codes to the toolkit's (GLFW's values); 0: not one it reads. */
int mapKey(SDL_Keycode k) {
    if (k >= SDLK_A && k <= SDLK_Z) {
        return Keys::A + int(k - SDLK_A);
    }
    switch (k) {
        case SDLK_SPACE: return Keys::SPACE;
        case SDLK_ESCAPE: return Keys::ESCAPE;
        case SDLK_RETURN: return Keys::ENTER;
        case SDLK_KP_ENTER: return Keys::KP_ENTER;
        case SDLK_TAB: return Keys::TAB;
        case SDLK_BACKSPACE: return Keys::BACKSPACE;
        case SDLK_INSERT: return Keys::INSERT;
        case SDLK_DELETE: return Keys::DELETE_;
        case SDLK_RIGHT: return Keys::RIGHT;
        case SDLK_LEFT: return Keys::LEFT;
        case SDLK_DOWN: return Keys::DOWN;
        case SDLK_UP: return Keys::UP;
        case SDLK_PAGEUP: return Keys::PAGE_UP;
        case SDLK_PAGEDOWN: return Keys::PAGE_DOWN;
        case SDLK_HOME: return Keys::HOME;
        case SDLK_END: return Keys::END;
        case SDLK_F1: return Keys::F1;
        case SDLK_F10: return Keys::F10;
        case SDLK_F11: return Keys::F11;
        default: return 0;
    }
}

int mapMods(SDL_Keymod m) {
    return ((m & SDL_KMOD_SHIFT) ? Keys::SHIFT : 0) | ((m & SDL_KMOD_CTRL) ? Keys::CTRL : 0) |
           ((m & SDL_KMOD_ALT) ? Keys::ALT : 0) | ((m & SDL_KMOD_GUI) ? Keys::SUPER : 0);
}

}  // namespace

SdlHost::SdlHost(const std::string& title, int w, int h) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL: ") + SDL_GetError());
    }
    bool es = false;
    for (int attempt = 0; attempt < 2 && !gl_; attempt++) {
        es = attempt == 1;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, es ? SDL_GL_CONTEXT_PROFILE_ES : SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, es ? 0 : 3);
        if (window_) {
            SDL_DestroyWindow(window_);
        }
        window_ = SDL_CreateWindow(title.c_str(), w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        gl_ = window_ ? SDL_GL_CreateContext(window_) : nullptr;
    }
    if (!gl_) {
        throw std::runtime_error(std::string("no OpenGL 3.3 / ES 3.0 context: ") + SDL_GetError());
    }
    SDL_GL_MakeCurrent(window_, gl_);
    SDL_GL_SetSwapInterval(1);
    painter_ = std::make_unique<GlPainter>(es);
    SDL_SystemCursor shapes[] = {SDL_SYSTEM_CURSOR_DEFAULT, SDL_SYSTEM_CURSOR_TEXT, SDL_SYSTEM_CURSOR_POINTER,
                                 SDL_SYSTEM_CURSOR_EW_RESIZE, SDL_SYSTEM_CURSOR_NS_RESIZE, SDL_SYSTEM_CURSOR_MOVE,
                                 SDL_SYSTEM_CURSOR_NWSE_RESIZE};
    for (int i = 0; i < 7; i++) {
        cursors_[i] = SDL_CreateSystemCursor(shapes[i]);
    }
    SDL_StartTextInput(window_);
}

SdlHost::~SdlHost() {
    painter_.reset();
    for (auto* c : cursors_) {
        if (c) {
            SDL_DestroyCursor(c);
        }
    }
    if (gl_) {
        SDL_GL_DestroyContext(gl_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

void SdlHost::setUi(Ui& ui) {
    ui_ = &ui;
    ui.host = this;
}

float SdlHost::ratio() const { return SDL_GetWindowPixelDensity(window_); }

bool SdlHost::frame(const std::function<void()>& beforeDraw) {
    SDL_Event e;
    Ui& ui = *ui_;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: closing_ = true; break;
            case SDL_EVENT_MOUSE_MOTION: {
                float r = ratio();
                ui.mouseMoved(e.motion.x * r / ui.unit, e.motion.y * r / ui.unit);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                ui.mods = mapMods(SDL_GetModState());
                int b = e.button.button == SDL_BUTTON_LEFT ? 0 : e.button.button == SDL_BUTTON_RIGHT ? 1 : 2;
                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    ui.mousePressed(b);
                } else {
                    ui.mouseReleased(b);
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: ui.wheel(e.wheel.y); break;
            case SDL_EVENT_KEY_DOWN: {
                int m = mapMods(e.key.mod);
                ui.mods = m;
                if (int k = mapKey(e.key.key)) {
                    ui.key(k, m);
                }
                break;
            }
            case SDL_EVENT_KEY_UP: ui.mods = mapMods(e.key.mod); break;
            case SDL_EVENT_TEXT_INPUT:
                for (char32_t c : toU32(e.text.text)) {
                    ui.typed(c);
                }
                break;
            default: break;
        }
    }
    if (closing_) {
        return false;
    }
    int fw = 0, fh = 0;
    SDL_GetWindowSizeInPixels(window_, &fw, &fh);
    if (fw <= 0 || fh <= 0) {
        return true;
    }
    float unit = uiScale_ > 0 ? uiScale_ * std::max(1.0f, std::round(ratio()))
                              : std::clamp(std::round(fh / 400.0f), 1.0f, 8.0f);
    ui.setSize(fw / unit, fh / unit, unit);
    std::uint64_t now = SDL_GetTicksNS();
    ui.frame(last_ == 0 ? 1 / 60.0f : float(double(now - last_) / 1e9));
    last_ = now;
    if (beforeDraw) {
        beforeDraw();
    }
    painter_->begin(fw, fh, unit);
    painter_->clear(clear_);
    ui.draw(*painter_);
    painter_->end();
    SDL_GL_SwapWindow(window_);
    return true;
}

void SdlHost::setSize(int w, int h) {
    if (fullscreen_) {
        setFullscreen(false);
    }
    SDL_SetWindowSize(window_, w, h);
}

void SdlHost::setFullscreen(bool on) {
    if (on == fullscreen_) {
        return;
    }
    SDL_SetWindowFullscreenMode(window_, nullptr);      // (the desktop's mode, no decorations)
    SDL_SetWindowFullscreen(window_, on);
    SDL_SyncWindow(window_);
    fullscreen_ = on;
    if (ui_) {
        ui_->setBarAutoHide(on);
    }
}

void SdlHost::setTitle(const std::string& t) { SDL_SetWindowTitle(window_, t.c_str()); }

std::string SdlHost::clipboard() {
    char* t = SDL_GetClipboardText();
    std::string s = t ? t : "";
    SDL_free(t);
    return s;
}

void SdlHost::setClipboard(const std::string& text) { SDL_SetClipboardText(text.c_str()); }

void SdlHost::setCursor(int shape) {
    if (shape != cursorShown_ && shape >= 0 && shape < 7 && cursors_[shape]) {
        SDL_SetCursor(cursors_[shape]);
        cursorShown_ = shape;
    }
}

}  // namespace slopui
