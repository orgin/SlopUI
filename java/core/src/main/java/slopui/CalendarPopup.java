package slopui;

/** The month under a {@link DatePicker}: header with month/year arrows, the weekdays, six weeks of days. */
final class CalendarPopup extends Popup {
    private static final String[] MONTHS = {"January", "February", "March", "April", "May", "June", "July", "August",
            "September", "October", "November", "December"};
    private static final String[] DAYS = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    private final DatePicker picker;
    private int year, month, day;       // shown month, chosen day (keyboard)

    CalendarPopup(DatePicker picker) {
        this.picker = picker;
        owner = picker;
        year = picker.year;
        month = picker.month;
        day = picker.day;
        matchAnchor = false;
        anchor(picker.x, picker.y, picker.w, picker.h, false);
    }

    /** Cell size and header heights from the font (set in layout). */
    private float cell = 15, headH = 16, daysH = 11;

    private void measure(Ui ui) {
        Font f = ui.theme.font;
        cell = Math.max(f.width("00"), f.width("Mo")) + 7;
        cell = Math.max(cell, f.lineHeight() + 6);
        headH = f.lineHeight() + 8;
        daysH = f.lineHeight() + 4;
    }

    private float cell() {
        return cell;
    }

    @Override
    public float prefWidth(Ui ui) {
        measure(ui);
        float head = ui.theme.font.width("September 0000") + 60;
        return Math.max(7 * cell + 8, head);
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        measure(ui);
        return headH + daysH + 6 * cell + 6;
    }

    @Override
    public void layout(Ui ui) {
        measure(ui);
        cell = Math.max(cell, (w - 8) / 7);
        // (anchored at the field's right edge: placed to end there)
    }

    private float gridY() {
        return y + headH + daysH;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        float o = opening(ui);
        float before = p.opacity();
        p.setOpacity(before * o);
        Skin.panel(p, t, x, y, w, h, t.s[Theme.CUT], true);
        Font f = t.font;
        String head = MONTHS[month - 1] + " " + year;
        f.draw(p, head, x + (w - f.width(head)) / 2, y + (headH - f.lineHeight()) / 2, t.c[Theme.TEXT_BRIGHT]);
        float[] ax = arrowX();
        float ah = headH - 4, ay = y + 2;
        for (int i = 0; i < 4; i++) {
            boolean on = ui.isHover(this) && ui.mx >= ax[i] && ui.mx < ax[i] + ah && ui.my >= ay && ui.my < ay + ah;
            if (on) {
                Shapes.poly(p, Shapes.cut(ax[i], ay, ah, ah, 3, 0, 3, 0), t.c[Theme.HOVER_TOP], t.c[Theme.HOVER_BOTTOM], ay, ah);
            }
            boolean right = i >= 2;
            float cy = ay + ah / 2, cx = ax[i] + ah / 2;
            int col = on ? t.c[Theme.ACCENT] : t.c[Theme.TEXT];
            if (i == 0 || i == 3) {
                Skin.chevron(p, cx - 2, cy, 6, right ? 1 : 3, col);
                Skin.chevron(p, cx + 2, cy, 6, right ? 1 : 3, col);
            } else {
                Skin.chevron(p, cx, cy, 6, right ? 1 : 3, col);
            }
        }
        float c = cell();
        for (int d = 0; d < 7; d++) {
            f.draw(p, DAYS[d], x + 4 + d * c + (c - f.width(DAYS[d])) / 2, y + headH + 1, d >= 5 ? t.c[Theme.ACCENT_SOFT] : t.c[Theme.TEXT_DIM]);
        }
        Shapes.hgradient(p, x + 4, gridY() - 1, w - 8, 1, t.c[Theme.PANEL_BORDER], 0);
        int first = DatePicker.weekday(year, month, 1);
        int n = DatePicker.daysIn(year, month);
        int[] today = DatePicker.today();
        int hot = dayAt(ui.mx, ui.my);
        for (int i = 0; i < 42; i++) {
            int dnum = i - first + 1;
            if (dnum < 1 || dnum > n) {
                continue;
            }
            float cx = x + 4 + i % 7 * c, cy = gridY() + i / 7 * c;
            boolean chosen = picker.year == year && picker.month == month && picker.day == dnum;
            boolean kb = day == dnum;
            if (chosen) {
                float[] pts = Shapes.cut(cx + 1, cy + 1, c - 2, c - 2, 3, 0, 3, 0);
                Shapes.poly(p, pts, t.c[Theme.FILL_TOP], t.c[Theme.FILL_BOTTOM], cy, c);
            } else if (hot == dnum && ui.isHover(this) || kb) {
                float[] pts = Shapes.cut(cx + 1, cy + 1, c - 2, c - 2, 3, 0, 3, 0);
                Shapes.poly(p, pts, t.c[Theme.HOVER_TOP], t.c[Theme.HOVER_BOTTOM], cy, c);
            }
            if (today[0] == year && today[1] == month && today[2] == dnum) {
                Shapes.outline(p, Shapes.cut(cx + 1, cy + 1, c - 2, c - 2, 3, 0, 3, 0), 1, t.c[Theme.ACCENT]);
            }
            String s = String.valueOf(dnum);
            int col = chosen ? 0xFF101008 : i % 7 >= 5 ? t.c[Theme.ACCENT_SOFT] : t.c[Theme.TEXT];
            f.draw(p, s, cx + (c - f.width(s)) / 2, cy + (c - f.lineHeight()) / 2, col);
        }
        p.setOpacity(before);
    }

