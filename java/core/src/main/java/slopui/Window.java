package slopui;

import java.util.List;
import java.util.function.Supplier;

/**
 * A window over the game: a title bar to drag it by (or none: a fixed pane), its content in a scroll pane
 * (whatever doesn't fit the window scrolls) and a row of buttons that stacks when narrow. At most
 * MAX_WINDOW of the UI; kept inside it when the UI changes size. Modal windows shade and block what is
 * under them. Enter presses the default button, Escape cancels. Resizable (an option): the right and bottom
 * edges and the grip in the bottom-right corner drag its size; it then keeps that size (within the UI).
 */
public class Window extends Container {
    private Supplier<String> title;
    private boolean modal = true, titleBar = true, closable = true;
    private final ScrollPane scroll;
    private final Box buttons = Box.row().responsive().justify(1);
    private Button defaultButton;
    private Runnable onCancel, onClose;
    private boolean placed;
    private float wantW;
    private float dragDX, dragDY;
    private boolean dragging, closePressed;
    private float open;
    private boolean resizable;
    /** The size the player dragged it to (0: its own). */
    private float userW, userH;
    /** While resizing: 1 right edge, 2 bottom edge, 3 the corner (0: not); the pointer's offset from the edge. */
    private int sizing;
    private float sizeDX, sizeDY;
    private static final float EDGE = 4, GRIP = 10;

    public Window(Supplier<String> title, Widget content) {
        this.title = title;
        scroll = new ScrollPane(content, true);
        add(scroll, buttons);
    }

    public Window(String title, Widget content) {
        this(() -> title, content);
    }

    public Window modal(boolean m) {
        modal = m;
        return this;
    }

    public boolean isModal() {
        return modal;
    }

    /** Without a title bar: a pane that can't be dragged or closed by a button. */
    public Window titleBar(boolean t) {
        titleBar = t;
        return this;
    }

    /** The player may drag its right and bottom edges and its corner to resize it. */
    public Window resizable(boolean r) {
        resizable = r;
        return this;
    }

    /** Back to its own size. */
    public void resetSize() {
        userW = userH = 0;
    }

    public Window closable(boolean c) {
        closable = c;
        return this;
    }

    /** The width it asks for (0: its content's). */
    public Window width(float w) {
        wantW = w;
        return this;
    }

    public Window button(Button b) {
        buttons.add(b);
        return this;
    }

    public Window defaultButton(Button b) {
        defaultButton = b;
        b.accent = true;
        return button(b);
    }

    /** What Escape and the close button do (else: the window closes). */
    public Window onCancel(Runnable r) {
        onCancel = r;
        return this;
    }

    public Window onClose(Runnable r) {
        onClose = r;
        return this;
    }

    void closed() {
        if (onClose != null) {
            onClose.run();
        }
    }

    public void cancel(Ui ui) {
        if (onCancel != null) {
            onCancel.run();
        } else {
            ui.close(this);
        }
    }

    /** Placed again (centred) the next time it is shown. */
    public void recentre() {
        placed = false;
    }

    private float titleH(Ui ui) {
        return titleBar ? ui.theme.s[Theme.TITLE_HEIGHT] : 0;
    }

    private boolean hasButtons() {
        for (Widget k : buttons.kids()) {
            if (k.visible) {
                return true;
            }
        }
        return false;
    }

    @Override
    public float prefWidth(Ui ui) {
        float pad = ui.theme.s[Theme.PAD];
        float t = titleBar ? ui.theme.titleFont().width(title.get()) + 30 : 0;
        float c = wantW > 0 ? wantW : Math.max(scroll.prefWidth(ui), buttons.prefWidth(ui));
        return Math.max(t, c) + 2 * pad;
    }

    @Override
    public float minWidth(Ui ui) {
        float pad = ui.theme.s[Theme.PAD];
        return Math.max(scroll.minWidth(ui), buttons.minWidth(ui)) + 2 * pad;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        float pad = ui.theme.s[Theme.PAD];
        float inner = width - 2 * pad;
        float bh = hasButtons() ? buttons.prefHeight(ui, inner) + pad : 0;
        return titleH(ui) + pad + scroll.prefHeight(ui, inner) + bh + pad;
    }

