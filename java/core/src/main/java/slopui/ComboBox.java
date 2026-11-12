package slopui;

import java.util.ArrayList;
import java.util.List;

/** A text field with suggestions: typing filters the list under it; the chevron opens all of it. */
public class ComboBox extends TextField {
    private final List<String> items = new ArrayList<>();
    private ListPopup list;

    public ComboBox(String initial, List<String> items) {
        super(initial);
        this.items.addAll(items);
        rightInset = 15;
    }

    private void showList(Ui ui, boolean all) {
        if (list != null) {
            ui.closePopup(list);
        }
        String q = text().toLowerCase();
        List<String> shown = new ArrayList<>();
        for (String s : items) {
            if (all || q.isEmpty() || s.toLowerCase().contains(q)) {
                shown.add(s);
            }
        }
        if (shown.isEmpty()) {
            list = null;
            return;
        }
        list = new ListPopup(this, shown, all ? Math.max(0, shown.indexOf(text())) : -1, i -> {
            set(shown.get(i));
            list = null;
        });
        ui.openPopup(list);
    }

    @Override
    public void draw(Ui ui, Painter p) {
        super.draw(ui, p);
        Theme t = ui.theme;
        boolean on = ui.isHover(this) && ui.mx >= x + w - rightInset;
        Shapes.fill(p, x + w - rightInset, y + 3, 1, h - 6, Shapes.alpha(t.c[Theme.FIELD_BORDER], 0.8f));
        Skin.chevron(p, x + w - 8, y + h / 2, 6, 0, on ? t.c[Theme.ACCENT] : t.c[Theme.TEXT]);
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button == 0 && mx >= x + w - rightInset) {
            showList(ui, true);
            return true;
        }
        return super.press(ui, mx, my, button, clicks);
    }

    @Override
    public boolean typed(Ui ui, int cp) {
        boolean r = super.typed(ui, cp);
        showList(ui, false);
        return r;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key == Keys.DOWN) {
            showList(ui, true);
            return true;
        }
        boolean r = super.key(ui, key, mods);
        if (key == Keys.BACKSPACE || key == Keys.DELETE || (mods & Keys.CTRL) != 0 && (key == Keys.X || key == Keys.V)) {
            showList(ui, false);
        }
        return r;
    }
}
