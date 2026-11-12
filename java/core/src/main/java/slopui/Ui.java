package slopui;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

/**
 * The UI over a game: the background widget (the game), the menu bar, windows, popups, toasts and tooltips,
 * in UI units ({@link #unit} device pixels each). The host feeds it the window's size, input and frame time,
 * and draws it with a {@link Painter} each frame.
 * <p>
 * Input goes to the topmost thing under the pointer: popups, the menu bar, windows (a modal window blocks
 * what is under it), then the background. A press's drags and release go to the widget that took it. Keys go
 * to the top popup, else the focused widget and its parents, the top window (Enter, Escape, Tab), the menu
 * bar (F10, Alt + letter), the shortcuts, and last the background.
 */
public final class Ui {
    public Theme theme;
    public Host host;
    /** Device pixels per UI unit, and the UI's size in units. */
    public float unit = 1, width, height;
    /** Seconds since the last frame, and since the start. */
    public float dt;
    public double time;
    /** Set while anything animates (the host may draw only when this or input asks). */
    public boolean animating;

    public final MenuBar menuBar = new MenuBar();
    private Widget background;
    private final List<Window> windows = new ArrayList<>();
    private final List<Popup> popups = new ArrayList<>();
    Widget hover, pressed, focus;
    private int pressedButton = -1;
    public float mx = -1000, my = -1000;
    /** The modifier keys held (Keys.SHIFT...), set by the host with each input event. */
    public int mods;
    private boolean barAutoHide, barShown;
    private boolean dirty = true;

    private Widget tipWidget;
    private float tipRest;
    private float tipX, tipY;
    private double lastPress = -10;
    private float lastPressX, lastPressY;
    private int clicks;

    private final List<Object[]> toasts = new ArrayList<>();
    private final Map<Integer, Runnable> shortcuts = new HashMap<>();

    public Ui(Theme theme, Host host) {
        this.theme = theme;
        this.host = host;
        menuBar.ui = this;
    }

    // Set-up

    /** The game (or whatever is under the UI): gets the input nothing above takes. */
    public void setBackground(Widget w) {
        background = w;
        dirty = true;
    }

    public void setSize(float w, float h, float unit) {
        if (w != width || h != height || unit != this.unit) {
            width = w;
            height = h;
            this.unit = unit;
            dirty = true;
        }
    }

    /** Fullscreen: the bar hidden until the pointer reaches the top edge. */
    public void setBarAutoHide(boolean on) {
        barAutoHide = on;
        barShown = false;
        dirty = true;
    }

    public void shortcut(int key, Runnable r) {
        shortcuts.put(key, r);
    }

    public float barHeight() {
        return theme.s[Theme.BAR_HEIGHT];
    }

    public boolean barVisible() {
        return !menuBar.isEmpty() && (!barAutoHide || barShown || menuBar.isOpen());
    }

    /** Where the game and the windows start: under the bar (0 when the bar hides itself). */
    public float contentTop() {
        return menuBar.isEmpty() || barAutoHide ? 0 : barHeight();
    }

    public boolean needsRedraw() {
        boolean d = dirty || animating;
        dirty = false;
        return d;
    }

    // Layers

    public void open(Window w) {
        windows.remove(w);
        windows.add(w);
        w.recentre();
        closePopups();
        focusFirst(w);
        dirty = true;
    }

    public void close(Window w) {
        if (windows.remove(w)) {
            if (focus != null && within(focus, w)) {
                focus = null;
            }
            w.closed();
            Window top = topWindow();
            if (top != null) {
                focusFirst(top);
            }
            dirty = true;
        }
    }

    public boolean isOpen(Window w) {
        return windows.contains(w);
    }

    public Window topWindow() {
        return windows.isEmpty() ? null : windows.get(windows.size() - 1);
    }

    private Window topModal() {
        for (int i = windows.size() - 1; i >= 0; i--) {
            if (windows.get(i).isModal()) {
                return windows.get(i);
            }
        }
        return null;
    }

    public boolean modalOpen() {
        return topModal() != null;
    }

    public void openPopup(Popup p) {
        popups.add(p);
        dirty = true;
    }

