package slopui;

import java.util.ArrayList;
import java.util.List;

/**
 * Pages with a tab each. Narrow: the tabs on top (wrapping onto more lines); at least 'sideWidth' wide: a
 * list on the left (Preferences). The lit tab has a sliding accent mark. Left/Right (or Up/Down at the side)
 * on a focused tab switch pages; Ctrl+Tab too.
 */
public class Tabs extends Widget {
    private final List<String> titles = new ArrayList<>();
    private final List<Widget> pages = new ArrayList<>();
    private int selected;
    private float sideWidth = Float.MAX_VALUE;
    private float markPos = -1, markLen;
    private final List<float[]> tabRects = new ArrayList<>();

    public Tabs add(String title, Widget page) {
        titles.add(title);
        pages.add(page);
        page.parent = this;
        return this;
    }

    /** At this width and more the tabs go to the left side as a list. */
    public Tabs sideWhenWider(float w) {
        sideWidth = w;
        return this;
    }

    public int selected() {
        return selected;
    }

    public void select(int i) {
        selected = Math.max(0, Math.min(pages.size() - 1, i));
    }

    private boolean side() {
        return w >= sideWidth;
    }

    private float listW(Ui ui) {
        float m = 0;
        for (String s : titles) {
            m = Math.max(m, ui.theme.font.width(s));
        }
        return m + 20;
    }

    @Override
    public void children(List<Widget> out) {
        out.add(pages.get(selected));
    }

    @Override
    public boolean focusable() {
        return true;
    }

    @Override
    public float prefWidth(Ui ui) {
        float m = 0;
        for (Widget p : pages) {
            m = Math.max(m, p.prefWidth(ui));
        }
        float tabs = 0;
        for (String s : titles) {
            tabs += ui.theme.font.width(s) + 18;
        }
        return Math.max(m, tabs);
    }

    @Override
    public float minWidth(Ui ui) {
        float m = 0;
        for (Widget p : pages) {
            m = Math.max(m, p.minWidth(ui));
        }
        return Math.max(m, listW(ui));
    }

    /** The tabs' rectangles for a width (top: wrapped lines), and the header's height. */
    private float headerFor(Ui ui, float width, float ox, float oy, List<float[]> out) {
        Font f = ui.theme.font;
        float th = ui.theme.s[Theme.ROW] + 4;
        if (width >= sideWidth) {
            float lw = listW(ui);
            for (int i = 0; i < titles.size(); i++) {
                if (out != null) {
                    out.add(new float[] {ox, oy + i * (th + 1), lw, th});
                }
            }
            return 0;
        }
        float px = 0, py = 0;
        for (String s : titles) {
            float tw = f.width(s) + 18;
            if (px > 0 && px + tw > width) {
                px = 0;
                py += th;
            }
            if (out != null) {
                out.add(new float[] {ox + px, oy + py, tw, th});
            }
            px += tw;
        }
        return py + th + 3;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        float hh = headerFor(ui, width, 0, 0, null);
        float m = 0;
        float pw = width >= sideWidth ? width - listW(ui) - 6 : width;
        for (Widget p : pages) {
            m = Math.max(m, p.prefHeight(ui, pw));
        }
        if (width >= sideWidth) {
            m = Math.max(m, titles.size() * (ui.theme.s[Theme.ROW] + 5));
        }
        return hh + m;
    }

    @Override
    public void layout(Ui ui) {
        tabRects.clear();
        float hh = headerFor(ui, w, x, y, tabRects);
        Widget p = pages.get(selected);
        if (side()) {
            float lw = listW(ui) + 6;
            p.setBounds(x + lw, y, w - lw, h);
        } else {
            p.setBounds(x, y + hh, w, h - hh);
        }
        p.layout(ui);
    }

