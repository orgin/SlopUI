package slopui;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Supplier;

/** Text: wraps to its width at spaces (and at newlines); aligned left, centred or right. */
public class Label extends Widget {
    private Supplier<String> text;
    private int colour;
    private float align;
    private boolean title;
    private boolean warning;

    public Label(Supplier<String> text) {
        this.text = text;
    }

    public Label(String text) {
        this(() -> text);
    }

    public Label set(String t) {
        text = () -> t;
        return this;
    }

    /** 0: the theme's text colour. */
    public Label colour(int argb) {
        colour = argb;
        return this;
    }

    public Label align(float a) {
        align = a;
        return this;
    }

    /** In the theme's warning colour. */
    public Label warning() {
        warning = true;
        return this;
    }

    /** In the title font and bright. */
    public Label title() {
        title = true;
        return this;
    }

    private Font font(Ui ui) {
        return title ? ui.theme.titleFont() : ui.theme.font;
    }

    @Override
    public float prefWidth(Ui ui) {
        float m = 0;
        for (String line : text.get().split("\n", -1)) {
            m = Math.max(m, font(ui).width(line));
        }
        return m;
    }

    @Override
    public float minWidth(Ui ui) {
        float m = 0;
        Font f = font(ui);
        for (String word : text.get().split("[ \n]")) {
            m = Math.max(m, f.width(word));
        }
        return Math.min(prefWidth(ui), Math.max(m, 40));
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return lines(font(ui), width).size() * font(ui).lineHeight() * 1.25f;
    }

    /** The text broken into lines that fit the width. */
    List<String> lines(Font f, float width) {
        List<String> out = new ArrayList<>();
        for (String para : text.get().split("\n", -1)) {
            String[] words = para.split(" ", -1);
            StringBuilder line = new StringBuilder();
            for (String word : words) {
                String tryLine = line.length() == 0 ? word : line + " " + word;
                if (line.length() > 0 && f.width(tryLine) > width) {
                    out.add(line.toString());
                    line = new StringBuilder(word);
                } else {
                    line = new StringBuilder(tryLine);
                }
            }
            out.add(line.toString());
        }
        return out;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        Font f = font(ui);
        int c = colour != 0 ? colour : warning ? t.c[Theme.WARNING] : !isEnabled() ? t.c[Theme.TEXT_DISABLED] : title ? t.c[Theme.TEXT_BRIGHT] : t.c[Theme.TEXT];
        List<String> ls = lines(f, w);
        float lh = f.lineHeight() * 1.25f;
        float py = y + Math.max(0, (h - ls.size() * lh) / 2) + (lh - f.lineHeight()) / 2;
        for (String l : ls) {
            f.draw(p, l, x + (w - f.width(l)) * align, py, c);
            py += lh;
        }
    }
}