    private int dayAt(float mx, float my) {
        float c = cell();
        if (mx < x + 4 || mx >= x + 4 + 7 * c || my < gridY() || my >= gridY() + 6 * c) {
            return -1;
        }
        int i = (int) ((my - gridY()) / c) * 7 + (int) ((mx - x - 4) / c);
        int d = i - DatePicker.weekday(year, month, 1) + 1;
        return d >= 1 && d <= DatePicker.daysIn(year, month) ? d : -1;
    }

    private void addMonths(int n) {
        int m = year * 12 + month - 1 + n;
        year = Math.floorDiv(m, 12);
        month = Math.floorMod(m, 12) + 1;
        day = Math.min(day, DatePicker.daysIn(year, month));
    }

    /** The four arrows' x: year back, month back, month on, year on. */
    private float[] arrowX() {
        float ah = headH - 4;
        return new float[] {x + 3, x + 4 + ah, x + w - 4 - 2 * ah, x + w - 3 - ah};
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        float ah = headH - 4;
        if (my >= y + 2 && my < y + 2 + ah) {
            float[] ax = arrowX();
            int[] steps = {-12, -1, 1, 12};
            for (int i = 0; i < 4; i++) {
                if (mx >= ax[i] && mx < ax[i] + ah) {
                    addMonths(steps[i]);
                }
            }
            return;
        }
        int d = dayAt(mx, my);
        if (d > 0) {
            ui.closePopup(this);
            picker.setDate(year, month, d);
        }
    }

    @Override
    public boolean wheel(Ui ui, float mx, float my, float dy) {
        addMonths(dy > 0 ? -1 : 1);
        return true;
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        long z = DatePicker.days(year, month, day);
        switch (key) {
            case Keys.LEFT -> z -= 1;
            case Keys.RIGHT -> z += 1;
            case Keys.UP -> z -= 7;
            case Keys.DOWN -> z += 7;
            case Keys.PAGE_UP -> {
                addMonths(-1);
                return true;
            }
            case Keys.PAGE_DOWN -> {
                addMonths(1);
                return true;
            }
            case Keys.ENTER, Keys.KP_ENTER, Keys.SPACE -> {
                ui.closePopup(this);
                picker.setDate(year, month, day);
                return true;
            }
            case Keys.ESCAPE -> {
                ui.closePopup(this);
                return true;
            }
            default -> {
                return false;
            }
        }
        int[] c = DatePicker.civil(z);
        year = c[0];
        month = c[1];
        day = c[2];
        return true;
    }
}
