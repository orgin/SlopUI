package slopui.gl;

import static org.lwjgl.opengl.GL33.*;

import java.nio.ByteBuffer;
import java.util.ArrayDeque;
import java.util.Deque;
import org.lwjgl.system.MemoryUtil;
import slopui.Image;
import slopui.Painter;

/**
 * The toolkit's painter on OpenGL 3.3: triangles batched per texture (a white pixel for plain colours),
 * positions snapped to device pixels, scissor clipping, a global opacity. Begin/end around a frame.
 */
public final class GlPainter implements Painter, AutoCloseable {
    private static final int STRIDE = 20, MAX = 1 << 16;
    private final int program, vao, vbo, uScreen;
    private final ByteBuffer buf = MemoryUtil.memAlloc(MAX * STRIDE);
    private int count;
    private GlImage current;
    private final GlImage white;
    private int fbW, fbH;
    private float unit = 1, opacity = 1;
    private final Deque<int[]> clips = new ArrayDeque<>();

    public GlPainter() {
        program = program(
                "#version 330 core\n"
                        + "layout(location=0) in vec2 pos; layout(location=1) in vec2 uv; layout(location=2) in vec4 col;\n"
                        + "uniform vec2 screen; out vec2 vUv; out vec4 vCol;\n"
                        + "void main(){ vUv=uv; vCol=col; gl_Position=vec4(pos.x/screen.x*2.0-1.0, 1.0-pos.y/screen.y*2.0, 0.0, 1.0); }\n",
                "#version 330 core\n"
                        + "in vec2 vUv; in vec4 vCol; uniform sampler2D tex; out vec4 o;\n"
                        + "void main(){ o = texture(tex, vUv) * vCol; }\n");
        uScreen = glGetUniformLocation(program, "screen");
        vao = glGenVertexArrays();
        vbo = glGenBuffers();
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (long) MAX * STRIDE, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, STRIDE, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, STRIDE, 8);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, STRIDE, 16);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        white = new GlImage(1, 1, new int[] {0xFFFFFFFF}, false, false);
    }

    private static int program(String vs, String fs) {
        int v = shader(GL_VERTEX_SHADER, vs), f = shader(GL_FRAGMENT_SHADER, fs);
        int p = glCreateProgram();
        glAttachShader(p, v);
        glAttachShader(p, f);
        glLinkProgram(p);
        if (glGetProgrami(p, GL_LINK_STATUS) == 0) {
            throw new IllegalStateException(glGetProgramInfoLog(p));
        }
        return p;
    }

    private static int shader(int type, String src) {
        int s = glCreateShader(type);
        glShaderSource(s, src);
        glCompileShader(s);
        if (glGetShaderi(s, GL_COMPILE_STATUS) == 0) {
            throw new IllegalStateException(glGetShaderInfoLog(s));
        }
        return s;
    }

    /** A frame begins: the framebuffer's size and the device pixels per UI unit. */
    public void begin(int fbW, int fbH, float unit) {
        this.fbW = fbW;
        this.fbH = fbH;
        this.unit = unit;
        opacity = 1;
        clips.clear();
        glViewport(0, 0, fbW, fbH);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(program);
        glUniform2f(uScreen, fbW, fbH);
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        current = null;
        count = 0;
    }

    public void end() {
        flush();
        glDisable(GL_SCISSOR_TEST);
    }

    private void flush() {
        if (count == 0) {
            return;
        }
        buf.flip();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, buf);
        glBindTexture(GL_TEXTURE_2D, current.texture);
        glDrawArrays(GL_TRIANGLES, 0, count);
        buf.clear();
        count = 0;
    }

    @Override
    public float unit() {
        return unit;
    }

    @Override
    public void triangles(Image image, float[] xy, float[] uv, int[] argb, int n) {
        GlImage img = image == null ? white : (GlImage) image;
        if (img != current) {
            flush();
            current = img;
        }
        float iw = img.w, ih = img.h;
        for (int i = 0; i < n; i++) {
            if (count == MAX) {
                flush();
            }
            float px = Math.round(xy[i * 2] * unit), py = Math.round(xy[i * 2 + 1] * unit);
            float u = image == null ? 0.5f : uv[i * 2] / iw, v = image == null ? 0.5f : uv[i * 2 + 1] / ih;
            int c = argb[i];
            int a = Math.round((c >>> 24) * opacity);
            buf.putFloat(px).putFloat(py).putFloat(u).putFloat(v)
                    .put((byte) (c >> 16)).put((byte) (c >> 8)).put((byte) c).put((byte) a);
            count++;
        }
    }

    @Override
    public void pushClip(float x, float y, float w, float h) {
        int x0 = Math.round(x * unit), y0 = Math.round(y * unit), x1 = Math.round((x + w) * unit),
                y1 = Math.round((y + h) * unit);
        int[] top = clips.peek();
        if (top != null) {
            x0 = Math.max(x0, top[0]);
            y0 = Math.max(y0, top[1]);
            x1 = Math.min(x1, top[2]);
            y1 = Math.min(y1, top[3]);
        }
        x1 = Math.max(x0, x1);
        y1 = Math.max(y0, y1);
        flush();
        clips.push(new int[] {x0, y0, x1, y1});
        scissor();
    }

    @Override
    public void popClip() {
        flush();
        clips.pop();
        scissor();
    }

    private void scissor() {
        int[] c = clips.peek();
        if (c == null) {
            glDisable(GL_SCISSOR_TEST);
        } else {
            glEnable(GL_SCISSOR_TEST);
            glScissor(c[0], fbH - c[3], c[2] - c[0], c[3] - c[1]);
        }
    }

    @Override
    public void setOpacity(float a) {
        opacity = Math.max(0, Math.min(1, a));
    }

    @Override
    public float opacity() {
        return opacity;
    }

    @Override
    public Image createImage(int w, int h, int[] argb, boolean smooth, boolean repeat) {
        flush();
        GlImage i = new GlImage(w, h, argb, smooth, repeat);
        if (current != null) {
            glBindTexture(GL_TEXTURE_2D, current.texture);
        }
        return i;
    }

    @Override
    public void close() {
        MemoryUtil.memFree(buf);
        white.dispose();
        glDeleteBuffers(vbo);
        glDeleteVertexArrays(vao);
        glDeleteProgram(program);
    }
}
