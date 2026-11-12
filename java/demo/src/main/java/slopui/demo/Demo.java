package slopui.demo;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Map;
import slopui.Font;
import slopui.Keys;
import slopui.Menu;
import slopui.MenuItem;
import slopui.Theme;
import slopui.Ui;
import slopui.gl.GlfwHost;
import slopui.gl.TtfFont;

/**
 * SlopUI's test program: the toolkit over a mockup of the game. The menus open every widget, a
 * Preferences mockup, the File dialogs; Display changes the window, the UI size and the theme.
 * <pre>java -jar slopui-demo-VERSION.jar [--font FILE] [--title-font FILE] [--ui-size 0-4] [--theme N]
 *     [--open gallery|prefs|disks|load|editors] [--fullscreen]</pre>
 * Fonts are not part of SlopUI: the demo uses the TrueType files given, else a system monospaced font
 * (DejaVu Sans Mono, Noto Mono, Consolas, Courier New).
 */
public final class Demo {
    private final GlfwHost host;
    private final Ui ui;
    private final List<Runnable> perFrame = new ArrayList<>();
    private final List<Theme> themes = new ArrayList<>();
    private int themeIndex;
    private final byte[] mono, bold;
    private final Map<String, TtfFont> fonts = new HashMap<>();
    private final MockGame game;
    private final DemoWindows windows;

    private Demo(Path font, Path title) throws IOException {
        host = new GlfwHost("SlopUI demo " + version(), 1280, 824);
        Path[] m = fontFiles(font, title);
        mono = Files.readAllBytes(m[0]);
        bold = Files.readAllBytes(m[1]);
        Theme game = new Theme();
        themes.add(game);
        themes.add(game.read(resource("/slopui/themes/dark.json")));
        Path dir = Path.of("themes");
        if (Files.isDirectory(dir)) {       // themes of one's own: ./themes/*.json
            try (var files = Files.list(dir)) {
                for (Path p : files.sorted().toList()) {
                    if (p.toString().endsWith(".json")) {
                        try {
                            Theme t = game.read(Files.readString(p));
                            if (themes.stream().noneMatch(x -> x.name.equals(t.name))) {
                                themes.add(t);
                            }
                        } catch (RuntimeException e) {
                            System.err.println(p + ": " + e.getMessage());
                        }
                    }
                }
            }
        }
        ui = new Ui(game, host);
        applyFonts();
        host.setUi(ui);
        this.game = new MockGame(this);
        ui.setBackground(this.game);
        windows = new DemoWindows(this, ui);
        menus();
        ui.shortcut(Keys.F11, () -> host.setFullscreen(!host.fullscreen()));
        ui.shortcut(Keys.F1, () -> ui.open(windows.gallery()));
    }

    /** The text and title fonts: the files given, else a system monospaced font (and its bold). */
    private static Path[] fontFiles(Path font, Path title) throws IOException {
        if (font != null) {
            return new Path[] {font, title != null ? title : font};
        }
        for (String[] sys : new String[][] {
                {"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"},
                {"/usr/share/fonts/truetype/noto/NotoMono-Regular.ttf", "/usr/share/fonts/truetype/noto/NotoMono-Regular.ttf"},
                {"C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/consolab.ttf"},
                {"/System/Library/Fonts/Supplemental/Courier New.ttf", "/System/Library/Fonts/Supplemental/Courier New Bold.ttf"}}) {
            if (Files.exists(Path.of(sys[0]))) {
                Path b = Path.of(sys[1]);
                return new Path[] {Path.of(sys[0]), title != null ? title : Files.exists(b) ? b : Path.of(sys[0])};
            }
        }
        throw new IOException("no font found: give a TrueType file with --font FILE");
    }

    /** A text font (or the title font) at size UI units per em; made once per size. */
    Font font(boolean isBold, float size) {
        return fonts.computeIfAbsent((isBold ? "b" : "m") + size, k -> {
            try {
                return new TtfFont(new java.io.ByteArrayInputStream(isBold ? bold : mono), size);
            } catch (IOException e) {
                throw new IllegalStateException(e);
            }
        });
    }

