package slopui;

import java.util.function.Consumer;
import java.util.function.DoubleFunction;

/**
 * A value between min and max in steps: drag the knob, press the track to jump there, the wheel and the
 * arrow keys step it. The value shown at the right (formatted).
 */
public class Slider extends Widget {
    private final double min, max, step;
    private double value;
    private Consumer<Double> onChange;
    private DoubleFunction<String> format = v -> String.valueOf(v);
    private boolean dragging;

    public Slider(double min, double max, double step, double value) {
        this.min = min;
        this.max = max;
        this.step = step;
        this.value = value;
    }

    public Slider onChange(Consumer<Double> c) {
        onChange = c;
        return this;
    }

    public Slider format(DoubleFunction<String> f) {
        format = f;
        return this;
    }

    public double value() {
        return value;
    }

    public void set(double v) {
        double s = step > 0 ? Math.round((v - min) / step) * step + min : v;
        s = Math.max(min, Math.min(max, s));
        if (s != value) {
            value = s;
            if (onChange != null) {
                onChange.accept(value);
            }
        }
    }

    private float labelW(Ui ui) {
        return format == null ? 0 : Math.max(ui.theme.font.width(format.apply(max)), ui.theme.font.width(format.apply(min))) + 6;
    }

    private float trackW(Ui ui) {
        return w - labelW(ui) - 8;
    }

    @Override
    public float prefWidth(Ui ui) {
        return 120 + labelW(ui);
    }

    @Override
    public float minWidth(Ui ui) {
        return 50 + labelW(ui);
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return ui.theme.s[Theme.ROW] + 2;
    }

    @Override
    public boolean focusable() {
        return true;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        boolean en = isEnabled();
        float hv = animate(ui, 0, en && (ui.isHover(this) || dragging) ? 1 : 0, t.s[Theme.HOVER_TIME]);
        float tx = x + 4, tw = trackW(ui), cy = y + h / 2;
        float frac = (float) ((value - min) / (max - min));
        Skin.field(p, t, tx, cy - 2.5f, tw, 5, ui.isFocused(this) ? 1 : 0, en);
        float fw = tw * frac;
        if (fw > 1) {
            Shapes.hgradient(p, tx + 1, cy - 1.5f, fw - 1, 3, t.c[Theme.FILL_BOTTOM], en ? t.c[Theme.FILL_TOP] : t.c[Theme.TEXT_DISABLED]);
        }
        float kx = tx + fw, kh = 11, kw = 7;
        float[] knob = {kx - kw / 2, cy - kh / 2 + 2, kx - kw / 2 + 2, cy - kh / 2, kx + kw / 2, cy - kh / 2,
                kx + kw / 2, cy + kh / 2 - 2, kx + kw / 2 - 2, cy + kh / 2, kx - kw / 2, cy + kh / 2};
        if (hv > 0) {
            Shapes.glow(p, knob, 4 * hv, Shapes.alpha(t.c[Theme.GLOW], hv));
        }
        Shapes.poly(p, knob, Shapes.lerp(t.c[Theme.BUTTON_TOP], t.c[Theme.HOVER_TOP], hv),
                Shapes.lerp(t.c[Theme.BUTTON_BOTTOM], t.c[Theme.HOVER_BOTTOM], hv), cy - kh / 2, kh);
        Shapes.outline(p, knob, 1, en ? Shapes.lerp(t.c[Theme.BUTTON_BORDER], t.c[Theme.ACCENT], hv) : t.c[Theme.FIELD_BORDER]);
        Shapes.fill(p, kx - 0.5f, cy - 2, 1, 4, Shapes.alpha(t.c[Theme.ACCENT], en ? 0.8f : 0.2f));
        if (format != null) {
            Font f = t.font;
            String s = format.apply(value);
            f.draw(p, s, x + w - f.width(s), y + (h - f.lineHeight()) / 2, en ? t.c[Theme.TEXT_BRIGHT] : t.c[Theme.TEXT_DISABLED]);
        }
    }

    private void setFrom(Ui ui, float mx) {
        float frac = (mx - x - 4) / trackW(ui);
        set(min + Math.max(0, Math.min(1, frac)) * (max - min));
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0) {
            return false;
        }
        dragging = true;
        setFrom(ui, mx);
        return true;
    }

    @Override
    public void drag(Ui ui, float mx, float my) {
        if (dragging) {
            setFrom(ui, mx);
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        dragging = false;
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        set(value + Math.signum(dy) * stepOr());
        return true;
    }

    private double stepOr() {
        return step > 0 ? step : (max - min) / 100;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        switch (key) {
            case Keys.LEFT, Keys.DOWN -> set(value - stepOr());
            case Keys.RIGHT, Keys.UP -> set(value + stepOr());
            case Keys.HOME -> set(min);
            case Keys.END -> set(max);
            default -> {
                return false;
            }
        }
        return true;
    }

    @Override
    public int cursor(float mx, float my) {
        return Cursor.HAND;
    }
}
