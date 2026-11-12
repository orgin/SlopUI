# Getting started: Java

## What you need

- Java 21 or newer.
- For the included backend: LWJGL 3.4 (`lwjgl`, `lwjgl-glfw`, `lwjgl-opengl`, `lwjgl-stb` and their natives).
- A TrueType font file for the text. SlopUI ships no fonts.

## Adding SlopUI to a project

The Java version is a Gradle build with three parts:

| Project | Package | Depends on | Use it for |
|---|---|---|---|
| `java/core` | `slopui` | nothing | The toolkit. Always needed. |
| `java/gl` | `slopui.gl` | core, LWJGL | The OpenGL/GLFW backend, if you don't write your own. |
| `java/demo` | `slopui.demo` | gl | The demo program only. |

**As a composite build** (source next to your project):

```groovy
// settings.gradle
includeBuild('../SlopUI/java')

// build.gradle
dependencies {
    implementation 'slopui:core'
    implementation 'slopui:gl'          // the LWJGL backend (optional)
}
```

**As jars:** run `./gradlew :core:jar :gl:jar` in `java/`. Copy `core/build/libs/core-<version>.jar` and
`gl/build/libs/gl-<version>.jar` into your project. The core jar contains the built-in themes as resources
(`slopui/themes/*.json`).

## A first window

```java
import slopui.*;
import slopui.gl.GlfwHost;
import slopui.gl.TtfFont;
import java.io.FileInputStream;

public class Hello {
    public static void main(String[] args) throws Exception {
        GlfwHost host = new GlfwHost("Hello SlopUI", 1280, 800);    // a window with an OpenGL 3.3 context

        Theme theme = new Theme();                                   // the built-in game look
        try (var in = new FileInputStream("fonts/MyFont.ttf")) {
            theme.font = new TtfFont(in, 10);                        // 10 UI units per em
        }

        Ui ui = new Ui(theme, host);
        host.setUi(ui);                                              // input from the window to the UI

        Label label = new Label("Hello! Buttons act when released.");
        Window[] w = new Window[1];
        w[0] = new Window("Hello", label).width(220);
        w[0].defaultButton(new Button("Close", () -> ui.close(w[0])));
        ui.open(w[0]);

        while (host.frame(null)) {
            // one frame per screen refresh: events, layout, drawing
        }
        host.close();
    }
}
```

`GlfwHost.frame(beforeDraw)` polls the window's events, sizes the UI, advances its timers and draws it.
`beforeDraw` (a `Runnable`, or `null`) runs just before drawing; that's the place to update widgets from your
game's state.

## Under your game

Usually the game draws its own picture first, and the UI draws on top. There are two ways to do it:

1. **The game as the background widget.** Make a `Widget` subclass that draws the game, and pass it to
   `ui.setBackground(w)`. It gets every click, wheel turn and key that the UI doesn't take, and it is laid out
   under the menu bar. The demo's `MockGame` works this way.
2. **Your own loop.** Draw the game with your renderer, then the UI: call `ui.setSize(...)`, `ui.frame(dt)` and
   `ui.draw(painter)` yourself, and feed the input to `ui.mouseMoved/mousePressed/mouseReleased/wheel/key/typed`.
   See [Backends](backends.md).

```java
class GameView extends Widget {
    @Override public float prefWidth(Ui ui) { return 320; }
    @Override public float prefHeight(Ui ui, float width) { return 200; }
    @Override public void draw(Ui ui, Painter p) {
        Shapes.fill(p, x, y, w, h, 0xFF000000);                     // your game's picture here
    }
    @Override public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        // a click the UI didn't take
        return true;                                                 // true: this widget gets the drags and the release
    }
}
ui.setBackground(new GameView());
```

## Menus and shortcuts

```java
Menu file = new Menu("File");
file.add(MenuItem.action("New Game", () -> startNewGame()));
file.add(MenuItem.action("Quit", host::requestClose).shortcut("Ctrl+Q"));
ui.menuBar.add(file);

ui.shortcut(Keys.F11, () -> host.setFullscreen(!host.fullscreen()));
```

## UI size and fullscreen

```java
host.setUiScale(0);          // Auto: one UI unit per 400th of the window's height (whole device pixels)
host.setUiScale(3);          // or a fixed 3 device pixels per UI unit
host.setFullscreen(true);    // the monitor's own mode, no decorations; the menu bar shows at the top edge
```

Next: [Concepts](concepts.md), [Widgets](widgets.md).
