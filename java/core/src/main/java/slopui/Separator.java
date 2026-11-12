package slopui;

/** A line between parts, fading at its ends; horizontal or vertical. */
public class Separator extends Widget {
    private final boolean vertical;

    public Separator(boolean vertical) {
        this.vertical = vertical;
    }

    public Separator() {
        this(false);
    }

    @Override
    public float prefWidth(Ui ui) {
        return vertical ? 7 : 10;
    }

    @Override
    public float minWidth(Ui ui) {
        return vertical ? 7 : 0;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return vertical ? 10 : 7;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        int c = ui.theme.c[Theme.PANEL_BORDER];
        int lite = Shapes.alpha(ui.theme.c[Theme.ACCENT_SOFT], 0.25f);
        if (vertical) {
            float cx = Math.round(x + w / 2);
            Shapes.vgradient(p, cx, y, 1, h / 2, 0, c);
            Shapes.vgradient(p, cx, y + h / 2, 1, h / 2, c, 0);
            Shapes.vgradient(p, cx + 1, y, 1, h / 2, 0, lite);
            Shapes.vgradient(p, cx + 1, y + h / 2, 1, h / 2, lite, 0);
        } else {
            float cy = Math.round(y + h / 2);
            Shapes.hgradient(p, x, cy, w / 2, 1, 0, c);
            Shapes.hgradient(p, x + w / 2, cy, w / 2, 1, c, 0);
            Shapes.hgradient(p, x, cy + 1, w / 2, 1, 0, lite);
            Shapes.hgradient(p, x + w / 2, cy + 1, w / 2, 1, lite, 0);
        }
    }
}
