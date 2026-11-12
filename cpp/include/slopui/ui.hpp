#pragma once
#include <algorithm>
// SlopUI widgets, layout, windows, popups, menus and the Ui root. The same classes as the Java version
// (java/core, package slopui); keep the two in step. Widgets are owned by std::shared_ptr (make them with
// slopui::make<T>(...)); a parent owns its children, the Ui its windows, popups and background.
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "slopui/core.hpp"

namespace slopui {

class Ui;

template <class T, class... A>
std::shared_ptr<T> make(A&&... a) {
    return std::make_shared<T>(std::forward<A>(a)...);
}

using Text = std::function<std::string()>;
inline Text text(std::string s) {
    return [s = std::move(s)] { return s; };
}

/**
 * A part of the UI: a rectangle (UI units, absolute) that measures, lays out, draws and takes input. Events go
 * to the deepest widget under the pointer and up its parents until one takes them (Ui).
 */
class Widget : public std::enable_shared_from_this<Widget> {
public:
    float x = 0, y = 0, w = 0, h = 0;
    /** Share of a row's or column's spare room this widget takes (0: none). */
    float grow = 0;
    Text tooltip;

    virtual ~Widget() = default;
    virtual float prefWidth(Ui& ui) = 0;
    virtual float minWidth(Ui& ui) { return prefWidth(ui); }
    virtual float prefHeight(Ui& ui, float width) = 0;
    virtual void layout(Ui&) {}
    virtual void draw(Ui& ui, Painter& p) = 0;