    /** The popup and every popup opened after it closed. */
    public void closePopup(Popup p) {
        int i = popups.indexOf(p);
        if (i >= 0) {
            while (popups.size() > i) {
                popups.remove(popups.size() - 1);
            }
        }
        if (popups.isEmpty()) {
            menuBar.closed();
        }
        dirty = true;
    }

    public void closePopups() {
        popups.clear();
        menuBar.closed();
        dirty = true;
    }

    public boolean popupOpen() {
        return !popups.isEmpty();
    }

    public List<Popup> popups() {
        return popups;
    }

    /** A context menu at the pointer. */
    public void contextMenu(Menu m) {
        closePopups();
        MenuPopup p = new MenuPopup(m, null);
        p.anchor(mx, my, 0, 0, false);
        openPopup(p);
    }

    /** A short message at the bottom that fades out after a few seconds. */
    public void toast(String text) {
        toasts.add(new Object[] {text, time});
        dirty = true;
    }

    // Focus

    public Widget focused() {
        return focus;
    }

    public void setFocus(Widget w) {
        if (focus != w) {
            focus = w;
            dirty = true;
        }
    }

    public boolean isFocused(Widget w) {
        return focus == w;
    }

    private static boolean within(Widget w, Widget root) {
        for (Widget t = w; t != null; t = t.parent) {
            if (t == root) {
                return true;
            }
        }
        return false;
    }

    private List<Widget> focusables(Widget root) {
        List<Widget> out = new ArrayList<>();
        collect(root, out);
        return out;
    }

    private void collect(Widget w, List<Widget> out) {
        if (!w.visible || !w.isEnabled()) {
            return;
        }
        if (w.focusable()) {
            out.add(w);
        }
        List<Widget> kids = new ArrayList<>();
        w.children(kids);
        for (Widget k : kids) {
            collect(k, out);
        }
    }

    private void focusFirst(Window w) {
        List<Widget> f = focusables(w);
        Widget pick = null;
        for (Widget k : f) {
            if (k instanceof TextField || k instanceof TextArea) {
                pick = k;
                break;
            }
        }
        focus = pick;
    }

    public void focusNext(boolean back) {
        Window top = topWindow();
        Widget root = top != null ? top : background;
        if (root == null) {
            return;
        }
        List<Widget> f = focusables(root);
        if (f.isEmpty()) {
            return;
        }
        int i = f.indexOf(focus);
        i = i < 0 ? (back ? f.size() - 1 : 0) : (i + (back ? f.size() - 1 : 1)) % f.size();
        setFocus(f.get(i));
        revealFocus();
    }

    private void revealFocus() {
        for (Widget t = focus; t != null; t = t.parent) {
            if (t.parent instanceof ScrollPane sp) {
                sp.reveal(this, focus.y, focus.h);
            }
        }
    }

    // Frames

    /** A frame passed: timers, held presses, the pointer's shape. */
    public void frame(float seconds) {
        dt = Math.min(0.1f, seconds);
        time += dt;
        animating = false;
        if (pressed != null) {
            pressed.tick(this);
        }
        if (hover != null && hover == tipWidget && pressed == null) {
            float before = tipRest;
            tipRest += dt;
            if (before < theme.s[Theme.TOOLTIP_DELAY] && tipRest >= theme.s[Theme.TOOLTIP_DELAY]) {
                dirty = true;
            }
        }
        for (int i = toasts.size() - 1; i >= 0; i--) {
            if (time - (Double) toasts.get(i)[1] > 3.2) {
                toasts.remove(i);
                dirty = true;
            }
        }
        if (!toasts.isEmpty()) {
            animating = true;
        }
        if (host != null) {
            Widget c = pressed != null ? pressed : hover;
            host.setCursor(c == null ? Cursor.ARROW : c.cursor(mx, my));
        }
    }

    public void layout() {
        float bh = barHeight();
        menuBar.setBounds(0, 0, width, bh);
        menuBar.layout(this);
        if (background != null) {
            float top = contentTop();
            background.setBounds(0, top, width, height - top);
            background.layout(this);
        }
        for (Window w : windows) {
            w.place(this);
        }
        for (Popup p : new ArrayList<>(popups)) {
            p.place(this);
        }
    }

