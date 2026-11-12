package slopui.gl;

import static org.lwjgl.opengl.GL33.*;

import java.nio.ByteBuffer;
import org.lwjgl.system.MemoryUtil;
import slopui.Image;

/** A texture from ARGB pixels (RGBA8, non-premultiplied). */
public final class GlImage implements Image {
    final int texture, w, h;

    GlImage(int w, int h, int[] argb, boolean smooth, boolean repeat) {
        this.w = w;
        this.h = h;
        texture = glGenTextures();
        glBindTexture(GL_TEXTURE_2D, texture);
        ByteBuffer b = MemoryUtil.memAlloc(w * h * 4);
        for (int i = 0; i < w * h; i++) {
            int c = argb == null ? 0xFFFFFFFF : argb[i];
            b.put((byte) (c >> 16)).put((byte) (c >> 8)).put((byte) c).put((byte) (c >>> 24));
        }
        b.flip();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, b);
        MemoryUtil.memFree(b);
        int f = smooth ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
        int wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    }

    @Override
    public int width() {
        return w;
    }

    @Override
    public int height() {
        return h;
    }

    @Override
    public void dispose() {
        glDeleteTextures(texture);
    }
}
