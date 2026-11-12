package slopui;

import java.util.List;

/**
 * A view onto a widget larger than its area: the wheel scrolls it (smoothly), bars show and drag the position
 * (with bars) or not (without: wheel and dragging with the middle button only). Horizontal scrolling when the
 * content's least width is wider than the view.
 */
public class ScrollPane extends Widget {
    private final Widget content;
    private final boolean bars;
    private float sx, sy, targetX, targetY;
    private float contentW, contentH;
    private int dragging;       // 0 none, 1 vertical thumb, 2 horizontal thumb, 3 middle-button pan
    private float dragFrom, dragStart, dragFromX, dragStartX;
    /** The preferred height when the content is taller (0: the content's own). */
    private float viewHeight;

    public ScrollPane(Widget content, boolean bars) {
        this.content = content;
        this.bars = bars;
        content.parent = this;
    }

    public ScrollPane viewHeight(float h) {
        viewHeight = h;
        return this;
    }

    public Widget content() {
        return content;
    }

    @Override
    public void children(List<Widget> out) {
        out.add(content);
    }

    private float bar(Ui ui) {
        return bars ? ui.theme.s[Theme.SCROLLBAR] : 0;
    }

    @Override
    public float prefWidth(Ui ui) {
        return content.prefWidth(ui) + bar(ui);
    }

    @Override
    public float minWidth(Ui ui) {
        return Math.min(content.minWidth(ui), 40) + bar(ui);
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        float ch = content.prefHeight(ui, Math.max(content.minWidth(ui), width - bar(ui)));
        return viewHeight > 0 ? Math.min(ch, viewHeight) : ch;
    }

    private float viewW(Ui ui) {
        return w - (contentH > h ? bar(ui) : 0);
    }

    private float viewH(Ui ui) {
        return h - (contentW > viewW(ui) + 0.5f ? bar(ui) : 0);
    }

    @Override
    public void layout(Ui ui) {
        barW = bar(ui);
        float vw = w - bar(ui);
        contentW = Math.max(content.minWidth(ui), vw);
        contentH = content.prefHeight(ui, contentW);
        if (contentH <= h) {        // no vertical scrolling: the bar's room to the content
            contentW = Math.max(content.minWidth(ui), w);
            contentH = content.prefHeight(ui, contentW);
        }
        clampTargets(ui);
        float k = Math.min(1, ui.dt * 18);
        sx += (targetX - sx) * k;
        sy += (targetY - sy) * k;
        if (Math.abs(targetX - sx) < 0.05f) {
            sx = targetX;
        }
        if (Math.abs(targetY - sy) < 0.05f) {
            sy = targetY;
        }
        if (sx != targetX || sy != targetY) {
            ui.animating = true;
        }
        content.setBounds(x - Math.round(sx), y - Math.round(sy), contentW, Math.max(contentH, viewH(ui)));
        content.layout(ui);
    }

    private void clampTargets(Ui ui) {
        targetX = Math.max(0, Math.min(targetX, contentW - viewW(ui)));
        targetY = Math.max(0, Math.min(targetY, contentH - viewH(ui)));
    }

    /** Scrolls so the rectangle (content coordinates, absolute) is in view. */
    public void reveal(Ui ui, float ry, float rh) {
        float top = ry - content.y;
        if (top < targetY) {
            targetY = top;
        } else if (top + rh > targetY + viewH(ui)) {
            targetY = top + rh - viewH(ui);
        }
        clampTargets(ui);
    }

    @Override
    public Widget hit(float px, float py) {
        if (!visible || !contains(px, py)) {
            return null;
        }
        if (px < x + w - (contentH > h ? barWidth() : 0) && py < y + h - (contentW > w ? barWidth() : 0)) {
            Widget c = content.hit(px, py);
            if (c != null) {
                return c;
            }
        }
        return this;
    }

    private float barWidth() {
        return barW;
    }

    /** The bar's width at the last layout (hit testing has no Ui). */
    private float barW;

