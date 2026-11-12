package slopui;

/**
 * Two fonts as one: the primary's glyphs, and the fallback's for code points the primary lacks (a font with
 * fewer letters, symbols from another font). The fallback is centred on the primary's line.
 */
public final class FallbackFont implements Font {
    private final Font primary, fallback;

    public FallbackFont(Font primary, Font fallback) {
        this.primary = primary;
        this.fallback = fallback;
    }

    private Font of(int cp) {
        return primary.has(cp) || !fallback.has(cp) ? primary : fallback;
    }

    @Override
    public boolean has(int cp) {
        return primary.has(cp) || fallback.has(cp);
    }

    @Override
    public float advance(int cp) {
        return of(cp).advance(cp);
    }

    @Override
    public float lineHeight() {
        return primary.lineHeight();
    }

    @Override
    public void drawGlyph(Painter p, int cp, float x, float y, int argb) {
        Font f = of(cp);
        f.drawGlyph(p, cp, x, f == primary ? y : y + (primary.lineHeight() - f.lineHeight()) / 2, argb);
    }
}