    void setBounds(float nx, float ny, float nw, float nh) {
        x = nx;
        y = ny;
        w = nw;
        h = nh;
    }
    bool contains(float px, float py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    virtual Widget* hit(float px, float py) { return visible_ && contains(px, py) ? this : nullptr; }
    virtual void children(std::vector<Widget*>&) {}

    virtual bool press(Ui&, float, float, int, int) { return false; }
    virtual void drag(Ui&, float, float) {}
    virtual void release(Ui&, float, float, int) {}
    virtual bool wheel(Ui&, float, float, float) { return false; }
    virtual bool key(Ui&, int, int) { return false; }
    virtual bool typed(Ui&, char32_t) { return false; }
    virtual void tick(Ui&) {}
    virtual bool focusable() const { return false; }
    /** Takes typed text: plain keys (no Ctrl/Alt) stop here instead of reaching the shortcuts and the game. */
    virtual bool takesText() const { return false; }
    virtual int cursor(float, float) const { return Cursor::ARROW; }

    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    bool isEnabled() const;
    void setEnabled(bool e) { enabled_ = e; }
    void setTooltip(std::string t) { tooltip = text(std::move(t)); }
    Widget* parent() const { return parent_; }

protected:
    /** An animated value in slot (0-3) moving towards target over 'time' seconds. */
    float animate(Ui& ui, int slot, float target, float time);
    void adopt(Widget& child) { child.parent_ = this; }
    Theme& theme(Ui& ui) const;
    bool visible_ = true, enabled_ = true;

private:
    Widget* parent_ = nullptr;
    float anims_[4] = {};
    friend class Container;
};
using WidgetPtr = std::shared_ptr<Widget>;

/** A widget holding others: draws them in order, finds the topmost under the pointer. */
class Container : public Widget {
public:
    Container& add(WidgetPtr w);
    template <class... W>
    Container& add(WidgetPtr a, W... more) {
        add(std::move(a));
        (add(WidgetPtr(more)), ...);
        return *this;
    }
    void remove(Widget* w);
    void clear();
    const std::vector<WidgetPtr>& kids() const { return kids_; }
    void children(std::vector<Widget*>& out) override;
    Widget* hit(float px, float py) override;
    void draw(Ui& ui, Painter& p) override;

protected:
    std::vector<WidgetPtr> kids_;
};

/**
 * A row or a column. Children get their preferred size along the axis; spare room goes to those that grow (or
 * to 'justify'); short of room they shrink towards their least width. A responsive row wraps onto more lines
 * when even its least widths don't fit. Children stretch across the axis.
 */
class Box : public Container {
public:
    static std::shared_ptr<Box> row(std::vector<WidgetPtr> ws = {});
    static std::shared_ptr<Box> column(std::vector<WidgetPtr> ws = {});
    Box& responsive() { responsive_ = true; return *this; }
    Box& gap(float g) { gap_ = g; return *this; }
    Box& pad(float p) { pad_ = p; return *this; }
    Box& justify(float j) { justify_ = j; return *this; }
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void layout(Ui& ui) override;

private:
    float gapOf(Ui& ui) const;
    std::vector<Widget*> shown() const;
    bool asRow(Ui& ui, float width);
    std::vector<float> widths(Ui& ui, const std::vector<Widget*>& l, float inner);
    std::vector<std::vector<Widget*>> lines(Ui& ui, const std::vector<Widget*>& l, float inner);
    bool horizontal_ = false, responsive_ = false;
    float gap_ = -1, pad_ = 0, justify_ = 0;
};

/** Children in rows of n columns (forms); too narrow for the columns' least widths, it stacks. */
class Grid : public Container {
public:
    explicit Grid(int columns);
    Grid& columnGrow(std::vector<float> g);
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void layout(Ui& ui) override;

private:
    float gx(Ui& ui) const;
    float gy(Ui& ui) const;
    int rows() const { return int((kids_.size() + columns_ - 1) / columns_); }
    std::vector<float> colPref(Ui& ui, bool min);
    bool stacked(Ui& ui, float width);
    std::vector<float> widths(Ui& ui, float width);
    int columns_;
    std::vector<float> colGrow_;
};

/** A view onto a widget larger than its area: the wheel scrolls it; bars (or none: wheel, middle-drag). */
class ScrollPane : public Widget {
public:
    ScrollPane(WidgetPtr content, bool bars);
    ScrollPane& viewHeight(float h) { viewHeight_ = h; return *this; }
    Widget& content() { return *content_; }
    void children(std::vector<Widget*>& out) override { out.push_back(content_.get()); }
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void layout(Ui& ui) override;
    void reveal(Ui& ui, float ry, float rh);
    Widget* hit(float px, float py) override;
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void drag(Ui& ui, float mx, float my) override;
    void release(Ui&, float, float, int) override { dragging_ = 0; }
    bool wheel(Ui& ui, float mx, float my, float dy) override;

private:
    float bar(Ui& ui) const;
    float viewW(Ui& ui) const;
    float viewH(Ui& ui) const;
    void clampTargets(Ui& ui);
    std::array<float, 2> thumbV(Ui& ui) const;
    std::array<float, 2> thumbH(Ui& ui) const;
    WidgetPtr content_;
    bool bars_;
    float sx_ = 0, sy_ = 0, targetX_ = 0, targetY_ = 0, contentW_ = 0, contentH_ = 0, barW_ = 0;
    int dragging_ = 0;
    float dragFrom_ = 0, dragStart_ = 0, dragFromX_ = 0, dragStartX_ = 0, viewHeight_ = 0;
};

class Button;

/**
 * A window over the game: a title bar to drag it by (or none: a fixed pane), its content in a scroll pane and
 * a row of buttons that wraps when narrow. At most MAX_WINDOW of the UI; modal windows shade and block what
 * is under them. Enter presses the default button, Escape cancels.
 */
class Window : public Container {
public:
    Window(Text title, WidgetPtr content);
    Window(std::string title, WidgetPtr content) : Window(text(std::move(title)), std::move(content)) {}
    Window& modal(bool m) { modal_ = m; return *this; }
    bool isModal() const { return modal_; }
    Window& titleBar(bool t) { titleBar_ = t; return *this; }
    Window& closable(bool c) { closable_ = c; return *this; }
    /** The player may drag its right and bottom edges and its corner to resize it. */
    Window& resizable(bool r) { resizable_ = r; return *this; }
    /** Back to its own size. */
    void resetSize() { userW_ = userH_ = 0; }
    Widget* hit(float px, float py) override;
    Window& width(float w) { wantW_ = w; return *this; }
    Window& button(std::shared_ptr<Button> b);
    Window& defaultButton(std::shared_ptr<Button> b);
    Window& onCancel(std::function<void()> r) { onCancel_ = std::move(r); return *this; }
    Window& onClose(std::function<void()> r) { onClose_ = std::move(r); return *this; }
    void cancel(Ui& ui);
    void recentre() { placed_ = false; }
    void closed() { if (onClose_) onClose_(); }
    void place(Ui& ui);

    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void layout(Ui& ui) override;
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void drag(Ui& ui, float mx, float my) override;
    void release(Ui& ui, float mx, float my, int button) override;
    int cursor(float mx, float my) const override;
    bool key(Ui& ui, int key, int mods) override;
    bool wheel(Ui&, float, float, float) override { return true; }
    void children(std::vector<Widget*>& out) override;

private:
    float titleH(Ui& ui) const;
    bool hasButtons() const;
    bool onCloseButton(Ui& ui, float mx, float my) const;
    float minHeight(Ui& ui, float width);
    /** The resizing edge at the point: 1 right, 2 bottom, 3 corner, 0 none. */
    int edgeAt(float mx, float my) const;
    bool resizable_ = false;
    float userW_ = 0, userH_ = 0, sizeDX_ = 0, sizeDY_ = 0;
    int sizing_ = 0;
    Text title_;
    bool modal_ = true, titleBar_ = true, closable_ = true;
    std::shared_ptr<ScrollPane> scroll_;
    std::shared_ptr<Box> buttons_;
    std::shared_ptr<Button> default_;
    std::function<void()> onCancel_, onClose_;
    bool placed_ = false, dragging_ = false, closePressed_ = false;
    float wantW_ = 0, dragDX_ = 0, dragDY_ = 0, open_ = 0;
};

/** Something shown over everything for a moment (menus, lists, the calendar), placed next to an anchor. */
class Popup : public Container {
public:
    void anchor(float ax, float ay, float aw, float ah, bool side);
    void place(Ui& ui);
    Widget* owner = nullptr;
    bool key(Ui& ui, int key, int mods) override;
    bool press(Ui&, float, float, int, int) override { return true; }
    bool wheel(Ui&, float, float, float) override { return true; }

protected:
    float opening(Ui& ui);
    float ax_ = 0, ay_ = 0, aw_ = 0, ah_ = 0;
    bool side_ = false;
    /** At least as wide as the anchor (lists under fields); false: its own width, ending at the anchor's right. */
    bool matchAnchor_ = true;
};

/** A menu entry: an action (with a tick or a radio mark), a submenu or a separator. */
class Menu;
struct MenuItem {
    Text label;
    std::function<void()> action;
    std::shared_ptr<Menu> submenu;
    bool separator = false;
    std::function<bool()> checked;
    bool radio = false;
    std::function<bool()> enabled = [] { return true; };
    std::string shortcutText;

