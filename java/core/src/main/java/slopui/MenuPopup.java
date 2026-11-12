package slopui;

import java.util.List;

/** An open menu: its items, ticks, shortcuts and submenus; runs an item on the release over it. */
public final class MenuPopup extends Popup {
    private final Menu menu;
    private final MenuBar bar;
    private int sel = -1;
    private MenuPopup child;
    private int childIndex = -1;
    private float scroll;

    MenuPopup(Menu menu, MenuBar bar) {
        this.menu = menu;
        this.bar = bar;
    }

    private float rowH(Ui ui, MenuItem it) {
        return it.separator ? 5 : ui.theme.s[Theme.ROW] + 1;
    }

    @Override
    public float prefWidth(Ui ui) {
        Font f = ui.theme.font;
        float lw = 0, sw = 0;
        for (MenuItem it : menu.items) {
            lw = Math.max(lw, f.width(it.label.get()));
            if (it.shortcut != null) {
                sw = Math.max(sw, f.width(it.shortcut) + 12);
            }
        }
        return 16 + lw + sw + 18;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        float s = 6;
        for (MenuItem it : menu.items) {
            s += rowH(ui, it);
        }
        return s;
    }

    void selectFirst() {
        sel = -1;
        move(1);
    }

    private void move(int d) {
        List<MenuItem> items = menu.items;
        for (int k = 0; k < items.size(); k++) {
            sel = (sel + d + items.size()) % items.size();
            if (items.get(sel).selectable()) {
                return;
            }
        }
        sel = -1;
    }

    private int itemAt(Ui ui, float mx, float my) {
        if (!contains(mx, my)) {
            return -1;
        }
        float py = y + 3 - scroll;
        for (int i = 0; i < menu.items.size(); i++) {
            float rh = rowH(ui, menu.items.get(i));
            if (my >= py && my < py + rh) {
                return menu.items.get(i).separator ? -1 : i;
            }
            py += rh;
        }
        return -1;
    }

    private float itemY(Ui ui, int index) {
        float py = y + 3 - scroll;
        for (int i = 0; i < index; i++) {
            py += rowH(ui, menu.items.get(i));
        }
        return py;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        float o = opening(ui);
        float before = p.opacity();
        p.setOpacity(before * o);
        float drop = (1 - o) * -4;
        Skin.panel(p, t, x, y + drop, w, h, t.s[Theme.CUT], true);
        Font f = t.font;
        p.pushClip(x, y, w, h);
        float py = y + 3 - scroll + drop;
        for (int i = 0; i < menu.items.size(); i++) {
            MenuItem it = menu.items.get(i);
            float rh = rowH(ui, it);
            if (it.separator) {
                Shapes.hgradient(p, x + 6, py + 2, (w - 12) / 2, 1, 0, t.c[Theme.PANEL_BORDER]);
                Shapes.hgradient(p, x + w / 2, py + 2, (w - 12) / 2, 1, t.c[Theme.PANEL_BORDER], 0);
                py += rh;
                continue;
            }
            boolean on = i == sel && it.selectable();
            if (on) {
                float[] pts = Shapes.cut(x + 3, py, w - 6, rh, 3, 0, 3, 0);
                Shapes.poly(p, pts, t.c[Theme.HOVER_TOP], t.c[Theme.HOVER_BOTTOM], py, rh);
                Shapes.fill(p, x + 3, py, 2, rh, t.c[Theme.ACCENT]);
            }
            int col = !it.enabled.getAsBoolean() ? t.c[Theme.TEXT_DISABLED] : on ? t.c[Theme.TEXT_BRIGHT] : t.c[Theme.TEXT];
            float ty = py + (rh - f.lineHeight()) / 2;
            if (it.checked != null && it.checked.getAsBoolean()) {
                if (it.radio) {
                    float cx = x + 9, cy = py + rh / 2;
                    float[] d = {cx, cy - 3, cx + 3, cy, cx, cy + 3, cx - 3, cy};
                    Shapes.poly(p, d, t.c[Theme.ACCENT], t.c[Theme.ACCENT], cy - 3, 6);
                } else {
                    Skin.tick(p, x + 4, py + (rh - 9) / 2, 9, 1, t.c[Theme.ACCENT]);
                }
            }
            f.draw(p, it.label.get(), x + 16, ty, col);
            if (it.shortcut != null) {
                f.draw(p, it.shortcut, x + w - 10 - f.width(it.shortcut), ty, t.c[Theme.TEXT_DIM]);
            }
            if (it.submenu != null) {
                Skin.chevron(p, x + w - 8, py + rh / 2, 5, 1, col);
            }
            py += rh;
        }
        p.popClip();
        p.setOpacity(before);
    }

    /** The pointer over the menu: its item selected, a submenu opened. */
    void hoverAt(Ui ui, float mx, float my) {
        int hot = itemAt(ui, mx, my);
        if (hot >= 0 && hot != sel) {
            sel = hot;
            openChild(ui);
        }
    }

    private void openChild(Ui ui) {
        if (child != null) {
            ui.closePopup(child);
            child = null;
            childIndex = -1;
        }
        if (sel < 0) {
            return;
        }
        MenuItem it = menu.items.get(sel);
        if (it.submenu != null && it.selectable()) {
            if (it.submenu.onOpen != null) {
                it.submenu.onOpen.run();
            }
            child = new MenuPopup(it.submenu, bar);
            child.owner = this;
            child.anchor(x, itemY(ui, sel) - 3, w, rowH(ui, it), true);
            childIndex = sel;
            ui.openPopup(child);
        }
    }

    private void run(Ui ui, int i) {
        MenuItem it = menu.items.get(i);
        if (!it.selectable() || it.submenu != null) {
            return;
        }
        ui.closePopups();
        if (it.action != null) {
            it.action.run();
        }
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        return true;
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        releaseAt(ui, mx, my);
    }

    /** The release (also of a press that began on the bar): the item under the pointer runs. */
    void releaseAt(Ui ui, float mx, float my) {
        List<Popup> open = ui.popups();
        for (int k = open.size() - 1; k >= 0; k--) {
            if (open.get(k) instanceof MenuPopup mp && mp.contains(mx, my)) {
                int i = mp.itemAt(ui, mx, my);
                if (i >= 0) {
                    mp.run(ui, i);
                }
                return;
            }
        }
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        float over = prefHeight(ui, w) - h;
        if (over > 0) {
            scroll = Math.max(0, Math.min(over, scroll - dy * 12));
        }
        return true;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        switch (key) {
            case Keys.DOWN -> move(1);
            case Keys.UP -> move(-1);
            case Keys.RIGHT -> {
                if (sel >= 0 && menu.items.get(sel).submenu != null) {
                    openChild(ui);
                    if (child != null) {
                        child.selectFirst();
                    }
                } else if (bar != null) {
                    bar.step(ui, 1);
                }
            }
            case Keys.LEFT -> {
                if (owner instanceof MenuPopup) {
                    ui.closePopup(this);
                } else if (bar != null) {
                    bar.step(ui, -1);
                }
            }
            case Keys.ENTER, Keys.KP_ENTER, Keys.SPACE -> {
                if (sel >= 0) {
                    if (menu.items.get(sel).submenu != null) {
                        openChild(ui);
                        if (child != null) {
                            child.selectFirst();
                        }
                    } else {
                        run(ui, sel);
                    }
                }
            }
            case Keys.ESCAPE -> ui.closePopup(this);
            default -> {
                return false;
            }
        }
        return true;
    }
}
