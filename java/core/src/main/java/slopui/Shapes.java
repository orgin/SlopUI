package slopui;

/**
 * Shapes built from {@link Painter#triangles}: rectangles, gradients, images, convex polygons (cut corners),
 * outlines, glows and lines. Polygons are point lists x0, y0, x1, y1... clockwise on the screen (y down).
 */
public final class Shapes {
    private static final float[] NO_UV = new float[64 * 6];

    private Shapes() {
    }

    public static void quad(Painter p, float x, float y, float w, float h, int c00, int c10, int c11, int c01) {
        if (w <= 0 || h <= 0) {
            return;
        }
        float[] xy = {x, y, x + w, y, x + w, y + h, x, y, x + w, y + h, x, y + h};
        int[] c = {c00, c10, c11, c00, c11, c01};
        p.triangles(null, xy, NO_UV, c, 6);
    }

    public static void fill(Painter p, float x, float y, float w, float h, int argb) {
        quad(p, x, y, w, h, argb, argb, argb, argb);
    }

    public static void vgradient(Painter p, float x, float y, float w, float h, int top, int bottom) {
        quad(p, x, y, w, h, top, top, bottom, bottom);
    }

    public static void hgradient(Painter p, float x, float y, float w, float h, int left, int right) {
        quad(p, x, y, w, h, left, right, right, left);
    }

    /** Part (sx, sy, sw, sh) of the image (pixels) drawn at (x, y, w, h), tinted (white: as it is). */
    public static void image(Painter p, Image img, float sx, float sy, float sw, float sh, float x, float y, float w,
                             float h, int tint) {
        float[] xy = {x, y, x + w, y, x + w, y + h, x, y, x + w, y + h, x, y + h};
        float[] uv = {sx, sy, sx + sw, sy, sx + sw, sy + sh, sx, sy, sx + sw, sy + sh, sx, sy + sh};
        int[] c = {tint, tint, tint, tint, tint, tint};
        p.triangles(img, xy, uv, c, 6);
    }

    /** The image tiled over (x, y, w, h), one image pixel per device pixel, tinted. */
    public static void tile(Painter p, Image img, float x, float y, float w, float h, int tint) {
        float u = p.unit();
        image(p, img, x * u, y * u, w * u, h * u, x, y, w, h, tint);
    }

    /**
     * A rectangle with its corners cut (sci-fi panels): cuts top-left, top-right, bottom-right, bottom-left.
     * Clockwise; corners not cut are single points.
     */
    public static float[] cut(float x, float y, float w, float h, float tl, float tr, float br, float bl) {
        float m = Math.min(w, h) / 2;
        tl = Math.min(tl, m);
        tr = Math.min(tr, m);
        br = Math.min(br, m);
        bl = Math.min(bl, m);
        float[] raw = {x + tl, y, x + w - tr, y, x + w, y + tr, x + w, y + h - br, x + w - br, y + h, x + bl, y + h,
                x, y + h - bl, x, y + tl};
        float[] out = new float[16];
        int n = 0;
        for (int i = 0; i < 8; i++) {
            float px = raw[i * 2], py = raw[i * 2 + 1];
            if (n > 0 && Math.abs(out[n * 2 - 2] - px) < 1e-4f && Math.abs(out[n * 2 - 1] - py) < 1e-4f) {
                continue;
            }
            if (i == 7 && Math.abs(out[0] - px) < 1e-4f && Math.abs(out[1] - py) < 1e-4f) {
                continue;
            }
            out[n * 2] = px;
            out[n * 2 + 1] = py;
            n++;
        }
        float[] r = new float[n * 2];
        System.arraycopy(out, 0, r, 0, n * 2);
        return r;
    }

    /** A convex polygon filled; the colour by height, top at y0 to bottom at y0 + h. */
    public static void poly(Painter p, float[] pts, int top, int bottom, float y0, float h) {
        polyTextured(p, null, pts, top, bottom, y0, h);
    }

