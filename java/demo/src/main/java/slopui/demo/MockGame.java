package slopui.demo;

import java.util.HashMap;
import java.util.Map;
import java.util.Random;
import slopui.Font;
import slopui.Keys;
import slopui.Menu;
import slopui.MenuItem;
import slopui.Painter;
import slopui.Shapes;
import slopui.Ui;
import slopui.Widget;

/**
 * A mockup of the game under the UI: a 320 x 200 screen (fitted, 16:10) with the game's layout (icon column,
 * hover line, status boxes) and three made-up screens (station view, star map, production). Its icons take
 * clicks (the input the UI leaves to the game), the right button opens a context menu.
 */
final class MockGame extends Widget {
    private final Demo demo;
    private int screen;
    private int flash = -1;
    private double flashAt;
    private final float[][] stars = new float[160][];
    private final Map<Float, Font> fonts = new HashMap<>();
    private String hoverText = "";
    private static final String[] ICONS = {"Station View", "Star Map", "Production", "Research", "Training",
            "Shuttle Bay", "Resource", "Mining Store", "Stocktaker", "Disk Access", "News", "Earth City"};
    private static final String[] SCREENS = {"Station View", "Star Map", "Production"};

    MockGame(Demo demo) {
        this.demo = demo;
        Random r = new Random(1977);
        for (int i = 0; i < stars.length; i++) {
            stars[i] = new float[] {r.nextFloat() * 256, r.nextFloat() * 168, r.nextFloat(), r.nextFloat() * 6.28f};
        }
    }

    void show(int s) {
        screen = s;
    }

    int screen() {
        return screen;
    }

    @Override
    public float prefWidth(Ui ui) {
        return 320;
    }

    @Override
    public float prefHeight(Ui ui, float width) {
        return 200;
    }

    /** The game screen's place: x, y, one game pixel in UI units. */
    private float[] frame() {
        float gp = Math.min(w / 320f, h / 200f);
        return new float[] {x + (w - 320 * gp) / 2, y + (h - 200 * gp) / 2, gp};
    }

    private Font gameFont(Ui ui, float gp) {
        float s = Math.max(0.25f, Math.round(gp * 4) / 4f);
        return fonts.computeIfAbsent(s, k -> demo.gameFont(k));
    }

    private int iconAt(float mx, float my) {
        float[] f = frame();
        float gx = (mx - f[0]) / f[2], gy = (my - f[1]) / f[2];
        if (gx < 0 || gx >= 48 || gy < 88 || gy >= 184) {
            return -1;
        }
        int i = (int) ((gy - 88) / 16) * 2 + (int) (gx / 24);
        return i < ICONS.length ? i : -1;
    }

    @Override
    public void draw(Ui ui, Painter p) {
        Shapes.fill(p, x, y, w, h, 0xFF000000);
        float[] f = frame();
        float ox = f[0], oy = f[1], g = f[2];
        Font font = gameFont(ui, g);
        int hot = ui.isHover(this) ? iconAt(ui.mx, ui.my) : -1;
        hoverText = hot >= 0 ? ICONS[hot] : "";
        // top line
        Shapes.fill(p, ox, oy, 320 * g, 16 * g, 0xFF000000);
        font.draw(p, hoverText.isEmpty() ? "MOCKUP - SLOPUI DEMO" : hoverText.toUpperCase(), ox + 2 * g, oy + 4 * g, 0xFFFFFFFF);
        float[] sun = Shapes.cut(ox + 232 * g, oy + 1 * g, 86 * g, 14 * g, 2 * g, 2 * g, 2 * g, 2 * g);
        Shapes.poly(p, sun, 0xFF2A3420, 0xFF141A10, oy, 16 * g);
        Shapes.outline(p, sun, g, 0xFF8A9A70);
        font.draw(p, "THE SUN", ox + 248 * g, oy + 4 * g, 0xFFFFF060);
        // left column: logo area and icons
        Shapes.vgradient(p, ox, oy + 16 * g, 48 * g, 72 * g, 0xFF101810, 0xFF000000);
        Shapes.fill(p, ox + 4 * g, oy + 22 * g, 30 * g, 8 * g, 0xFF2040A0);
        for (int i = 0; i < ICONS.length; i++) {
            float ix = ox + (i % 2) * 24 * g, iy = oy + (88 + i / 2 * 16) * g;
            boolean lit = i == flash && ui.time - flashAt < 0.25;
            if (lit) {
                ui.animating = true;
            }
            float[] r = Shapes.cut(ix + g, iy + g, 22 * g, 14 * g, 0, 0, 0, 0);
            Shapes.poly(p, r, lit ? 0xFF8AA060 : i == hot ? 0xFF3A4A2A : 0xFF1E2618, lit ? 0xFF4A6030 : 0xFF0A0E08, iy, 16 * g);
            Shapes.outline(p, r, Math.max(1, g * 0.6f), i == screen ? 0xFFE8D860 : 0xFFB0B0A0);
            icon(p, i, ix + 12 * g, iy + 8 * g, g);
        }
        // main area
        float mx = ox + 48 * g, my = oy + 16 * g, mw = 272 * g, mh = 168 * g;
        p.pushClip(mx, my, mw, mh);
        switch (screen) {
            case 0 -> stationView(ui, p, mx, my, g, font);
            case 1 -> starMap(ui, p, mx, my, g, font);
            default -> production(ui, p, mx, my, g, font);
        }
        p.popClip();
        // status boxes
        float sy = oy + 184 * g;
        float[] b1 = Shapes.cut(ox + 48 * g, sy + g, 112 * g, 14 * g, 3 * g, 3 * g, 3 * g, 3 * g);
        float[] b2 = Shapes.cut(ox + 162 * g, sy + g, 156 * g, 14 * g, 3 * g, 3 * g, 3 * g, 3 * g);
        for (float[] b : new float[][] {b1, b2}) {
            Shapes.poly(p, b, 0xFF242C1C, 0xFF0C100A, sy, 16 * g);
            Shapes.outline(p, b, g, 0xFF8A9A70);
        }
        font.draw(p, "3100 580.00", ox + 60 * g, sy + 5 * g, 0xFFFFFFFF);
        font.draw(p, SCREENS[screen].toUpperCase(), ox + 172 * g, sy + 5 * g, 0xFFFFFFFF);
    }

