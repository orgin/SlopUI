package slopui;

import java.util.List;

/** A titled frame around related parts (the title set into its top edge). */
public class GroupBox extends Widget {
    private final String title;
    private final Widget content;

    public GroupBox(String title, Widget content) {
        this.title = title;
        this.content = content;
        content.parent = this;
    }

    private float top(Ui ui) {
        return ui.theme.font.lineHeight() + 4;
    }

    @Override
    public void children(List<Widget> out) {
        out.add(content);
    }

    @Override
    public float prefWidth(Ui ui) {
        return Math.max(content.prefWidth(ui), ui.theme.font.width(title) + 16) + 2 * ui.theme.s[Theme.PAD];
    }

    @Override
    public float minWidth(Ui ui) {
        return content.minWidth(ui) + 2 * ui.theme.s[Theme.PAD];
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        float pad = ui.theme.s[Theme.PAD];
        return top(ui) + content.prefHeight(ui, width - 2 * pad) + pad;
    }

    @Override
    public void layout(Ui ui) {
        float pad = ui.theme.s[Theme.PAD];
        content.setBounds(x + pad, y + top(ui), w - 2 * pad, h - top(ui) - pad);
        content.layout(ui);
    }

    @Override
    public Widget hit(float px, float py) {
        if (!visible || !contains(px, py)) {
            return null;
        }
        Widget c = content.hit(px, py);
        return c != null ? c : this;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        Font f = t.font;
        float ty = y + f.lineHeight() / 2;
        float[] pts = Shapes.cut(x, ty, w, h - (ty - y), 4, 0, 4, 0);
        Shapes.poly(p, pts, 0x18000000, 0x30000000, ty, h);
        Shapes.outline(p, pts, 1, Shapes.alpha(t.c[Theme.PANEL_BORDER], 0.8f));
        float tw = f.width(title) + 8;
        Shapes.fill(p, x + 8, ty - 1, tw, 3, t.c[Theme.PANEL_BOTTOM]);
        f.draw(p, title, x + 12, y, t.c[Theme.ACCENT_SOFT]);
        content.draw(ui, p);
    }
}
