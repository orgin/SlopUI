package slopui;

import java.util.Map;

/**
 * Colours, sizes and fonts the widgets are drawn with. Colours and sizes are indexed by the constants below
 * ({@code theme.c[TEXT]}, {@code theme.s[ROW]}); a theme file (JSON) sets them by name:
 * <pre>{ "name": "Game", "colours": { "text": "#C8C8B8", ... }, "sizes": { "row": 12, ... } }</pre>
 * Colours are "#RRGGBB" or "#AARRGGBB". Fonts are given by the host ({@link #font}).
 */
public final class Theme {
    // Colours
    public static final int TEXT = 0, TEXT_DIM = 1, TEXT_DISABLED = 2, TEXT_BRIGHT = 3, ACCENT = 4, ACCENT_SOFT = 5,
            PANEL_TOP = 6, PANEL_BOTTOM = 7, PANEL_BORDER = 8, PANEL_LIGHT = 9, TITLE_TOP = 10, TITLE_BOTTOM = 11,
            TITLE_TEXT = 12, BAR_TOP = 13, BAR_BOTTOM = 14, BUTTON_TOP = 15, BUTTON_BOTTOM = 16, HOVER_TOP = 17,
            HOVER_BOTTOM = 18, PRESS_TOP = 19, PRESS_BOTTOM = 20, BUTTON_BORDER = 21, BUTTON_TEXT = 22,
            FIELD_TOP = 23, FIELD_BOTTOM = 24, FIELD_BORDER = 25, FOCUS = 26, SELECTION = 27, SELECTION_TEXT = 28,
            ROW_HOVER = 29, TRACK_TOP = 30, TRACK_BOTTOM = 31, FILL_TOP = 32, FILL_BOTTOM = 33, GLOW = 34,
            SHADOW = 35, WARNING = 36, TOOLTIP_TOP = 37, TOOLTIP_BOTTOM = 38, SHADE = 39, ROW_ALT = 40,
            TEXTURE = 41;
    public static final String[] COLOUR_NAMES = {"text", "textDim", "textDisabled", "textBright", "accent",
            "accentSoft", "panelTop", "panelBottom", "panelBorder", "panelLight", "titleTop", "titleBottom",
            "titleText", "barTop", "barBottom", "buttonTop", "buttonBottom", "hoverTop", "hoverBottom", "pressTop",
            "pressBottom", "buttonBorder", "buttonText", "fieldTop", "fieldBottom", "fieldBorder", "focus",
            "selection", "selectionText", "rowHover", "trackTop", "trackBottom", "fillTop", "fillBottom", "glow",
            "shadow", "warning", "tooltipTop", "tooltipBottom", "shade", "rowAlt", "texture"};

    // Sizes (UI units; times in seconds)
    public static final int ROW = 0, PAD = 1, GAP = 2, CUT = 3, BORDER = 4, GLOW_SIZE = 5, TITLE_HEIGHT = 6,
            BAR_HEIGHT = 7, SCROLLBAR = 8, SHADOW_SIZE = 9, TEXTURE_STRENGTH = 10, HOVER_TIME = 11, PRESS_TIME = 12,
            OPEN_TIME = 13, TOOLTIP_DELAY = 14, MAX_WINDOW = 15, BUTTON_HEIGHT = 16, SPLITTER = 17;
    public static final String[] SIZE_NAMES = {"row", "pad", "gap", "cut", "border", "glowSize", "titleHeight",
            "barHeight", "scrollbar", "shadowSize", "textureStrength", "hoverTime", "pressTime", "openTime",
            "tooltipDelay", "maxWindow", "buttonHeight", "splitter"};

    public String name = "Game";
    public final int[] c = new int[COLOUR_NAMES.length];
    public final float[] s = new float[SIZE_NAMES.length];
    /** The text font (set by the host); bold/title font: the same unless set. */
    public Font font, titleFont;
    /** The panel texture, made from TEXTURE / TEXTURE_STRENGTH on first use per painter (Skin). */
    Image texture;
    Painter textureOwner;

