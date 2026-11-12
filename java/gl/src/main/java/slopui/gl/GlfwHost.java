package slopui.gl;

import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL33.*;

import org.lwjgl.PointerBuffer;
import org.lwjgl.glfw.GLFWErrorCallback;
import org.lwjgl.glfw.GLFWVidMode;
import org.lwjgl.opengl.GL;
import org.lwjgl.system.MemoryUtil;
import slopui.Cursor;
import slopui.Host;
import slopui.Ui;

/**
 * A GLFW window running a {@link Ui}: input to it in UI units, a frame each refresh (vsync), the UI size
 * from the window (Auto: one UI unit per 400th of the height, whole units) or a fixed setting, fullscreen
 * (the monitor's own mode, no decorations; the bar hides itself there).
 */
public final class GlfwHost implements Host, AutoCloseable {
    private final long window;
    private Ui ui;
    private GlPainter painter;
    private final long[] cursors = new long[7];
    private int cursorShown = -1;
    private int uiScale;            // 0 Auto, else UI units per... device pixels per unit
    private boolean fullscreen;
    private final int[] windowed = new int[4];
    private double lastTime;
    private int clearColour = 0xFF000000;

    public GlfwHost(String title, int w, int h) {
        GLFWErrorCallback.createPrint(System.err).set();
        if (!glfwInit()) {
            throw new IllegalStateException("GLFW could not start (no display?)");
        }
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        window = glfwCreateWindow(w, h, title, MemoryUtil.NULL, MemoryUtil.NULL);
        if (window == MemoryUtil.NULL) {
            throw new IllegalStateException("no OpenGL 3.3 core window");
        }
        glfwMakeContextCurrent(window);
        GL.createCapabilities();
        glfwSwapInterval(1);
        painter = new GlPainter();
        int[] shapes = {GLFW_ARROW_CURSOR, GLFW_IBEAM_CURSOR, GLFW_HAND_CURSOR, GLFW_HRESIZE_CURSOR,
                GLFW_VRESIZE_CURSOR, GLFW_CROSSHAIR_CURSOR, GLFW_RESIZE_NWSE_CURSOR};
        for (int i = 0; i < shapes.length; i++) {
            cursors[i] = glfwCreateStandardCursor(shapes[i]);
        }
    }

    public void setUi(Ui ui) {
        this.ui = ui;
        ui.host = this;
        glfwSetCursorPosCallback(window, (win, x, y) -> {
            float r = ratio();
            ui.mouseMoved((float) (x * r / ui.unit), (float) (y * r / ui.unit));
        });
        glfwSetMouseButtonCallback(window, (win, b, action, mods) -> {
            ui.mods = mods;
            int button = b == GLFW_MOUSE_BUTTON_LEFT ? 0 : b == GLFW_MOUSE_BUTTON_RIGHT ? 1 : 2;
            if (action == GLFW_PRESS) {
                ui.mousePressed(button);
            } else if (action == GLFW_RELEASE) {
                ui.mouseReleased(button);
            }
        });
        glfwSetScrollCallback(window, (win, dx, dy) -> ui.wheel((float) dy));
        glfwSetKeyCallback(window, (win, key, sc, action, mods) -> {
            ui.mods = mods;
            if (action != GLFW_RELEASE) {
                ui.key(key, mods);
            }
        });
        glfwSetCharCallback(window, (win, cp) -> ui.typed(cp));
    }

    public GlPainter painter() {
        return painter;
    }

    public void setClearColour(int argb) {
        clearColour = argb;
    }

    /** UI size: 0 Auto, else device pixels per UI unit. */
    public void setUiScale(int s) {
        uiScale = s;
    }

    public int uiScale() {
        return uiScale;
    }

    private float ratio() {
        int[] fw = new int[1], fh = new int[1], ww = new int[1], wh = new int[1];
        glfwGetFramebufferSize(window, fw, fh);
        glfwGetWindowSize(window, ww, wh);
        return ww[0] > 0 ? fw[0] / (float) ww[0] : 1;
    }