    private void icon(Painter p, int i, float cx, float cy, float g) {
        int c = 0xFFD8D8C8;
        switch (i % 4) {
            case 0 -> {
                float[] d = {cx - 5 * g, cy, cx, cy - 4 * g, cx + 5 * g, cy, cx, cy + 4 * g};
                Shapes.outline(p, d, g, c);
            }
            case 1 -> {
                Shapes.fill(p, cx - 5 * g, cy - 3 * g, 10 * g, g, c);
                Shapes.fill(p, cx - 5 * g, cy, 10 * g, g, c);
                Shapes.fill(p, cx - 5 * g, cy + 3 * g, 10 * g, g, c);
            }
            case 2 -> {
                Shapes.line(p, cx - 5 * g, cy + 4 * g, cx, cy - 4 * g, g, c);
                Shapes.line(p, cx, cy - 4 * g, cx + 5 * g, cy + 4 * g, g, c);
            }
            default -> Shapes.outline(p, Shapes.cut(cx - 5 * g, cy - 4 * g, 10 * g, 8 * g, 2 * g, 0, 2 * g, 0), g, c);
        }
    }

    private void stationView(Ui ui, Painter p, float mx, float my, float g, Font font) {
        Shapes.vgradient(p, mx, my, 272 * g, 168 * g, 0xFF020408, 0xFF0A1020);
        for (float[] s : stars) {
            float tw = (float) (0.55 + 0.45 * Math.sin(ui.time * (0.6 + s[2]) + s[3]));
            int a = (int) (255 * (0.3 + 0.7 * s[2]) * tw);
            float sz = s[2] > 0.85f ? 1.5f * g : g;
            Shapes.fill(p, mx + s[0] * g, my + s[1] * g, sz, sz, a << 24 | 0xFFFFF0);
        }
        ui.animating = true;
        // a planet: a fan of triangles with a lit edge
        float pcx = mx + 196 * g, pcy = my + 120 * g, pr = 54 * g;
        int n = 48;
        float[] pts = new float[n * 2];
        for (int i = 0; i < n; i++) {
            double a = i * Math.PI * 2 / n;
            pts[i * 2] = pcx + (float) Math.cos(a) * pr;
            pts[i * 2 + 1] = pcy + (float) Math.sin(a) * pr;
        }
        Shapes.glow(p, pts, 8 * g, 0x502A60C0);
        Shapes.poly(p, pts, 0xFF4A78B0, 0xFF0A1830, pcy - pr, 2 * pr);
        // the station
        float sx = mx + 60 * g + (float) Math.sin(ui.time * 0.3) * 3 * g, sy = my + 70 * g;
        Shapes.fill(p, sx, sy, 90 * g, 6 * g, 0xFF8A8A80);
        Shapes.fill(p, sx + 30 * g, sy - 14 * g, 30 * g, 34 * g, 0xFF6A6A60);
        Shapes.fill(p, sx + 36 * g, sy - 10 * g, 18 * g, 4 * g, 0xFFE8D860);
        Shapes.fill(p, sx - 20 * g, sy - 2 * g, 20 * g, 10 * g, 0xFF4A5A70);
        Shapes.fill(p, sx + 90 * g, sy - 2 * g, 20 * g, 10 * g, 0xFF4A5A70);
        font.draw(p, "RIGHT-CLICK: CONTEXT MENU", mx + 6 * g, my + 156 * g, 0x90FFFFFF);
    }

