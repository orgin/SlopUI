package slopui;

import java.util.function.Consumer;

/**
 * A number to type or step: up and down arrows at the right (holding one repeats), Up/Down keys and the wheel
 * step it; kept within min..max.
 */
public class NumberField extends TextField {
    private final double min, max, step;
    private final int decimals;
    private double value;
    private Consumer<Double> onValue;
    private int held;        // 1 up, -1 down while an arrow is held
    private double heldSince;
    private double lastRepeat;

    public NumberField(double value, double min, double max, double step, int decimals) {
        super("");
        this.min = min;
        this.max = max;
        this.step = step;
        this.decimals = decimals;
        rightInset = 12;
        allow(c -> Character.isDigit(c) || c == '.' || c == '-');
        maxLength(16);
        setValue(value);
    }

    public NumberField onValue(Consumer<Double> c) {
        onValue = c;
        return this;
    }

    public double value() {
        return value;
    }

    public void setValue(double v) {
        value = Math.max(min, Math.min(max, v));
        set(format(value));
        if (onValue != null) {
            onValue.accept(value);
        }
    }

    private String format(double v) {
        return decimals == 0 ? String.valueOf(Math.round(v)) : String.format(java.util.Locale.ROOT, "%." + decimals + "f", v);
    }

    @Override
    protected void changed() {
        super.changed();
        try {
            double v = Double.parseDouble(text());
            if (v >= min && v <= max) {
                value = v;
                if (onValue != null) {
                    onValue.accept(value);
                }
            }
        } catch (NumberFormatException e) {
            // (being typed: kept until it parses)
        }
    }

    @Override
    public float prefWidth(Ui ui) {
        return Math.max(ui.theme.font.width(format(max)), ui.theme.font.width(format(min))) + 26;
    }

    @Override
    public float minWidth(Ui ui) {
        return prefWidth(ui);
    }

    @Override
    public void draw(Ui ui, Painter p) {
        super.draw(ui, p);
        Theme t = ui.theme;
        float bx = x + w - rightInset;
        Shapes.fill(p, bx, y + 2, 1, h - 4, Shapes.alpha(t.c[Theme.FIELD_BORDER], 0.8f));
        boolean onUp = ui.isHover(this) && ui.mx >= bx && ui.my < y + h / 2;
        boolean onDown = ui.isHover(this) && ui.mx >= bx && ui.my >= y + h / 2;
        if (held != 0) {
            Shapes.fill(p, bx + 1, held > 0 ? y + 1 : y + h / 2, rightInset - 2, h / 2 - 1, Shapes.alpha(t.c[Theme.ACCENT], 0.3f));
        }
        Skin.chevron(p, bx + rightInset / 2, y + h * 0.3f, 5, 2, onUp ? t.c[Theme.ACCENT] : t.c[Theme.TEXT]);
        Skin.chevron(p, bx + rightInset / 2, y + h * 0.72f, 5, 0, onDown ? t.c[Theme.ACCENT] : t.c[Theme.TEXT]);
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button == 0 && mx >= x + w - rightInset) {
            held = my < y + h / 2 ? 1 : -1;
            heldSince = ui.time;
            lastRepeat = ui.time;
            setValue(value + held * step);
            return true;
        }
        return super.press(ui, mx, my, button, clicks);
    }

    @Override
    public void tick(Ui ui) {
        if (held != 0 && ui.time - heldSince > 0.45 && ui.time - lastRepeat > 0.06) {
            lastRepeat = ui.time;
            setValue(value + held * step);
        }
        if (held != 0) {
            ui.animating = true;
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        held = 0;
        super.release(ui, mx, my, button);
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        setValue(value + Math.signum(dy) * step);
        return true;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key == Keys.UP || key == Keys.DOWN) {
            setValue(value + (key == Keys.UP ? step : -step));
            return true;
        }
        return super.key(ui, key, mods);
    }
}