    public void draw(Painter p) {
        layout();
        if (background != null) {
            background.draw(this, p);
        }
        Window modal = topModal();
        boolean barOver = barAutoHide;      // (fullscreen: the bar over the game and the windows)
        if (barVisible() && !barOver) {
            menuBar.draw(this, p);
        }
        for (Window w : windows) {
            if (w == modal) {
                Shapes.fill(p, 0, 0, width, height, theme.c[Theme.SHADE]);
            }
            w.draw(this, p);
        }
        if (barVisible() && barOver) {
            menuBar.draw(this, p);
        }
        for (Popup pp : new ArrayList<>(popups)) {
            pp.draw(this, p);
        }
        drawToasts(p);
        drawTooltip(p);
    }

    private void drawToasts(Painter p) {
        float y = height - 12;
        Font f = theme.font;
        for (int i = toasts.size() - 1; i >= 0; i--) {
            String text = (String) toasts.get(i)[0];
            double age = time - (Double) toasts.get(i)[1];
            float a = (float) Math.min(1, Math.min(age / 0.15, (3.2 - age) / 0.5));
            float tw = f.width(text) + 24, th = f.lineHeight() + 10;
            float tx = (width - tw) / 2, ty = y - th + (1 - a) * 8;
            float before = p.opacity();
            p.setOpacity(before * a);
            Skin.panel(p, theme, tx, ty, tw, th, theme.s[Theme.CUT], true);
            Shapes.fill(p, tx + 4, ty + th - 3, tw - 8, 1, Shapes.alpha(theme.c[Theme.ACCENT], 0.6f));
            f.draw(p, text, tx + 12, ty + 5, theme.c[Theme.TEXT_BRIGHT]);
            p.setOpacity(before);
            y = ty - 4;
        }
    }

    private void drawTooltip(Painter p) {
        if (tipWidget == null || tipWidget.tooltip == null || pressed != null
                || tipRest < theme.s[Theme.TOOLTIP_DELAY]) {
            return;
        }
        String text = tipWidget.tooltip.get();
        if (text == null || text.isEmpty()) {
            return;
        }
        Font f = theme.font;
        float a = Math.min(1, (tipRest - theme.s[Theme.TOOLTIP_DELAY]) / 0.12f);
        if (a < 1) {
            animating = true;
        }
        float tw = f.width(text) + 12, th = f.lineHeight() + 8;
        float tx = Math.max(0, Math.min(tipX + 4, width - tw));
        float ty = tipY + 14;
        if (ty + th > height) {
            ty = tipY - th - 4;
        }
        float before = p.opacity();
        p.setOpacity(before * a);
        float[] pts = Shapes.cut(tx, ty, tw, th, 3, 0, 3, 0);
        Shapes.ring(p, pts, Shapes.offset(pts, 5), theme.c[Theme.SHADOW], 0);
        Shapes.poly(p, pts, theme.c[Theme.TOOLTIP_TOP], theme.c[Theme.TOOLTIP_BOTTOM], ty, th);
        Shapes.outline(p, pts, 1, theme.c[Theme.ACCENT_SOFT]);
        f.draw(p, text, tx + 6, ty + 4, theme.c[Theme.TEXT_BRIGHT]);
        p.setOpacity(before);
    }

    // Input (UI units; buttons 0 left, 1 right, 2 middle)

    public Widget hitAt(float x, float y) {
        for (int i = popups.size() - 1; i >= 0; i--) {
            Widget h = popups.get(i).hit(x, y);
            if (h != null) {
                return h;
            }
        }
        Window modal = topModal();
        if (barVisible() && (modal == null || barAutoHide && barShown) && y < barHeight()) {
            Widget h = menuBar.hit(x, y);
            if (h != null) {
                return h;
            }
        }
        for (int i = windows.size() - 1; i >= 0; i--) {
            Window w = windows.get(i);
            Widget h = w.hit(x, y);
            if (h != null) {
                return h;
            }
            if (w == modal) {
                return null;
            }
        }
        return background == null ? null : background.hit(x, y);
    }

