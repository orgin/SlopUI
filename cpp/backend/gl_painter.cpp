// SlopUI GlPainter (Java: slopui.gl.GlPainter, GlImage): OpenGL 3.3 core or ES 3.0, functions from SDL.
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "slopui/sdl.hpp"

namespace slopui {

namespace {

using GLuint = unsigned;
using GLint = int;
using GLenum = unsigned;
using GLsizei = int;
using GLboolean = unsigned char;
using GLchar = char;
using GLfloat = float;
using GLsizeiptr = std::ptrdiff_t;
using GLintptr = std::ptrdiff_t;
constexpr GLenum TEXTURE_2D = 0x0DE1, RGBA = 0x1908, RGBA8 = 0x8058, UNSIGNED_BYTE = 0x1401, FLOAT = 0x1406,
                 LINEAR = 0x2601, NEAREST = 0x2600, MIN_FILTER = 0x2801, MAG_FILTER = 0x2800, WRAP_S = 0x2802,
                 WRAP_T = 0x2803, REPEAT = 0x2901, CLAMP = 0x812F, UNPACK_ALIGNMENT = 0x0CF5, VERTEX_SHADER = 0x8B31,
                 FRAGMENT_SHADER = 0x8B30, COMPILE_STATUS = 0x8B81, LINK_STATUS = 0x8B82, ARRAY_BUFFER = 0x8892,
                 STREAM_DRAW = 0x88E0, BLEND = 0x0BE2, DEPTH_TEST = 0x0B71, CULL_FACE = 0x0B44,
                 SCISSOR_TEST = 0x0C11, SRC_ALPHA = 0x0302, ONE_MINUS_SRC_ALPHA = 0x0303, ONE = 1, TRIANGLES = 4,
                 TEXTURE0 = 0x84C0, COLOR_BUFFER_BIT = 0x4000;

/** The GL functions the painter uses, loaded through SDL. */
struct Gl {
#define SLOPUI_GL(ret, name, ...) ret(SDLCALL* name)(__VA_ARGS__) = nullptr;
#define SLOPUI_GL_LIST                                                                                         \
    SLOPUI_GL(void, GenTextures, GLsizei, GLuint*)                                                             \
    SLOPUI_GL(void, BindTexture, GLenum, GLuint)                                                               \
    SLOPUI_GL(void, TexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)    \
    SLOPUI_GL(void, TexParameteri, GLenum, GLenum, GLint)                                                      \
    SLOPUI_GL(void, DeleteTextures, GLsizei, const GLuint*)                                                    \
    SLOPUI_GL(void, PixelStorei, GLenum, GLint)                                                                \
    SLOPUI_GL(GLuint, CreateShader, GLenum)                                                                    \
    SLOPUI_GL(void, ShaderSource, GLuint, GLsizei, const GLchar* const*, const GLint*)                         \
    SLOPUI_GL(void, CompileShader, GLuint)                                                                     \
    SLOPUI_GL(void, GetShaderiv, GLuint, GLenum, GLint*)                                                       \
    SLOPUI_GL(void, GetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*)                                      \
    SLOPUI_GL(GLuint, CreateProgram)                                                                           \
    SLOPUI_GL(void, AttachShader, GLuint, GLuint)                                                              \
    SLOPUI_GL(void, BindAttribLocation, GLuint, GLuint, const GLchar*)                                         \
    SLOPUI_GL(void, LinkProgram, GLuint)                                                                       \
    SLOPUI_GL(void, GetProgramiv, GLuint, GLenum, GLint*)                                                      \
    SLOPUI_GL(void, GetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*)                                     \
    SLOPUI_GL(GLint, GetUniformLocation, GLuint, const GLchar*)                                                \
    SLOPUI_GL(void, GenVertexArrays, GLsizei, GLuint*)                                                         \
    SLOPUI_GL(void, BindVertexArray, GLuint)                                                                   \
    SLOPUI_GL(void, GenBuffers, GLsizei, GLuint*)                                                              \
    SLOPUI_GL(void, BindBuffer, GLenum, GLuint)                                                                \
    SLOPUI_GL(void, BufferData, GLenum, GLsizeiptr, const void*, GLenum)                                       \
    SLOPUI_GL(void, VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)               \
    SLOPUI_GL(void, EnableVertexAttribArray, GLuint)                                                           \
    SLOPUI_GL(void, Viewport, GLint, GLint, GLsizei, GLsizei)                                                  \
    SLOPUI_GL(void, Disable, GLenum)                                                                           \
    SLOPUI_GL(void, Enable, GLenum)                                                                            \
    SLOPUI_GL(void, BlendFuncSeparate, GLenum, GLenum, GLenum, GLenum)                                         \
    SLOPUI_GL(void, UseProgram, GLuint)                                                                        \
    SLOPUI_GL(void, Uniform2f, GLint, GLfloat, GLfloat)                                                        \
    SLOPUI_GL(void, Uniform1i, GLint, GLint)                                                                   \
    SLOPUI_GL(void, ActiveTexture, GLenum)                                                                     \
    SLOPUI_GL(void, DrawArrays, GLenum, GLint, GLsizei)                                                        \
    SLOPUI_GL(void, Scissor, GLint, GLint, GLsizei, GLsizei)                                                   \
    SLOPUI_GL(void, ClearColor, GLfloat, GLfloat, GLfloat, GLfloat)                                            \
    SLOPUI_GL(void, Clear, GLenum)                                                                             \
    SLOPUI_GL(void, DeleteBuffers, GLsizei, const GLuint*)                                                     \
    SLOPUI_GL(void, DeleteVertexArrays, GLsizei, const GLuint*)                                                \
    SLOPUI_GL(void, DeleteProgram, GLuint)
    SLOPUI_GL_LIST
#undef SLOPUI_GL

