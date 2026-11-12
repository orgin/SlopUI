package slopui;

import java.util.ArrayList;
import java.util.List;

/**
 * A row or a column. Children get their preferred size along the axis; spare room goes to those that grow
 * (or to 'justify' when none do); short of room they shrink towards their least width. A responsive row wraps
 * onto more lines when even its least widths don't fit (a window's buttons on a narrow window), each line
 * shared out across the width. Children stretch across the axis.
 */
public class Box extends Container {
    private boolean horizontal;
    private boolean responsive;
    private float gap = -1, pad;
    private float justify;

    public static Box row(Widget... ws) {
        Box b = new Box();
        b.horizontal = true;
        b.add(ws);
        return b;
    }

    public static Box column(Widget... ws) {
        Box b = new Box();
        b.add(ws);
        return b;
    }

    /** A row that stacks into a column when it doesn't fit. */
    public Box responsive() {
        responsive = true;
        return this;
    }

    public Box gap(float g) {
        gap = g;
        return this;
    }

    public Box pad(float p) {
        pad = p;
        return this;
    }

    /** Where the children sit when none grows and there is room to spare: 0 start, 0.5 centre, 1 end. */
    public Box justify(float j) {
        justify = j;
        return this;
    }

    private float gap(Ui ui) {
        return gap >= 0 ? gap : ui.theme.s[Theme.GAP];
    }

    private List<Widget> shown() {
        List<Widget> l = new ArrayList<>();
        for (Widget k : kids) {
            if (k.visible) {
                l.add(k);
            }
        }
        return l;
    }

    /** Whether the row is laid out as a row at this width (responsive rows stack when too narrow). */
    private boolean asRow(Ui ui, float width) {
        if (!horizontal) {
            return false;
        }
        if (!responsive) {
            return true;
        }
        List<Widget> l = shown();
        float need = 2 * pad + Math.max(0, l.size() - 1) * gap(ui);
        for (Widget k : l) {
            need += k.minWidth(ui);
        }
        return need <= width + 0.01f;
    }

    @Override
    public float prefWidth(Ui ui) {
        List<Widget> l = shown();
        float s = 0;
        for (Widget k : l) {
            s = horizontal ? s + k.prefWidth(ui) : Math.max(s, k.prefWidth(ui));
        }
        if (horizontal) {
            s += Math.max(0, l.size() - 1) * gap(ui);
        }
        return s + 2 * pad;
    }

    @Override
    public float minWidth(Ui ui) {
        List<Widget> l = shown();
        float s = 0;
        boolean stackable = responsive || !horizontal;
        for (Widget k : l) {
            s = stackable ? Math.max(s, k.minWidth(ui)) : s + k.minWidth(ui);
        }
        if (!stackable) {
            s += Math.max(0, l.size() - 1) * gap(ui);
        }
        return s + 2 * pad;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        List<Widget> l = shown();
        float inner = width - 2 * pad;
        float s = 0;
        if (asRow(ui, width)) {
            float[] ws = widths(ui, l, inner);
            for (int i = 0; i < l.size(); i++) {
                s = Math.max(s, l.get(i).prefHeight(ui, ws[i]));
            }
        } else if (horizontal) {
            for (List<Widget> line : lines(ui, l, inner)) {
                float lh = 0;
                float[] ws = widths(ui, line, inner);
                for (int i = 0; i < line.size(); i++) {
                    lh = Math.max(lh, line.get(i).prefHeight(ui, ws[i]));
                }
                s += lh + gap(ui);
            }
            s -= gap(ui);
        } else {
            for (Widget k : l) {
                s += k.prefHeight(ui, inner);
            }
            s += Math.max(0, l.size() - 1) * gap(ui);
        }
        return s + 2 * pad;
    }

