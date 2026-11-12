# Concepts

## The Ui and its layers

A `Ui` holds everything SlopUI shows. It draws its layers bottom to top, and input goes to them top first:

| Layer | What | Notes |
|---|---|---|
| Background | Your game (`ui.setBackground(widget)`) | Laid out under the menu bar; gets the input nothing above takes. |
| Menu bar | `ui.menuBar` | Shown when it has menus. In fullscreen it is hidden until the pointer reaches the top edge. |
| Windows | `ui.open(window)` | In order of opening; a click raises a window. A modal window shades and blocks everything under it. |
| Popups | menus, dropdown lists, the calendar | A press outside them closes them, and that press does nothing else. |
| Toasts | `ui.toast("Saved")` | Short messages at the bottom that fade out. |
| Tooltip | `widget.tooltip("...")` | After the pointer rests for `tooltipDelay` seconds. |

## UI units and scaling

All sizes and positions are in **UI units**. The backend draws one UI unit as `ui.unit` device pixels. With the
included hosts, the unit is either:

- **Auto** (`setUiScale(0)`): one unit per 400th of the window's height, rounded to whole pixels, from 1 to 8.
  An 800-pixel-high window gives 2 pixels per unit, and a 1200-pixel one gives 3.
- **Fixed** (`setUiScale(n)`): n device pixels per unit, times the screen's pixel density.

The painter snaps every corner to device pixels, so lines stay sharp at every size. Fonts are drawn again for
each pixel size.

## The frame

Each frame, the host:

1. passes the window's events on to the Ui (`mouseMoved`, `mousePressed`, `mouseReleased`, `wheel`, `key`, `typed`);
2. sets the size with `ui.setSize(width, height, unit)`;
3. advances the timers with `ui.frame(seconds)`: animations, tooltips, held buttons;
4. draws with `ui.draw(painter)`, which lays everything out first.

`ui.animating` is true while something moves. `ui.needsRedraw()` tells a host that only draws on change whether
to draw this frame.

## Widgets

Every visible part is a `Widget`. It has:

- a rectangle (`x`, `y`, `w`, `h`), in absolute UI units, set by its parent;
- sizes: `prefWidth`, `minWidth` (it can work at this width; text wraps, tables shorten their text) and
  `prefHeight(width)`;
- `layout` to place its children, and `draw`;
- input methods: `press`, `drag`, `release`, `wheel`, `key`, `typed`, `tick` (each frame while it holds the press),
  `cursor` (the pointer's shape over it);
- state: visible, enabled (a disabled parent disables its children), `tooltip`, and `grow` (its share of spare
  room in a row or column).

### Writing your own widget

```java
class Meter extends Widget {
    float value = 0.5f;
    @Override public float prefWidth(Ui ui) { return 80; }
    @Override public float prefHeight(Ui ui, float width) { return ui.theme.s[Theme.ROW]; }
    @Override public void draw(Ui ui, Painter p) {
        Theme t = ui.theme;
        Skin.field(p, t, x, y, w, h, 0, isEnabled());
        Shapes.hgradient(p, x + 1, y + 1, (w - 2) * value, h - 2, t.c[Theme.FILL_BOTTOM], t.c[Theme.FILL_TOP]);
    }
    @Override public boolean press(Ui ui, float mx, float my, int button, int clicks) {
        value = (mx - x) / w;
        return true;                       // taken: drag() and release() come here
    }
    @Override public void drag(Ui ui, float mx, float my) { value = Math.max(0, Math.min(1, (mx - x) / w)); }
}
```

Use `animate(ui, slot, target, seconds)` for smooth hover and press effects. It returns a value moving towards the
target and keeps the UI drawing until it gets there. Each widget has four slots.

## Input routing

**Pointer:** the Ui finds the topmost widget under the pointer (the *hover*). On a press, it tries that widget and
then its parents in turn, until one's `press` returns true. That widget gets the drags and the release, even if the
pointer leaves it. Buttons act in `release`, and only when the pointer is still over them.

**Wheel:** the widget under the pointer and then its parents, until one takes it. Windows always take it, so the
game under a window doesn't scroll.

**Keys** are tried in this order:

1. the top popup (menus, lists, the calendar);
2. the focused widget and its parents;
3. Tab or Shift+Tab, which move the focus;
4. the top window: Enter presses the default button, Escape cancels;
5. the menu bar: F10, or Alt + a menu's first letter;
6. the shortcuts (`ui.shortcut(key, action)`);
7. the background (the game), unless a modal window is open.

While a text field has the focus, plain keys stop there, so typing never reaches the shortcuts or the game.
Keys with Ctrl or Alt still go through.

**Typed text** (`typed(codepoint)`) goes to the focused widget.

Key codes are GLFW's values (`Keys.ENTER = 257`...); the SDL host translates SDL's codes to them.

## Focus

A click focuses the nearest focusable widget under the pointer. Opening a window focuses its first text field.
Tab moves through the focusable widgets of the top window in tree order. Focused widgets show a thin accent
outline.

## Text

Labels and buttons take a `Supplier<String>` (Java) or `std::function<std::string()>` (C++) as well as plain
text. The text is read each time it's drawn, so changing the language changes the UI at once:

```java
new Button(() -> texts.get("OK"), this::apply);
```

## Java and C++

The two versions have the same classes, members and behaviour. The differences are:

| | Java | C++ |
|---|---|---|
| Widgets | `new Button(...)` | `make<Button>(...)` (`shared_ptr`) |
| Children | `Box.row(a, b)` | `Box::row({a, b})` |
| Callbacks | `Runnable`, `Consumer`, `IntConsumer` | `std::function` |
| Text | `String`, `Supplier<String>` | UTF-8 `std::string`, `Text` (= `std::function<std::string()>`) |
| Menus | `MenuItem.action(label, action)` | `menu->item(label, action)` |
| Delete key | `Keys.DELETE` | `Keys::DELETE_` (a macro on some systems is called `DELETE`) |
| Theme title font | `theme.titleFont` | `theme.titleFontOverride` |
