package slopui;

import java.util.function.BooleanSupplier;
import java.util.function.Supplier;

/** A menu entry: an action (with a tick or a radio mark), a submenu or a separator; a shortcut shown. */
public final class MenuItem {
    final Supplier<String> label;
    final Runnable action;
    final Menu submenu;
    final boolean separator;
    BooleanSupplier checked;
    boolean radio;
    BooleanSupplier enabled = () -> true;
    String shortcut;

    private MenuItem(Supplier<String> label, Runnable action, Menu submenu, boolean separator) {
        this.label = label;
        this.action = action;
        this.submenu = submenu;
        this.separator = separator;
    }

    public static MenuItem action(Supplier<String> label, Runnable action) {
        return new MenuItem(label, action, null, false);
    }

    public static MenuItem action(String label, Runnable action) {
        return action(() -> label, action);
    }

    public static MenuItem submenu(Menu m) {
        return new MenuItem(m.label, null, m, false);
    }

    public static MenuItem separator() {
        return new MenuItem(() -> "", null, null, true);
    }

    /** A tick shown while checked. */
    public MenuItem check(BooleanSupplier c) {
        checked = c;
        return this;
    }

    /** A radio mark shown while selected. */
    public MenuItem radio(BooleanSupplier c) {
        checked = c;
        radio = true;
        return this;
    }

    public MenuItem enabled(BooleanSupplier e) {
        enabled = e;
        return this;
    }

    /** The key shown at the right (the shortcut itself is set on the Ui). */
    public MenuItem shortcut(String s) {
        shortcut = s;
        return this;
    }

    boolean selectable() {
        return !separator && enabled.getAsBoolean();
    }
}
