# Backends

The toolkit has no renderer and no window of its own. It needs two things from its surroundings:

- a **`Painter`**, to draw triangles;
- a **`Host`**, for the clipboard and the pointer's shape.

Both versions include an OpenGL backend:

| | Java (`java/gl`) | C++ (`cpp/backend`) |
|---|---|---|
| Window and input | `GlfwHost` (LWJGL GLFW) | `SdlHost` (SDL3) |
| Painter | `GlPainter`: OpenGL 3.3 core | `GlPainter`: OpenGL 3.3 core or ES 3.0 |
| Fonts | `TtfFont` (LWJGL stb_truetype) | `TtfFont` (stb_truetype) |

## The Painter contract

```java
public interface Painter {
    float unit();                                    // device pixels per UI unit
    void triangles(Image image, float[] xy, float[] uv, int[] argb, int n);
    void pushClip(float x, float y, float w, float h);
    void popClip();
    void setOpacity(float a);
    float opacity();
    Image createImage(int w, int h, int[] argb, boolean smooth, boolean repeat);
}
```

| Method | What to do |
|---|---|
| `triangles` | `n` vertices, a multiple of 3. Positions are in UI units: multiply by `unit()`, and round to whole device pixels for sharp edges. `uv` is in the image's pixels (divide by its size). With no image, draw plain colour (a 1×1 white texture works). Colours are ARGB per vertex and not premultiplied: blend with source alpha, one minus source alpha. Multiply each alpha by the opacity. |
| `pushClip` / `popClip` | A clip rectangle in UI units, intersected with the current one (a scissor rectangle). |
| `setOpacity` | Multiplies the alpha of everything drawn afterwards (window fades). |
| `createImage` | A texture from ARGB pixels. `smooth`: linear filtering, else nearest. `repeat`: wrap (the panel texture), else clamp. |

That's all: everything the toolkit draws is made of these. Calls arrive in drawing order, so batching vertices by
texture until the texture or the clip changes is enough. The included painters do this.

## The Host contract

```java
public interface Host {
    String clipboard();
    void setClipboard(String text);
    void setCursor(int shape);       // Cursor.ARROW, TEXT, HAND, RESIZE_H, RESIZE_V, MOVE
}
```

A `null` host also works: there's no clipboard, and the pointer never changes shape.

## Driving the Ui yourself

To run SlopUI inside your own engine loop, without the included hosts:

```java
// once
Ui ui = new Ui(theme, myHost);

// on input (positions in UI units: window pixels × framebuffer/window ratio ÷ unit)
ui.mods = currentModifiers;                 // Keys.SHIFT | CTRL | ALT | SUPER
ui.mouseMoved(x, y);
ui.mousePressed(0);  ui.mouseReleased(0);   // 0 left, 1 right, 2 middle
ui.wheel(dy);                               // positive: away from the user
ui.key(keyCode, mods);                      // Keys.* (GLFW's codes); call again for key repeats
ui.typed(codepoint);                        // text input

// each frame, after drawing the game
float unit = Math.max(1, Math.round(fbHeight / 400f));
ui.setSize(fbWidth / unit, fbHeight / unit, unit);
ui.frame(secondsSinceLastFrame);
myPainter.begin(...);                       // your painter's own frame setup
ui.draw(myPainter);
myPainter.end();
```

Key codes are GLFW's values. To translate from another library, see `mapKey` in `cpp/backend/sdl_host.cpp` for
SDL's codes. The keys SlopUI reads are listed in `Keys`.

`ui.needsRedraw()` is true when input arrived or something is animating. A host that only draws on change can skip
the other frames.

## Fullscreen

Both hosts' `setFullscreen(true)` give a borderless window over the whole monitor: GLFW uses the monitor's own
mode, SDL the desktop's mode. They also call `ui.setBarAutoHide(true)`. On systems with real screen modes, open
the system's mode requester yourself and create the window and context there; the toolkit doesn't change.
