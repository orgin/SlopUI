package slopui;

import java.util.function.BooleanSupplier;
import java.util.function.Supplier;

/**
 * A push button: acts on the release over it; moving off before releasing cancels. Space or Enter when
 * focused. Hover glows, the press sinks it; the default button has an accent edge.
 */
public class Button extends Widget {
    private Supplier<String> text;
    private Runnable action;
    boolean accent;
    private boolean down;
    private BooleanSupplier enabledWhen;

    public Button(Supplier<String> text, Runnable action) {
        this.text = text;
        this.action = action;
    }

    public Button(String text, Runnable action) {
        this(() -> text, action);
    }

    public Button accent(boolean a) {
        accent = a;
        return this;
    }

    /** Enabled only while the condition holds (asked each time it is drawn or pressed). */
    public Button enabledWhen(BooleanSupplier condition) {
        enabledWhen = condition;
        return this;
    }

    @Override
    public boolean isEnabled() {
        return super.isEnabled() && (enabledWhen == null || enabledWhen.getAsBoolean());
    }

    public Button onClick(Runnable r) {
        action = r;
        return this;
    }

    public void click(Ui ui) {
        if (action != null && isEnabled()) {
            action.run();
        }
    }

    @Override
    public float prefWidth(Ui ui) {
        return Math.max(44, ui.theme.font.width(text.get()) + 2 * ui.theme.s[Theme.PAD] + 8);
    }

    @Override
    public float minWidth(Ui ui) {
        return Math.min(prefWidth(ui), ui.theme.font.width(text.get()) + 10);
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return ui.theme.s[Theme.BUTTON_HEIGHT];
    }

    @Override
    public boolean focusable() {
        return true;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        boolean en = isEnabled();
        boolean over = ui.isHover(this);
        float hv = animate(ui, 0, en && (over || down) ? 1 : 0, t.s[Theme.HOVER_TIME]);
        float pr = animate(ui, 1, down && over ? 1 : 0, t.s[Theme.PRESS_TIME]);
        float bh = Math.min(h, t.s[Theme.BUTTON_HEIGHT]);
        float by = y + (h - bh) / 2;
        Skin.button(p, t, x, by, w, bh, hv, pr, en, accent, ui.isFocused(this));
        Font f = t.font;
        String s = Skin.fit(f, text.get(), w - 6);
        float tx = x + (w - f.width(s)) / 2, ty = by + (bh - f.lineHeight()) / 2 + pr;
        int c = !en ? t.c[Theme.TEXT_DISABLED] : Shapes.lerp(t.c[Theme.BUTTON_TEXT], t.c[Theme.TEXT_BRIGHT], hv);
        f.draw(p, s, tx + 0.5f, ty + 0.5f, 0x90000000);
        f.draw(p, s, tx, ty, c);
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0) {
            return false;
        }
        down = true;
        return true;
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        boolean on = down && contains(mx, my);
        down = false;
        if (on) {
            click(ui);
        }
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key == Keys.SPACE || Keys.isEnter(key)) {
            click(ui);
            return true;
        }
        return false;
    }

    @Override
    public int cursor(float mx, float my) {
        return Cursor.HAND;
    }
}
