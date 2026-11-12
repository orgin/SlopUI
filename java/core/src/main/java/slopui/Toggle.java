package slopui;

import java.util.function.Consumer;

/** An on/off switch: the knob slides and the track lights up; text beside it. */
public class Toggle extends Widget {
    private final String text;
    private boolean on;
    private Consumer<Boolean> onChange;
    private boolean down;

    public Toggle(String text, boolean on) {
        this.text = text;
        this.on = on;
    }

    public Toggle onChange(Consumer<Boolean> c) {
        onChange = c;
        return this;
    }

    public boolean isOn() {
        return on;
    }

    public void set(boolean v) {
        on = v;
    }

    private void toggle() {
        on = !on;
        if (onChange != null) {
            onChange.accept(on);
        }
    }

    @Override
    public float prefWidth(Ui ui) {
        return 32 + ui.theme.font.width(text);
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
        float k = animate(ui, 1, on ? 1 : 0, 0.12f);
        float tw = 20, th = 9, tx = x, ty = y + (h - th) / 2;
        float[] track = Shapes.cut(tx, ty, tw, th, 3, 0, 3, 0);
        Shapes.poly(p, track, t.c[Theme.TRACK_TOP], t.c[Theme.TRACK_BOTTOM], ty, th);
        if (k > 0) {
            float[] lit = Shapes.cut(tx, ty, tw, th, 3, 0, 3, 0);
            Shapes.poly(p, lit, Shapes.alpha(t.c[Theme.FILL_TOP], k * (en ? 1 : 0.3f)),
                    Shapes.alpha(t.c[Theme.FILL_BOTTOM], k * (en ? 1 : 0.3f)), ty, th);
            Shapes.glow(p, lit, 3 * k, Shapes.alpha(t.c[Theme.GLOW], 0.5f * k * (en ? 1 : 0)));
        }
        Shapes.outline(p, track, 1, Shapes.lerp(t.c[Theme.FIELD_BORDER], t.c[Theme.ACCENT_SOFT], Math.max(hv, k * 0.6f)));
        float kw = 9, kx = tx + 1 + (tw - kw - 2) * k;
        Skin.button(p, t, kx, ty - 1, kw, th + 2, hv, 0, en, false, false);
        if (ui.isFocused(this)) {
            Shapes.outline(p, Shapes.offset(track, 2), 1, Shapes.alpha(t.c[Theme.FOCUS], 0.8f));
        }
        Font f = t.font;
        f.draw(p, text, x + 26, y + (h - f.lineHeight()) / 2,
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
        if (key == Keys.SPACE || Keys.isEnter(key)) {
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
