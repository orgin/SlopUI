package slopui.demo;

import java.util.ArrayList;
import java.util.List;
import slopui.Box;
import slopui.Button;
import slopui.CheckBox;
import slopui.ComboBox;
import slopui.DatePicker;
import slopui.Dropdown;
import slopui.Grid;
import slopui.GroupBox;
import slopui.ImageView;
import slopui.Label;
import slopui.NumberField;
import slopui.ProgressBar;
import slopui.RadioGroup;
import slopui.ScrollPane;
import slopui.Separator;
import slopui.Slider;
import slopui.SplitPane;
import slopui.Table;
import slopui.Tabs;
import slopui.TextArea;
import slopui.TextField;
import slopui.Toggle;
import slopui.Ui;
import slopui.Widget;
import slopui.Window;

/** The demo's windows: every widget, a Preferences mockup, the File dialogs, editors and panes. */
final class DemoWindows {
    private final Demo demo;
    private final Ui ui;

    DemoWindows(Demo demo, Ui ui) {
        this.demo = demo;
        this.ui = ui;
    }

    private static Label label(String s) {
        return new Label(s);
    }

    /** A form row pair for a Grid: label then widget. */
    private static Widget[] row(String l, Widget w) {
        return new Widget[] {new Label(l), w};
    }

    private static Grid form(Widget[]... rows) {
        Grid g = new Grid(2);
        for (Widget[] r : rows) {
            g.add(r);
        }
        return g;
    }

    // Widget gallery

    Window gallery() {
        Tabs tabs = new Tabs();
        tabs.add("Buttons", buttonsPage());
        tabs.add("Inputs", inputsPage());
        tabs.add("Progress", progressPage());
        tabs.add("Table", tablePage());
        tabs.add("Text", textPage());
        tabs.add("Panes", panesPage());
        Window[] w = new Window[1];
        w[0] = new Window("Widget gallery", tabs).width(440).modal(false).resizable(true);
        w[0].defaultButton(new Button("Close", () -> ui.close(w[0])));
        return w[0];
    }

    private Widget buttonsPage() {
        Button disabled = new Button("Disabled", () -> { });
        disabled.setEnabled(false);
        Button tip = new Button("With a tooltip", () -> ui.toast("Clicked"));
        tip.tooltip("Buttons act on the release; move off before releasing to cancel.");
        Box buttons = Box.row(new Button("Normal", () -> ui.toast("Normal clicked")),
                new Button("Default", () -> ui.toast("Default clicked")).accent(true), tip, disabled).responsive();
        CheckBox c1 = new CheckBox("Check box", true);
        CheckBox c2 = new CheckBox("Another one", false);
        CheckBox c3 = new CheckBox("Disabled", true);
        c3.setEnabled(false);
        Toggle t1 = new Toggle("Toggle (3D effects)", true);
        Toggle t2 = new Toggle("Toggle (sound)", false);
        Toggle t3 = new Toggle("Disabled", false);
        t3.setEnabled(false);
        RadioGroup g = new RadioGroup(0).onChange(i -> ui.toast("Radio " + (i + 1)));
        Box radios = Box.row(g.button("First"), g.button("Second"), g.button("Third")).responsive();
        return Box.column(
                new GroupBox("Buttons", buttons),
                Box.row(new GroupBox("Check boxes", Box.column(c1, c2, c3)).grow(1),
                        new GroupBox("Toggles", Box.column(t1, t2, t3)).grow(1)).responsive(),
                new GroupBox("Radio buttons", radios),
                new Separator(),
                label("Separators fade at their ends. Tab moves the focus; Space and Enter press.").colour(0)).gap(6);
    }

