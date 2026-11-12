package slopui;

/**
 * Several lines of text (notes, catalog texts): the TextField's editing plus Enter for new lines, Up/Down,
 * Home/End per line (Ctrl: the whole text), Page Up/Down; scrolls both ways, the wheel scrolls it.
 */
public class TextArea extends TextField {
    private final int rows;
    private float scrollY, scrollX;
    private float goalX = -1;
    private int shownCaret = -1, shownLength = -1;

    public TextArea(String initial, int rows) {
        super(initial);
        this.rows = rows;
        allow(c -> c >= 32 || c == '\n');
        maxLength(1 << 20);
    }

    @Override
    protected boolean multiline() {
        return true;
    }

    @Override
    public float prefWidth(Ui ui) {
        return 200;
    }

    @Override
    public float minWidth(Ui ui) {
        return 60;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return rows * lineH(ui) + 6;
    }

    private float lineH(Ui ui) {
        return ui.theme.font.lineHeight() + 3;
    }

    private int lineStart(int i) {
        while (i > 0 && text.charAt(i - 1) != '\n') {
            i--;
        }
        return i;
    }

    private int lineEnd(int i) {
        while (i < text.length() && text.charAt(i) != '\n') {
            i++;
        }
        return i;
    }

    private int lineOf(int i) {
        int n = 0;
        for (int k = 0; k < i; k++) {
            if (text.charAt(k) == '\n') {
                n++;
            }
        }
        return n;
    }

    private int startOfLine(int line) {
        int i = 0;
        for (int n = 0; n < line && i <= text.length(); n++) {
            i = lineEnd(i) + 1;
        }
        return Math.min(i, text.length());
    }

    private int lineCount() {
        return lineOf(text.length()) + 1;
    }

    @Override
    protected int homeOf(int i, boolean ctrl) {
        return ctrl ? 0 : lineStart(i);
    }

    @Override
    protected int endOf(int i, boolean ctrl) {
        return ctrl ? text.length() : lineEnd(i);
    }

    private int columnAt(Ui ui, int start, float relX) {
        Font f = ui.theme.font;
        int end = lineEnd(start);
        float acc = 0;
        for (int i = start; i < end; i++) {
            float cw = f.advance(text.charAt(i));
            if (relX < acc + cw / 2) {
                return i;
            }
            acc += cw;
        }
        return end;
    }

    @Override
    protected boolean verticalKey(Ui ui, int key, boolean shift) {
        int line = lineOf(caret);
        int rowsPage = Math.max(1, (int) ((h - 6) / lineH(ui)) - 1);
        int target;
        switch (key) {
            case Keys.UP -> target = line - 1;
            case Keys.DOWN -> target = line + 1;
            case Keys.PAGE_UP -> target = line - rowsPage;
            case Keys.PAGE_DOWN -> target = line + rowsPage;
            default -> {
                return false;
            }
        }
        Font f = ui.theme.font;
        if (goalX < 0) {
            goalX = f.width(text.substring(lineStart(caret), caret));
        }
        if (target < 0) {
            caret = 0;
        } else if (target >= lineCount()) {
            caret = text.length();
        } else {
            caret = columnAt(ui, startOfLine(target), goalX);
        }
        if (!shift) {
            anchor = caret;
        }
        return true;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key != Keys.UP && key != Keys.DOWN && key != Keys.PAGE_UP && key != Keys.PAGE_DOWN) {
            goalX = -1;
        }
        if (key == Keys.TAB) {
            return false;       // (Tab moves the focus)
        }
        return super.key(ui, key, mods);
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        boolean focus = ui.isFocused(this);
        Skin.field(p, t, x, y, w, h, focus ? 1 : 0, isEnabled());
        Font f = t.font;
        float lh = lineH(ui);
        float innerW = w - 8, innerH = h - 6;
        // keep the caret in view when it moved (the wheel may scroll it out)
        int cl = lineOf(caret);
        float cx = f.width(text.substring(lineStart(caret), caret));
        boolean moved = caret != shownCaret || text.length() != shownLength;
        shownCaret = caret;
        shownLength = text.length();
        if (moved && cl * lh - scrollY < 0) {
            scrollY = cl * lh;
        }
        if (moved && (cl + 1) * lh - scrollY > innerH) {
            scrollY = (cl + 1) * lh - innerH;
        }
        if (cx - scrollX > innerW - 2) {
            scrollX = cx - innerW + 2;
        }
        if (cx - scrollX < 0) {
            scrollX = cx;
        }
        scrollY = Math.max(0, Math.min(scrollY, Math.max(0, lineCount() * lh - innerH)));
        p.pushClip(x + 2, y + 2, w - 4, h - 4);
        int a = Math.min(caret, anchor), b = Math.max(caret, anchor);
        int start = 0;
        for (int line = 0; start <= text.length(); line++) {
            int end = lineEnd(start);
            float ly = y + 3 + line * lh - scrollY;
            if (ly > y + h) {
                break;
            }
            if (ly + lh >= y) {
                float lx = x + 4 - scrollX;
                if (a != b && b >= start && a <= end) {
                    int s0 = Math.max(a, start), s1 = Math.min(b, end);
                    float sx = f.width(text.substring(start, s0)), sw = f.width(text.substring(s0, s1));
                    if (b > end) {
                        sw += 3;
                    }
                    Shapes.fill(p, lx + sx, ly, sw, lh, focus ? t.c[Theme.SELECTION] : Shapes.alpha(t.c[Theme.SELECTION], 0.5f));
                }
                f.draw(p, text.substring(start, end), lx, ly + 1.5f, isEnabled() ? t.c[Theme.TEXT_BRIGHT] : t.c[Theme.TEXT_DISABLED]);
            }
            if (end >= text.length()) {
                break;
            }
            start = end + 1;
        }
        if (focus) {
            ui.animating = true;
            if (ui.time % 1.0 < 0.6) {
                Shapes.fill(p, x + 4 - scrollX + cx, y + 3 + cl * lh - scrollY, 1, lh, t.c[Theme.ACCENT]);
            }
        }
        p.popClip();
        // a thin position mark when longer than the view
        float total = lineCount() * lh;
        if (total > innerH) {
            float mh = Math.max(8, innerH * innerH / total), my = y + 3 + (innerH - mh) * scrollY / (total - innerH);
            Shapes.fill(p, x + w - 3, my, 2, mh, Shapes.alpha(t.c[Theme.ACCENT_SOFT], 0.6f));
        }
    }

    @Override
    protected int indexAt(Ui ui, float mx) {
        return indexAt(ui, mx, ui.my);
    }

    private int indexAt(Ui ui, float mx, float my) {
        float lh = lineH(ui);
        int line = (int) Math.floor((my - y - 3 + scrollY) / lh);
        line = Math.max(0, Math.min(lineCount() - 1, line));
        return columnAt(ui, startOfLine(line), mx - (x + 4 - scrollX));
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        float lh = lineH(ui);
        float max = Math.max(0, lineCount() * lh - (h - 6));
        float before = scrollY;
        scrollY = Math.max(0, Math.min(max, scrollY - dy * lh * 3));
        // (the caret is kept in view only when it moves; scrolling leaves it)
        return scrollY != before || max > 0;
    }
}