    void load() {
#define SLOPUI_GL(ret, name, ...)                                                                  \
    name = reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress("gl" #name));                     \
    if (!name) throw std::runtime_error("OpenGL function gl" #name " missing");
        SLOPUI_GL_LIST
#undef SLOPUI_GL
    }
};

Gl gl;
bool loaded = false;

class GlImage : public Image {
public:
    GlImage(int w, int h, const argb* px, bool smooth, bool repeat) : w_(w), h_(h) {
        gl.GenTextures(1, &tex);
        gl.BindTexture(TEXTURE_2D, tex);
        std::vector<unsigned char> b(size_t(w) * h * 4);
        for (int i = 0; i < w * h; i++) {
            argb c = px ? px[i] : 0xFFFFFFFF;
            b[i * 4] = c >> 16 & 0xFF;
            b[i * 4 + 1] = c >> 8 & 0xFF;
            b[i * 4 + 2] = c & 0xFF;
            b[i * 4 + 3] = c >> 24;
        }
        gl.PixelStorei(UNPACK_ALIGNMENT, 1);
        gl.TexImage2D(TEXTURE_2D, 0, GLint(RGBA8), w, h, 0, RGBA, UNSIGNED_BYTE, b.data());
        GLint f = GLint(smooth ? LINEAR : NEAREST), wrap = GLint(repeat ? REPEAT : CLAMP);
        gl.TexParameteri(TEXTURE_2D, MIN_FILTER, f);
        gl.TexParameteri(TEXTURE_2D, MAG_FILTER, f);
        gl.TexParameteri(TEXTURE_2D, WRAP_S, wrap);
        gl.TexParameteri(TEXTURE_2D, WRAP_T, wrap);
    }
    ~GlImage() override { gl.DeleteTextures(1, &tex); }
    int width() const override { return w_; }
    int height() const override { return h_; }
    GLuint tex = 0;

private:
    int w_, h_;
};

GLuint shader(GLenum type, const std::string& src) {
    GLuint s = gl.CreateShader(type);
    const char* c = src.c_str();
    gl.ShaderSource(s, 1, &c, nullptr);
    gl.CompileShader(s);
    GLint ok = 0;
    gl.GetShaderiv(s, COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl.GetShaderInfoLog(s, sizeof log, nullptr, log);
        throw std::runtime_error(std::string("shader: ") + log);
    }
    return s;
}

}  // namespace

constexpr size_t MAX_VERTS = 1 << 16;

GlPainter::GlPainter(bool es) {
    if (!loaded) {
        gl.load();
        loaded = true;
    }
    std::string head = es ? "#version 300 es\nprecision mediump float;\n" : "#version 330 core\n";
    std::string vs = head +
                     "in vec2 pos; in vec2 uv; in vec4 col; uniform vec2 screen; out vec2 vUv; out vec4 vCol;\n"
                     "void main(){ vUv=uv; vCol=col; gl_Position=vec4(pos.x/screen.x*2.0-1.0, 1.0-pos.y/screen.y*2.0, 0.0, 1.0); }\n";
    std::string fs = head + "in vec2 vUv; in vec4 vCol; uniform sampler2D tex; out vec4 o;\n"
                            "void main(){ o = texture(tex, vUv) * vCol; }\n";
    program_ = gl.CreateProgram();
    gl.AttachShader(program_, shader(VERTEX_SHADER, vs));
    gl.AttachShader(program_, shader(FRAGMENT_SHADER, fs));
    gl.BindAttribLocation(program_, 0, "pos");
    gl.BindAttribLocation(program_, 1, "uv");
    gl.BindAttribLocation(program_, 2, "col");
    gl.LinkProgram(program_);
    GLint ok = 0;
    gl.GetProgramiv(program_, LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl.GetProgramInfoLog(program_, sizeof log, nullptr, log);
        throw std::runtime_error(std::string("program: ") + log);
    }
    uScreen_ = gl.GetUniformLocation(program_, "screen");
    gl.GenVertexArrays(1, &vao_);
    gl.GenBuffers(1, &vbo_);
    gl.BindVertexArray(vao_);
    gl.BindBuffer(ARRAY_BUFFER, vbo_);
    gl.BufferData(ARRAY_BUFFER, GLsizeiptr(MAX_VERTS * sizeof(Vertex)), nullptr, STREAM_DRAW);
    gl.VertexAttribPointer(0, 2, FLOAT, 0, sizeof(Vertex), reinterpret_cast<void*>(0));
    gl.VertexAttribPointer(1, 2, FLOAT, 0, sizeof(Vertex), reinterpret_cast<void*>(8));
    gl.VertexAttribPointer(2, 4, UNSIGNED_BYTE, 1, sizeof(Vertex), reinterpret_cast<void*>(16));
    gl.EnableVertexAttribArray(0);
    gl.EnableVertexAttribArray(1);
    gl.EnableVertexAttribArray(2);
    argb white = 0xFFFFFFFF;
    white_ = std::make_shared<GlImage>(1, 1, &white, false, false);
    verts_.reserve(MAX_VERTS);
}

GlPainter::~GlPainter() {
    white_.reset();
    gl.DeleteBuffers(1, &vbo_);
    gl.DeleteVertexArrays(1, &vao_);
    gl.DeleteProgram(program_);
}

void GlPainter::begin(int fbW, int fbH, float unit) {
    fbW_ = fbW;
    fbH_ = fbH;
    unit_ = unit;
    opacity_ = 1;
    clips_.clear();
    gl.Viewport(0, 0, fbW, fbH);
    gl.Disable(DEPTH_TEST);
    gl.Disable(CULL_FACE);
    gl.Disable(SCISSOR_TEST);
    gl.Enable(BLEND);
    gl.BlendFuncSeparate(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, ONE, ONE_MINUS_SRC_ALPHA);
    gl.UseProgram(program_);
    gl.Uniform2f(uScreen_, float(fbW), float(fbH));
    gl.Uniform1i(gl.GetUniformLocation(program_, "tex"), 0);
    gl.BindVertexArray(vao_);
    gl.BindBuffer(ARRAY_BUFFER, vbo_);
    gl.ActiveTexture(TEXTURE0);
    current_ = nullptr;
    verts_.clear();
}

void GlPainter::clear(argb c) {
    gl.ClearColor((c >> 16 & 0xFF) / 255.0f, (c >> 8 & 0xFF) / 255.0f, (c & 0xFF) / 255.0f, 1);
    gl.Clear(COLOR_BUFFER_BIT);
}

void GlPainter::end() {
    flush();
    gl.Disable(SCISSOR_TEST);
}

void GlPainter::flush() {
    if (verts_.empty()) {
        return;
    }
    gl.BufferData(ARRAY_BUFFER, GLsizeiptr(MAX_VERTS * sizeof(Vertex)), nullptr, STREAM_DRAW);
    using SubData = void(SDLCALL*)(GLenum, GLintptr, GLsizeiptr, const void*);
    static SubData sub = reinterpret_cast<SubData>(SDL_GL_GetProcAddress("glBufferSubData"));
    sub(ARRAY_BUFFER, 0, GLsizeiptr(verts_.size() * sizeof(Vertex)), verts_.data());
    gl.BindTexture(TEXTURE_2D, static_cast<const GlImage*>(current_)->tex);
    gl.DrawArrays(TRIANGLES, 0, GLsizei(verts_.size()));
    verts_.clear();
}

void GlPainter::triangles(const Image* image, const float* xy, const float* uv, const argb* colours, int n) {
    const Image* img = image ? image : white_.get();
    if (img != current_) {
        flush();
        current_ = img;
    }
    float iw = float(img->width()), ih = float(img->height());
    for (int i = 0; i < n; i++) {
        if (verts_.size() == MAX_VERTS) {
            flush();
        }
        argb c = colours[i];
        Vertex v;
        v.x = std::round(xy[i * 2] * unit_);
        v.y = std::round(xy[i * 2 + 1] * unit_);
        v.u = image ? uv[i * 2] / iw : 0.5f;
        v.v = image ? uv[i * 2 + 1] / ih : 0.5f;
        v.r = c >> 16 & 0xFF;
        v.g = c >> 8 & 0xFF;
        v.b = c & 0xFF;
        v.a = static_cast<unsigned char>(std::lround((c >> 24) * opacity_));
        verts_.push_back(v);
    }
}

void GlPainter::pushClip(float x, float y, float w, float h) {
    int x0 = int(std::lround(x * unit_)), y0 = int(std::lround(y * unit_)), x1 = int(std::lround((x + w) * unit_)),
        y1 = int(std::lround((y + h) * unit_));
    if (!clips_.empty()) {
        auto& t = clips_.back();
        x0 = std::max(x0, t[0]);
        y0 = std::max(y0, t[1]);
        x1 = std::min(x1, t[2]);
        y1 = std::min(y1, t[3]);
    }
    x1 = std::max(x0, x1);
    y1 = std::max(y0, y1);
    flush();
    clips_.push_back({x0, y0, x1, y1});
    scissor();
}

void GlPainter::popClip() {
    flush();
    clips_.pop_back();
    scissor();
}

void GlPainter::scissor() {
    if (clips_.empty()) {
        gl.Disable(SCISSOR_TEST);
    } else {
        auto& c = clips_.back();
        gl.Enable(SCISSOR_TEST);
        gl.Scissor(c[0], fbH_ - c[3], c[2] - c[0], c[3] - c[1]);
    }
}

void GlPainter::setOpacity(float a) { opacity_ = std::clamp(a, 0.0f, 1.0f); }

std::shared_ptr<Image> GlPainter::createImage(int w, int h, const argb* pixels, bool smooth, bool repeat) {
    flush();
    auto i = std::make_shared<GlImage>(w, h, pixels, smooth, repeat);
    current_ = nullptr;     // (the texture binding changed)
    return i;
}

}  // namespace slopui
