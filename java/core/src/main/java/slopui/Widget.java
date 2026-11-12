package slopui;

import java.util.List;
import java.util.function.Supplier;

/**
 * A part of the UI: a rectangle (UI units, absolute) that measures, lays out, draws and takes input. Events go
 * to the deepest widget under the pointer and up its parents until one takes them ({@link Ui}).
 */
public abstract class Widget {
    public float x, y, w, h;
    Widget parent;
    boolean visible = true;
    boolean enabled = true;
    Supplier<String> tooltip;
    /** Share of a row's or column's spare room this widget takes (0: none). */
    public float grow;
    private final float[] anims = new float[4];

    /** The width it would like, UI units. */
    public abstract float prefWidth(Ui ui);

    /** The least width it can work with (text wraps, tables truncate); the preferred one by default. */
    public float minWidth(Ui ui) {
        return prefWidth(ui);
    }

    /** The height it would like at this width. */
    public abstract float prefHeight(Ui ui, float width);

    /** Children placed inside x, y, w, h (set by the parent). */
    public void layout(Ui ui) {
    }

    public abstract void draw(Ui ui, Painter p);

    public void setBounds(float x, float y, float w, float h) {
        this.x = x;
        this.y = y;
        this.w = w;
        this.h = h;
    }

    public boolean contains(float px, float py) {
        return px >= x && py >= y && px < x + w && py < y + h;
    }

    /** The deepest visible widget at the point (this, a child, or null). */
    public Widget hit(float px, float py) {
        return visible && contains(px, py) ? this : null;
    }

    /** The children, in drawing order (focus order, tooltips). */
    public void children(List<Widget> out) {
    }

    // Input: true when taken. A widget that takes a press gets the drags and the release.

    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        return false;
    }

    public void drag(Ui ui, float mx, float my) {
    }

    public void release(Ui ui, float mx, float my, int button) {
    }

    public boolean wheel(Ui ui, float mx, float my, float dy) {
        return false;
    }

    public boolean key(Ui ui, int key, int mods) {
        return false;
    }

    public boolean typed(Ui ui, int cp) {
        return false;
    }

    /** Each frame while this widget holds the press (repeats on hold). */
    public void tick(Ui ui) {
    }

    public boolean focusable() {
        return false;
    }

    /** Takes typed text: plain keys (no Ctrl/Alt) stop here instead of reaching the shortcuts and the game. */
    public boolean takesText() {
        return false;
    }

    public int cursor(float mx, float my) {
        return Cursor.ARROW;
    }

    // State

    public boolean isVisible() {
        return visible;
    }

    public Widget setVisible(boolean v) {
        visible = v;
        return this;
    }

    public boolean isEnabled() {
        for (Widget t = this; t != null; t = t.parent) {
            if (!t.enabled) {
                return false;
            }
        }
        return true;
    }

    public Widget setEnabled(boolean e) {
        enabled = e;
        return this;
    }

    public Widget tooltip(String t) {
        tooltip = t == null ? null : () -> t;
        return this;
    }

    public Widget tooltip(Supplier<String> t) {
        tooltip = t;
        return this;
    }

    public Widget grow(float g) {
        grow = g;
        return this;
    }

    public Widget parent() {
        return parent;
    }

    /**
     * An animated value in slot (0-3) moving towards target over 'time' seconds (snappy: linear on the frame
     * time); keeps the UI drawing while it moves.
     */
    protected float animate(Ui ui, int slot, float target, float time) {
        float v = anims[slot];
        if (v != target) {
            float step = time <= 0 ? 1 : ui.dt / time;
            v = target > v ? Math.min(target, v + step) : Math.max(target, v - step);
            anims[slot] = v;
            ui.animating = true;
        }
        return v;
    }

    protected Theme theme(Ui ui) {
        return ui.theme;
    }
}