    static std::shared_ptr<MenuItem> make(std::string label, std::function<void()> action);
    static std::shared_ptr<MenuItem> sub(std::shared_ptr<Menu> m);
    static std::shared_ptr<MenuItem> line();
    MenuItem& check(std::function<bool()> c) { checked = std::move(c); return *this; }
    MenuItem& radioMark(std::function<bool()> c) { checked = std::move(c); radio = true; return *this; }
    MenuItem& isEnabled(std::function<bool()> e) { enabled = std::move(e); return *this; }
    MenuItem& shortcut(std::string s) { shortcutText = std::move(s); return *this; }
    bool selectable() const { return !separator && enabled(); }
};

/** A menu: a label and its items (made again before it opens when onOpen is set). */
class Menu {
public:
    explicit Menu(std::string label) : label(text(std::move(label))) {}
    Menu& add(std::shared_ptr<MenuItem> it) { items.push_back(std::move(it)); return *this; }
    MenuItem& item(std::string label, std::function<void()> action);
    Menu& separator() { items.push_back(MenuItem::line()); return *this; }
    Text label;
    std::vector<std::shared_ptr<MenuItem>> items;
    std::function<void()> onOpen;
};

class MenuPopup;

/** The bar of menus at the top: press opens, moving over another title switches; F10 / Alt + letter. */
class MenuBar : public Widget {
public:
    MenuBar& add(std::shared_ptr<Menu> m) { menus_.push_back(std::move(m)); return *this; }
    bool isEmpty() const { return menus_.empty(); }
    bool isOpen() const { return open_ >= 0; }
    void closed() { open_ = -1; popup_.reset(); }
    float prefWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float) override;
    void layout(Ui& ui) override;
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void release(Ui& ui, float mx, float my, int button) override;
    void openMenu(Ui& ui, int i, bool keyboard);
    void step(Ui& ui, int d);
    bool openByKey(Ui& ui, int key);
    void moved(Ui& ui, float mx, float my);

private:
    int titleAt(float mx, float my) const;
    std::vector<std::shared_ptr<Menu>> menus_;
    int open_ = -1;
    std::weak_ptr<MenuPopup> popup_;
    std::vector<float> titleX_, titleW_;
};

/** An open menu: items, ticks, shortcuts, submenus; runs an item on the release over it. */
class MenuPopup : public Popup {
public:
    MenuPopup(std::shared_ptr<Menu> menu, MenuBar* bar) : menu_(std::move(menu)), bar_(bar) {}
    float prefWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void draw(Ui& ui, Painter& p) override;
    void release(Ui& ui, float mx, float my, int) override { releaseAt(ui, mx, my); }
    void releaseAt(Ui& ui, float mx, float my);
    bool wheel(Ui& ui, float mx, float my, float dy) override;
    bool key(Ui& ui, int key, int mods) override;
    void hoverAt(Ui& ui, float mx, float my);
    void selectFirst();

private:
    float rowH(Ui& ui, const MenuItem& it) const;
    void move(int d);
    int itemAt(Ui& ui, float mx, float my);
    float itemY(Ui& ui, int index);
    void openChild(Ui& ui);
    void run(Ui& ui, int i);
    std::shared_ptr<Menu> menu_;
    MenuBar* bar_;
    int sel_ = -1;
    std::weak_ptr<MenuPopup> child_;
    float scroll_ = 0;
};

/**
 * The UI over a game: the background widget (the game), the menu bar, windows, popups, toasts and tooltips,
 * in UI units (unit device pixels each). The host feeds it the window's size, input and frame time, and draws
 * it with a Painter each frame. Input and keys are routed as in the Java version (see its Ui).
 */
class Ui {
public:
    Ui(Theme theme, Host* host);
    Theme theme;
    Host* host;
    float unit = 1, width = 0, height = 0;
    float dt = 0;
    double time = 0;
    bool animating = false;
    float mx = -1000, my = -1000;
    /** The modifier keys held (Keys::SHIFT...), set by the host with each input event. */
    int mods = 0;
    std::shared_ptr<MenuBar> menuBar;

