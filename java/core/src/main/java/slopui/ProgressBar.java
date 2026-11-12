package slopui;

import java.util.function.Supplier;

/** How far something is: a lit bar with a moving sheen; busy (value < 0): a light sweeping across. Text on it. */
public class ProgressBar extends Widget {
    private float value;
    private float shown;
    private Supplier<String> text;

    public ProgressBar(float value) {
        this.value = value;
    }

    /** 0..1; negative: busy, no amount known. */
    public void set(float v) {
        value = v;
    }

    public float value() {
        return value;
    }

    public ProgressBar text(Supplier<String> t) {
        text = t;
        return this;
    }

    @Override
    public float prefWidth(Ui ui) {
        return 120;
    }

    @Override
    public float minWidth(Ui ui) {
        return 40;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return ui.theme.s[Theme.ROW];
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        float bh = Math.min(h, 10), by = y + (h - bh) / 2;
        Skin.field(p, t, x, by, w, bh, 0, true);
        ui.animating = true;
        p.pushClip(x + 1, by + 1, w - 2, bh - 2);
        if (value < 0) {
            float period = 1.4f, f = (float) (ui.time % period / period);
            float sw = w * 0.3f, sx = x - sw + (w + sw) * f;
            Shapes.hgradient(p, sx, by + 1, sw / 2, bh - 2, 0, t.c[Theme.FILL_TOP]);
            Shapes.hgradient(p, sx + sw / 2, by + 1, sw / 2, bh - 2, t.c[Theme.FILL_TOP], 0);
        } else {
            shown += (value - shown) * Math.min(1, ui.dt * 12);
            float fw = (w - 2) * Math.max(0, Math.min(1, shown));
            if (fw > 0) {
                float[] pts = Shapes.cut(x + 1, by + 1, fw, bh - 2, 0, 0, Math.min(3, fw), 0);
                Shapes.poly(p, pts, t.c[Theme.FILL_TOP], t.c[Theme.FILL_BOTTOM], by, bh);
                Shapes.vgradient(p, x + 1, by + 1, fw, (bh - 2) / 2, 0x50FFFFFF, 0x00FFFFFF);
                // the sheen: a steady sweep along the whole bar (its own period, not the fill's width, which
                // changes every frame), shown only over the filled part
                float span = w + 40, sheen = (float) (ui.time * 0.7 % 1.0) * span - 20;
                p.pushClip(x + 1, by + 1, fw, bh - 2);
                Shapes.hgradient(p, x + 1 + sheen, by + 1, 10, bh - 2, 0, 0x40FFFFFF);
                Shapes.hgradient(p, x + 11 + sheen, by + 1, 10, bh - 2, 0x40FFFFFF, 0);
                p.popClip();
            }
        }
        p.popClip();
        if (text != null) {
            Font f = t.font;
            String s = text.get();
            float tx = x + (w - f.width(s)) / 2, ty = by + (bh - f.lineHeight()) / 2;
            f.draw(p, s, tx + 0.5f, ty + 0.5f, 0xC0000000);
            f.draw(p, s, tx, ty, t.c[Theme.TEXT_BRIGHT]);
        }
    }
}