    private static String resource(String name) throws IOException {
        try (InputStream in = Demo.class.getResourceAsStream(name)) {
            return new String(in.readAllBytes(), StandardCharsets.UTF_8);
        }
    }

    static String version() {
        String v = Demo.class.getPackage().getImplementationVersion();
        return v == null ? "dev" : v;
    }

    static String buildTime() {
        try (InputStream in = Demo.class.getResourceAsStream("buildtime.txt")) {
            return in == null ? "" : new String(in.readAllBytes(), StandardCharsets.UTF_8).trim();
        } catch (IOException e) {
            return "";
        }
    }

    GlfwHost host() {
        return host;
    }

    void everyFrame(Runnable r) {
        perFrame.add(r);
    }

    List<String> themeNames() {
        List<String> l = new ArrayList<>();
        for (Theme t : themes) {
            l.add(t.name);
        }
        return l;
    }

    int themeIndex() {
        return themeIndex;
    }

    void setTheme(int i) {
        themeIndex = i;
        ui.theme = themes.get(i);
        applyFonts();
    }

    /** The mockup game's text: the text font at the game's 8 x 8 cell (1.11 cells per em). */
    Font gameFont(float scale) {
        return font(false, 8 * 1.11f * scale);
    }

    private void applyFonts() {
        for (Theme t : themes) {
            t.font = font(false, 10);
            t.titleFont = font(true, 10);
        }
    }

    private void menus() {
        Menu file = new Menu("File");
        file.add(MenuItem.action("New Game", () -> ui.confirm("New Game", "Start a new game? The game in progress is lost.",
                "New Game", "Cancel", () -> ui.toast("A new game"))));
        file.add(MenuItem.action("Load Game...", () -> ui.open(windows.loadGame())));
        file.add(MenuItem.action("Save Game...", () -> ui.ask("Save Game", "Name of the save:", "my game",
                s -> ui.toast("Saved as " + s))));
        file.add(MenuItem.action("Manage Save Disks...", () -> ui.open(windows.saveDisks())));
        file.separator();
        file.add(MenuItem.action("Quit", () -> ui.confirm("Quit", "Quit the demo?", "Quit", "Cancel", host::requestClose)));

        Menu display = new Menu("Display");
        for (int k = 1; k <= 4; k++) {
            int s = k;
            display.add(MenuItem.action(k + "x" + k + " pixels", () -> host.setSize(320 * s, 200 * s + 24)));
        }
        display.add(MenuItem.action("720p", () -> host.setSize(1280, 720)));
        display.add(MenuItem.action("1080p", () -> host.setSize(1920, 1080)));
        display.add(MenuItem.action("Fullscreen", () -> host.setFullscreen(!host.fullscreen()))
                .check(host::fullscreen).shortcut("F11"));
        display.separator();
        Menu size = new Menu("UI size");
        String[] sizes = {"Auto", "1x", "2x", "3x", "4x"};
        for (int i = 0; i < sizes.length; i++) {
            int s = i;
            size.add(MenuItem.action(sizes[i], () -> host.setUiScale(s)).radio(() -> host.uiScale() == s));
        }
        display.add(MenuItem.submenu(size));
        Menu theme = new Menu("Theme");
        theme.onOpen(() -> {
            theme.clear();
            for (int i = 0; i < themes.size(); i++) {
                int t = i;
                theme.add(MenuItem.action(themes.get(i).name, () -> setTheme(t)).radio(() -> themeIndex == t));
            }
        });
        display.add(MenuItem.submenu(theme));

        Menu widgets = new Menu("Widgets");
        widgets.add(MenuItem.action("Widget gallery...", () -> ui.open(windows.gallery())).shortcut("F1"));
        widgets.add(MenuItem.action("Preferences...", () -> ui.open(windows.preferences())));
        widgets.add(MenuItem.action("Text editors...", () -> ui.open(windows.editors())));
        slopui.Window[] fixed = new slopui.Window[1];
        widgets.add(MenuItem.action("Floating pane", () -> ui.open(windows.floating(true))));
        widgets.add(MenuItem.action("Fixed pane (no title bar)", () -> {
            if (fixed[0] != null && ui.isOpen(fixed[0])) {
                ui.close(fixed[0]);
            } else {
                fixed[0] = windows.floating(false);
                ui.open(fixed[0]);
            }
        }).check(() -> fixed[0] != null && ui.isOpen(fixed[0])));
        widgets.separator();
        Menu dialogs = new Menu("Dialogs");
        dialogs.add(MenuItem.action("Message", () -> ui.message("Message", "Something happened. This is a longer "
                + "message that wraps onto several lines when the window is narrow.", null)));
        dialogs.add(MenuItem.action("Question", () -> ui.confirm("Question", "Overwrite the save \"quick\"?", "Overwrite",
                "Cancel", () -> ui.toast("Overwritten"))));
        dialogs.add(MenuItem.action("Ask for a name", () -> ui.ask("Name", "Name of the station:", "Station 1",
                s -> ui.toast("Named " + s))));
        dialogs.add(MenuItem.action("Busy (progress)", () -> ui.open(windows.busy())));
        widgets.add(MenuItem.submenu(dialogs));
        widgets.add(MenuItem.action("Toast", () -> ui.toast("A toast: a short message that fades")));
        Menu screens = new Menu("Game screen");
        String[] names = {"Station View", "Star Map", "Production"};
        for (int i = 0; i < names.length; i++) {
            int s = i;
            screens.add(MenuItem.action(names[i], () -> game.show(s)).radio(() -> game.screen() == s));
        }
        widgets.add(MenuItem.submenu(screens));

        Menu about = new Menu("About");
        about.add(MenuItem.action("About...", () -> ui.message("About", "SlopUI demo " + version()
                + "\nBuilt " + buildTime() + "\n\nThe toolkit (core) draws with a Painter and has no game or GL code; "
                + "this demo runs it on OpenGL (gl) over a mockup of the game.", null)));
        ui.menuBar.add(file).add(display).add(widgets).add(about);
    }

