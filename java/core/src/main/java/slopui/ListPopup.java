package slopui;

import java.util.List;
import java.util.function.IntConsumer;

/** A list to pick from under a field (dropdowns, comboboxes): hover, arrows, Enter; scrolls when long. */
public final class ListPopup extends Popup {
    private final List<String> items;
    private final IntConsumer onPick;
    private int sel;
    private float scroll;
    private static final int MAX_ROWS = 10;

    public ListPopup(Widget owner, List<String> items, int selected, IntConsumer onPick) {
        this.owner = owner;
        this.items = items;
        this.sel = selected;
        this.onPick = onPick;
        anchor(owner.x, owner.y, owner.w, owner.h, false);
    }

    private float rowH(Ui ui) {
        return ui.theme.s[Theme.ROW] + 1;
    }

    @Override
    public float prefWidth(Ui ui) {
        float m = 0;
        for (String s : items) {
            m = Math.max(m, ui.theme.font.width(s));
        }
        return m + 16;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return Math.min(items.size(), MAX_ROWS) * rowH(ui) + 4;
    }

    private int at(Ui ui, float my) {
        int i = (int) Math.floor((my - y - 2 + scroll) / rowH(ui));
        return i >= 0 && i < items.size() ? i : -1;
    }

    private void reveal(Ui ui) {
        float rh = rowH(ui), view = h - 4;
        if (sel * rh < scroll) {
            scroll = sel * rh;
        }
        if ((sel + 1) * rh > scroll + view) {
            scroll = (sel + 1) * rh - view;
        }
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        float o = opening(ui);
        float before = p.opacity();
        p.setOpacity(before * o);
        Skin.panel(p, t, x, y, w, h * (0.6f + 0.4f * o), t.s[Theme.CUT], true);
        if (contains(ui.mx, ui.my) && ui.isHover(this)) {
            int i = at(ui, ui.my);
            if (i >= 0) {
                sel = i;
            }
        }
        Font f = t.font;
        float rh = rowH(ui);
        p.pushClip(x + 1, y + 2, w - 2, h - 4);
        for (int i = 0; i < items.size(); i++) {
            float ry = y + 2 + i * rh - scroll;
            if (ry + rh < y || ry > y + h) {
                continue;
            }
            if (i == sel) {
                float[] pts = Shapes.cut(x + 2, ry, w - 4, rh, 3, 0, 3, 0);
                Shapes.poly(p, pts, t.c[Theme.HOVER_TOP], t.c[Theme.HOVER_BOTTOM], ry, rh);
                Shapes.fill(p, x + 2, ry, 2, rh, t.c[Theme.ACCENT]);
            }
            f.draw(p, items.get(i), x + 8, ry + (rh - f.lineHeight()) / 2,
                    i == sel ? t.c[Theme.TEXT_BRIGHT] : t.c[Theme.TEXT]);
        }
        p.popClip();
        float total = items.size() * rh, view = h - 4;
        if (total > view) {
            float mh = Math.max(8, view * view / total), my = y + 2 + (view - mh) * scroll / (total - view);
            Shapes.fill(p, x + w - 3, my, 2, mh, t.c[Theme.ACCENT_SOFT]);
        }
        p.setOpacity(before);
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        int i = contains(mx, my) ? at(ui, my) : -1;
        if (i >= 0) {
            ui.closePopup(this);
            onPick.accept(i);
        }
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        float max = Math.max(0, items.size() * rowH(ui) - (h - 4));
        scroll = Math.max(0, Math.min(max, scroll - dy * rowH(ui) * 2));
        return true;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        switch (key) {
            case Keys.DOWN -> sel = Math.min(items.size() - 1, sel + 1);
            case Keys.UP -> sel = Math.max(0, sel - 1);
            case Keys.PAGE_DOWN -> sel = Math.min(items.size() - 1, sel + MAX_ROWS);
            case Keys.PAGE_UP -> sel = Math.max(0, sel - MAX_ROWS);
            case Keys.HOME -> sel = 0;
            case Keys.END -> sel = items.size() - 1;
            case Keys.ENTER, Keys.KP_ENTER, Keys.SPACE -> {
                ui.closePopup(this);
                if (sel >= 0) {
                    onPick.accept(sel);
                }
                return true;
            }
            case Keys.ESCAPE -> {
                ui.closePopup(this);
                return true;
            }
            default -> {
                return false;
            }
        }
        reveal(ui);
        return true;
    }
}
