package slopui;

/** Key codes and modifiers the toolkit reads (GLFW's values; a host maps its own to these). */
public final class Keys {
    public static final int SPACE = 32, A = 65, C = 67, V = 86, X = 88, Z = 90;
    public static final int ESCAPE = 256, ENTER = 257, TAB = 258, BACKSPACE = 259, INSERT = 260, DELETE = 261,
            RIGHT = 262, LEFT = 263, DOWN = 264, UP = 265, PAGE_UP = 266, PAGE_DOWN = 267, HOME = 268, END = 269,
            F1 = 290, F10 = 299, F11 = 300, KP_ENTER = 335, LEFT_ALT = 342, RIGHT_ALT = 346;
    public static final int SHIFT = 1, CTRL = 2, ALT = 4, SUPER = 8;

    private Keys() {
    }

    public static boolean isEnter(int key) {
        return key == ENTER || key == KP_ENTER;
    }
}
