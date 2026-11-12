package slopui;

/**
 * How the widgets look, drawn from the theme: textured gradient panels with cut corners, bevelled buttons
 * that glow on hover, recessed fields. (Step 6 adds parts from image assemblies; a part without art falls
 * back to these.)
 */
public final class Skin {
    private Skin() {
    }

    /** The theme's panel texture: brushed streaks, white, alpha from TEXTURE_STRENGTH (repeat, smooth). */
    public static Image texture(Painter p, Theme t) {
        if (t.texture == null || t.textureOwner != p) {
            int n = 128;
            int[] px = new int[n * n];
            long seed = 0x5EED1977L;
            float[] row = new float[n];
            for (int y = 0; y < n; y++) {
                for (int x = 0; x < n; x++) {
                    seed = seed * 6364136223846793005L + 1442695040888963407L;
                    row[x] = (seed >>> 40 & 0xFFFF) / 65535f;
                }
                // streaks: each row smoothed along x (brushed metal), plus a little grain
                seed = seed * 6364136223846793005L + 1442695040888963407L;
                float base = (seed >>> 40 & 0xFFFF) / 65535f;
                for (int x = 0; x < n; x++) {
                    float sum = 0;
                    for (int k = -6; k <= 6; k++) {
                        sum += row[(x + k + n) % n];
                    }
                    float v = 0.55f * base + 0.25f * (sum / 13) + 0.2f * row[x];
                    int a = Math.max(0, Math.min(255, Math.round(v * 255)));
                    px[y * n + x] = a << 24 | 0xFFFFFF;
                }
            }
            if (t.texture != null) {
                t.texture.dispose();
            }
            t.texture = p.createImage(n, n, px, true, true);
            t.textureOwner = p;
        }
        return t.texture;
    }

    /** The texture over a polygon at the theme's strength. */
    public static void texture(Painter p, Theme t, float[] pts, float y0, float h, float strength) {
        float a = t.s[Theme.TEXTURE_STRENGTH] * strength;
        if (a <= 0) {
            return;
        }
        int tint = Shapes.alpha(t.c[Theme.TEXTURE], a);
        Shapes.polyTextured(p, texture(p, t), pts, tint, tint, y0, h);
    }

    /** A panel (windows, menus, popups): shadow, textured gradient, light top edge, border. */
    public static void panel(Painter p, Theme t, float x, float y, float w, float h, float cut, boolean shadow) {
        float[] pts = Shapes.cut(x, y, w, h, cut, 0, cut, 0);
        if (shadow) {
            float ss = t.s[Theme.SHADOW_SIZE];
            float[] sh = Shapes.cut(x + 1, y + 2, w, h, cut, 0, cut, 0);
            Shapes.ring(p, sh, Shapes.offset(sh, ss), t.c[Theme.SHADOW], t.c[Theme.SHADOW] & 0xFFFFFF);
            Shapes.poly(p, sh, t.c[Theme.SHADOW], t.c[Theme.SHADOW], y, h);
        }
        Shapes.poly(p, pts, t.c[Theme.PANEL_TOP], t.c[Theme.PANEL_BOTTOM], y, h);
        texture(p, t, pts, y, h, 1);
        Shapes.vgradient(p, x + cut, y + 1, w - cut - 1, Math.min(6, h / 3), t.c[Theme.PANEL_LIGHT], 0);
        Shapes.outline(p, pts, t.s[Theme.BORDER], t.c[Theme.PANEL_BORDER]);
    }

    /**
     * A button face: hover (0..1) brightens and glows, press (0..1) sinks it; accent: the default button's
     * yellow edge; focus: a focus edge.
     */
    public static void button(Painter p, Theme t, float x, float y, float w, float h, float hover, float press,
                              boolean enabled, boolean accent, boolean focus) {
        float cut = Math.min(t.s[Theme.CUT], h / 3);
        float[] pts = Shapes.cut(x, y, w, h, cut, 0, cut, 0);
        int top = Shapes.lerp(t.c[Theme.BUTTON_TOP], t.c[Theme.HOVER_TOP], hover);
        int bottom = Shapes.lerp(t.c[Theme.BUTTON_BOTTOM], t.c[Theme.HOVER_BOTTOM], hover);
        top = Shapes.lerp(top, t.c[Theme.PRESS_TOP], press);
        bottom = Shapes.lerp(bottom, t.c[Theme.PRESS_BOTTOM], press);
        if (!enabled) {
            top = Shapes.lerp(t.c[Theme.BUTTON_TOP], t.c[Theme.PANEL_BOTTOM], 0.5f);
            bottom = Shapes.lerp(t.c[Theme.BUTTON_BOTTOM], t.c[Theme.PANEL_BOTTOM], 0.5f);
        }
        if (enabled && hover > 0) {
            Shapes.glow(p, pts, t.s[Theme.GLOW_SIZE] * hover, Shapes.alpha(t.c[Theme.GLOW], hover * (1 - press * 0.5f)));
        }
        Shapes.poly(p, pts, top, bottom, y, h);
        texture(p, t, pts, y, h, 0.8f);
        // bevel: light top edge, dark bottom edge (swapped while pressed)
        int light = Shapes.alpha(0xFFFFFFFF, enabled ? 0.22f * (1 - press) + 0.06f : 0.06f);
        int dark = Shapes.alpha(0xFF000000, 0.35f + 0.3f * press);
        Shapes.vgradient(p, x + cut, y + 1, w - cut - 1, Math.max(1, h * 0.45f), light, 0);
        if (press > 0) {
            Shapes.vgradient(p, x + 1, y + 1, w - 2, Math.max(1, h * 0.3f), Shapes.alpha(0xFF000000, 0.4f * press), 0);
        }
        Shapes.fill(p, x + 1, y + h - 1, w - cut - 1, 1, dark);
        int edge = !enabled ? t.c[Theme.FIELD_BORDER]
                : accent ? Shapes.lerp(t.c[Theme.ACCENT], 0xFFFFFFFF, hover * 0.3f)
                : Shapes.lerp(t.c[Theme.BUTTON_BORDER], t.c[Theme.ACCENT_SOFT], hover);
        Shapes.outline(p, pts, t.s[Theme.BORDER], edge);
        if (focus) {
            Shapes.outline(p, Shapes.offset(pts, 2), 1, Shapes.alpha(t.c[Theme.FOCUS], 0.8f));
        }
    }