    /** One frame: size, timers, drawing, swap; false when the window is closing. */
    public boolean frame(Runnable beforeDraw) {
        glfwPollEvents();
        if (glfwWindowShouldClose(window)) {
            return false;
        }
        int[] fw = new int[1], fh = new int[1];
        glfwGetFramebufferSize(window, fw, fh);
        if (fw[0] <= 0 || fh[0] <= 0) {
            return true;
        }
        float unit = uiScale > 0 ? uiScale * Math.max(1, Math.round(ratio())) : Math.max(1, Math.min(8, Math.round(fh[0] / 400f)));
        ui.setSize(fw[0] / unit, fh[0] / unit, unit);
        double now = glfwGetTime();
        ui.frame((float) (lastTime == 0 ? 1 / 60.0 : now - lastTime));
        lastTime = now;
        if (beforeDraw != null) {
            beforeDraw.run();
        }
        glClearColor((clearColour >> 16 & 0xFF) / 255f, (clearColour >> 8 & 0xFF) / 255f, (clearColour & 0xFF) / 255f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        painter.begin(fw[0], fh[0], unit);
        ui.draw(painter);
        painter.end();
        glfwSwapBuffers(window);
        return true;
    }

    public void requestClose() {
        glfwSetWindowShouldClose(window, true);
    }

    public void setSize(int w, int h) {
        if (fullscreen) {
            windowed[2] = w;
            windowed[3] = h;
            setFullscreen(false);
        } else {
            glfwSetWindowSize(window, w, h);
        }
    }

    public boolean fullscreen() {
        return fullscreen;
    }

    public void setFullscreen(boolean on) {
        if (on == fullscreen) {
            return;
        }
        if (on) {
            int[] x = new int[1], y = new int[1], w = new int[1], h = new int[1];
            glfwGetWindowPos(window, x, y);
            glfwGetWindowSize(window, w, h);
            windowed[0] = x[0];
            windowed[1] = y[0];
            windowed[2] = w[0];
            windowed[3] = h[0];
            long m = monitorAt(x[0] + w[0] / 2, y[0] + h[0] / 2);
            GLFWVidMode mode = glfwGetVideoMode(m);
            if (mode == null) {
                return;
            }
            glfwSetWindowMonitor(window, m, 0, 0, mode.width(), mode.height(), mode.refreshRate());
        } else {
            glfwSetWindowMonitor(window, MemoryUtil.NULL, windowed[0], windowed[1], Math.max(1, windowed[2]),
                    Math.max(1, windowed[3]), GLFW_DONT_CARE);
        }
        fullscreen = on;
        glfwSwapInterval(1);
        ui.setBarAutoHide(on);
    }

    private static long monitorAt(int px, int py) {
        PointerBuffer ms = glfwGetMonitors();
        if (ms != null) {
            for (int i = 0; i < ms.limit(); i++) {
                long m = ms.get(i);
                int[] x = new int[1], y = new int[1];
                glfwGetMonitorPos(m, x, y);
                GLFWVidMode mode = glfwGetVideoMode(m);
                if (mode != null && px >= x[0] && px < x[0] + mode.width() && py >= y[0] && py < y[0] + mode.height()) {
                    return m;
                }
            }
        }
        return glfwGetPrimaryMonitor();
    }

    @Override
    public String clipboard() {
        return glfwGetClipboardString(window);
    }

    @Override
    public void setClipboard(String text) {
        glfwSetClipboardString(window, text);
    }

    @Override
    public void setCursor(int shape) {
        if (shape != cursorShown && shape >= 0 && shape < cursors.length) {
            glfwSetCursor(window, shape == Cursor.ARROW ? MemoryUtil.NULL : cursors[shape]);
            cursorShown = shape;
        }
    }

    public void setTitle(String t) {
        glfwSetWindowTitle(window, t);
    }

    @Override
    public void close() {
        painter.close();
        for (long c : cursors) {
            if (c != 0) {
                glfwDestroyCursor(c);
            }
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}