    private void starMap(Ui ui, Painter p, float mx, float my, float g, Font font) {
        Shapes.fill(p, mx, my, 272 * g, 168 * g, 0xFF04060A);
        for (int i = 0; i <= 272; i += 34) {
            Shapes.fill(p, mx + i * g, my, Math.max(1, g / 2), 168 * g, 0x302A6A3A);
        }
        for (int i = 0; i <= 168; i += 28) {
            Shapes.fill(p, mx, my + i * g, 272 * g, Math.max(1, g / 2), 0x302A6A3A);
        }
        String[] names = {"SOL", "PROXIMA", "BARNARD", "SIRIUS", "LALANDE", "TAU CETI", "PROCYON", "CYGNI"};
        Random r = new Random(7);
        for (String nm : names) {
            float sx = mx + (20 + r.nextFloat() * 230) * g, sy = my + (14 + r.nextFloat() * 140) * g;
            float pulse = (float) (0.6 + 0.4 * Math.sin(ui.time * 2 + sx));
            float[] d = {sx, sy - 3 * g, sx + 3 * g, sy, sx, sy + 3 * g, sx - 3 * g, sy};
            Shapes.glow(p, d, 5 * g * pulse, 0x80FFE080);
            Shapes.poly(p, d, 0xFFFFF0C0, 0xFFE0A040, sy - 3 * g, 6 * g);
            font.draw(p, nm, sx + 5 * g, sy - 3 * g, 0xFFB8C890);
        }
        ui.animating = true;
    }

    private void production(Ui ui, Painter p, float mx, float my, float g, Font font) {
        Shapes.vgradient(p, mx, my, 272 * g, 168 * g, 0xFF181410, 0xFF080604);
        String[] items = {"MINING RIG", "SHUTTLE", "ORBITAL FACTORY", "DRONE", "PROBE", "SOLAR PANEL", "CARGO POD",
                "MARINES", "LASER", "TORPEDO"};
        for (int i = 0; i < items.length; i++) {
            float ry = my + (8 + i * 15) * g;
            float[] row = Shapes.cut(mx + 8 * g, ry, 256 * g, 13 * g, 3 * g, 0, 3 * g, 0);
            Shapes.poly(p, row, i % 2 == 0 ? 0xFF2A2418 : 0xFF221E14, 0xFF100C08, ry, 13 * g);
            Shapes.outline(p, row, Math.max(1, g / 2), 0xFF5A5040);
            font.draw(p, items[i], mx + 14 * g, ry + 3 * g, 0xFFE0D8C0);
            font.draw(p, String.valueOf((i * 37 + 11) % 99), mx + 220 * g, ry + 3 * g, 0xFFE8D860);
        }
    }

    @Override
    public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button == 1) {
            Menu m = new Menu("Context");
            m.add(MenuItem.action("Station View", () -> show(0)), MenuItem.action("Star Map", () -> show(1)),
                    MenuItem.action("Production", () -> show(2)));
            m.separator();
            m.add(MenuItem.action("Toast here", () -> ui.toast("A context menu item ran")));
            ui.contextMenu(m);
            return true;
        }
        int i = iconAt(mx, my);
        if (button == 0 && i >= 0) {
            flash = i;
            flashAt = ui.time;
            return true;
        }
        return false;
    }

    @Override
    public void release(Ui ui, float mx, float my, int button) {
        int i = iconAt(mx, my);
        if (button == 0 && i >= 0 && i == flash) {
            if (i < SCREENS.length) {
                show(i);
            } else {
                ui.toast("The game got a click: " + ICONS[i]);
            }
        }
    }

    @Override
    public boolean key(Ui ui, int key, int mods) {
        if (key >= Keys.A && key <= Keys.Z && mods == 0) {
            ui.toast("The game got the key " + (char) key);
            return true;
        }
        return false;
    }
}
