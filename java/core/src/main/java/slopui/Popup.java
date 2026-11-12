package slopui;

/**
 * Something shown over everything for a moment (menus, dropdown lists, the calendar): placed next to an
 * anchor rectangle, below it or above when there is no room below, kept inside the UI. A press outside the
 * popups closes them ({@link Ui}).
 */
public abstract class Popup extends Container {
    /** The rectangle it opens next to (UI units), and whether it opens to the side (submenus). */
    protected float ax, ay, aw, ah;
    protected boolean side;
    /** The widget it belongs to (pressing it again doesn't count as outside). */
    public Widget owner;
    private float open;
    /** At least as wide as the anchor (lists under fields); false: its own width (the calendar). */
    protected boolean matchAnchor = true;

    public void anchor(float x, float y, float w, float h, boolean side) {
        ax = x;
        ay = y;
        aw = w;
        ah = h;
        this.side = side;
    }

    /** Sized and placed (Ui.layout). */
    void place(Ui ui) {
        float pw = Math.min(ui.width, Math.max(prefWidth(ui), side || !matchAnchor ? 0 : aw));
        float maxH = ui.height;
        float ph = Math.min(maxH, prefHeight(ui, pw));
        float px, py;
        if (side) {
            px = ax + aw;
            if (px + pw > ui.width) {
                px = ax - pw;       // no room on the right: to the left
            }
            py = Math.min(ay, ui.height - ph);
        } else {
            px = Math.min(matchAnchor ? ax : ax + aw - pw, ui.width - pw);   // (own width: ends at the anchor's right)
            py = ay + ah;
            if (py + ph > ui.height && ay - ph >= 0) {
                py = ay - ph;       // no room below: above
            }
            py = Math.min(py, ui.height - ph);
        }
        setBounds(Math.round(Math.max(0, px)), Math.round(Math.max(0, py)), Math.round(pw), Math.round(ph));
        layout(ui);
    }

    /** The opening animation (0..1) for subclasses' drawing. */
    protected float opening(Ui ui) {
        open = animate(ui, 3, 1, ui.theme.s[Theme.OPEN_TIME] * 0.6f);
        return open;
    }

    /** Keys while it is the top popup; true: taken. */
    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key == Keys.ESCAPE) {
            ui.closePopup(this);
            return true;
        }
        return false;
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        return true;
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        return true;
    }
}
