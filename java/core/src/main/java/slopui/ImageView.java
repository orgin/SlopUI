package slopui;

/** An image (or a part of one), scaled to fit and centred; made from ARGB pixels on first draw. */
public class ImageView extends Widget {
    private final int iw, ih;
    private final int[] argb;
    private final boolean smooth;
    private Image image;
    private Painter owner;
    private final float prefW, prefH;

    public ImageView(int iw, int ih, int[] argb, float prefW, float prefH, boolean smooth) {
        this.iw = iw;
        this.ih = ih;
        this.argb = argb;
        this.prefW = prefW;
        this.prefH = prefH;
        this.smooth = smooth;
    }

    @Override
    public float prefWidth(Ui ui) {
        return prefW;
    }

    @Override
    public float minWidth(Ui ui) {
        return Math.min(prefW, 24);
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return prefH * Math.min(1, width / prefW);
    }

    @Override
    public void draw(Ui ui, Painter p) {
        if (image == null || owner != p) {
            image = p.createImage(iw, ih, argb, smooth, false);
            owner = p;
        }
        float s = Math.min(w / iw, h / ih);
        float dw = iw * s, dh = ih * s;
        Shapes.image(p, image, 0, 0, iw, ih, x + (w - dw) / 2, y + (h - dh) / 2, dw, dh, 0xFFFFFFFF);
    }
}