    void setBackground(WidgetPtr w);
    void setSize(float w, float h, float unit);
    void setBarAutoHide(bool on);
    void shortcut(int key, std::function<void()> r) { shortcuts_.push_back({key, std::move(r)}); }
    float barHeight() const { return theme.s[Theme::BAR_HEIGHT]; }
    bool barVisible() const;
    float contentTop() const;
    bool needsRedraw();

    void open(std::shared_ptr<Window> w);
    void close(Window* w);
    bool isOpen(const Window* w) const;
    Window* topWindow() const;
    bool modalOpen() const { return topModal() != nullptr; }
    void openPopup(std::shared_ptr<Popup> p);
    void closePopup(Popup* p);
    void closePopups();
    bool popupOpen() const { return !popups_.empty(); }
    const std::vector<std::shared_ptr<Popup>>& popups() const { return popups_; }
    void contextMenu(std::shared_ptr<Menu> m);
    void toast(std::string text);

    Widget* focused() const { return focus_.lock().get(); }
    void setFocus(Widget* w);
    bool isFocused(const Widget* w) const { return w && focused() == w; }
    void focusNext(bool back);
    bool isHover(const Widget* w) const;
    Widget* hover() const { return hover_.lock().get(); }
    bool leftDown() const { return !pressed_.expired() && pressedButton_ == 0; }

    void frame(float seconds);
    void layout();
    void draw(Painter& p);
    Widget* hitAt(float x, float y);
    void mouseMoved(float x, float y);
    void mousePressed(int button);
    void mouseReleased(int button);
    void wheel(float dy);
    void key(int key, int mods);
    void typed(char32_t cp);

    std::shared_ptr<Window> message(std::string title, std::string text, std::function<void()> done = {});
    std::shared_ptr<Window> confirm(std::string title, std::string text, std::string yes, std::string no,
                                    std::function<void()> onYes);
    std::shared_ptr<Window> ask(std::string title, std::string label, std::string initial,
                                std::function<void(const std::string&)> onOk);

private:
    Window* topModal() const;
    void focusFirst(Window& w);
    void collect(Widget* w, std::vector<Widget*>& out);
    void revealFocus();
    void drawToasts(Painter& p);
    void drawTooltip(Painter& p);
    void bury(WidgetPtr w) { graveyard_.push_back(std::move(w)); }

    WidgetPtr background_;
    std::vector<std::shared_ptr<Window>> windows_;
    std::vector<std::shared_ptr<Popup>> popups_;
    /** Closed windows and popups kept to the next frame (closed from inside their own handlers). */
    std::vector<WidgetPtr> graveyard_;
    std::weak_ptr<Widget> hover_, pressed_, focus_, tipWidget_;
    int pressedButton_ = -1;
    bool barAutoHide_ = false, barShown_ = false, dirty_ = true;
    float tipRest_ = 0, tipX_ = 0, tipY_ = 0;
    double lastPress_ = -10;
    float lastPressX_ = 0, lastPressY_ = 0;
    int clicks_ = 0;
    struct Toast {
        std::u32string text;
        double at;
    };
    std::vector<Toast> toasts_;
    std::vector<std::pair<int, std::function<void()>>> shortcuts_;
};

// Widgets

/** Text: wraps to its width at spaces (and newlines); aligned left, centred or right. */
class Label : public Widget {
public:
    explicit Label(Text t) : text_(std::move(t)) {}
    explicit Label(std::string t) : text_(text(std::move(t))) {}
    Label& set(std::string t) { text_ = text(std::move(t)); return *this; }
    Label& colour(argb c) { colour_ = c; return *this; }
    Label& align(float a) { align_ = a; return *this; }
    Label& title() { title_ = true; return *this; }
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void draw(Ui& ui, Painter& p) override;

private:
    Font& font(Ui& ui) const;
    std::vector<std::u32string> lines(Font& f, float width) const;
    Text text_;
    argb colour_ = 0;
    float align_ = 0;
    bool title_ = false;
};

/** A push button: acts on the release over it; moving off cancels. Space or Enter when focused. */
class Button : public Widget {
public:
    Button(Text t, std::function<void()> action) : text_(std::move(t)), action_(std::move(action)) {}
    Button(std::string t, std::function<void()> action) : Button(text(std::move(t)), std::move(action)) {}
    Button& accent(bool a) { accent_ = a; return *this; }
    Button& onClick(std::function<void()> r) { action_ = std::move(r); return *this; }
    void click(Ui& ui);
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float) override;
    bool focusable() const override { return true; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void release(Ui& ui, float mx, float my, int button) override;
    bool key(Ui& ui, int key, int mods) override;
    int cursor(float, float) const override { return Cursor::HAND; }

private:
    friend class Window;
    Text text_;
    std::function<void()> action_;
    bool accent_ = false, down_ = false;
};

/** Base of the toggling widgets (CheckBox, Toggle): text beside, changes on the release over it. */
class CheckBox : public Widget {
public:
    CheckBox(std::string t, bool on) : text_(toU32(t)), on_(on) {}
    CheckBox& onChange(std::function<void(bool)> c) { onChange_ = std::move(c); return *this; }
    bool isOn() const { return on_; }
    void set(bool v) { on_ = v; }
    float prefWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float) override;
    bool focusable() const override { return true; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui&, float, float, int button, int) override { down_ = button == 0; return down_; }
    void release(Ui& ui, float mx, float my, int button) override;
    bool key(Ui& ui, int key, int mods) override;
    int cursor(float, float) const override { return Cursor::HAND; }

protected:
    void toggle();
    std::u32string text_;
    bool on_, down_ = false;
    std::function<void(bool)> onChange_;
};