    @Override
    public Widget hit(float px, float py) {
        if (!visible || !contains(px, py)) {
            return null;
        }
        Widget r = pages.get(selected).hit(px, py);
        return r != null ? r : this;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        Font f = t.font;
        boolean side = side();
        if (side) {
            float lw = listW(ui);
            Shapes.vgradient(p, x + lw + 2, y, 1, h, t.c[Theme.PANEL_BORDER], 0);
        } else if (!tabRects.isEmpty()) {
            float[] last = tabRects.get(tabRects.size() - 1);
            float ly = last[1] + last[3];
            Shapes.hgradient(p, x, ly, w, 1, t.c[Theme.PANEL_BORDER], Shapes.alpha(t.c[Theme.PANEL_BORDER], 0.2f));
        }
        for (int i = 0; i < tabRects.size(); i++) {
            float[] r = tabRects.get(i);
            boolean on = i == selected;
            boolean hot = ui.isHover(this) && ui.mx >= r[0] && ui.mx < r[0] + r[2] && ui.my >= r[1] && ui.my < r[1] + r[3];
            if (on || hot) {
                float[] pts = side ? Shapes.cut(r[0], r[1], r[2], r[3], 3, 0, 3, 0) : Shapes.cut(r[0], r[1], r[2] - 1, r[3], 4, 0, 0, 0);
                Shapes.poly(p, pts, on ? t.c[Theme.HOVER_TOP] : Shapes.alpha(t.c[Theme.HOVER_TOP], 0.5f),
                        on ? Shapes.alpha(t.c[Theme.HOVER_BOTTOM], 0.6f) : 0, r[1], r[3]);
            }
            f.draw(p, titles.get(i), r[0] + (side ? 10 : 9), r[1] + (r[3] - f.lineHeight()) / 2,
                    on ? t.c[Theme.TEXT_BRIGHT] : hot ? t.c[Theme.TEXT] : t.c[Theme.TEXT_DIM]);
        }
        if (selected < tabRects.size()) {
            float[] r = tabRects.get(selected);
            float target = side ? r[1] : r[0], len = side ? r[3] : r[2];
            if (markPos < 0) {
                markPos = target;
                markLen = len;
            }
            float k = Math.min(1, ui.dt * 20);
            markPos += (target - markPos) * k;
            markLen += (len - markLen) * k;
            if (Math.abs(target - markPos) > 0.1f || Math.abs(len - markLen) > 0.1f) {
                ui.animating = true;
            }
            if (side) {
                Shapes.fill(p, r[0], markPos, 2, markLen, t.c[Theme.ACCENT]);
            } else {
                Shapes.fill(p, markPos + 2, r[1] + r[3] - 2, markLen - 5, 2, t.c[Theme.ACCENT]);
            }
            if (ui.isFocused(this)) {
                Shapes.outline(p, Shapes.cut(r[0] - 1, r[1] - 1, r[2] + 1, r[3] + 2, 3, 0, 0, 0), 1, Shapes.alpha(t.c[Theme.FOCUS], 0.7f));
            }
        }
        pages.get(selected).draw(ui, p);
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0) {
            return false;
        }
        for (int i = 0; i < tabRects.size(); i++) {
            float[] r = tabRects.get(i);
            if (mx >= r[0] && mx < r[0] + r[2] && my >= r[1] && my < r[1] + r[3]) {
                selected = i;
                return true;
            }
        }
        return false;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        boolean ctrl = (mods & Keys.CTRL) != 0;
        if (key == Keys.TAB && ctrl) {
            selected = (selected + ((mods & Keys.SHIFT) != 0 ? pages.size() - 1 : 1)) % pages.size();
            return true;
        }
        if (ui.focused() != this) {
            return false;
        }
        boolean side = side();
        if (key == (side ? Keys.DOWN : Keys.RIGHT)) {
            selected = (selected + 1) % pages.size();
            return true;
        }
        if (key == (side ? Keys.UP : Keys.LEFT)) {
            selected = (selected + pages.size() - 1) % pages.size();
            return true;
        }
        return false;
    }
}