    private Widget inputsPage() {
        TextField name = new TextField("Station One").placeholder("Type a name");
        TextField empty = new TextField("").placeholder("Placeholder text");
        NumberField num = new NumberField(25, 1, 100, 1, 0);
        NumberField dec = new NumberField(0.5, 0.1, 2.0, 0.1, 1);
        Dropdown dd = new Dropdown(List.of("Auto", "1x", "2x", "4x", "8x"), 0).onChange(i -> ui.toast("Scale " + i));
        List<String> saves = List.of("Alpha Centauri run", "Barnard outpost", "Before the war", "Cygni rigs",
                "Earth only", "Last station", "Proxima trading", "Sirius battle", "Tau Ceti mining");
        ComboBox combo = new ComboBox("", saves);
        combo.placeholder("Type to filter...");
        int[] t = DatePicker.today();
        DatePicker date = new DatePicker(t[0], t[1], t[2]).onDate(d -> { });
        Slider vol = new Slider(0, 100, 1, 70).format(v -> (int) v + "%");
        Slider hold = new Slider(0.1, 0.5, 0.05, 0.5).format(v -> String.format("%.2f s", v));
        return form(
                row("Text field", name),
                row("Empty", empty),
                row("Number", num),
                row("Decimal", dec),
                row("Dropdown", dd),
                row("Combobox", combo),
                row("Date picker", date),
                row("Volume", vol),
                row("Hold delay", hold));
    }

    private Widget progressPage() {
        ProgressBar p1 = new ProgressBar(0);
        ProgressBar p2 = new ProgressBar(0.3f);
        ProgressBar busy = new ProgressBar(-1);
        p1.text(() -> Math.round(p1.value() * 100) + "%");
        demo.everyFrame(() -> p1.set((float) ((ui.time * 0.15) % 1.0)));
        Button more = new Button("+10%", () -> p2.set(Math.min(1, p2.value() + 0.1f)));
        Button less = new Button("-10%", () -> p2.set(Math.max(0, p2.value() - 0.1f)));
        return Box.column(
                label("Running:"), p1,
                label("Stepped (animates to the value):"), p2, Box.row(less, more),
                label("Busy (no amount known):"), busy).gap(5);
    }

    private Widget tablePage() {
        Table table = new Table(new String[] {"Name", "Date", "Stations", "Credits"}, new float[] {3, 2, 1.2f, 1.6f}, 8);
        List<String[]> rows = new ArrayList<>();
        String[] names = {"Alpha Centauri run", "Barnard outpost", "Before the war", "Cygni rigs", "Earth only",
                "Last station", "Proxima trading", "Sirius battle", "Tau Ceti mining", "The long one",
                "Tutorial", "With a much too long name that will not fit"};
        for (int i = 0; i < names.length; i++) {
            rows.add(new String[] {names[i], String.format("21%02d-%02d-%02d", 10 + i * 3, 1 + i % 12, 1 + i * 2 % 28),
                    String.valueOf(i % 7 + 1), String.valueOf(1000 + i * 7919 % 90000)});
        }
        table.setRows(rows);
        table.onActivate(r -> ui.toast("Opened: " + table.row(r)[0]));
        return Box.column(table, label("Click a header to sort (again: reversed). Double click or Enter opens a row.")).gap(5);
    }

    private Widget textPage() {
        String lorem = "Long text wraps at the window's width. Make the window narrower (or the UI larger) and the "
                + "lines break again; rows of buttons stack into columns when they no longer fit, and tabs wrap "
                + "onto more lines.\n\nA new paragraph after an empty line. Accents: åäö éèê ü ß ç ÅÄÖ.";
        return Box.column(new Label("A title label").title(), new Label(lorem),
                new GroupBox("Group box", new Label("A titled frame around related settings."))).gap(6);
    }

    private Widget panesPage() {
        Table list = new Table(new String[] {"Item", "Qty"}, new float[] {3, 1}, 6);
        List<String[]> rows = new ArrayList<>();
        for (String s : new String[] {"Mining rig", "Shuttle", "Factory", "Drone", "Probe", "Panel", "Pod", "Laser"}) {
            rows.add(new String[] {s, String.valueOf(s.length() * 3)});
        }
        list.setRows(rows);
        Box lots = Box.column();
        for (int i = 1; i <= 20; i++) {
            lots.add(new Label("Scrolled line " + i));
        }
        ScrollPane withBars = new ScrollPane(lots, true).viewHeight(60);
        Box lots2 = Box.column();
        for (int i = 1; i <= 20; i++) {
            lots2.add(new Label("No bars: wheel or middle-drag " + i));
        }
        ScrollPane noBars = new ScrollPane(lots2, false).viewHeight(60);
        SplitPane right = new SplitPane(withBars, noBars, true, 0.5f);
        SplitPane split = new SplitPane(list, right, false, 0.45f);
        return Box.column(label("Drag the separators (double click: back)."), split).gap(4);
    }