/** An on/off switch: the knob slides and the track lights up. */
class Toggle : public CheckBox {
public:
    using CheckBox::CheckBox;
    float prefWidth(Ui& ui) override;
    void draw(Ui& ui, Painter& p) override;
    bool key(Ui& ui, int key, int mods) override;
};

class RadioButton;

/** Radio buttons that belong together: one selected. */
class RadioGroup : public std::enable_shared_from_this<RadioGroup> {
public:
    explicit RadioGroup(int selected) : selected_(selected) {}
    RadioGroup& onChange(std::function<void(int)> c) { onChange_ = std::move(c); return *this; }
    std::shared_ptr<RadioButton> button(std::string text);
    int selected() const { return selected_; }
    void select(int i);

private:
    friend class RadioButton;
    std::vector<std::weak_ptr<RadioButton>> buttons_;
    int selected_;
    std::function<void(int)> onChange_;
};

/** One of a RadioGroup: a diamond lit when selected; arrows move the selection. */
class RadioButton : public Widget {
public:
    RadioButton(std::shared_ptr<RadioGroup> g, int index, std::string t) : group_(std::move(g)), index_(index), text_(toU32(t)) {}
    float prefWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float) override;
    bool focusable() const override { return true; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui&, float, float, int button, int) override { down_ = button == 0; return down_; }
    void release(Ui& ui, float mx, float my, int button) override;
    bool key(Ui& ui, int key, int mods) override;
    int cursor(float, float) const override { return Cursor::HAND; }

private:
    std::shared_ptr<RadioGroup> group_;
    int index_;
    std::u32string text_;
    bool down_ = false;
};

/** A line between parts, fading at its ends. */
class Separator : public Widget {
public:
    explicit Separator(bool vertical = false) : vertical_(vertical) {}
    float prefWidth(Ui&) override { return vertical_ ? 7.0f : 10.0f; }
    float minWidth(Ui&) override { return vertical_ ? 7.0f : 0.0f; }
    float prefHeight(Ui&, float) override { return vertical_ ? 10.0f : 7.0f; }
    void draw(Ui& ui, Painter& p) override;

private:
    bool vertical_;
};

/** How far something is (0..1), or busy (negative): a light sweeping across. */
class ProgressBar : public Widget {
public:
    explicit ProgressBar(float v) : value_(v) {}
    void set(float v) { value_ = v; }
    float value() const { return value_; }
    ProgressBar& text(Text t) { text_ = std::move(t); return *this; }
    float prefWidth(Ui&) override { return 120; }
    float minWidth(Ui&) override { return 40; }
    float prefHeight(Ui& ui, float) override;
    void draw(Ui& ui, Painter& p) override;

private:
    float value_, shown_ = 0;
    Text text_;
};

/** A value between min and max in steps: drag, press the track, wheel, arrow keys. */
class Slider : public Widget {
public:
    Slider(double min, double max, double step, double value) : min_(min), max_(max), step_(step), value_(value) {}
    Slider& onChange(std::function<void(double)> c) { onChange_ = std::move(c); return *this; }
    Slider& format(std::function<std::string(double)> f) { format_ = std::move(f); return *this; }
    double value() const { return value_; }
    void set(double v);
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float) override;
    bool focusable() const override { return true; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void drag(Ui& ui, float mx, float) override { if (dragging_) setFrom(ui, mx); }
    void release(Ui&, float, float, int) override { dragging_ = false; }
    bool wheel(Ui& ui, float mx, float my, float dy) override;
    bool key(Ui& ui, int key, int mods) override;
    int cursor(float, float) const override { return Cursor::HAND; }

private:
    float labelW(Ui& ui);
    float trackW(Ui& ui) { return w - labelW(ui) - 8; }
    void setFrom(Ui& ui, float mx);
    double stepOr() const { return step_ > 0 ? step_ : (max_ - min_) / 100; }
    double min_, max_, step_, value_;
    std::function<void(double)> onChange_;
    std::function<std::string(double)> format_ = [](double v) { return std::to_string(v); };
    bool dragging_ = false;
};