    @Override
    public void draw(Ui ui, Painter p) {
        p.pushClip(x, y, viewW(ui), viewH(ui));
        content.draw(ui, p);
        p.popClip();
        if (!bars) {
            return;
        }
        Theme t = ui.theme;
        float b = bar(ui);
        if (contentH > h) {
            float tx = x + w - b, th = viewH(ui);
            Shapes.vgradient(p, tx, y, b, th, t.c[Theme.TRACK_TOP], t.c[Theme.TRACK_BOTTOM]);
            float[] thumb = thumbV(ui);
            boolean on = dragging == 1 || ui.hover == this && ui.mx >= tx;
            float hv = animate(ui, 0, on ? 1 : 0, t.s[Theme.HOVER_TIME]);
            float[] pts = Shapes.cut(tx + 1, thumb[0], b - 2, thumb[1], 0, 0, 0, 0);
            Shapes.poly(p, pts, Shapes.lerp(t.c[Theme.BUTTON_TOP], t.c[Theme.HOVER_TOP], hv),
                    Shapes.lerp(t.c[Theme.BUTTON_BOTTOM], t.c[Theme.HOVER_BOTTOM], hv), thumb[0], thumb[1]);
            Shapes.outline(p, pts, 1, Shapes.lerp(t.c[Theme.BUTTON_BORDER], t.c[Theme.ACCENT_SOFT], hv));
        }
        if (contentW > viewW(ui) + 0.5f) {
            float ty = y + h - b, tw = viewW(ui);
            Shapes.vgradient(p, x, ty, tw, b, t.c[Theme.TRACK_TOP], t.c[Theme.TRACK_BOTTOM]);
            float[] thumb = thumbH(ui);
            float[] pts = Shapes.cut(thumb[0], ty + 1, thumb[1], b - 2, 0, 0, 0, 0);
            Shapes.poly(p, pts, t.c[Theme.BUTTON_TOP], t.c[Theme.BUTTON_BOTTOM], ty, b);
            Shapes.outline(p, pts, 1, t.c[Theme.BUTTON_BORDER]);
        }
    }

    /** The vertical thumb: y, height. */
    private float[] thumbV(Ui ui) {
        float th = viewH(ui);
        float len = Math.max(12, th * th / contentH);
        float range = contentH - th;
        return new float[] {y + (range <= 0 ? 0 : (th - len) * sy / range), len};
    }

    private float[] thumbH(Ui ui) {
        float tw = viewW(ui);
        float len = Math.max(12, tw * tw / contentW);
        float range = contentW - tw;
        return new float[] {x + (range <= 0 ? 0 : (tw - len) * sx / range), len};
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        float b = bar(ui);
        if (button == 2) {
            dragging = 3;
            dragFrom = my;
            dragStart = targetY;
            dragFromX = mx;
            dragStartX = targetX;
            return true;
        }
        if (button != 0 || !bars) {
            return false;
        }
        if (contentH > h && mx >= x + w - b) {
            float[] t = thumbV(ui);
            if (my < t[0] || my >= t[0] + t[1]) {     // the track: a page towards the pointer
                targetY += (my < t[0] ? -1 : 1) * viewH(ui) * 0.9f;
                clampTargets(ui);
            }
            dragging = 1;
            dragFrom = my;
            dragStart = targetY;
            return true;
        }
        if (contentW > viewW(ui) + 0.5f && my >= y + h - b) {
            dragging = 2;
            dragFrom = mx;
            dragStart = targetX;
            return true;
        }
        return false;
    }

    @Override
    public void drag(Ui ui, float mx, float my) {
        if (dragging == 1) {
            float th = viewH(ui), len = thumbV(ui)[1];
            targetY = dragStart + (my - dragFrom) * (contentH - th) / Math.max(1, th - len);
            clampTargets(ui);
            sy = targetY;
        } else if (dragging == 2) {
            float tw = viewW(ui), len = thumbH(ui)[1];
            targetX = dragStart + (mx - dragFrom) * (contentW - tw) / Math.max(1, tw - len);
            clampTargets(ui);
            sx = targetX;
        } else if (dragging == 3) {
            targetY = dragStart - (my - dragFrom);
            targetX = dragStartX - (mx - dragFromX);
            clampTargets(ui);
            sx = targetX;
            sy = targetY;
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        dragging = 0;
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        float before = targetY;
        targetY -= dy * ui.theme.s[Theme.ROW] * 3;
        clampTargets(ui);
        return targetY != before || contentH > h;
    }
}