    // Preferences mockup

    Window preferences() {
        Dropdown size = new Dropdown(List.of("Auto", "1x", "2x", "3x", "4x"), demo.host().uiScale())
                .onChange(i -> demo.host().setUiScale(i));
        Dropdown theme = new Dropdown(demo.themeNames(), demo.themeIndex()).onChange(demo::setTheme);
        Toggle full = new Toggle("Fullscreen", demo.host().fullscreen()).onChange(on -> demo.host().setFullscreen(on));
        CheckBox threeD = new CheckBox("3D effects", true);
        RadioGroup gs = new RadioGroup(0);
        Box scale = Box.row(gs.button("Auto"), gs.button("1x"), gs.button("2x"), gs.button("4x"), gs.button("8x")).responsive();
        Widget display = Box.column(form(row("UI size", size), row("Theme", theme)), full, threeD,
                new GroupBox("Graphics scale", scale)).gap(5);
        Slider hold = new Slider(0.1, 0.5, 0.05, 0.5).format(v -> String.format("%.2f s", v));
        Slider scroll = new Slider(1, 10, 1, 3).format(v -> String.valueOf((int) v));
        Widget controls = Box.column(form(row("Hold delay", hold), row("Scroll speed", scroll),
                row("Double click", new NumberField(400, 100, 900, 50, 0))),
                new Label("Hold delay: how long a held button waits before it repeats.").colour(0)).gap(5);
        Slider music = new Slider(0, 100, 1, 80).format(v -> (int) v + "%");
        Slider fx = new Slider(0, 100, 1, 100).format(v -> (int) v + "%");
        Widget sound = Box.column(form(row("Music", music), row("Effects", fx)), new Toggle("Sound on", true),
                new Toggle("Alarm while away", false)).gap(5);
        RadioGroup lang = new RadioGroup(0);
        Widget language = Box.column(lang.button("English"), lang.button("Français"), lang.button("Deutsch"),
                lang.button("Svenska")).gap(2);
        Tabs tabs = new Tabs().add("Display", display).add("Controls", controls).add("Sound", sound).add("Language", language)
                .sideWhenWider(330);
        Window[] w = new Window[1];
        w[0] = new Window("Preferences", tabs).width(420);
        w[0].button(new Button("Cancel", () -> ui.close(w[0])));
        w[0].button(new Button("Apply", () -> ui.toast("Applied")));
        w[0].defaultButton(new Button("OK", () -> ui.close(w[0])));
        return w[0];
    }

    // File dialogs (mockups)

    Window loadGame() {
        Table t = new Table(new String[] {"Name", "Saved", "Game date"}, new float[] {3, 2.2f, 1.6f}, 8);
        List<String[]> rows = new ArrayList<>();
        String[] n = {"autosave", "before war", "checkpoint", "my game", "mining", "quick", "the long game"};
        for (int i = 0; i < n.length; i++) {
            rows.add(new String[] {n[i], String.format("2026-11-%02d %02d:%02d", 1 + i * 2, 8 + i, i * 7 % 60),
                    String.format("21%02d-%02d-01", 30 + i * 4, 1 + i)});
        }
        t.setRows(rows);
        Window[] w = new Window[1];
        Button load = new Button("Load", () -> {
            if (t.selected() >= 0) {
                ui.close(w[0]);
                ui.toast("Loaded " + t.row(t.selected())[0]);
            }
        });
        t.onActivate(r -> load.click(ui));
        w[0] = new Window("Load Game", t).width(380);
        w[0].button(new Button("Delete", () -> {
            if (t.selected() >= 0) {
                ui.confirm("Delete", "Delete \"" + t.row(t.selected())[0] + "\"?", "Delete", "Keep",
                        () -> ui.toast("Deleted"));
            }
        }));
        w[0].button(new Button("Cancel", () -> ui.close(w[0])));
        w[0].defaultButton(load);
        return w[0];
    }