    /** A wrapped row's lines: as many children per line as fit at their preferred widths (at least one). */
    private List<List<Widget>> lines(Ui ui, List<Widget> l, float inner) {
        List<List<Widget>> out = new ArrayList<>();
        List<Widget> line = new ArrayList<>();
        float used = 0;
        for (Widget k : l) {
            float kw = Math.min(inner, k.prefWidth(ui));
            if (!line.isEmpty() && used + gap(ui) + kw > inner) {
                out.add(line);
                line = new ArrayList<>();
                used = 0;
            }
            used += (line.isEmpty() ? 0 : gap(ui)) + kw;
            line.add(k);
        }
        if (!line.isEmpty()) {
            out.add(line);
        }
        return out;
    }

    /** The children's widths along a row of this inner width. */
    private float[] widths(Ui ui, List<Widget> l, float inner) {
        int n = l.size();
        float[] ws = new float[n];
        float avail = inner - Math.max(0, n - 1) * gap(ui);
        float sum = 0, growSum = 0;
        for (int i = 0; i < n; i++) {
            ws[i] = l.get(i).prefWidth(ui);
            sum += ws[i];
            growSum += l.get(i).grow;
        }
        if (sum <= avail) {
            if (growSum > 0) {
                for (int i = 0; i < n; i++) {
                    ws[i] += (avail - sum) * l.get(i).grow / growSum;
                }
            }
        } else {
            float slack = 0;
            float[] mins = new float[n];
            for (int i = 0; i < n; i++) {
                mins[i] = Math.min(ws[i], l.get(i).minWidth(ui));
                slack += ws[i] - mins[i];
            }
            float over = sum - avail;
            for (int i = 0; i < n; i++) {
                ws[i] -= slack > 0 ? Math.min(1, over / slack) * (ws[i] - mins[i]) : 0;
            }
        }
        return ws;
    }

    @Override
    public void layout(Ui ui) {
        List<Widget> l = shown();
        float inner = w - 2 * pad, innerH = h - 2 * pad;
        float g = gap(ui);
        if (asRow(ui, w)) {
            float[] ws = widths(ui, l, inner);
            float used = 0, growSum = 0;
            for (int i = 0; i < l.size(); i++) {
                used += ws[i];
                growSum += l.get(i).grow;
            }
            used += Math.max(0, l.size() - 1) * g;
            float px = x + pad + (growSum > 0 ? 0 : Math.max(0, inner - used) * justify);
            for (int i = 0; i < l.size(); i++) {
                Widget k = l.get(i);
                k.setBounds(px, y + pad, ws[i], innerH);
                k.layout(ui);
                px += ws[i] + g;
            }
        } else if (horizontal) {
            float py = y + pad;
            for (List<Widget> line : lines(ui, l, inner)) {
                // each line shared out across the width (equal shares on top of the preferred widths)
                float[] ws = widths(ui, line, inner);
                float sum = (line.size() - 1) * g;
                for (float v : ws) {
                    sum += v;
                }
                float extra = Math.max(0, inner - sum) / line.size();
                float lh = 0;
                for (int i = 0; i < line.size(); i++) {
                    ws[i] += extra;
                    lh = Math.max(lh, line.get(i).prefHeight(ui, ws[i]));
                }
                float px = x + pad;
                for (int i = 0; i < line.size(); i++) {
                    Widget k = line.get(i);
                    k.setBounds(px, py, ws[i], lh);
                    k.layout(ui);
                    px += ws[i] + g;
                }
                py += lh + g;
            }
        } else {
            int n = l.size();
            float[] hs = new float[n];
            float sum = 0, growSum = 0;
            for (int i = 0; i < n; i++) {
                hs[i] = l.get(i).prefHeight(ui, inner);
                sum += hs[i];
                growSum += l.get(i).grow;
            }
            float avail = innerH - Math.max(0, n - 1) * g;
            if (growSum > 0 && avail > sum) {
                for (int i = 0; i < n; i++) {
                    hs[i] += (avail - sum) * l.get(i).grow / growSum;
                }
            }
            float py = y + pad;
            for (int i = 0; i < n; i++) {
                Widget k = l.get(i);
                k.setBounds(x + pad, py, inner, hs[i]);
                k.layout(ui);
                py += hs[i] + g;
            }
        }
    }
}
