package slopui;

/** One of a {@link RadioGroup}: a diamond lit when selected; arrow keys move the selection in the group. */
public class RadioButton extends Widget {
    private final RadioGroup group;
    private final int index;
    private final String text;
    private boolean down;

    RadioButton(RadioGroup group, int index, String text) {
        this.group = group;
        this.index = index;
        this.text = text;
    }

    @Override
    public float prefWidth(Ui ui) {
        return 20 + ui.theme.font.width(text);
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return ui.theme.s[Theme.ROW];
    }

    @Override
    public boolean focusable() {
        return true;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        boolean en = isEnabled();
        boolean sel = group.selected() == index;
        float hv = animate(ui, 0, en && ui.isHover(this) ? 1 : 0, t.s[Theme.HOVER_TIME]);
        float s = animate(ui, 1, sel ? 1 : 0, 0.12f);
        float cx = x + 5, cy = y + h / 2, r = 5;
        float[] d = {cx, cy - r, cx + r, cy, cx, cy + r, cx - r, cy};
        Shapes.poly(p, d, t.c[Theme.FIELD_TOP], t.c[Theme.FIELD_BOTTOM], cy - r, 2 * r);
        Shapes.outline(p, d, 1, Shapes.lerp(t.c[Theme.FIELD_BORDER], t.c[Theme.FOCUS], ui.isFocused(this) ? 1 : hv * 0.6f));
        if (s > 0) {
            float q = 2.6f * s;
            float[] in = {cx, cy - q, cx + q, cy, cx, cy + q, cx - q, cy};
            int c = en ? t.c[Theme.ACCENT] : t.c[Theme.TEXT_DISABLED];
            Shapes.glow(p, in, 2.5f * s, Shapes.alpha(t.c[Theme.GLOW], en ? 0.7f * s : 0));
            Shapes.poly(p, in, c, c, cy - q, 2 * q);
        }
        Font f = t.font;
        f.draw(p, text, x + 14, y + (h - f.lineHeight()) / 2,
                !en ? t.c[Theme.TEXT_DISABLED] : Shapes.lerp(t.c[Theme.TEXT], t.c[Theme.TEXT_BRIGHT], hv));
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        down = button == 0;
        return down;
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        if (down && contains(mx, my)) {
            group.select(index);
        }
        down = false;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        int n = group.buttons.size();
        if (key == Keys.SPACE) {
            group.select(index);
            return true;
        }
        if (key == Keys.DOWN || key == Keys.RIGHT || key == Keys.UP || key == Keys.LEFT) {
            int d = key == Keys.DOWN || key == Keys.RIGHT ? 1 : n - 1;
            int i = (group.selected() + d) % n;
            group.select(i);
            ui.setFocus(group.buttons.get(i));
            return true;
        }
        return false;
    }

    @Override
    public int cursor(float mx, float my) {
        return Cursor.HAND;
    }
}