    /** Sized and placed in the UI (centred the first time, kept inside after). */
    void place(Ui ui) {
        float top = ui.contentTop();
        float maxW = ui.width * ui.theme.s[Theme.MAX_WINDOW];
        float maxH = (ui.height - top) * ui.theme.s[Theme.MAX_WINDOW];
        float pw = Math.min(maxW, Math.max(Math.min(minWidth(ui), maxW), userW > 0 ? userW : prefWidth(ui)));
        float ph = userH > 0 ? Math.min(maxH, Math.max(minHeight(ui, pw), userH)) : Math.min(maxH, prefHeight(ui, pw));
        float px = x, py = y;
        if (!placed) {
            px = (ui.width - pw) / 2;
            py = top + (ui.height - top - ph) * 0.42f;
            placed = true;
            open = 0;
        }
        px = Math.max(0, Math.min(px, ui.width - pw));
        py = Math.max(top, Math.min(py, ui.height - ph));
        setBounds(Math.round(px), Math.round(py), Math.round(pw), Math.round(ph));
        layout(ui);
    }

    /** The least height when resized: the title, the buttons and a little content. */
    private float minHeight(Ui ui, float width) {
        float pad = ui.theme.s[Theme.PAD];
        float bh = hasButtons() ? buttons.prefHeight(ui, width - 2 * pad) + pad : 0;
        return titleH(ui) + 2 * pad + bh + 24;
    }

    /** The resizing edge at the point: 1 right, 2 bottom, 3 corner, 0 none. */
    private int edgeAt(float mx, float my) {
        if (!resizable || !contains(mx, my)) {
            return 0;
        }
        if (mx >= x + w - GRIP && my >= y + h - GRIP) {
            return 3;
        }
        return (mx >= x + w - EDGE ? 1 : 0) | (my >= y + h - EDGE ? 2 : 0);
    }

    @Override
    public Widget hit(float px, float py) {
        if (visible && edgeAt(px, py) != 0) {
            return this;        // (the edges before the content under them)
        }
        return super.hit(px, py);
    }

    @Override
    public void layout(Ui ui) {
        float pad = ui.theme.s[Theme.PAD];
        float inner = w - 2 * pad;
        float bh = hasButtons() ? buttons.prefHeight(ui, inner) : 0;
        float th = titleH(ui);
        buttons.setVisible(hasButtons());
        float sh = h - th - 2 * pad - (bh > 0 ? bh + pad : 0);
        scroll.setBounds(x + pad, y + th + pad, inner, Math.max(0, sh));
        scroll.layout(ui);
        buttons.setBounds(x + pad, y + h - pad - bh, inner, bh);
        buttons.layout(ui);
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        open = animate(ui, 0, 1, t.s[Theme.OPEN_TIME]);
        float before = p.opacity();
        p.setOpacity(before * open);
        float rise = (1 - open) * 6;
        float cut = t.s[Theme.CUT] * 2;
        Skin.panel(p, t, x, y + rise, w, h, cut, true);
        if (titleBar) {
            float th = titleH(ui);
            float[] tb = Shapes.cut(x + 1, y + 1 + rise, w - 2, th - 1, cut - 1, 0, 0, 0);
            boolean front = ui.topWindow() == this;
            Shapes.poly(p, tb, front ? t.c[Theme.TITLE_TOP] : t.c[Theme.BUTTON_BOTTOM],
                    front ? t.c[Theme.TITLE_BOTTOM] : t.c[Theme.PANEL_BOTTOM], y + rise, th);
            Skin.texture(p, t, tb, y + rise, th, 1.2f);
            Shapes.hgradient(p, x + cut, y + th + rise, (w - cut) * 0.7f, 1, t.c[Theme.ACCENT_SOFT], 0);
            Font f = t.titleFont();
            float ty = y + rise + (th - f.lineHeight()) / 2;
            String s = Skin.fit(f, title.get(), w - cut - 24);
            f.draw(p, s, x + cut + 3, ty + 0.5f, 0x80000000);
            f.draw(p, s, x + cut + 2, ty, front ? t.c[Theme.TITLE_TEXT] : t.c[Theme.TEXT_DIM]);
            if (closable) {
                float cs = th - 4, cx = x + w - cs - 3, cy = y + 2 + rise;
                boolean on = ui.mx >= cx && ui.mx < cx + cs && ui.my >= cy && ui.my < cy + cs && ui.hover == this;
                float hv = animate(ui, 1, on ? 1 : 0, t.s[Theme.HOVER_TIME]);
                if (hv > 0) {
                    Shapes.fill(p, cx, cy, cs, cs, Shapes.alpha(t.c[Theme.WARNING], 0.5f * hv));
                }
                int xc = Shapes.lerp(t.c[Theme.TEXT_DIM], 0xFFFFFFFF, hv);
                Shapes.line(p, cx + 3, cy + 3, cx + cs - 3, cy + cs - 3, 1.3f, xc);
                Shapes.line(p, cx + cs - 3, cy + 3, cx + 3, cy + cs - 3, 1.3f, xc);
            }
        }
        if (rise > 0.01f) {
            p.pushClip(x, y + rise, w, h);
        }
        super.draw(ui, p);
        if (rise > 0.01f) {
            p.popClip();
        }
        if (resizable) {        // the grip: three short diagonal lines in the corner
            boolean on = sizing != 0 || ui.hover == this && edgeAt(ui.mx, ui.my) != 0;
            int gc = on ? t.c[Theme.ACCENT] : Shapes.alpha(t.c[Theme.ACCENT_SOFT], 0.7f);
            float gx = x + w - 2, gy = y + h - 2 + rise;
            for (int i = 1; i <= 3; i++) {
                Shapes.line(p, gx - i * 3, gy, gx, gy - i * 3, 1, gc);
            }
        }
        p.setOpacity(before);
    }

