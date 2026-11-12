package slopui;

import java.util.ArrayList;
import java.util.List;

/** A widget holding others: draws them in order, finds the topmost under the pointer. */
public abstract class Container extends Widget {
    protected final List<Widget> kids = new ArrayList<>();

    public Container add(Widget... ws) {
        for (Widget w : ws) {
            w.parent = this;
            kids.add(w);
        }
        return this;
    }

    public void remove(Widget w) {
        if (kids.remove(w)) {
            w.parent = null;
        }
    }

    public void clear() {
        for (Widget w : kids) {
            w.parent = null;
        }
        kids.clear();
    }

    public List<Widget> kids() {
        return kids;
    }

    @Override
    public void children(List<Widget> out) {
        out.addAll(kids);
    }

    @Override
    public Widget hit(float px, float py) {
        if (!visible || !contains(px, py)) {
            return null;
        }
        for (int i = kids.size() - 1; i >= 0; i--) {
            Widget k = kids.get(i);
            Widget h = k.visible ? k.hit(px, py) : null;
            if (h != null) {
                return h;
            }
        }
        return this;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        for (Widget k : kids) {
            if (k.visible) {
                k.draw(ui, p);
            }
        }
    }
}
