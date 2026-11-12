# Examples

Complete small programs and the common patterns. The demos (`java/demo`, `cpp/demo/main.cpp`) build every widget,
and are the longest example.

## A settings window (Java)

```java
import slopui.*;
import java.util.List;

final class Settings {
    static Window create(Ui ui, Prefs prefs) {
        Dropdown size = new Dropdown(List.of("Auto", "1x", "2x", "3x", "4x"), prefs.uiSize);
        Slider music = new Slider(0, 100, 1, prefs.music).format(v -> (int) v + "%");
        Toggle full = new Toggle("Fullscreen", prefs.fullscreen);
        Grid form = new Grid(2);
        form.add(new Label("UI size"), size, new Label("Music"), music);

        Tabs tabs = new Tabs()
                .add("Display", Box.column(form, full).gap(5))
                .add("Sound", Box.column(new Toggle("Sound on", prefs.sound)))
                .sideWhenWider(330);

        Window w = new Window("Settings", tabs).width(400);
        w.button(new Button("Cancel", () -> ui.close(w)));
        w.defaultButton(new Button("OK", () -> {
            prefs.uiSize = size.selected();
            prefs.music = (int) music.value();
            prefs.fullscreen = full.isOn();
            prefs.save();
            ui.close(w);
            ui.toast("Settings saved");
        }));
        return w;
    }
}
// ui.open(Settings.create(ui, prefs));
```

## A save-game list (C++)

```cpp
#include "slopui/ui.hpp"
using namespace slopui;

std::shared_ptr<Window> loadGameWindow(Ui& ui, const std::vector<SaveInfo>& saves, std::function<void(int)> load) {
    auto table = make<Table>(std::vector<std::string>{"Name", "Saved"}, std::vector<float>{3, 2}, 8);
    std::vector<std::vector<std::string>> rows;
    for (auto& s : saves) rows.push_back({s.name, s.date});
    table->setRows(rows);

    auto self = std::make_shared<std::weak_ptr<Window>>();
    auto close = [&ui, self] { if (auto w = self->lock()) ui.close(w.get()); };
    std::weak_ptr<Table> tw = table;
    auto loadButton = make<Button>("Load", [tw, close, load] {
        if (auto t = tw.lock(); t && t->selected() >= 0) {
            int row = t->selected();
            close();
            load(row);
        }
    });
    std::weak_ptr<Button> lw = loadButton;
    table->onActivate([&ui, lw](int) { if (auto b = lw.lock()) b->click(ui); });   // double click / Enter

    auto w = make<Window>("Load Game", table);
    w->width(360).button(make<Button>("Cancel", close)).defaultButton(loadButton);
    *self = w;
    return w;
}
```

## The game under the UI, with a HUD menu (Java)

```java
final class Game extends Widget {
    private final Ui ui;
    Game(Ui ui) { this.ui = ui; }

    @Override public float prefWidth(Ui ui) { return 320; }
    @Override public float prefHeight(Ui ui, float w) { return 200; }

    @Override public void draw(Ui ui, Painter p) {
        Shapes.vgradient(p, x, y, w, h, 0xFF02040A, 0xFF0A1020);       // the game's picture
    }

    @Override public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        if (button == 1) {                                             // right button: a context menu
            Menu m = new Menu("Orders");
            m.add(MenuItem.action("Move here", () -> order(mx, my)));
            m.add(MenuItem.action("Hold", this::hold));
            ui.contextMenu(m);
            return true;
        }
        select(mx, my);                                                // left button: the game's own click
        return true;
    }

    @Override public boolean key(Ui ui, int key, int mods) {
        if (key == Keys.SPACE) { togglePause(); return true; }         // keys no window or field took
        return false;
    }
    // order, hold, select, togglePause: the game's own
}

// ui.setBackground(new Game(ui));
```

## A language-following UI

```java
Translations t = loadTranslations();   // your own
Button ok = new Button(() -> t.get("ok"), this::apply);
Menu file = new Menu(() -> t.get("menu.file"));
file.add(MenuItem.action(() -> t.get("menu.quit"), host::requestClose));
// switching the language: change t; the UI shows it on the next frame
```

## Keeping a progress bar in step

```java
ProgressBar bar = new ProgressBar(0);
bar.text(() -> Math.round(bar.value() * 100) + "%");
while (host.frame(() -> bar.set(loader.fraction()))) { }
```
```cpp
auto bar = make<ProgressBar>(0.0f);
while (host.frame([&] { bar->set(loader.fraction()); })) {}
```