    Window saveDisks() {
        Table t = new Table(new String[] {"Disk", "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5"},
                new float[] {1.4f, 2, 2, 2, 2, 2}, 6);
        List<String[]> rows = new ArrayList<>();
        for (int d = 1; d <= 9; d++) {
            rows.add(new String[] {"Save disk " + d, "Game " + d + "A", d % 2 == 0 ? "Empty" : "Game " + d + "B",
                    "Empty", d == 3 ? "War" : "Empty", "Empty"});
        }
        t.setRows(rows);
        Window[] w = new Window[1];
        w[0] = new Window("Manage Save Disks", Box.column(t,
                new Label("A wide window: it never leaves the screen; narrow it and the table truncates, the buttons stack."))
                .gap(4)).width(640);
        w[0].button(new Button("New disk", () -> ui.toast("New disk")));
        w[0].button(new Button("Rename", () -> ui.ask("Rename", "Name of the disk:", "Save disk", s -> ui.toast("Renamed: " + s))));
        w[0].button(new Button("Delete", () -> ui.toast("Delete")));
        w[0].button(new Button("Play", () -> ui.toast("Play")));
        w[0].defaultButton(new Button("Close", () -> ui.close(w[0])));
        return w[0];
    }

    Window editors() {
        TextArea area = new TextArea("Several lines of text.\nEnter makes a new line, Tab moves on.\n"
                + "Ctrl+A, Ctrl+C, Ctrl+X, Ctrl+V; Ctrl+arrows by words;\nHome/End per line, Ctrl+Home/End the text.\n"
                + "Shift+arrows select.\n\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11\nLine 12", 8);
        TextField single = new TextField("One line").placeholder("...");
        Window[] w = new Window[1];
        w[0] = new Window("Text editors", Box.column(new Label("Single line"), single, new Label("Multiline"), area).gap(4))
                .width(360).modal(false).resizable(true);
        w[0].defaultButton(new Button("Close", () -> ui.close(w[0])));
        return w[0];
    }

    /** A floating pane with a title bar to drag it by, and one without. */
    Window floating(boolean titleBar) {
        int[] px = new int[24 * 16];
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 24; x++) {
                boolean edge = x == 0 || y == 0 || x == 23 || y == 15;
                px[y * 24 + x] = edge ? 0xFFB8C890 : ((x / 4 + y / 4) % 2 == 0 ? 0xFF3A4A2A : 0xFF1A2214);
            }
        }
        Window w = new Window(titleBar ? "Floating pane" : "", Box.column(
                new Label(titleBar ? "Drag me by the title bar. Not modal: the game still takes clicks."
                        : "A fixed pane without a title bar (closed from the Widgets menu)."),
                new ImageView(24, 16, px, 72, 48, false))).width(200).modal(false);
        if (!titleBar) {
            w.titleBar(false);
        }
        return w;
    }

    Window busy() {
        ProgressBar bar = new ProgressBar(0);
        bar.text(() -> Math.round(bar.value() * 100) + "%");
        Window[] w = new Window[1];
        double start = ui.time;
        demo.everyFrame(() -> {
            if (ui.isOpen(w[0])) {
                float v = (float) Math.min(1, (ui.time - start) / 4);
                bar.set(v);
                if (v >= 1) {
                    ui.close(w[0]);
                    ui.toast("Extraction done");
                }
            }
        });
        w[0] = new Window("Extracting", Box.column(new Label("Extracting the game's pictures..."), bar)).width(240).closable(false);
        w[0].button(new Button("Cancel", () -> ui.close(w[0])));
        return w[0];
    }
}