    /** As poly, textured: the image tiled one pixel per device pixel (repeat images), tinted by the colours. */
    public static void polyTextured(Painter p, Image tex, float[] pts, int top, int bottom, float y0, float h) {
        int n = pts.length / 2;
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
        float[] xy = new float[n * 6];
        float[] uv = new float[n * 6];
        int[] c = new int[n * 3];
        float u = p.unit();
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            float[] v = {cx, cy, pts[i * 2], pts[i * 2 + 1], pts[j * 2], pts[j * 2 + 1]};
            for (int k = 0; k < 3; k++) {
                xy[i * 6 + k * 2] = v[k * 2];
                xy[i * 6 + k * 2 + 1] = v[k * 2 + 1];
                uv[i * 6 + k * 2] = v[k * 2] * u;
                uv[i * 6 + k * 2 + 1] = v[k * 2 + 1] * u;
                c[i * 3 + k] = lerp(top, bottom, h <= 0 ? 0 : (v[k * 2 + 1] - y0) / h);
            }
        }
        p.triangles(tex, xy, uv, c, n * 3);
    }

    /** The polygon moved outwards by d (inwards when negative), edge by edge. */
    public static float[] offset(float[] pts, float d) {
        int n = pts.length / 2;
        float[] out = new float[n * 2];
        for (int i = 0; i < n; i++) {
            int a = (i + n - 1) % n, b = (i + 1) % n;
            float[] n1 = normal(pts[a * 2], pts[a * 2 + 1], pts[i * 2], pts[i * 2 + 1]);
            float[] n2 = normal(pts[i * 2], pts[i * 2 + 1], pts[b * 2], pts[b * 2 + 1]);
            float mx = n1[0] + n2[0], my = n1[1] + n2[1];
            float len = (float) Math.sqrt(mx * mx + my * my);
            if (len < 1e-5f) {
                mx = n2[0];
                my = n2[1];
                len = 1;
            }
            mx /= len;
            my /= len;
            float dot = mx * n2[0] + my * n2[1];
            float dist = d / Math.max(0.2f, dot);
            out[i * 2] = pts[i * 2] + mx * dist;
            out[i * 2 + 1] = pts[i * 2 + 1] + my * dist;
        }
        return out;
    }

    private static float[] normal(float x1, float y1, float x2, float y2) {
        float dx = x2 - x1, dy = y2 - y1;
        float len = (float) Math.sqrt(dx * dx + dy * dy);
        if (len < 1e-6f) {
            return new float[] {0, 0};
        }
        return new float[] {dy / len, -dx / len};
    }

    /** The band between two polygons of the same points (a, b), coloured ca at a and cb at b. */
    public static void ring(Painter p, float[] a, float[] b, int ca, int cb) {
        int n = a.length / 2;
        float[] xy = new float[n * 12];
        int[] c = new int[n * 6];
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            float[] v = {a[i * 2], a[i * 2 + 1], a[j * 2], a[j * 2 + 1], b[j * 2], b[j * 2 + 1],
                    a[i * 2], a[i * 2 + 1], b[j * 2], b[j * 2 + 1], b[i * 2], b[i * 2 + 1]};
            System.arraycopy(v, 0, xy, i * 12, 12);
            c[i * 6] = ca;
            c[i * 6 + 1] = ca;
            c[i * 6 + 2] = cb;
            c[i * 6 + 3] = ca;
            c[i * 6 + 4] = cb;
            c[i * 6 + 5] = cb;
        }
        p.triangles(null, xy, new float[n * 12], c, n * 6);
    }

    /** The polygon's edge, t thick, inside it. */
    public static void outline(Painter p, float[] pts, float t, int argb) {
        ring(p, pts, offset(pts, -t), argb, argb);
    }

    /** A soft glow outside the polygon: argb at its edge, fading out over size. */
    public static void glow(Painter p, float[] pts, float size, int argb) {
        ring(p, pts, offset(pts, size), argb, argb & 0x00FFFFFF);
    }

    /** A line t thick from (x1, y1) to (x2, y2). */
    public static void line(Painter p, float x1, float y1, float x2, float y2, float t, int argb) {
        float[] n = normal(x1, y1, x2, y2);
        float ox = n[0] * t / 2, oy = n[1] * t / 2;
        float[] xy = {x1 + ox, y1 + oy, x2 + ox, y2 + oy, x2 - ox, y2 - oy, x1 + ox, y1 + oy, x2 - ox, y2 - oy,
                x1 - ox, y1 - oy};
        int[] c = {argb, argb, argb, argb, argb, argb};
        p.triangles(null, xy, NO_UV, c, 6);
    }

    /** a to b by t (0..1), per channel. */
    public static int lerp(int a, int b, float t) {
        if (t <= 0) {
            return a;
        }
        if (t >= 1) {
            return b;
        }
        int r = 0;
        for (int s = 0; s < 32; s += 8) {
            int ca = a >>> s & 0xFF, cb = b >>> s & 0xFF;
            r |= Math.round(ca + (cb - ca) * t) << s;
        }
        return r;
    }

    /** The colour with its alpha multiplied by f. */
    public static int alpha(int argb, float f) {
        int a = Math.round((argb >>> 24) * Math.max(0, Math.min(1, f)));
        return a << 24 | argb & 0xFFFFFF;
    }
}
