package slopui;

import java.util.function.Consumer;
import java.util.function.IntPredicate;

/**
 * One line of text to type: caret, selection (mouse drag, double click: a word, Shift + keys), Ctrl+A/C/X/V,
 * Home/End, Ctrl + arrows by words; scrolls sideways when longer than the field. A placeholder while empty.
 */
public class TextField extends Widget {
    protected final StringBuilder text = new StringBuilder();
    protected int caret, anchor;
    private float scrollX;
    private String placeholder = "";
    private int maxLength = 256;
    private IntPredicate allowed = c -> c >= 32;
    private Consumer<String> onChange, onEnter;
    private boolean selecting;
    private double lastEdit;
    /** Room at the right kept for buttons drawn over the field (dropdowns, spinners). */
    protected float rightInset;

    public TextField(String initial) {
        text.append(initial);
        caret = anchor = text.length();
    }

    public String text() {
        return text.toString();
    }

    public void set(String s) {
        text.setLength(0);
        text.append(s);
        caret = anchor = text.length();
        changed();
    }

    public TextField placeholder(String p) {
        placeholder = p;
        return this;
    }

    public TextField maxLength(int n) {
        maxLength = n;
        return this;
    }

    /** Which typed characters it takes. */
    public TextField allow(IntPredicate p) {
        allowed = p;
        return this;
    }

    public TextField onChange(Consumer<String> c) {
        onChange = c;
        return this;
    }

    public TextField onEnter(Consumer<String> c) {
        onEnter = c;
        return this;
    }

    public void selectAll() {
        anchor = 0;
        caret = text.length();
    }

    protected void changed() {
        if (onChange != null) {
            onChange.accept(text.toString());
        }
    }

    @Override
    public float prefWidth(Ui ui) {
        return 120;
    }

    @Override
    public float minWidth(Ui ui) {
        return 40;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return ui.theme.s[Theme.ROW] + 3;
    }

    @Override
    public boolean focusable() {
        return true;
    }

    @Override
    public boolean takesText() {
        return true;
    }

    @Override
    public int cursor(float mx, float my) {
        return mx < x + w - rightInset ? Cursor.TEXT : Cursor.ARROW;
    }

    private float pad() {
        return 4;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        boolean focus = ui.isFocused(this);
        float fo = animate(ui, 0, focus ? 1 : 0, t.s[Theme.HOVER_TIME]);
        float hv = animate(ui, 1, ui.isHover(this) ? 1 : 0, t.s[Theme.HOVER_TIME]);
        Skin.field(p, t, x, y, w, h, Math.max(fo, hv * 0.35f), isEnabled());
        Font f = t.font;
        float inner = w - 2 * pad() - rightInset;
        float caretX = f.width(text.substring(0, caret));
        if (caretX - scrollX > inner - 2) {
            scrollX = caretX - inner + 2;
        }
        if (caretX - scrollX < 0) {
            scrollX = caretX;
        }
        scrollX = Math.max(0, Math.min(scrollX, Math.max(0, f.width(text.toString()) - inner + 2)));
        float tx = x + pad() - scrollX, ty = y + (h - f.lineHeight()) / 2;
        p.pushClip(x + 2, y + 1, w - 4 - rightInset, h - 2);
        if (caret != anchor) {
            int a = Math.min(caret, anchor), b = Math.max(caret, anchor);
            float sx = f.width(text.substring(0, a)), sw = f.width(text.substring(a, b));
            Shapes.fill(p, tx + sx, ty - 1, sw, f.lineHeight() + 2,
                    focus ? t.c[Theme.SELECTION] : Shapes.alpha(t.c[Theme.SELECTION], 0.5f));
        }
        if (text.length() == 0 && !placeholder.isEmpty()) {
            f.draw(p, placeholder, tx, ty, t.c[Theme.TEXT_DIM]);
        } else {
            f.draw(p, text.toString(), tx, ty, isEnabled() ? t.c[Theme.TEXT_BRIGHT] : t.c[Theme.TEXT_DISABLED]);
        }
        if (focus) {
            boolean on = (ui.time - lastEdit) % 1.0 < 0.6;
            ui.animating = true;
            if (on) {
                Shapes.fill(p, tx + caretX, ty - 1, 1, f.lineHeight() + 2, t.c[Theme.ACCENT]);
            }
        }
        p.popClip();
    }

