package slopui;

/** An image the backend made ({@link Painter#createImage}); size in pixels. */
public interface Image {
    int width();

    int height();

    /** The backend's resources released. */
    void dispose();
}