    private Window windowOf(Widget w) {
        for (Widget t = w; t != null; t = t.parent) {
            if (t instanceof Window win) {
                return win;
            }
        }
        return null;
    }

    private boolean inPopup(Widget w) {
        for (Widget t = w; t != null; t = t.parent) {
            if (t instanceof Popup) {
                return true;
            }
        }
        return false;
    }

    public void mouseMoved(float x, float y) {
        if (Math.abs(x - mx) > 2 || Math.abs(y - my) > 2) {
            tipRest = 0;
            tipX = x;
            tipY = y;
        }
        mx = x;
        my = y;
        if (barAutoHide) {
            boolean show = y <= 1 || barShown && (y < barHeight() || menuBar.isOpen());
            if (show != barShown) {
                barShown = show;
                dirty = true;
            }
        }
        Widget h = hitAt(x, y);
        if (h != hover) {
            hover = h;
            tipWidget = h;
            while (tipWidget != null && tipWidget.tooltip == null) {
                tipWidget = tipWidget.parent;
            }
            tipRest = 0;
            tipX = x;
            tipY = y;
        }
        if (pressed != null) {
            pressed.drag(this, x, y);
        }
        for (Widget t = h; t != null; t = t.parent) {
            if (t instanceof MenuPopup mp) {
                mp.hoverAt(this, x, y);
                break;
            }
        }
        menuBar.moved(this, x, y);
        dirty = true;
    }

    public void mousePressed(int button) {
        dirty = true;
        tipRest = -1000;    // (no tooltip until the pointer moves again)
        if (time - lastPress < 0.4 && Math.abs(mx - lastPressX) < 4 && Math.abs(my - lastPressY) < 4) {
            clicks++;
        } else {
            clicks = 1;
        }
        lastPress = time;
        lastPressX = mx;
        lastPressY = my;
        Widget h = hitAt(mx, my);
        if (!popups.isEmpty() && !inPopup(h) && !(h instanceof MenuBar)) {
            boolean ownerHit = false;
            for (Popup pp : popups) {
                ownerHit |= h != null && h == pp.owner;
            }
            closePopups();
            if (!ownerHit) {
                return;     // (the press only closed the popups)
            }
            return;
        }
        Window win = windowOf(h);
        if (win != null && windows.indexOf(win) < windows.size() - 1 && topModal() != windows.get(windows.size() - 1)) {
            windows.remove(win);
            windows.add(win);
        }
        if (!inPopup(h)) {
            Widget f = h;
            while (f != null && !f.focusable()) {
                f = f.parent;
            }
            setFocus(f);
        }
        for (Widget t = h; t != null; t = t.parent) {
            if (t.isEnabled() && t.press(this, mx, my, button, clicks)) {
                pressed = t;
                pressedButton = button;
                return;
            }
        }
    }

    public void mouseReleased(int button) {
        dirty = true;
        if (pressed != null && button == pressedButton) {
            Widget p = pressed;
            pressed = null;
            pressedButton = -1;
            p.release(this, mx, my, button);
        }
        hover = hitAt(mx, my);
    }

    public void wheel(float dy) {
        dirty = true;
        for (Widget t = hitAt(mx, my); t != null; t = t.parent) {
            if (t.isEnabled() && t.wheel(this, mx, my, dy)) {
                return;
            }
        }
    }

