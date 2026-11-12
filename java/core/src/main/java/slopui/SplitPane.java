package slopui;

import java.util.List;

/**
 * Two panes side by side (or one over the other) with a separator between them to drag; each keeps at least
 * its least width. Double click on the separator: back to the first share.
 */
public class SplitPane extends Widget {
    private final Widget a, b;
    private final boolean vertical;
    private float ratio, initial;
    private boolean dragging;

    /** vertical: a above b; else a left of b. ratio: a's share. */
    public SplitPane(Widget a, Widget b, boolean vertical, float ratio) {
        this.a = a;
        this.b = b;
        this.vertical = vertical;
        this.ratio = ratio;
        this.initial = ratio;
        a.parent = this;
        b.parent = this;
    }

    private float bar(Ui ui) {
        return ui.theme.s[Theme.SPLITTER];
    }

    @Override
    public void children(List<Widget> out) {
        out.add(a);
        out.add(b);
    }

    @Override
    public float prefWidth(Ui ui) {
        return vertical ? Math.max(a.prefWidth(ui), b.prefWidth(ui)) : a.prefWidth(ui) + b.prefWidth(ui) + bar(ui);
    }

    @Override
    public float minWidth(Ui ui) {
        return vertical ? Math.max(a.minWidth(ui), b.minWidth(ui)) : a.minWidth(ui) + b.minWidth(ui) + bar(ui);
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        if (vertical) {
            return a.prefHeight(ui, width) + b.prefHeight(ui, width) + bar(ui);
        }
        float aw = (width - bar(ui)) * ratio;
        return Math.max(a.prefHeight(ui, aw), b.prefHeight(ui, width - bar(ui) - aw));
    }

    private float split(Ui ui) {
        float total = (vertical ? h : w) - bar(ui);
        float s = total * ratio;
        float minA = vertical ? 20 : a.minWidth(ui), minB = vertical ? 20 : b.minWidth(ui);
        if (minA + minB <= total) {
            s = Math.max(minA, Math.min(total - minB, s));
        }
        return Math.round(s);
    }

    @Override
    public void layout(Ui ui) {
        float s = split(ui), g = bar(ui);
        if (vertical) {
            a.setBounds(x, y, w, s);
            b.setBounds(x, y + s + g, w, h - s - g);
        } else {
            a.setBounds(x, y, s, h);
            b.setBounds(x + s + g, y, w - s - g, h);
        }
        a.layout(ui);
        b.layout(ui);
    }

    private boolean onBar(float mx, float my) {
        return vertical ? my >= a.y + a.h && my < b.y : mx >= a.x + a.w && mx < b.x;
    }

    @Override
    public Widget hit(float px, float py) {
        if (!visible || !contains(px, py)) {
            return null;
        }
        if (onBar(px, py)) {
            return this;
        }
        Widget r = a.hit(px, py);
        if (r == null) {
            r = b.hit(px, py);
        }
        return r != null ? r : this;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        p.pushClip(a.x, a.y, a.w, a.h);
        a.draw(ui, p);
        p.popClip();
        p.pushClip(b.x, b.y, b.w, b.h);
        b.draw(ui, p);
        p.popClip();
        Theme t = ui.theme;
        boolean on = dragging || ui.hover == this && onBar(ui.mx, ui.my);
        float hv = animate(ui, 0, on ? 1 : 0, t.s[Theme.HOVER_TIME]);
        float g = bar(ui);
        if (vertical) {
            float by = a.y + a.h;
            Shapes.hgradient(p, x, by + g / 2 - 0.5f, w / 2, 1, 0, Shapes.lerp(t.c[Theme.PANEL_BORDER], t.c[Theme.ACCENT], hv));
            Shapes.hgradient(p, x + w / 2, by + g / 2 - 0.5f, w / 2, 1, Shapes.lerp(t.c[Theme.PANEL_BORDER], t.c[Theme.ACCENT], hv), 0);
            for (int i = -1; i <= 1; i++) {
                Shapes.fill(p, x + w / 2 + i * 4 - 1, by + g / 2 - 1, 2, 2, Shapes.lerp(t.c[Theme.TEXT_DIM], t.c[Theme.ACCENT], hv));
            }
        } else {
            float bx = a.x + a.w;
            Shapes.vgradient(p, bx + g / 2 - 0.5f, y, 1, h / 2, 0, Shapes.lerp(t.c[Theme.PANEL_BORDER], t.c[Theme.ACCENT], hv));
            Shapes.vgradient(p, bx + g / 2 - 0.5f, y + h / 2, 1, h / 2, Shapes.lerp(t.c[Theme.PANEL_BORDER], t.c[Theme.ACCENT], hv), 0);
            for (int i = -1; i <= 1; i++) {
                Shapes.fill(p, bx + g / 2 - 1, y + h / 2 + i * 4 - 1, 2, 2, Shapes.lerp(t.c[Theme.TEXT_DIM], t.c[Theme.ACCENT], hv));
            }
        }
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0 || !onBar(mx, my)) {
            return false;
        }
        if (clicks == 2) {
            ratio = initial;
            return true;
        }
        dragging = true;
        return true;
    }

    @Override
    public void drag(Ui ui, float mx, float my) {
        if (dragging) {
            float total = (vertical ? h : w) - bar(ui);
            float pos = (vertical ? my - y : mx - x) - bar(ui) / 2;
            ratio = Math.max(0.02f, Math.min(0.98f, pos / Math.max(1, total)));
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        dragging = false;
    }

    @Override
    public int cursor(float mx, float my) {
        return dragging || onBar(mx, my) ? (vertical ? Cursor.RESIZE_V : Cursor.RESIZE_H) : Cursor.ARROW;
    }
}
