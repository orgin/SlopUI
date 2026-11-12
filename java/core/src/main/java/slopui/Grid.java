package slopui;

/**
 * Children in rows of n columns (forms: label, field). Column widths: the widest preferred width in each,
 * spare room to the growing columns; too narrow for the columns' least widths, it stacks into one column.
 */
public class Grid extends Container {
    private final int columns;
    private final float[] colGrow;
    private float gapX = -1, gapY = -1;

    public Grid(int columns) {
        this.columns = columns;
        colGrow = new float[columns];
        colGrow[columns - 1] = 1;
    }

    /** Share of spare room for each column. */
    public Grid columnGrow(float... g) {
        System.arraycopy(g, 0, colGrow, 0, Math.min(g.length, columns));
        return this;
    }

    public Grid gaps(float gx, float gy) {
        gapX = gx;
        gapY = gy;
        return this;
    }

    private float gx(Ui ui) {
        return gapX >= 0 ? gapX : ui.theme.s[Theme.PAD];
    }

    private float gy(Ui ui) {
        return gapY >= 0 ? gapY : ui.theme.s[Theme.GAP];
    }

    private int rows() {
        return (kids.size() + columns - 1) / columns;
    }

    private float[] colPref(Ui ui, boolean min) {
        float[] c = new float[columns];
        for (int i = 0; i < kids.size(); i++) {
            Widget k = kids.get(i);
            c[i % columns] = Math.max(c[i % columns], min ? k.minWidth(ui) : k.prefWidth(ui));
        }
        return c;
    }

    private boolean stacked(Ui ui, float width) {
        float need = (columns - 1) * gx(ui);
        for (float v : colPref(ui, true)) {
            need += v;
        }
        return need > width + 0.01f;
    }

    @Override
    public float prefWidth(Ui ui) {
        float s = (columns - 1) * gx(ui);
        for (float v : colPref(ui, false)) {
            s += v;
        }
        return s;
    }

    @Override
    public float minWidth(Ui ui) {
        float m = 0;
        for (float v : colPref(ui, true)) {
            m = Math.max(m, v);
        }
        return m;
    }

    private float[] widths(Ui ui, float width) {
        float[] c = colPref(ui, false);
        float sum = (columns - 1) * gx(ui), g = 0;
        for (int i = 0; i < columns; i++) {
            sum += c[i];
            g += colGrow[i];
        }
        float spare = width - sum;
        if (spare > 0 && g > 0) {
            for (int i = 0; i < columns; i++) {
                c[i] += spare * colGrow[i] / g;
            }
        } else if (spare < 0) {
            float[] m = colPref(ui, true);
            float slack = 0;
            for (int i = 0; i < columns; i++) {
                slack += c[i] - m[i];
            }
            for (int i = 0; i < columns; i++) {
                c[i] -= slack > 0 ? Math.min(1, -spare / slack) * (c[i] - m[i]) : 0;
            }
        }
        return c;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        if (stacked(ui, width)) {
            float s = 0;
            for (Widget k : kids) {
                s += k.prefHeight(ui, width);
            }
            return s + Math.max(0, kids.size() - 1) * gy(ui) * 0.5f;
        }
        float[] c = widths(ui, width);
        float s = 0;
        for (int r = 0; r < rows(); r++) {
            float rh = 0;
            for (int col = 0; col < columns && r * columns + col < kids.size(); col++) {
                rh = Math.max(rh, kids.get(r * columns + col).prefHeight(ui, c[col]));
            }
            s += rh;
        }
        return s + Math.max(0, rows() - 1) * gy(ui);
    }

    @Override
    public void layout(Ui ui) {
        if (stacked(ui, w)) {
            float py = y;
            for (Widget k : kids) {
                float kh = k.prefHeight(ui, w);
                k.setBounds(x, py, w, kh);
                k.layout(ui);
                py += kh + gy(ui) * 0.5f;
            }
            return;
        }
        float[] c = widths(ui, w);
        float py = y;
        for (int r = 0; r < rows(); r++) {
            float rh = 0;
            for (int col = 0; col < columns && r * columns + col < kids.size(); col++) {
                rh = Math.max(rh, kids.get(r * columns + col).prefHeight(ui, c[col]));
            }
            float px = x;
            for (int col = 0; col < columns && r * columns + col < kids.size(); col++) {
                Widget k = kids.get(r * columns + col);
                k.setBounds(px, py, c[col], rh);
                k.layout(ui);
                px += c[col] + gx(ui);
            }
            py += rh + gy(ui);
        }
    }
}
