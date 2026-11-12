package slopui;

import java.util.function.Consumer;

/** A box with a tick (it draws itself in) and text; toggles on the release over it, Space when focused. */
public class CheckBox extends Widget {
    private final String text;
    private boolean on;
    private Consumer<Boolean> onChange;
    private boolean down;

    public CheckBox(String text, boolean on) {
        this.text = text;
        this.on = on;
    }

    public CheckBox onChange(Consumer<Boolean> c) {
        onChange = c;
        return this;
    }

    public boolean isOn() {
        return on;
    }

    public void set(boolean v) {
        on = v;
    }

    void toggle() {
        on = !on;
        if (onChange != null) {
            onChange.accept(on);
        }
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
        float hv = animate(ui, 0, en && ui.isHover(this) ? 1 : 0, t.s[Theme.HOVER_TIME]);
        float tk = animate(ui, 1, on ? 1 : 0, 0.14f);
        float s = 10, bx = x, by = y + (h - s) / 2;
        Skin.field(p, t, bx, by, s, s, Math.max(hv * 0.6f, ui.isFocused(this) ? 1 : 0), en);
        if (tk > 0) {
            Skin.tick(p, bx, by, s, tk, en ? t.c[Theme.ACCENT] : t.c[Theme.TEXT_DISABLED]);
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
            toggle();
        }
        down = false;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key == Keys.SPACE) {
            toggle();
            return true;
        }
        return false;
    }

    @Override
    public int cursor(float mx, float my) {
        return Cursor.HAND;
    }
}
