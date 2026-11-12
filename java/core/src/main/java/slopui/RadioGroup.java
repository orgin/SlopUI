package slopui;

import java.util.ArrayList;
import java.util.List;
import java.util.function.IntConsumer;

/** Radio buttons that belong together: one selected. */
public final class RadioGroup {
    final List<RadioButton> buttons = new ArrayList<>();
    private int selected;
    private IntConsumer onChange;

    public RadioGroup(int selected) {
        this.selected = selected;
    }

    public RadioGroup onChange(IntConsumer c) {
        onChange = c;
        return this;
    }

    /** A new button of this group (its index: the order made). */
    public RadioButton button(String text) {
        RadioButton b = new RadioButton(this, buttons.size(), text);
        buttons.add(b);
        return b;
    }

    public int selected() {
        return selected;
    }

    public void select(int i) {
        if (i != selected) {
            selected = i;
            if (onChange != null) {
                onChange.accept(i);
            }
        }
    }
}
