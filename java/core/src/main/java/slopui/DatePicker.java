package slopui;

import java.util.function.Consumer;

/**
 * A calendar date (real dates only): typed as YYYY-MM-DD or picked from a calendar under the field (months
 * and years stepped by its arrows, weeks from Monday, today marked). Arrow keys move the day in the calendar,
 * Page Up/Down the month.
 */
public class DatePicker extends TextField {
    int year, month, day;      // month 1-12
    private Consumer<int[]> onDate;
    private CalendarPopup calendar;

    public DatePicker(int year, int month, int day) {
        super("");
        rightInset = 14;
        allow(c -> Character.isDigit(c) || c == '-');
        maxLength(10);
        setDate(year, month, day);
    }

    /** Today (the system clock, UTC). */
    public static int[] today() {
        return civil(Math.floorDiv(System.currentTimeMillis(), 86_400_000L));
    }

    public DatePicker onDate(Consumer<int[]> c) {
        onDate = c;
        return this;
    }

    public int[] date() {
        return new int[] {year, month, day};
    }

    public void setDate(int y, int m, int d) {
        year = y;
        month = m;
        day = Math.min(d, daysIn(y, m));
        set(String.format("%04d-%02d-%02d", year, month, day));
        if (onDate != null) {
            onDate.accept(date());
        }
    }

    @Override
    protected void changed() {
        super.changed();
        String s = text();
        if (s.length() == 10 && s.charAt(4) == '-' && s.charAt(7) == '-') {
            try {
                int y = Integer.parseInt(s.substring(0, 4)), m = Integer.parseInt(s.substring(5, 7)),
                        d = Integer.parseInt(s.substring(8, 10));
                if (m >= 1 && m <= 12 && d >= 1 && d <= daysIn(y, m)) {
                    year = y;
                    month = m;
                    day = d;
                    if (onDate != null) {
                        onDate.accept(date());
                    }
                }
            } catch (NumberFormatException e) {
                // (not a date yet)
            }
        }
    }

    public static boolean leap(int y) {
        return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
    }

    public static int daysIn(int y, int m) {
        return m == 2 ? (leap(y) ? 29 : 28) : m == 4 || m == 6 || m == 9 || m == 11 ? 30 : 31;
    }

    /** Days since 1970-01-01 (proleptic Gregorian). */
    public static long days(int y, int m, int d) {
        y -= m <= 2 ? 1 : 0;
        long era = Math.floorDiv(y, 400);
        long yoe = y - era * 400;
        long doy = (153L * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + doe - 719468;
    }

    /** Year, month, day of days since 1970-01-01. */
    public static int[] civil(long z) {
        z += 719468;
        long era = Math.floorDiv(z, 146097);
        long doe = z - era * 146097;
        long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        long y = yoe + era * 400;
        long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        long mp = (5 * doy + 2) / 153;
        long d = doy - (153 * mp + 2) / 5 + 1;
        long m = mp + (mp < 10 ? 3 : -9);
        return new int[] {(int) (y + (m <= 2 ? 1 : 0)), (int) m, (int) d};
    }

    /** 0 Monday .. 6 Sunday. */
    public static int weekday(int y, int m, int d) {
        return (int) Math.floorMod(days(y, m, d) + 3, 7);
    }

    @Override
    public float prefWidth(Ui ui) {
        return ui.theme.font.width("0000-00-00") + 26;
    }

    @Override
    public float minWidth(Ui ui) {
        return prefWidth(ui);
    }

    @Override
    public void draw(Ui ui, Painter p) {
        super.draw(ui, p);
        Theme t = ui.theme;
        float bx = x + w - rightInset;
        boolean on = ui.isHover(this) && ui.mx >= bx || calendar != null && ui.popups().contains(calendar);
        Shapes.fill(p, bx, y + 2, 1, h - 6 + 2, Shapes.alpha(t.c[Theme.FIELD_BORDER], 0.8f));
        int c = on ? t.c[Theme.ACCENT] : t.c[Theme.TEXT];
        float cx = bx + 3, cy = y + 3, cw = 8, ch = h - 6;      // a small calendar sheet
        Shapes.outline(p, Shapes.cut(cx, cy, cw, ch, 0, 0, 0, 0), 1, c);
        Shapes.fill(p, cx, cy, cw, 2, c);
        Shapes.fill(p, cx + 2, cy + 4, 1, 1, c);
        Shapes.fill(p, cx + 5, cy + 4, 1, 1, c);
    }

    private void openCalendar(Ui ui) {
        calendar = new CalendarPopup(this);
        ui.openPopup(calendar);
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button == 0 && mx >= x + w - rightInset) {
            openCalendar(ui);
            return true;
        }
        return super.press(ui, mx, my, button, clicks);
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key == Keys.DOWN && (mods & Keys.ALT) != 0) {
            openCalendar(ui);
            return true;
        }
        return super.key(ui, key, mods);
    }
}