/** An image from ARGB pixels, scaled to fit and centred. */
class ImageView : public Widget {
public:
    ImageView(int iw, int ih, std::vector<argb> px, float prefW, float prefH, bool smooth)
        : iw_(iw), ih_(ih), px_(std::move(px)), prefW_(prefW), prefH_(prefH), smooth_(smooth) {}
    float prefWidth(Ui&) override { return prefW_; }
    float minWidth(Ui&) override { return std::min(prefW_, 24.0f); }
    float prefHeight(Ui&, float width) override { return prefH_ * std::min(1.0f, width / prefW_); }
    void draw(Ui& ui, Painter& p) override;

private:
    int iw_, ih_;
    std::vector<argb> px_;
    float prefW_, prefH_;
    bool smooth_;
    std::shared_ptr<Image> image_;
    const Painter* owner_ = nullptr;
};

/** A titled frame around related parts. */
class GroupBox : public Widget {
public:
    GroupBox(std::string title, WidgetPtr content);
    void children(std::vector<Widget*>& out) override { out.push_back(content_.get()); }
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void layout(Ui& ui) override;
    Widget* hit(float px, float py) override;
    void draw(Ui& ui, Painter& p) override;

private:
    float top(Ui& ui) const;
    std::u32string title_;
    WidgetPtr content_;
};

/**
 * One line of text to type: caret, selection (drag, double click a word, Shift + keys), Ctrl+A/C/X/V,
 * Home/End, Ctrl + arrows by words; scrolls sideways. A placeholder while empty.
 */