    /** A recessed field (text fields, tracks, lists): dark gradient, inner shadow, border; focus (0..1). */
    public static void field(Painter p, Theme t, float x, float y, float w, float h, float focus, boolean enabled) {
        float cut = Math.min(t.s[Theme.CUT] * 0.5f, h / 3);
        float[] pts = Shapes.cut(x, y, w, h, 0, cut, 0, cut);
        Shapes.poly(p, pts, t.c[Theme.FIELD_TOP], t.c[Theme.FIELD_BOTTOM], y, h);
        Shapes.vgradient(p, x + 1, y + 1, w - 2 - cut, Math.min(4, h / 2), 0x60000000, 0);
        int edge = Shapes.lerp(t.c[Theme.FIELD_BORDER], t.c[Theme.FOCUS], focus);
        if (!enabled) {
            edge = Shapes.lerp(t.c[Theme.FIELD_BORDER], t.c[Theme.PANEL_BOTTOM], 0.5f);
        }
        if (focus > 0) {
            Shapes.glow(p, pts, 3 * focus, Shapes.alpha(t.c[Theme.FOCUS], 0.35f * focus));
        }
        Shapes.outline(p, pts, t.s[Theme.BORDER], edge);
    }

    /** A chevron (dropdowns, menus): down, right, up or left (0-3), centred on (cx, cy), size s. */
    public static void chevron(Painter p, float cx, float cy, float s, int dir, int argb) {
        float h = s / 2;
        switch (dir) {
            case 0 -> {
                Shapes.line(p, cx - h, cy - h / 2, cx, cy + h / 2, 1.2f, argb);
                Shapes.line(p, cx, cy + h / 2, cx + h, cy - h / 2, 1.2f, argb);
            }
            case 1 -> {
                Shapes.line(p, cx - h / 2, cy - h, cx + h / 2, cy, 1.2f, argb);
                Shapes.line(p, cx + h / 2, cy, cx - h / 2, cy + h, 1.2f, argb);
            }
            case 2 -> {
                Shapes.line(p, cx - h, cy + h / 2, cx, cy - h / 2, 1.2f, argb);
                Shapes.line(p, cx, cy - h / 2, cx + h, cy + h / 2, 1.2f, argb);
            }
            default -> {
                Shapes.line(p, cx + h / 2, cy - h, cx - h / 2, cy, 1.2f, argb);
                Shapes.line(p, cx - h / 2, cy, cx + h / 2, cy + h, 1.2f, argb);
            }
        }
    }

    /** A tick mark in a box of size s at (x, y), drawn to 'amount' (0..1: animates in). */
    public static void tick(Painter p, float x, float y, float s, float amount, int argb) {
        float ax = x + s * 0.2f, ay = y + s * 0.52f, bx = x + s * 0.42f, by = y + s * 0.74f, cx = x + s * 0.82f,
                cy = y + s * 0.26f;
        float first = Math.min(1, amount * 2.5f), second = Math.max(0, Math.min(1, (amount - 0.4f) / 0.6f));
        if (first > 0) {
            Shapes.line(p, ax, ay, ax + (bx - ax) * first, ay + (by - ay) * first, 1.6f, argb);
        }
        if (second > 0) {
            Shapes.line(p, bx, by, bx + (cx - bx) * second, by + (cy - by) * second, 1.6f, argb);
        }
    }

    /** Text shortened with "..." to fit width. */
    public static String fit(Font f, String text, float width) {
        if (f.width(text) <= width) {
            return text;
        }
        String dots = "...";
        float dw = f.width(dots);
        int end = text.length();
        while (end > 0 && f.width(text.substring(0, end)) + dw > width) {
            end--;
        }
        return text.substring(0, end) + dots;
    }
}
