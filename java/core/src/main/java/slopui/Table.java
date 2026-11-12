package slopui;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.function.IntConsumer;

/**
 * Rows of text in columns (save games, disks): a header to sort by (again: the other way; numbers sort as
 * numbers), one row selected (click, arrows, Page Up/Down, Home/End), a double click or Enter activates it.
 * Column widths share the width by weight; text that doesn't fit ends in "...". Scrolls with a bar.
 */
public class Table extends Widget {
    private final String[] headers;
    private final float[] weights;
    private final List<String[]> rows = new ArrayList<>();
    private final List<Integer> order = new ArrayList<>();
    private int selected = -1;          // a row index (rows), not a position
    private int sortColumn = -1;
    private boolean descending;
    private final int visibleRows;
    private float scroll;
    private IntConsumer onSelect, onActivate;
    private boolean dragThumb;
    private float dragFrom, dragStart;

    public Table(String[] headers, float[] weights, int visibleRows) {
        this.headers = headers;
        this.weights = weights;
        this.visibleRows = visibleRows;
    }

    public Table onSelect(IntConsumer c) {
        onSelect = c;
        return this;
    }

    public Table onActivate(IntConsumer c) {
        onActivate = c;
        return this;
    }

    public void setRows(List<String[]> r) {
        rows.clear();
        rows.addAll(r);
        selected = -1;
        resort();
    }

    public int selected() {
        return selected;
    }

    public String[] row(int i) {
        return rows.get(i);
    }

    public void select(int row) {
        if (row != selected) {
            selected = row;
            if (onSelect != null) {
                onSelect.accept(row);
            }
        }
    }

    private void resort() {
        order.clear();
        for (int i = 0; i < rows.size(); i++) {
            order.add(i);
        }
        if (sortColumn >= 0) {
            Comparator<Integer> c = (a, b) -> compare(rows.get(a)[sortColumn], rows.get(b)[sortColumn]);
            order.sort(descending ? c.reversed() : c);
        }
    }

    private static int compare(String a, String b) {
        try {
            return Double.compare(Double.parseDouble(a.replace(",", "")), Double.parseDouble(b.replace(",", "")));
        } catch (NumberFormatException e) {
            return a.compareToIgnoreCase(b);
        }
    }

    private float rowH(Ui ui) {
        return ui.theme.s[Theme.ROW] + 1;
    }

    private float headH(Ui ui) {
        return ui.theme.s[Theme.ROW] + 3;
    }

    @Override
    public float prefWidth(Ui ui) {
        float s = 0;
        for (int c = 0; c < headers.length; c++) {
            float m = ui.theme.font.width(headers[c]) + 16;
            for (String[] r : rows) {
                m = Math.max(m, ui.theme.font.width(r[c]) + 10);
            }
            s += Math.min(m, 220);
        }
        return s + ui.theme.s[Theme.SCROLLBAR];
    }

    @Override
    public float minWidth(Ui ui) {
        return headers.length * 34f;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return headH(ui) + visibleRows * rowH(ui) + 2;
    }

    @Override
    public boolean focusable() {
        return true;
    }

    private float[] colX(Ui ui) {
        float inner = w - ui.theme.s[Theme.SCROLLBAR] - 2;
        float sum = 0;
        for (float wt : weights) {
            sum += wt;
        }
        float[] cx = new float[headers.length + 1];
        cx[0] = x + 1;
        for (int c = 0; c < headers.length; c++) {
            cx[c + 1] = cx[c] + inner * weights[c] / sum;
        }
        return cx;
    }

    private float viewH(Ui ui) {
        return h - headH(ui) - 2;
    }

    private void clampScroll(Ui ui) {
        scroll = Math.max(0, Math.min(scroll, Math.max(0, rows.size() * rowH(ui) - viewH(ui))));
    }

