package slopui;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Supplier;

/** A menu: a label and its items (made again before it opens when onOpen is set). */
public final class Menu {
    final Supplier<String> label;
    final List<MenuItem> items = new ArrayList<>();
    Runnable onOpen;

    public Menu(Supplier<String> label) {
        this.label = label;
    }

    public Menu(String label) {
        this(() -> label);
    }

    public Menu add(MenuItem... it) {
        for (MenuItem i : it) {
            items.add(i);
        }
        return this;
    }

    public Menu separator() {
        items.add(MenuItem.separator());
        return this;
    }

    public Menu onOpen(Runnable r) {
        onOpen = r;
        return this;
    }

    public void clear() {
        items.clear();
    }

    public String label() {
        return label.get();
    }

    public List<MenuItem> items() {
        return items;
    }
}
