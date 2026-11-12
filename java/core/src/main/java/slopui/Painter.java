package slopui;

/**
 * What the toolkit draws with: triangles, optionally textured, in UI units (the backend multiplies by
 * {@link #unit()} and snaps to device pixels). The one drawing primitive; {@link Shapes} builds the rest.
 */
public interface Painter {
    /** Device pixels per UI unit. */
    float unit();

    /**
     * n vertices (a multiple of 3): xy in UI units, uv in image pixels (ignored when image is null: a plain
     * colour), one ARGB colour per vertex (non-premultiplied).
     */
    void triangles(Image image, float[] xy, float[] uv, int[] argb, int n);

    /** Drawing after this is cut to the rectangle (UI units), inside the current clip. */
    void pushClip(float x, float y, float w, float h);

    void popClip();

    /** Every colour after this has its alpha multiplied by a (0..1; window fades). */
    void setOpacity(float a);

    float opacity();

    /** An image from ARGB pixels; smooth: linear filtering (else nearest); repeat: tiles when uv runs outside. */
    Image createImage(int w, int h, int[] argb, boolean smooth, boolean repeat);
}