    /** The built-in game look: dark olive panels, light olive edges, yellow accents . */
    public Theme() {
        int[] cs = {
                0xFFC8C8B8, 0xFF8A9078, 0xFF545C48, 0xFFF4F0D8, 0xFFE8D860, 0xFFB8C890,
                0xF2242C1E, 0xF20C100A, 0xFF5A6A48, 0x50B8C890, 0xFF3E4E2E, 0xFF1A2214,
                0xFFF0E8B8, 0xF02C3624, 0xF0101410, 0xFF3A4630, 0xFF1A2216, 0xFF5A6E40,
                0xFF26321C, 0xFF10140C, 0xFF2C3822, 0xFF74885A, 0xFFE0E0C8,
                0xFF080A06, 0xFF141810, 0xFF4A5838, 0xFFE8D860, 0xFF586A2C, 0xFFFFFFE8,
                0x28B8C890, 0xFF080A06, 0xFF1A2014, 0xFFE0E880, 0xFF6A8434, 0x90D0E070,
                0xA0000000, 0xFFFF7060, 0xF8303C26, 0xF8141A10, 0x80000000, 0x10B8C890,
                0xFFB8C890};
        System.arraycopy(cs, 0, c, 0, c.length);
        float[] ss = {12, 6, 4, 4, 1, 5, 14, 12, 6, 8, 0.07f, 0.12f, 0.06f, 0.14f, 0.6f, 0.92f, 16, 5};
        System.arraycopy(ss, 0, s, 0, s.length);
    }

    /** A copy (fonts shared). */
    public Theme copy() {
        Theme t = new Theme();
        t.name = name;
        System.arraycopy(c, 0, t.c, 0, c.length);
        System.arraycopy(s, 0, t.s, 0, s.length);
        t.font = font;
        t.titleFont = titleFont;
        return t;
    }

    public Font titleFont() {
        return titleFont != null ? titleFont : font;
    }

    /** A theme file read over this theme's values (missing keys keep them). */
    @SuppressWarnings("unchecked")
    public Theme read(String json) {
        Theme t = copy();
        Map<String, Object> root = (Map<String, Object>) Json.parse(json);
        if (root.get("name") instanceof String n) {
            t.name = n;
        }
        if (root.get("colours") instanceof Map<?, ?> m) {
            for (Map.Entry<?, ?> e : m.entrySet()) {
                int k = index(COLOUR_NAMES, (String) e.getKey());
                if (k >= 0) {
                    t.c[k] = parseColour((String) e.getValue());
                }
            }
        }
        if (root.get("sizes") instanceof Map<?, ?> m) {
            for (Map.Entry<?, ?> e : m.entrySet()) {
                int k = index(SIZE_NAMES, (String) e.getKey());
                if (k >= 0) {
                    t.s[k] = ((Number) e.getValue()).floatValue();
                }
            }
        }
        return t;
    }

    /** This theme as a theme file. */
    public String write() {
        StringBuilder b = new StringBuilder("{\n  \"name\": \"").append(name).append("\",\n  \"colours\": {\n");
        for (int i = 0; i < c.length; i++) {
            b.append("    \"").append(COLOUR_NAMES[i]).append("\": \"#")
                    .append(String.format("%08X", c[i])).append(i + 1 < c.length ? "\",\n" : "\"\n");
        }
        b.append("  },\n  \"sizes\": {\n");
        for (int i = 0; i < s.length; i++) {
            b.append("    \"").append(SIZE_NAMES[i]).append("\": ").append(s[i]).append(i + 1 < s.length ? ",\n" : "\n");
        }
        return b.append("  }\n}\n").toString();
    }

    private static int index(String[] names, String key) {
        for (int i = 0; i < names.length; i++) {
            if (names[i].equals(key)) {
                return i;
            }
        }
        return -1;
    }

    public static int parseColour(String v) {
        String h = v.startsWith("#") ? v.substring(1) : v;
        long x = Long.parseLong(h, 16);
        return (int) (h.length() <= 6 ? 0xFF000000L | x : x);
    }
}
