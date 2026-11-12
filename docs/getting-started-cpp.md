# Getting started: C++

## What you need

- A C++23 compiler (GCC 13+, Clang 17+, MSVC 19.38+) and CMake 3.25+.
- For the included backend: SDL3, and OpenGL 3.3 core or OpenGL ES 3.0. stb_truetype is included in
  `cpp/third_party`.
- A TrueType font file for the text. SlopUI ships no fonts.

## Adding SlopUI to a CMake project

```cmake
add_subdirectory(path/to/SlopUI/cpp slopui)
target_link_libraries(my_game PRIVATE slopui_sdl)   # the toolkit and the SDL3/OpenGL backend
# or only the toolkit, with your own backend:
target_link_libraries(my_game PRIVATE slopui)
```

| Target | Headers | What |
|---|---|---|
| `slopui` | `slopui/core.hpp`, `slopui/ui.hpp` | The toolkit: no SDL, no GL. |
| `slopui_sdl` | `slopui/sdl.hpp` | `SdlHost`, `GlPainter`, `TtfFont`. |
| `slopui_demo` | | The demo program. |

To build without SDL (only the toolkit), use `-DSLOPUI_SDL=OFF`.

## A first window

```cpp
#include "slopui/sdl.hpp"
using namespace slopui;

int main() {
    SdlHost host("Hello SlopUI", 1280, 800);                  // an SDL3 window with an OpenGL context
    Theme theme;                                               // the built-in game look
    theme.font = TtfFont::load("fonts/MyFont.ttf", 10);        // 10 UI units per em

    Ui ui(theme, &host);
    host.setUi(ui);

    auto self = std::make_shared<std::weak_ptr<Window>>();     // the window, for its own button
    auto w = make<Window>("Hello", make<Label>("Hello! Buttons act when released."));
    w->width(220).defaultButton(make<Button>("Close", [&ui, self] {
        if (auto win = self->lock()) ui.close(win.get());
    }));
    *self = w;
    ui.open(w);

    while (host.frame()) {
        // one frame per screen refresh: events, layout, drawing
    }
}
```

## Ownership

- Widgets are `std::shared_ptr`s. Make them with `make<T>(...)`. A container owns its children; the `Ui` owns
  its windows, popups and background.
- The `Ui` refers to widgets with `weak_ptr`s (hover, press, focus), so removing a widget is always safe.
- A window closed from inside one of its own buttons stays alive until the next frame, so the button can finish
  running.
- A button's action should not hold a `shared_ptr` to its own window: that would be a cycle, and the window
  would never be freed. Use a `weak_ptr`, as above.
- Text is UTF-8 in `std::string` at the API, and code points (`std::u32string`) inside.

## Under your game

The same two ways as in Java (see [Getting started: Java](getting-started-java.md#under-your-game)): make the game
a background widget with `ui.setBackground(...)`, or run the UI from your own loop with your own `Painter`.

```cpp
class GameView : public Widget {
public:
    float prefWidth(Ui&) override { return 320; }
    float prefHeight(Ui&, float) override { return 200; }
    void draw(Ui& ui, Painter& p) override { Shapes::fill(p, x, y, w, h, 0xFF000000); }
    bool press(Ui&, float, float, int button, int) override { return button == 0; }
};
ui.setBackground(make<GameView>());
```

## Menus and shortcuts

```cpp
auto file = std::make_shared<Menu>("File");
file->item("New Game", [] { startNewGame(); });
file->item("Quit", [&host] { host.requestClose(); }).shortcut("Ctrl+Q");
ui.menuBar->add(file);
ui.shortcut(Keys::F11, [&host] { host.setFullscreen(!host.fullscreen()); });
```

## Notes for AmigaOS 4 and OpenGL ES

`SdlHost` asks for an OpenGL 3.3 core context first, then OpenGL ES 3.0. The painter's shaders work with both.
SDL3's fullscreen uses the desktop's mode. On a system with screen modes, such as AmigaOS, open the system's
screen-mode requester yourself and create the window on the chosen screen; SlopUI only needs the GL context.

Next: [Concepts](concepts.md), [Widgets](widgets.md).