    protected int indexAt(Ui ui, float mx) {
        Font f = ui.theme.font;
        float rel = mx - (x + pad() - scrollX);
        float acc = 0;
        for (int i = 0; i < text.length(); i++) {
            float cw = f.advance(text.charAt(i));
            if (rel < acc + cw / 2) {
                return i;
            }
            acc += cw;
        }
        return text.length();
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button != 0) {
            return false;
        }
        int i = indexAt(ui, mx);
        if (clicks == 2) {
            anchor = wordStart(i);
            caret = wordEnd(i);
        } else if (clicks >= 3) {
            selectAll();
        } else {
            caret = i;
            if ((ui.mods & Keys.SHIFT) == 0) {
                anchor = i;
            }
        }
        selecting = true;
        lastEdit = ui.time;
        return true;
    }

    @Override
    public void drag(Ui ui, float mx, float my) {
        if (selecting) {
            caret = indexAt(ui, mx);
        }
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        selecting = false;
    }

    private int wordStart(int i) {
        while (i > 0 && Character.isLetterOrDigit(text.charAt(i - 1))) {
            i--;
        }
        return i;
    }

    private int wordEnd(int i) {
        while (i < text.length() && Character.isLetterOrDigit(text.charAt(i))) {
            i++;
        }
        return i;
    }

    private int wordLeft(int i) {
        while (i > 0 && !Character.isLetterOrDigit(text.charAt(i - 1))) {
            i--;
        }
        return wordStart(i);
    }

    private int wordRight(int i) {
        while (i < text.length() && !Character.isLetterOrDigit(text.charAt(i))) {
            i++;
        }
        return wordEnd(i);
    }

    protected void deleteSelection() {
        int a = Math.min(caret, anchor), b = Math.max(caret, anchor);
        text.delete(a, b);
        caret = anchor = a;
    }

    protected void insert(Ui ui, String s) {
        if (caret != anchor) {
            deleteSelection();
        }
        StringBuilder ok = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (allowed.test(c) && text.length() + ok.length() < maxLength) {
                ok.append(c);
            }
        }
        text.insert(caret, ok);
        caret += ok.length();
        anchor = caret;
        changed();
    }

    @Override
    public boolean typed(Ui ui, int cp) {
        lastEdit = ui.time;
        insert(ui, new String(Character.toChars(cp)));
        return true;
    }

    /** Line-wise moves for TextArea; none here. */
    protected boolean verticalKey(Ui ui, int key, boolean shift) {
        return false;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        boolean shift = (mods & Keys.SHIFT) != 0, ctrl = (mods & Keys.CTRL) != 0;
        lastEdit = ui.time;
        switch (key) {
            case Keys.LEFT -> caret = ctrl ? wordLeft(caret) : caret != anchor && !shift ? Math.min(caret, anchor) : Math.max(0, caret - 1);
            case Keys.RIGHT -> caret = ctrl ? wordRight(caret) : caret != anchor && !shift ? Math.max(caret, anchor) : Math.min(text.length(), caret + 1);
            case Keys.HOME -> caret = homeOf(caret, ctrl);
            case Keys.END -> caret = endOf(caret, ctrl);
            case Keys.BACKSPACE -> {
                if (caret != anchor) {
                    deleteSelection();
                } else if (caret > 0) {
                    int from = ctrl ? wordLeft(caret) : caret - 1;
                    text.delete(from, caret);
                    caret = anchor = from;
                }
                changed();
                return true;
            }
            case Keys.DELETE -> {
                if (caret != anchor) {
                    deleteSelection();
                } else if (caret < text.length()) {
                    text.delete(caret, ctrl ? wordRight(caret) : caret + 1);
                }
                changed();
                return true;
            }
            case Keys.A -> {
                if (!ctrl) {
                    return false;
                }
                selectAll();
                return true;
            }
            case Keys.C, Keys.X -> {
                if (!ctrl) {
                    return false;
                }
                if (caret != anchor && ui.host != null) {
                    ui.host.setClipboard(text.substring(Math.min(caret, anchor), Math.max(caret, anchor)));
                    if (key == Keys.X) {
                        deleteSelection();
                        changed();
                    }
                }
                return true;
            }
            case Keys.V -> {
                if (!ctrl) {
                    return false;
                }
                String c = ui.host != null ? ui.host.clipboard() : null;
                if (c != null) {
                    insert(ui, multiline() ? c : c.replace('\n', ' '));
                }
                return true;
            }
            case Keys.ENTER, Keys.KP_ENTER -> {
                if (multiline()) {
                    insert(ui, "\n");
                    return true;
                }
                if (onEnter != null) {
                    onEnter.accept(text.toString());
                    return true;
                }
                return false;
            }
            default -> {
                if (!verticalKey(ui, key, shift)) {
                    return false;
                }
                return true;
            }
        }
        if (!shift) {
            anchor = caret;
        }
        return true;
    }

    protected boolean multiline() {
        return false;
    }

    protected int homeOf(int i, boolean ctrl) {
        return 0;
    }

    protected int endOf(int i, boolean ctrl) {
        return text.length();
    }
}