class TextField : public Widget {
public:
    explicit TextField(std::string initial);
    std::string text() const { return toUtf8(text_); }
    void set(std::string s);
    TextField& placeholder(std::string p) { placeholder_ = toU32(p); return *this; }
    TextField& maxLength(size_t n) { maxLength_ = n; return *this; }
    TextField& allow(std::function<bool(char32_t)> p) { allowed_ = std::move(p); return *this; }
    TextField& onChange(std::function<void(const std::string&)> c) { onChange_ = std::move(c); return *this; }
    TextField& onEnter(std::function<void(const std::string&)> c) { onEnter_ = std::move(c); return *this; }
    void selectAll() { anchor_ = 0; caret_ = text_.size(); }
    float prefWidth(Ui&) override { return 120; }
    float minWidth(Ui&) override { return 40; }
    float prefHeight(Ui& ui, float) override;
    bool focusable() const override { return true; }
    bool takesText() const override { return true; }
    int cursor(float mx, float) const override { return mx < x + w - rightInset_ ? Cursor::TEXT : Cursor::ARROW; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void drag(Ui& ui, float mx, float my) override;
    void release(Ui&, float, float, int) override { selecting_ = false; }
    bool typed(Ui& ui, char32_t cp) override;
    bool key(Ui& ui, int key, int mods) override;

protected:
    virtual void changed();
    virtual bool multiline() const { return false; }
    virtual size_t homeOf(size_t, bool) const { return 0; }
    virtual size_t endOf(size_t, bool) const { return text_.size(); }
    virtual bool verticalKey(Ui&, int, bool) { return false; }
    virtual size_t indexAt(Ui& ui, float mx);
    void deleteSelection();
    void insert(std::u32string_view s);
    size_t wordStart(size_t i) const;
    size_t wordEnd(size_t i) const;
    size_t wordLeft(size_t i) const;
    size_t wordRight(size_t i) const;
    std::u32string text_;
    size_t caret_ = 0, anchor_ = 0;
    float rightInset_ = 0;
    double lastEdit_ = 0;

private:
    float scrollX_ = 0;
    std::u32string placeholder_;
    size_t maxLength_ = 256;
    std::function<bool(char32_t)> allowed_ = [](char32_t c) { return c >= 32; };
    std::function<void(const std::string&)> onChange_, onEnter_;
    bool selecting_ = false;
};

/** Several lines of text: Enter for new lines, Up/Down, Home/End per line, Page Up/Down; scrolls both ways. */
class TextArea : public TextField {
public:
    TextArea(std::string initial, int rows);
    float prefWidth(Ui&) override { return 200; }
    float minWidth(Ui&) override { return 60; }
    float prefHeight(Ui& ui, float) override;
    void draw(Ui& ui, Painter& p) override;
    bool key(Ui& ui, int key, int mods) override;
    bool wheel(Ui& ui, float mx, float my, float dy) override;

protected:
    bool multiline() const override { return true; }
    size_t homeOf(size_t i, bool ctrl) const override { return ctrl ? 0 : lineStart(i); }
    size_t endOf(size_t i, bool ctrl) const override { return ctrl ? text_.size() : lineEnd(i); }
    bool verticalKey(Ui& ui, int key, bool shift) override;
    size_t indexAt(Ui& ui, float mx) override;

private:
    float lineH(Ui& ui) const;
    size_t lineStart(size_t i) const;
    size_t lineEnd(size_t i) const;
    int lineOf(size_t i) const;
    size_t startOfLine(int line) const;
    int lineCount() const { return lineOf(text_.size()) + 1; }
    size_t columnAt(Ui& ui, size_t start, float relX);
    int rows_;
    float scrollY_ = 0, scrollX_ = 0, goalX_ = -1;
    size_t shownCaret_ = size_t(-1), shownLength_ = size_t(-1);
};

/** A number to type or step: arrows at the right (holding repeats), Up/Down, the wheel; within min..max. */
class NumberField : public TextField {
public:
    NumberField(double value, double min, double max, double step, int decimals);
    NumberField& onValue(std::function<void(double)> c) { onValue_ = std::move(c); return *this; }
    double value() const { return value_; }
    void setValue(double v);
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override { return prefWidth(ui); }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void tick(Ui& ui) override;
    void release(Ui& ui, float mx, float my, int button) override;
    bool wheel(Ui& ui, float mx, float my, float dy) override;
    bool key(Ui& ui, int key, int mods) override;

protected:
    void changed() override;

private:
    std::string format(double v) const;
    double min_, max_, step_, value_ = 0;
    int decimals_;
    std::function<void(double)> onValue_;
    int held_ = 0;
    double heldSince_ = 0, lastRepeat_ = 0;
    bool setting_ = false;
};

class ListPopup;

/** A text field with suggestions: typing filters the list under it; the chevron opens all of it. */
class ComboBox : public TextField {
public:
    ComboBox(std::string initial, std::vector<std::string> items);
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    bool typed(Ui& ui, char32_t cp) override;
    bool key(Ui& ui, int key, int mods) override;

private:
    void showList(Ui& ui, bool all);
    std::vector<std::string> items_;
    std::weak_ptr<ListPopup> list_;
};

/** Pick one of a list (not typed): shows the choice and a chevron; opens a list; Up/Down change it. */
class Dropdown : public Widget {
public:
    Dropdown(std::vector<std::string> items, int selected) : items_(std::move(items)), selected_(selected) {}
    Dropdown& onChange(std::function<void(int)> c) { onChange_ = std::move(c); return *this; }
    int selected() const { return selected_; }
    void select(int i);
    float prefWidth(Ui& ui) override;
    float minWidth(Ui&) override { return 50; }
    float prefHeight(Ui& ui, float) override;
    bool focusable() const override { return true; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void release(Ui&, float, float, int) override { down_ = false; }
    bool key(Ui& ui, int key, int mods) override;
    int cursor(float, float) const override { return Cursor::HAND; }

private:
    void openList(Ui& ui);
    std::vector<std::string> items_;
    int selected_;
    std::function<void(int)> onChange_;
    bool down_ = false;
};

/** A list to pick from under a field: hover, arrows, Enter; scrolls when long. */
class ListPopup : public Popup {
public:
    ListPopup(Widget& owner, std::vector<std::string> items, int selected, std::function<void(int)> onPick);
    float prefWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float) override;
    void draw(Ui& ui, Painter& p) override;
    void release(Ui& ui, float mx, float my, int button) override;
    bool wheel(Ui& ui, float mx, float my, float dy) override;
    bool key(Ui& ui, int key, int mods) override;

private:
    float rowH(Ui& ui) const;
    int at(Ui& ui, float my) const;
    void reveal(Ui& ui);
    std::vector<std::u32string> items_;
    std::function<void(int)> onPick_;
    int sel_;
    float scroll_ = 0;
};

/**
 * A calendar date (real dates only): typed as YYYY-MM-DD or picked from a calendar under the field (months and
 * years stepped by its arrows, weeks from Monday, today marked).
 */
class DatePicker : public TextField {
public:
    DatePicker(int year, int month, int day);
    static std::array<int, 3> today();
    DatePicker& onDate(std::function<void(std::array<int, 3>)> c) { onDate_ = std::move(c); return *this; }
    std::array<int, 3> date() const { return {year_, month_, day_}; }
    void setDate(int y, int m, int d);
    static bool leap(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
    static int daysIn(int y, int m);
    /** Days since 1970-01-01 and back. */
    static long long days(int y, int m, int d);
    static std::array<int, 3> civil(long long z);
    /** 0 Monday .. 6 Sunday. */
    static int weekday(int y, int m, int d);
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override { return prefWidth(ui); }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    bool key(Ui& ui, int key, int mods) override;

protected:
    void changed() override;

private:
    friend class CalendarPopup;
    void openCalendar(Ui& ui);
    int year_ = 2000, month_ = 1, day_ = 1;
    std::function<void(std::array<int, 3>)> onDate_;
    std::weak_ptr<Popup> calendar_;
};

/**
 * Rows of text in columns: a header to sort by (again: reversed; numbers as numbers), one row selected
 * (click, arrows, Page Up/Down, Home/End), double click or Enter activates it. Widths by weight; "..." fits.
 */
class Table : public Widget {
public:
    Table(std::vector<std::string> headers, std::vector<float> weights, int visibleRows);
    Table& onSelect(std::function<void(int)> c) { onSelect_ = std::move(c); return *this; }
    Table& onActivate(std::function<void(int)> c) { onActivate_ = std::move(c); return *this; }
    void setRows(std::vector<std::vector<std::string>> rows);
    int selected() const { return selected_; }
    const std::vector<std::string>& row(int i) const { return rows_[i]; }
    void select(int row);
    float prefWidth(Ui& ui) override;
    float minWidth(Ui&) override { return float(headers_.size()) * 34; }
    float prefHeight(Ui& ui, float) override;
    bool focusable() const override { return true; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void drag(Ui& ui, float mx, float my) override;
    void release(Ui& ui, float mx, float my, int button) override;
    bool wheel(Ui& ui, float mx, float my, float dy) override;
    bool key(Ui& ui, int key, int mods) override;
    int cursor(float mx, float my) const override;
    /** The column shown at each position (after the player moved columns). */
    const std::vector<int>& columnOrder() const { return colOrder_; }

private:
    void moveColumn(int from, int gap);
    /** The header separator at the point (index of the column to its right), or -1. */
    int boundaryAt(float mx, float my) const;
    void resort();
    float rowH(Ui& ui) const;
    float headH(Ui& ui) const;
    std::vector<float> colX(Ui& ui) const;
    float viewH(Ui& ui) const;
    void clampScroll(Ui& ui);
    void reveal(Ui& ui);
    std::array<float, 2> thumb(Ui& ui) const;
    int rowAt(Ui& ui, float mx, float my) const;
    std::vector<std::string> headers_;
    std::vector<float> weights_, initialWeights_;
    int resizing_ = -1;
    float resizeFrom_ = 0, weightA_ = 0, weightB_ = 0, lastHeadH_ = 0;
    std::vector<float> lastColX_;
    std::vector<int> colOrder_;
    int pressCol_ = -1, dropAt_ = -1;
    float pressX_ = 0;
    bool movingCol_ = false;
    std::vector<std::vector<std::string>> rows_;
    std::vector<int> order_;
    int selected_ = -1, sortColumn_ = -1, visibleRows_;
    bool descending_ = false, dragThumb_ = false;
    float scroll_ = 0, dragFrom_ = 0, dragStart_ = 0;
    std::function<void(int)> onSelect_, onActivate_;
};

/** Two panes with a separator between them to drag; double click: back to the first share. */
class SplitPane : public Widget {
public:
    SplitPane(WidgetPtr a, WidgetPtr b, bool vertical, float ratio);
    void children(std::vector<Widget*>& out) override { out.push_back(a_.get()); out.push_back(b_.get()); }
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void layout(Ui& ui) override;
    Widget* hit(float px, float py) override;
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void drag(Ui& ui, float mx, float my) override;
    void release(Ui&, float, float, int) override { dragging_ = false; }
    int cursor(float mx, float my) const override;

private:
    float bar(Ui& ui) const;
    float split(Ui& ui);
    bool onBar(float mx, float my) const;
    WidgetPtr a_, b_;
    bool vertical_;
    float ratio_, initial_;
    bool dragging_ = false;
};

/** Pages with a tab each: on top (wrapping) when narrow, a list on the left when at least sideWidth wide. */
class Tabs : public Widget {
public:
    Tabs& add(std::string title, WidgetPtr page);
    Tabs& sideWhenWider(float w) { sideWidth_ = w; return *this; }
    int selected() const { return selected_; }
    void select(int i);
    void children(std::vector<Widget*>& out) override { out.push_back(pages_[selected_].get()); }
    bool focusable() const override { return true; }
    float prefWidth(Ui& ui) override;
    float minWidth(Ui& ui) override;
    float prefHeight(Ui& ui, float width) override;
    void layout(Ui& ui) override;
    Widget* hit(float px, float py) override;
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    bool key(Ui& ui, int key, int mods) override;

private:
    bool side() const { return w >= sideWidth_; }
    float listW(Ui& ui) const;
    float headerFor(Ui& ui, float width, float ox, float oy, std::vector<std::array<float, 4>>* out) const;
    std::vector<std::u32string> titles_;
    std::vector<WidgetPtr> pages_;
    int selected_ = 0;
    float sideWidth_ = 1e30f, markPos_ = std::numeric_limits<float>::quiet_NaN(), markLen_ = 0;
    std::vector<std::array<float, 4>> tabRects_;
};

}  // namespace slopui
