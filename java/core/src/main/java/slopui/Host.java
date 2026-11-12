package slopui;

/** What the toolkit asks of the window it runs in (the GL host, the game window). */
public interface Host {
    String clipboard();

    void setClipboard(String text);

    /** The pointer's shape: one of {@link Cursor}. */
    void setCursor(int shape);
}
