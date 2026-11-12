package slopui;

/** Text in UI units: a TrueType font (the backend's), or several as one ({@link FallbackFont}). */
public interface Font {
    /** Whether the font has a glyph for the code point. */
    boolean has(int cp);

    /** The pen's advance after the code point, UI units. */
    float advance(int cp);

    /** Line height, UI units (the text's box: drawn with its top at y). */
    float lineHeight();

    /** One glyph with the line's top at (x, y). */
    void drawGlyph(Painter p, int cp, float x, float y, int argb);

    default float width(String s) {
        float w = 0;
        for (int i = 0; i < s.length(); ) {
            int cp = s.codePointAt(i);
            w += advance(cp);
            i += Character.charCount(cp);
        }
        return w;
    }

    default void draw(Painter p, String s, float x, float y, int argb) {
        float px = x;
        for (int i = 0; i < s.length(); ) {
            int cp = s.codePointAt(i);
            if (cp != ' ') {
                drawGlyph(p, cp, px, y, argb);
            }
            px += advance(cp);
            i += Character.charCount(cp);
        }
    }
}
