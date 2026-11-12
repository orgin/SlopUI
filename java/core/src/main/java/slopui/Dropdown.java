package slopui;

import java.util.ArrayList;
import java.util.List;
import java.util.function.IntConsumer;

/** Pick one of a list (not typed): shows the choice and a chevron; opens a list; Up/Down change it at once. */
public class Dropdown extends Widget {
    private final List<String> items = new ArrayList<>();
    private int selected;
    private IntConsumer onChange;
    private boolean down;

    public Dropdown(List<String> items, int selected) {
        this.items.addAll(items);
        this.selected = selected;
    }

    public Dropdown onChange(IntConsumer c) {
        onChange = c;
        return this;
    }

    public int selected() {
        return selected;
    }

    public void select(int i) {
        if (i != selected && i >= 0 && i < items.size()) {
            selected = i;
            if (onChange != null) {
                onChange.accept(i);
            }
        }
    }

    @Override
    public float prefWidth(Ui ui) {
        float m = 0;
        for (String s : items) {
            m = Math.max(m, ui.theme.font.width(s));
        }
        return m + 26;
    }

    @Override
    public float minWidth(Ui ui) {
        return 50;
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
        boolean open = ui.popupOpen() && ui.popups().get(ui.popups().size() - 1).owner == this;
        float hv = animate(ui, 0, isEnabled() && (ui.isHover(this) || open) ? 1 : 0, t.s[Theme.HOVER_TIME]);
        Skin.button(p, t, x, y, w, h, hv * 0.7f, down ? 0.5f : 0, isEnabled(), false, ui.isFocused(this));
        Font f = t.font;
        String s = selected >= 0 && selected < items.size() ? items.get(selected) : "";
        s = Skin.fit(f, s, w - 22);
        f.draw(p, s, x + 7, y + (h - f.lineHeight()) / 2, isEnabled() ? t.c[Theme.TEXT_BRIGHT] : t.c[Theme.TEXT_DISABLED]);
        Shapes.fill(p, x + w - 15, y + 3, 1, h - 6, Shapes.alpha(t.c[Theme.BUTTON_BORDER], 0.6f));
        Skin.chevron(p, x + w - 8, y + h / 2, 6, open ? 2 : 0, Shapes.lerp(t.c[Theme.TEXT], t.c[Theme.ACCENT], hv));
    }

    private void openList(Ui ui) {
        ui.openPopup(new ListPopup(this, items, selected, this::select));
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0) {
            return false;
        }
        down = true;
        openList(ui);
        return true;
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        down = false;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        switch (key) {
            case Keys.DOWN -> select(Math.min(items.size() - 1, selected + 1));
            case Keys.UP -> select(Math.max(0, selected - 1));
            case Keys.SPACE, Keys.ENTER, Keys.KP_ENTER -> openList(ui);
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