    private void run() {
        while (host.frame(() -> {
            for (Runnable r : new ArrayList<>(perFrame)) {
                r.run();
            }
        })) {
            // a frame per refresh
        }
        host.close();
    }

    public static void main(String[] args) throws IOException {
        int uiSize = 0, theme = 0;
        Path font = null, title = null;
        String open = null;
        boolean full = false;
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("--ui-size") && i + 1 < args.length) {
                uiSize = Integer.parseInt(args[++i]);
            } else if (args[i].equals("--theme") && i + 1 < args.length) {
                theme = Integer.parseInt(args[++i]);
            } else if (args[i].equals("--fullscreen")) {
                full = true;
            } else if (args[i].equals("--open") && i + 1 < args.length) {
                open = args[++i];
            } else if (args[i].equals("--font") && i + 1 < args.length) {
                font = Path.of(args[++i]);
            } else if (args[i].equals("--title-font") && i + 1 < args.length) {
                title = Path.of(args[++i]);
            }
        }
        Demo d = new Demo(font, title);
        if (full) {
            d.host.setFullscreen(true);
        }
        d.host.setUiScale(uiSize);
        d.setTheme(Math.min(theme, d.themes.size() - 1));
        if (open != null) {
            switch (open) {
                case "gallery" -> d.ui.open(d.windows.gallery());
                case "prefs" -> d.ui.open(d.windows.preferences());
                case "disks" -> d.ui.open(d.windows.saveDisks());
                case "load" -> d.ui.open(d.windows.loadGame());
                case "editors" -> d.ui.open(d.windows.editors());
                default -> System.err.println("--open: gallery, prefs, disks, load or editors");
            }
        }
        d.run();
    }
}
