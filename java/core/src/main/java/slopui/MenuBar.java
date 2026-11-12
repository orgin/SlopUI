package slopui;

import java.util.ArrayList;
import java.util.List;

/**
 * The bar of menus at the top. A press on a title opens its menu (again: closes it); with a menu open,
 * moving over another title opens that one. F10 opens the first menu, Alt + a title's first letter that menu;
 * the arrow keys move through the menus.
 */
public final class MenuBar extends Widget {
    final List<Menu> menus = new ArrayList<>();
    Ui ui;
    private int open = -1;
    private MenuPopup popup;
    private float[] titleX = new float[0], titleW = new float[0];

    public MenuBar add(Menu m) {
        menus.add(m);
        return this;
    }

    public boolean isEmpty() {
        return menus.isEmpty();
    }

    public boolean isOpen() {
        return open >= 0;
    }

    void closed() {
        open = -1;
        popup = null;
    }

    @Override
    public float prefWidth(Ui ui) {
        return ui.width;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return ui.barHeight();
    }

    @Override
    public void layout(Ui ui) {
        Font f = ui.theme.font;
        titleX = new float[menus.size()];
        titleW = new float[menus.size()];
        float px = 6;
        for (int i = 0; i < menus.size(); i++) {
            float tw = f.width(menus.get(i).label()) + 12;
            titleX[i] = px;
            titleW[i] = tw;
            px += tw;
        }
    }

    private int titleAt(float mx, float my) {
        if (my < y || my >= y + h) {
            return -1;
        }
        for (int i = 0; i < titleX.length; i++) {
            if (mx >= titleX[i] && mx < titleX[i] + titleW[i]) {
                return i;
            }
        }
        return -1;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        Shapes.vgradient(p, x, y, w, h, t.c[Theme.BAR_TOP], t.c[Theme.BAR_BOTTOM]);
        Skin.texture(p, t, Shapes.cut(x, y, w, h, 0, 0, 0, 0), y, h, 1);
        Shapes.hgradient(p, x, y + h - 1, w * 0.6f, 1, t.c[Theme.PANEL_BORDER], Shapes.alpha(t.c[Theme.PANEL_BORDER], 0));
        Font f = t.font;
        int hot = titleAt(ui.mx, ui.my);
        for (int i = 0; i < menus.size(); i++) {
            boolean on = i == open;
            float hv = animate(ui, Math.min(3, i), on ? 1 : i == hot && ui.isHover(this) ? 0.6f : 0, t.s[Theme.HOVER_TIME]);
            if (i > 3) {
                hv = on ? 1 : i == hot ? 0.6f : 0;
            }
            if (hv > 0) {
                float[] pts = Shapes.cut(titleX[i], y + 1, titleW[i], h - 2, 3, 0, 3, 0);
                Shapes.poly(p, pts, Shapes.alpha(t.c[Theme.HOVER_TOP], hv), Shapes.alpha(t.c[Theme.HOVER_BOTTOM], hv), y, h);
                Shapes.fill(p, titleX[i] + 3, y + h - 2, titleW[i] - 6, 1, Shapes.alpha(t.c[Theme.ACCENT], hv));
            }
            f.draw(p, menus.get(i).label(), titleX[i] + 6, y + (h - f.lineHeight()) / 2,
                    Shapes.lerp(t.c[Theme.TEXT], t.c[Theme.TEXT_BRIGHT], hv));
        }
    }

    void openMenu(Ui ui, int i, boolean keyboard) {
        ui.closePopups();
        Menu m = menus.get(i);
        if (m.onOpen != null) {
            m.onOpen.run();
        }
        open = i;
        popup = new MenuPopup(m, this);
        popup.owner = this;
        popup.anchor(titleX[i], y, titleW[i], h, false);
        if (keyboard) {
            popup.selectFirst();
        }
        ui.openPopup(popup);
    }

    /** Left / right in an open menu: the next menu along the bar. */
    void step(Ui ui, int d) {
        if (open >= 0 && !menus.isEmpty()) {
            openMenu(ui, (open + d + menus.size()) % menus.size(), true);
        }
    }

    boolean openByKey(Ui ui, int key) {
        if (menus.isEmpty()) {
            return false;
        }
        layout(ui);
        if (key == Keys.F10) {
            openMenu(ui, 0, true);
            return true;
        }
        for (int i = 0; i < menus.size(); i++) {
            String l = menus.get(i).label();
            if (!l.isEmpty() && Character.toUpperCase(l.charAt(0)) == (char) key) {
                openMenu(ui, i, true);
                return true;
            }
        }
        return false;
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        int i = titleAt(mx, my);
        if (i < 0) {
            ui.closePopups();
            return true;
        }
        if (i == open) {
            ui.closePopups();
        } else {
            openMenu(ui, i, false);
        }
        return true;
    }

    /** With a menu open, the pointer over another title opens it. */
    void moved(Ui ui, float mx, float my) {
        if (open < 0) {
            return;
        }
        int i = titleAt(mx, my);
        if (i >= 0 && i != open) {
            openMenu(ui, i, false);
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        // a press on a title dragged onto an item and released there runs it (as native menus do)
        if (popup != null) {
            popup.releaseAt(ui, mx, my);
        }
    }
}