    public void key(int key, int mods) {
        dirty = true;
        if (!popups.isEmpty()) {
            Popup top = popups.get(popups.size() - 1);
            if (top.key(this, key, mods)) {
                return;
            }
            if (key == Keys.ESCAPE || focus == null || top.owner != focus) {
                if (key == Keys.ESCAPE) {
                    closePopup(top);
                }
                return;
            }
            // (a list under a focused field, a combobox's: the other keys edit the field)
        }
        for (Widget t = focus; t != null; t = t.parent) {
            if (t.isEnabled() && t.key(this, key, mods)) {
                return;
            }
        }
        Window top = topWindow();
        if (key == Keys.TAB && (top != null || focus != null)) {
            focusNext((mods & Keys.SHIFT) != 0);
            return;
        }
        if (top != null && (focus == null || !within(focus, top)) && top.key(this, key, mods)) {
            return;
        }
        if (focus != null && focus.takesText() && (mods & (Keys.CTRL | Keys.ALT)) == 0 && key < Keys.ESCAPE) {
            return;     // (typed into the field: not a shortcut or a key for the game)
        }
        if (top == null || !top.isModal()) {
            if (key == Keys.F10 || (mods & Keys.ALT) != 0 && key >= Keys.A && key <= Keys.Z) {
                if (menuBar.openByKey(this, key)) {
                    if (barAutoHide) {
                        barShown = true;
                    }
                    return;
                }
            }
        }
        Runnable r = shortcuts.get(key);
        if (r != null) {
            r.run();
            return;
        }
        if (background != null && (top == null || !top.isModal())) {
            background.key(this, key, mods);
        }
    }

    public void typed(int cp) {
        dirty = true;
        if (focus != null && (popups.isEmpty() || popups.get(popups.size() - 1).owner == focus)) {
            focus.typed(this, cp);
        }
    }

    public boolean leftDown() {
        return pressed != null && pressedButton == 0;
    }

    public Widget pressedWidget() {
        return pressed;
    }

    public boolean isHover(Widget w) {
        for (Widget t = hover; t != null; t = t.parent) {
            if (t == w) {
                return true;
            }
        }
        return false;
    }

    // Dialogs

    /** A message with an OK button. */
    public Window message(String title, String text, Runnable done) {
        Window[] win = new Window[1];
        Button ok = new Button("OK", () -> {
            close(win[0]);
            if (done != null) {
                done.run();
            }
        });
        win[0] = new Window(title, new Label(text)).width(Math.min(360, theme.font.width(text) + 20)).defaultButton(ok);
        open(win[0]);
        return win[0];
    }

    /** A question with two buttons; onYes runs on the first (the default button). */
    public Window confirm(String title, String text, String yes, String no, Runnable onYes) {
        return confirm(title, text, yes, no, true, onYes);
    }

    /** A question with two buttons; onYes runs on the first; yesDefault false: Enter presses the second. */
    public Window confirm(String title, String text, String yes, String no, boolean yesDefault, Runnable onYes) {
        Window[] win = new Window[1];
        Button y = new Button(yes, () -> {
            close(win[0]);
            onYes.run();
        });
        Button n = new Button(no, () -> close(win[0]));
        win[0] = new Window(title, new Label(text)).width(Math.min(360, theme.font.width(text) + 20));
        if (yesDefault) {
            win[0].button(n).defaultButton(y);
        } else {
            win[0].button(y).defaultButton(n);
        }
        open(win[0]);
        return win[0];
    }

    /** A text asked for; onOk gets it. */
    public Window ask(String title, String label, String initial, Consumer<String> onOk) {
        return ask(title, label, initial, "OK", "Cancel", null, onOk);
    }

    /**
     * A text asked for, with the buttons' labels; check (may be null) gives a warning for an unusable text (null:
     * fine), shown under the field while the window stays open; onOk gets a text that passed.
     */
    public Window ask(String title, String label, String initial, String okLabel, String cancelLabel,
                      java.util.function.Function<String, String> check, Consumer<String> onOk) {
        Window[] win = new Window[1];
        TextField field = new TextField(initial);
        Label warning = new Label("").warning();
        warning.setVisible(false);
        Button ok = new Button(okLabel, () -> {
            String problem = check == null ? null : check.apply(field.text());
            if (problem != null) {
                warning.set(problem);
                warning.setVisible(true);
                return;
            }
            close(win[0]);
            onOk.accept(field.text());
        });
        field.onChange(s -> warning.setVisible(false));
        Button cancel = new Button(cancelLabel, () -> close(win[0]));
        win[0] = new Window(title, Box.column(new Label(label), field, warning)).width(260);
        win[0].button(cancel).defaultButton(ok);
        open(win[0]);
        setFocus(field);
        field.selectAll();
        return win[0];
    }

}