    private boolean onClose(Ui ui, float mx, float my) {
        float th = titleH(ui), cs = th - 4, cx = x + w - cs - 3, cy = y + 2;
        return closable && titleBar && mx >= cx && mx < cx + cs && my >= cy && my < cy + cs;
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0) {
            return true;        // (taken: nothing under the window gets it)
        }
        if (onClose(ui, mx, my)) {
            closePressed = true;
            return true;
        }
        int e = edgeAt(mx, my);
        if (e != 0) {
            sizing = e;
            sizeDX = x + w - mx;
            sizeDY = y + h - my;
            userW = w;
            userH = h;
            return true;
        }
        if (titleBar && my < y + titleH(ui)) {
            dragging = true;
            dragDX = mx - x;
            dragDY = my - y;
        }
        return true;
    }

    @Override
    public void drag(Ui ui, float mx, float my) {
        if (sizing != 0) {
            if ((sizing & 1) != 0) {
                userW = Math.max(minWidth(ui), mx + sizeDX - x);
            }
            if ((sizing & 2) != 0) {
                userH = Math.max(minHeight(ui, w), my + sizeDY - y);
            }
            return;
        }
        if (dragging) {
            x = mx - dragDX;
            y = my - dragDY;
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        if (closePressed && onClose(ui, mx, my)) {
            cancel(ui);
        }
        closePressed = false;
        dragging = false;
        sizing = 0;
    }

    @Override
    public int cursor(float mx, float my) {
        int e = sizing != 0 ? sizing : edgeAt(mx, my);
        return e == 3 ? Cursor.RESIZE_DIAGONAL : e == 1 ? Cursor.RESIZE_H : e == 2 ? Cursor.RESIZE_V
                : dragging ? Cursor.MOVE : Cursor.ARROW;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (Keys.isEnter(key) && defaultButton != null && defaultButton.isEnabled()) {
            defaultButton.click(ui);
            return true;
        }
        if (key == Keys.ESCAPE) {
            cancel(ui);
            return true;
        }
        return false;
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        return true;        // (the wheel stops here: the game under the window doesn't scroll)
    }

    @Override
    public void children(List<Widget> out) {
        out.add(scroll);
        out.add(buttons);
    }
}