    private void reveal(Ui ui) {
        int pos = order.indexOf(selected);
        if (pos < 0) {
            return;
        }
        float rh = rowH(ui);
        if (pos * rh < scroll) {
            scroll = pos * rh;
        }
        if ((pos + 1) * rh > scroll + viewH(ui)) {
            scroll = (pos + 1) * rh - viewH(ui);
        }
        clampScroll(ui);
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        Font f = t.font;
        clampScroll(ui);
        Skin.field(p, t, x, y, w, h, ui.isFocused(this) ? 0.6f : 0, isEnabled());
        float[] cx = colX(ui);
        float hh = headH(ui);
        Shapes.vgradient(p, x + 1, y + 1, w - 2, hh - 1, t.c[Theme.BUTTON_TOP], t.c[Theme.BUTTON_BOTTOM]);
        Shapes.fill(p, x + 1, y + hh, w - 2, 1, t.c[Theme.PANEL_BORDER]);
        for (int c = 0; c < headers.length; c++) {
            boolean on = ui.isHover(this) && ui.my < y + hh && ui.mx >= cx[c] && ui.mx < cx[c + 1];
            if (on) {
                Shapes.vgradient(p, cx[c], y + 1, cx[c + 1] - cx[c], hh - 1, t.c[Theme.HOVER_TOP], t.c[Theme.HOVER_BOTTOM]);
            }
            float cw = cx[c + 1] - cx[c];
            String s = Skin.fit(f, headers[c], cw - 14);
            f.draw(p, s, cx[c] + 4, y + (hh - f.lineHeight()) / 2, c == sortColumn ? t.c[Theme.ACCENT] : t.c[Theme.TEXT_BRIGHT]);
            if (c == sortColumn) {
                Skin.chevron(p, cx[c + 1] - 7, y + hh / 2, 5, descending ? 0 : 2, t.c[Theme.ACCENT]);
            }
            if (c > 0) {
                Shapes.fill(p, cx[c], y + 2, 1, hh - 3, Shapes.alpha(t.c[Theme.PANEL_BORDER], 0.7f));
            }
        }
        float rh = rowH(ui), top = y + hh + 1;
        p.pushClip(x + 1, top, w - 2, viewH(ui));
        int hot = rowAt(ui, ui.mx, ui.my);
        for (int pos = 0; pos < order.size(); pos++) {
            float ry = top + pos * rh - scroll;
            if (ry + rh < top || ry > y + h) {
                continue;
            }
            int r = order.get(pos);
            if (r == selected) {
                Shapes.vgradient(p, x + 1, ry, cx[headers.length] - x - 1, rh, t.c[Theme.SELECTION],
                        Shapes.lerp(t.c[Theme.SELECTION], 0xFF000000, 0.3f));
                Shapes.fill(p, x + 1, ry, 2, rh, t.c[Theme.ACCENT]);
            } else if (r == hot && ui.isHover(this)) {
                Shapes.fill(p, x + 1, ry, cx[headers.length] - x - 1, rh, t.c[Theme.ROW_HOVER]);
            } else if (pos % 2 == 1) {
                Shapes.fill(p, x + 1, ry, cx[headers.length] - x - 1, rh, t.c[Theme.ROW_ALT]);
            }
            for (int c = 0; c < headers.length; c++) {
                String s = Skin.fit(f, rows.get(r)[c], cx[c + 1] - cx[c] - 8);
                f.draw(p, s, cx[c] + 4, ry + (rh - f.lineHeight()) / 2,
                        r == selected ? t.c[Theme.SELECTION_TEXT] : t.c[Theme.TEXT]);
            }
        }
        if (rows.isEmpty()) {
            String s = "(empty)";
            f.draw(p, s, x + (w - f.width(s)) / 2, top + 6, t.c[Theme.TEXT_DIM]);
        }
        p.popClip();
        // the bar
        float total = rows.size() * rh, view = viewH(ui), bw = t.s[Theme.SCROLLBAR];
        float bx = x + w - bw - 1;
        Shapes.vgradient(p, bx, top, bw, view, t.c[Theme.TRACK_TOP], t.c[Theme.TRACK_BOTTOM]);
        if (total > view) {
            float[] th = thumb(ui);
            float[] pts = Shapes.cut(bx + 1, th[0], bw - 2, th[1], 0, 0, 0, 0);
            Shapes.poly(p, pts, t.c[Theme.BUTTON_TOP], t.c[Theme.BUTTON_BOTTOM], th[0], th[1]);
            Shapes.outline(p, pts, 1, t.c[Theme.BUTTON_BORDER]);
        }
    }

    private float[] thumb(Ui ui) {
        float rh = rowH(ui), total = rows.size() * rh, view = viewH(ui);
        float len = Math.max(10, view * view / total);
        return new float[] {y + headH(ui) + 1 + (view - len) * scroll / Math.max(1, total - view), len};
    }

    private int rowAt(Ui ui, float mx, float my) {
        float top = y + headH(ui) + 1;
        if (my < top || my >= y + h || mx >= x + w - ui.theme.s[Theme.SCROLLBAR] - 1) {
            return -1;
        }
        int pos = (int) Math.floor((my - top + scroll) / rowH(ui));
        return pos >= 0 && pos < order.size() ? order.get(pos) : -1;
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0) {
            return false;
        }
        if (mx >= x + w - ui.theme.s[Theme.SCROLLBAR] - 1 && my > y + headH(ui)) {
            dragThumb = true;
            dragFrom = my;
            dragStart = scroll;
            return true;
        }
        if (my < y + headH(ui)) {
            float[] cx = colX(ui);
            for (int c = 0; c < headers.length; c++) {
                if (mx >= cx[c] && mx < cx[c + 1]) {
                    descending = sortColumn == c && !descending;
                    sortColumn = c;
                    resort();
                }
            }
            return true;
        }
        int r = rowAt(ui, mx, my);
        if (r >= 0) {
            select(r);
            if (clicks == 2 && onActivate != null) {
                onActivate.accept(r);
            }
        }
        return true;
    }

    @Override
    public void drag(Ui ui, float mx, float my) {
        if (dragThumb) {
            float rh = rowH(ui), total = rows.size() * rh, view = viewH(ui);
            float len = thumb(ui)[1];
            scroll = dragStart + (my - dragFrom) * (total - view) / Math.max(1, view - len);
            clampScroll(ui);
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        dragThumb = false;
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        scroll -= dy * rowH(ui) * 3;
        clampScroll(ui);
        return true;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (order.isEmpty()) {
            return false;
        }
        int pos = Math.max(0, order.indexOf(selected));
        int page = Math.max(1, (int) (viewH(ui) / rowH(ui)) - 1);
        switch (key) {
            case Keys.DOWN -> pos = selected < 0 ? 0 : pos + 1;
            case Keys.UP -> pos = pos - 1;
            case Keys.PAGE_DOWN -> pos += page;
            case Keys.PAGE_UP -> pos -= page;
            case Keys.HOME -> pos = 0;
            case Keys.END -> pos = order.size() - 1;
            case Keys.ENTER, Keys.KP_ENTER -> {
                if (selected >= 0 && onActivate != null) {
                    onActivate.accept(selected);
                    return true;
                }
                return false;
            }
            default -> {
                return false;
            }
        }
        select(order.get(Math.max(0, Math.min(order.size() - 1, pos))));
        reveal(ui);
        return true;
    }
}
