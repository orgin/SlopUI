// SlopUI: Ui, Window, Popup, Menu, MenuItem, MenuBar, MenuPopup (Java: the classes of the same names).
#include <algorithm>
#include <cmath>

#include "slopui/ui.hpp"

namespace slopui {

// Window

Window::Window(Text title, WidgetPtr content) : title_(std::move(title)) {
    scroll_ = make<ScrollPane>(std::move(content), true);
    buttons_ = Box::row();
    buttons_->responsive().justify(1);
    add(scroll_);
    add(buttons_);
}

Window& Window::button(std::shared_ptr<Button> b) {
    buttons_->add(std::move(b));
    return *this;
}

Window& Window::defaultButton(std::shared_ptr<Button> b) {
    default_ = b;
    b->accent_ = true;
    return button(std::move(b));
}

void Window::cancel(Ui& ui) {
    if (onCancel_) {
        onCancel_();
    } else {
        ui.close(this);
    }
}

float Window::titleH(Ui& ui) const { return titleBar_ ? ui.theme.s[Theme::TITLE_HEIGHT] : 0; }

bool Window::hasButtons() const {
    for (auto& k : buttons_->kids()) {
        if (k->isVisible()) {
            return true;
        }
    }
    return false;
}

float Window::prefWidth(Ui& ui) {
    float pad = ui.theme.s[Theme::PAD];
    float t = titleBar_ ? ui.theme.titleFont().width(title_()) + 30 : 0;
    float c = wantW_ > 0 ? wantW_ : std::max(scroll_->prefWidth(ui), buttons_->prefWidth(ui));
    return std::max(t, c) + 2 * pad;
}

float Window::minWidth(Ui& ui) {
    return std::max(scroll_->minWidth(ui), buttons_->minWidth(ui)) + 2 * ui.theme.s[Theme::PAD];
}

float Window::prefHeight(Ui& ui, float width) {
    float pad = ui.theme.s[Theme::PAD];
    float inner = width - 2 * pad;
    float bh = hasButtons() ? buttons_->prefHeight(ui, inner) + pad : 0;
    return titleH(ui) + pad + scroll_->prefHeight(ui, inner) + bh + pad;
}

void Window::place(Ui& ui) {
    float top = ui.contentTop();
    float maxW = ui.width * ui.theme.s[Theme::MAX_WINDOW];
    float maxH = (ui.height - top) * ui.theme.s[Theme::MAX_WINDOW];
    float pw = std::min(maxW, std::max(std::min(minWidth(ui), maxW), userW_ > 0 ? userW_ : prefWidth(ui)));
    float ph = userH_ > 0 ? std::min(maxH, std::max(minHeight(ui, pw), userH_)) : std::min(maxH, prefHeight(ui, pw));
    float px = x, py = y;
    if (!placed_) {
        px = (ui.width - pw) / 2;
        py = top + (ui.height - top - ph) * 0.42f;
        placed_ = true;
        open_ = 0;
    }
    px = std::max(0.0f, std::min(px, ui.width - pw));
    py = std::max(top, std::min(py, ui.height - ph));
    setBounds(std::round(px), std::round(py), std::round(pw), std::round(ph));
    layout(ui);
}

float Window::minHeight(Ui& ui, float width) {
    float pad = ui.theme.s[Theme::PAD];
    float bh = hasButtons() ? buttons_->prefHeight(ui, width - 2 * pad) + pad : 0;
    return titleH(ui) + 2 * pad + bh + 24;
}

namespace {
constexpr float EDGE = 4, GRIP = 10;
}

int Window::edgeAt(float mx, float my) const {
    if (!resizable_ || !contains(mx, my)) {
        return 0;
    }
    if (mx >= x + w - GRIP && my >= y + h - GRIP) {
        return 3;
    }
    return (mx >= x + w - EDGE ? 1 : 0) | (my >= y + h - EDGE ? 2 : 0);
}

Widget* Window::hit(float px, float py) {
    if (visible_ && edgeAt(px, py) != 0) {
        return this;
    }
    return Container::hit(px, py);
}

int Window::cursor(float mx, float my) const {
    int e = sizing_ ? sizing_ : edgeAt(mx, my);
    return e == 3 ? Cursor::RESIZE_DIAGONAL : e == 1 ? Cursor::RESIZE_H : e == 2 ? Cursor::RESIZE_V
           : dragging_ ? Cursor::MOVE : Cursor::ARROW;
}

void Window::layout(Ui& ui) {
    float pad = ui.theme.s[Theme::PAD];
    float inner = w - 2 * pad;
    float bh = hasButtons() ? buttons_->prefHeight(ui, inner) : 0;
    float th = titleH(ui);
    buttons_->setVisible(hasButtons());
    float sh = h - th - 2 * pad - (bh > 0 ? bh + pad : 0);
    scroll_->setBounds(x + pad, y + th + pad, inner, std::max(0.0f, sh));
    scroll_->layout(ui);
    buttons_->setBounds(x + pad, y + h - pad - bh, inner, bh);
    buttons_->layout(ui);
}

void Window::draw(Ui& ui, Painter& p) {
    using namespace Shapes;
    Theme& t = ui.theme;
    open_ = animate(ui, 0, 1, t.s[Theme::OPEN_TIME]);
    float before = p.opacity();
    p.setOpacity(before * open_);
    float rise = (1 - open_) * 6;
    float cutSize = t.s[Theme::CUT] * 2;
    Skin::panel(p, t, x, y + rise, w, h, cutSize, true);
    if (titleBar_) {
        float th = titleH(ui);
        auto tb = cut(x + 1, y + 1 + rise, w - 2, th - 1, cutSize - 1, 0, 0, 0);
        bool front = ui.topWindow() == this;
        poly(p, tb, front ? t.c[Theme::TITLE_TOP] : t.c[Theme::BUTTON_BOTTOM],
             front ? t.c[Theme::TITLE_BOTTOM] : t.c[Theme::PANEL_BOTTOM], y + rise, th);
        Skin::texture(p, t, tb, y + rise, th, 1.2f);
        hgradient(p, x + cutSize, y + th + rise, (w - cutSize) * 0.7f, 1, t.c[Theme::ACCENT_SOFT], 0);
        Font& f = t.titleFont();
        float ty = y + rise + (th - f.lineHeight()) / 2;
        auto s = Skin::fit(f, toU32(title_()), w - cutSize - 24);
        f.draw(p, s, x + cutSize + 3, ty + 0.5f, 0x80000000);
        f.draw(p, s, x + cutSize + 2, ty, front ? t.c[Theme::TITLE_TEXT] : t.c[Theme::TEXT_DIM]);
        if (closable_) {
            float cs = th - 4, cx = x + w - cs - 3, cy = y + 2 + rise;
            bool on = ui.mx >= cx && ui.mx < cx + cs && ui.my >= cy && ui.my < cy + cs && ui.hover() == this;
            float hv = animate(ui, 1, on ? 1 : 0, t.s[Theme::HOVER_TIME]);
            if (hv > 0) {
                fill(p, cx, cy, cs, cs, alpha(t.c[Theme::WARNING], 0.5f * hv));
            }
            argb xc = lerp(t.c[Theme::TEXT_DIM], 0xFFFFFFFF, hv);
            line(p, cx + 3, cy + 3, cx + cs - 3, cy + cs - 3, 1.3f, xc);
            line(p, cx + cs - 3, cy + 3, cx + 3, cy + cs - 3, 1.3f, xc);
        }
    }
    if (rise > 0.01f) {
        p.pushClip(x, y + rise, w, h);
    }
    Container::draw(ui, p);
    if (rise > 0.01f) {
        p.popClip();
    }
    if (resizable_) {
        bool on = sizing_ != 0 || (ui.hover() == this && edgeAt(ui.mx, ui.my) != 0);
        argb gc = on ? t.c[Theme::ACCENT] : alpha(t.c[Theme::ACCENT_SOFT], 0.7f);
        float gx = x + w - 2, gy = y + h - 2 + rise;
        for (int i = 1; i <= 3; i++) {
            line(p, gx - i * 3, gy, gx, gy - i * 3, 1, gc);
        }
    }
    p.setOpacity(before);
}

bool Window::onCloseButton(Ui& ui, float mx, float my) const {
    float th = titleH(ui), cs = th - 4, cx = x + w - cs - 3, cy = y + 2;
    return closable_ && titleBar_ && mx >= cx && mx < cx + cs && my >= cy && my < cy + cs;
}

bool Window::press(Ui& ui, float mx, float my, int button, int) {
    if (button != 0) {
        return true;
    }
    if (onCloseButton(ui, mx, my)) {
        closePressed_ = true;
        return true;
    }
    if (int e = edgeAt(mx, my)) {
        sizing_ = e;
        sizeDX_ = x + w - mx;
        sizeDY_ = y + h - my;
        userW_ = w;
        userH_ = h;
        return true;
    }
    if (titleBar_ && my < y + titleH(ui)) {
        dragging_ = true;
        dragDX_ = mx - x;
        dragDY_ = my - y;
    }
    return true;
}

void Window::drag(Ui& ui, float mx, float my) {
    if (sizing_) {
        if (sizing_ & 1) {
            userW_ = std::max(minWidth(ui), mx + sizeDX_ - x);
        }
        if (sizing_ & 2) {
            userH_ = std::max(minHeight(ui, w), my + sizeDY_ - y);
        }
        return;
    }
    if (dragging_) {
        x = mx - dragDX_;
        y = my - dragDY_;
    }
}

void Window::release(Ui& ui, float mx, float my, int) {
    bool close = closePressed_ && onCloseButton(ui, mx, my);
    closePressed_ = false;
    dragging_ = false;
    sizing_ = 0;
    if (close) {
        cancel(ui);
    }
}

bool Window::key(Ui& ui, int key, int) {
    if (Keys::isEnter(key) && default_ && default_->isEnabled()) {
        default_->click(ui);
        return true;
    }
    if (key == Keys::ESCAPE) {
        cancel(ui);
        return true;
    }
    return false;
}

void Window::children(std::vector<Widget*>& out) {
    out.push_back(scroll_.get());
    out.push_back(buttons_.get());
}

// Popup

void Popup::anchor(float ax, float ay, float aw, float ah, bool side) {
    ax_ = ax;
    ay_ = ay;
    aw_ = aw;
    ah_ = ah;
    side_ = side;
}

void Popup::place(Ui& ui) {
    float pw = std::min(ui.width, std::max(prefWidth(ui), side_ || !matchAnchor_ ? 0 : aw_));
    float ph = std::min(ui.height, prefHeight(ui, pw));
    float px, py;
    if (side_) {
        px = ax_ + aw_;
        if (px + pw > ui.width) {
            px = ax_ - pw;
        }
        py = std::min(ay_, ui.height - ph);
    } else {
        px = std::min(matchAnchor_ ? ax_ : ax_ + aw_ - pw, ui.width - pw);
        py = ay_ + ah_;
        if (py + ph > ui.height && ay_ - ph >= 0) {
            py = ay_ - ph;
        }
        py = std::min(py, ui.height - ph);
    }
    setBounds(std::round(std::max(0.0f, px)), std::round(std::max(0.0f, py)), std::round(pw), std::round(ph));
    layout(ui);
}

float Popup::opening(Ui& ui) { return animate(ui, 3, 1, ui.theme.s[Theme::OPEN_TIME] * 0.6f); }

bool Popup::key(Ui& ui, int key, int) {
    if (key == Keys::ESCAPE) {
        ui.closePopup(this);
        return true;
    }
    return false;
}

// Menus

std::shared_ptr<MenuItem> MenuItem::make(std::string label, std::function<void()> action) {
    auto it = std::make_shared<MenuItem>();
    it->label = text(std::move(label));
    it->action = std::move(action);
    return it;
}

std::shared_ptr<MenuItem> MenuItem::sub(std::shared_ptr<Menu> m) {
    auto it = std::make_shared<MenuItem>();
    it->label = m->label;
    it->submenu = std::move(m);
    return it;
}

std::shared_ptr<MenuItem> MenuItem::line() {
    auto it = std::make_shared<MenuItem>();
    it->label = [] { return std::string(); };
    it->separator = true;
    return it;
}

MenuItem& Menu::item(std::string label, std::function<void()> action) {
    items.push_back(MenuItem::make(std::move(label), std::move(action)));
    return *items.back();
}

float MenuBar::prefWidth(Ui& ui) { return ui.width; }
float MenuBar::prefHeight(Ui& ui, float) { return ui.barHeight(); }

void MenuBar::layout(Ui& ui) {
    Font& f = *ui.theme.font;
    titleX_.assign(menus_.size(), 0);
    titleW_.assign(menus_.size(), 0);
    float px = 6;
    for (size_t i = 0; i < menus_.size(); i++) {
        float tw = f.width(menus_[i]->label()) + 12;
        titleX_[i] = px;
        titleW_[i] = tw;
        px += tw;
    }
}

int MenuBar::titleAt(float mx, float my) const {
    if (my < y || my >= y + h) {
        return -1;
    }
    for (size_t i = 0; i < titleX_.size(); i++) {
        if (mx >= titleX_[i] && mx < titleX_[i] + titleW_[i]) {
            return int(i);
        }
    }
    return -1;
}

void MenuBar::draw(Ui& ui, Painter& p) {
    using namespace Shapes;
    Theme& t = ui.theme;
    vgradient(p, x, y, w, h, t.c[Theme::BAR_TOP], t.c[Theme::BAR_BOTTOM]);
    Skin::texture(p, t, cut(x, y, w, h, 0, 0, 0, 0), y, h, 1);
    hgradient(p, x, y + h - 1, w * 0.6f, 1, t.c[Theme::PANEL_BORDER], alpha(t.c[Theme::PANEL_BORDER], 0));
    Font& f = *t.font;
    int hot = titleAt(ui.mx, ui.my);
    for (size_t i = 0; i < menus_.size(); i++) {
        bool on = int(i) == open_;
        float target = on ? 1 : (int(i) == hot && ui.isHover(this)) ? 0.6f : 0;
        float hv = i < 4 ? animate(ui, int(i), target, t.s[Theme::HOVER_TIME]) : target;
        if (hv > 0) {
            auto pts = cut(titleX_[i], y + 1, titleW_[i], h - 2, 3, 0, 3, 0);
            poly(p, pts, alpha(t.c[Theme::HOVER_TOP], hv), alpha(t.c[Theme::HOVER_BOTTOM], hv), y, h);
            fill(p, titleX_[i] + 3, y + h - 2, titleW_[i] - 6, 1, alpha(t.c[Theme::ACCENT], hv));
        }
        f.draw(p, menus_[i]->label(), titleX_[i] + 6, y + (h - f.lineHeight()) / 2,
               lerp(t.c[Theme::TEXT], t.c[Theme::TEXT_BRIGHT], hv));
    }
}

void MenuBar::openMenu(Ui& ui, int i, bool keyboard) {
    ui.closePopups();
    auto& m = menus_[i];
    if (m->onOpen) {
        m->onOpen();
    }
    open_ = i;
    auto pop = make<MenuPopup>(m, this);
    pop->owner = this;
    pop->anchor(titleX_[i], y, titleW_[i], h, false);
    if (keyboard) {
        pop->selectFirst();
    }
    popup_ = pop;
    ui.openPopup(pop);
}

void MenuBar::step(Ui& ui, int d) {
    if (open_ >= 0 && !menus_.empty()) {
        openMenu(ui, (open_ + d + int(menus_.size())) % int(menus_.size()), true);
    }
}

bool MenuBar::openByKey(Ui& ui, int key) {
    if (menus_.empty()) {
        return false;
    }
    layout(ui);
    if (key == Keys::F10) {
        openMenu(ui, 0, true);
        return true;
    }
    for (size_t i = 0; i < menus_.size(); i++) {
        std::string l = menus_[i]->label();
        if (!l.empty() && std::toupper(static_cast<unsigned char>(l[0])) == key) {
            openMenu(ui, int(i), true);
            return true;
        }
    }
    return false;
}

bool MenuBar::press(Ui& ui, float mx, float my, int, int) {
    int i = titleAt(mx, my);
    if (i < 0 || i == open_) {
        ui.closePopups();
    } else {
        openMenu(ui, i, false);
    }
    return true;
}

void MenuBar::moved(Ui& ui, float mx, float my) {
    if (open_ < 0) {
        return;
    }
    int i = titleAt(mx, my);
    if (i >= 0 && i != open_) {
        openMenu(ui, i, false);
    }
}

void MenuBar::release(Ui& ui, float mx, float my, int) {
    if (auto p = popup_.lock()) {
        p->releaseAt(ui, mx, my);
    }
}

float MenuPopup::rowH(Ui& ui, const MenuItem& it) const { return it.separator ? 5 : ui.theme.s[Theme::ROW] + 1; }

float MenuPopup::prefWidth(Ui& ui) {
    Font& f = *ui.theme.font;
    float lw = 0, sw = 0;
    for (auto& it : menu_->items) {
        lw = std::max(lw, f.width(it->label()));
        if (!it->shortcutText.empty()) {
            sw = std::max(sw, f.width(it->shortcutText) + 12);
        }
    }
    return 16 + lw + sw + 18;
}

float MenuPopup::prefHeight(Ui& ui, float) {
    float s = 6;
    for (auto& it : menu_->items) {
        s += rowH(ui, *it);
    }
    return s;
}

void MenuPopup::selectFirst() {
    sel_ = -1;
    move(1);
}

void MenuPopup::move(int d) {
    int n = int(menu_->items.size());
    for (int k = 0; k < n; k++) {
        sel_ = (sel_ + d + n) % n;
        if (menu_->items[sel_]->selectable()) {
            return;
        }
    }
    sel_ = -1;
}

int MenuPopup::itemAt(Ui& ui, float mx, float my) {
    if (!contains(mx, my)) {
        return -1;
    }
    float py = y + 3 - scroll_;
    for (size_t i = 0; i < menu_->items.size(); i++) {
        float rh = rowH(ui, *menu_->items[i]);
        if (my >= py && my < py + rh) {
            return menu_->items[i]->separator ? -1 : int(i);
        }
        py += rh;
    }
    return -1;
}

float MenuPopup::itemY(Ui& ui, int index) {
    float py = y + 3 - scroll_;
    for (int i = 0; i < index; i++) {
        py += rowH(ui, *menu_->items[i]);
    }
    return py;
}

void MenuPopup::draw(Ui& ui, Painter& p) {
    using namespace Shapes;
    Theme& t = ui.theme;
    float o = opening(ui);
    float before = p.opacity();
    p.setOpacity(before * o);
    float drop = (1 - o) * -4;
    Skin::panel(p, t, x, y + drop, w, h, t.s[Theme::CUT], true);
    Font& f = *t.font;
    p.pushClip(x, y, w, h);
    float py = y + 3 - scroll_ + drop;
    for (size_t i = 0; i < menu_->items.size(); i++) {
        MenuItem& it = *menu_->items[i];
        float rh = rowH(ui, it);
        if (it.separator) {
            hgradient(p, x + 6, py + 2, (w - 12) / 2, 1, 0, t.c[Theme::PANEL_BORDER]);
            hgradient(p, x + w / 2, py + 2, (w - 12) / 2, 1, t.c[Theme::PANEL_BORDER], 0);
            py += rh;
            continue;
        }
        bool on = int(i) == sel_ && it.selectable();
        if (on) {
            poly(p, cut(x + 3, py, w - 6, rh, 3, 0, 3, 0), t.c[Theme::HOVER_TOP], t.c[Theme::HOVER_BOTTOM], py, rh);
            fill(p, x + 3, py, 2, rh, t.c[Theme::ACCENT]);
        }
        argb col = !it.enabled() ? t.c[Theme::TEXT_DISABLED] : on ? t.c[Theme::TEXT_BRIGHT] : t.c[Theme::TEXT];
        float ty = py + (rh - f.lineHeight()) / 2;
        if (it.checked && it.checked()) {
            if (it.radio) {
                float cx = x + 9, cy = py + rh / 2;
                Poly d = {cx, cy - 3, cx + 3, cy, cx, cy + 3, cx - 3, cy};
                poly(p, d, t.c[Theme::ACCENT], t.c[Theme::ACCENT], cy - 3, 6);
            } else {
                Skin::tick(p, x + 4, py + (rh - 9) / 2, 9, 1, t.c[Theme::ACCENT]);
            }
        }
        f.draw(p, it.label(), x + 16, ty, col);
        if (!it.shortcutText.empty()) {
            f.draw(p, it.shortcutText, x + w - 10 - f.width(it.shortcutText), ty, t.c[Theme::TEXT_DIM]);
        }
        if (it.submenu) {
            Skin::chevron(p, x + w - 8, py + rh / 2, 5, 1, col);
        }
        py += rh;
    }
    p.popClip();
    p.setOpacity(before);
}

void MenuPopup::hoverAt(Ui& ui, float mx, float my) {
    int hot = itemAt(ui, mx, my);
    if (hot >= 0 && hot != sel_) {
        sel_ = hot;
        openChild(ui);
    }
}

void MenuPopup::openChild(Ui& ui) {
    if (auto c = child_.lock()) {
        ui.closePopup(c.get());
        child_.reset();
    }
    if (sel_ < 0) {
        return;
    }
    MenuItem& it = *menu_->items[sel_];
    if (it.submenu && it.selectable()) {
        if (it.submenu->onOpen) {
            it.submenu->onOpen();
        }
        auto c = make<MenuPopup>(it.submenu, bar_);
        c->owner = this;
        c->anchor(x, itemY(ui, sel_) - 3, w, rowH(ui, it), true);
        child_ = c;
        ui.openPopup(c);
    }
}

void MenuPopup::run(Ui& ui, int i) {
    auto it = menu_->items[i];        // (kept: closing the popups may drop the menu)
    if (!it->selectable() || it->submenu) {
        return;
    }
    ui.closePopups();
    if (it->action) {
        it->action();
    }
}

void MenuPopup::releaseAt(Ui& ui, float mx, float my) {
    auto open = ui.popups();
    for (auto k = open.rbegin(); k != open.rend(); ++k) {
        if (auto* mp = dynamic_cast<MenuPopup*>(k->get()); mp && mp->contains(mx, my)) {
            int i = mp->itemAt(ui, mx, my);
            if (i >= 0) {
                mp->run(ui, i);
            }
            return;
        }
    }
}

bool MenuPopup::wheel(Ui& ui, float, float, float dy) {
    float over = prefHeight(ui, w) - h;
    if (over > 0) {
        scroll_ = std::clamp(scroll_ - dy * 12, 0.0f, over);
    }
    return true;
}

bool MenuPopup::key(Ui& ui, int key, int) {
    switch (key) {
        case Keys::DOWN: move(1); break;
        case Keys::UP: move(-1); break;
        case Keys::RIGHT:
            if (sel_ >= 0 && menu_->items[sel_]->submenu) {
                openChild(ui);
                if (auto c = child_.lock()) {
                    c->selectFirst();
                }
            } else if (bar_) {
                bar_->step(ui, 1);
            }
            break;
        case Keys::LEFT:
            if (dynamic_cast<MenuPopup*>(owner)) {
                ui.closePopup(this);
            } else if (bar_) {
                bar_->step(ui, -1);
            }
            break;
        case Keys::ENTER:
        case Keys::KP_ENTER:
        case Keys::SPACE:
            if (sel_ >= 0) {
                if (menu_->items[sel_]->submenu) {
                    openChild(ui);
                    if (auto c = child_.lock()) {
                        c->selectFirst();
                    }
                } else {
                    run(ui, sel_);
                }
            }
            break;
        case Keys::ESCAPE: ui.closePopup(this); break;
        default: return false;
    }
    return true;
}

// Ui

Ui::Ui(Theme t, Host* h) : theme(std::move(t)), host(h), menuBar(make<MenuBar>()) {}

void Ui::setBackground(WidgetPtr w) {
    background_ = std::move(w);
    dirty_ = true;
}

void Ui::setSize(float w, float h, float u) {
    if (w != width || h != height || u != unit) {
        width = w;
        height = h;
        unit = u;
        dirty_ = true;
    }
}

void Ui::setBarAutoHide(bool on) {
    barAutoHide_ = on;
    barShown_ = false;
    dirty_ = true;
}

bool Ui::barVisible() const { return !menuBar->isEmpty() && (!barAutoHide_ || barShown_ || menuBar->isOpen()); }

float Ui::contentTop() const { return menuBar->isEmpty() || barAutoHide_ ? 0 : barHeight(); }

bool Ui::needsRedraw() {
    bool d = dirty_ || animating;
    dirty_ = false;
    return d;
}

static bool within(const Widget* w, const Widget* root) {
    for (const Widget* t = w; t; t = t->parent()) {
        if (t == root) {
            return true;
        }
    }
    return false;
}

void Ui::open(std::shared_ptr<Window> w) {
    windows_.erase(std::remove(windows_.begin(), windows_.end(), w), windows_.end());
    windows_.push_back(w);
    w->recentre();
    closePopups();
    focusFirst(*w);
    dirty_ = true;
}

void Ui::close(Window* w) {
    auto it = std::find_if(windows_.begin(), windows_.end(), [&](auto& p) { return p.get() == w; });
    if (it == windows_.end()) {
        return;
    }
    auto keep = *it;
    windows_.erase(it);
    if (within(focused(), w)) {
        focus_.reset();
    }
    bury(keep);
    keep->closed();
    if (Window* top = topWindow()) {
        focusFirst(*top);
    }
    dirty_ = true;
}

bool Ui::isOpen(const Window* w) const {
    return std::any_of(windows_.begin(), windows_.end(), [&](auto& p) { return p.get() == w; });
}

Window* Ui::topWindow() const { return windows_.empty() ? nullptr : windows_.back().get(); }

Window* Ui::topModal() const {
    for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
        if ((*it)->isModal()) {
            return it->get();
        }
    }
    return nullptr;
}

void Ui::openPopup(std::shared_ptr<Popup> p) {
    popups_.push_back(std::move(p));
    dirty_ = true;
}

void Ui::closePopup(Popup* p) {
    auto it = std::find_if(popups_.begin(), popups_.end(), [&](auto& q) { return q.get() == p; });
    if (it != popups_.end()) {
        for (auto k = it; k != popups_.end(); ++k) {
            bury(*k);
        }
        popups_.erase(it, popups_.end());
    }
    if (popups_.empty()) {
        menuBar->closed();
    }
    dirty_ = true;
}

void Ui::closePopups() {
    for (auto& p : popups_) {
        bury(p);
    }
    popups_.clear();
    menuBar->closed();
    dirty_ = true;
}

void Ui::contextMenu(std::shared_ptr<Menu> m) {
    closePopups();
    auto p = make<MenuPopup>(std::move(m), nullptr);
    p->anchor(mx, my, 0, 0, false);
    openPopup(p);
}

void Ui::toast(std::string text) {
    toasts_.push_back({toU32(text), time});
    dirty_ = true;
}

void Ui::setFocus(Widget* w) {
    if (focused() != w) {
        focus_ = w ? w->weak_from_this() : std::weak_ptr<Widget>();
        dirty_ = true;
    }
}

bool Ui::isHover(const Widget* w) const {
    for (const Widget* t = hover(); t; t = t->parent()) {
        if (t == w) {
            return true;
        }
    }
    return false;
}

void Ui::collect(Widget* w, std::vector<Widget*>& out) {
    if (!w->isVisible() || !w->isEnabled()) {
        return;
    }
    if (w->focusable()) {
        out.push_back(w);
    }
    std::vector<Widget*> kids;
    w->children(kids);
    for (Widget* k : kids) {
        collect(k, out);
    }
}

void Ui::focusFirst(Window& w) {
    std::vector<Widget*> f;
    collect(&w, f);
    Widget* pick = nullptr;
    for (Widget* k : f) {
        if (dynamic_cast<TextField*>(k)) {
            pick = k;
            break;
        }
    }
    setFocus(pick);
}

void Ui::focusNext(bool back) {
    Widget* root = topWindow() ? static_cast<Widget*>(topWindow()) : background_.get();
    if (!root) {
        return;
    }
    std::vector<Widget*> f;
    collect(root, f);
    if (f.empty()) {
        return;
    }
    auto it = std::find(f.begin(), f.end(), focused());
    int n = int(f.size());
    int i = it == f.end() ? (back ? n - 1 : 0) : (int(it - f.begin()) + (back ? n - 1 : 1)) % n;
    setFocus(f[i]);
    revealFocus();
}

void Ui::revealFocus() {
    Widget* f = focused();
    for (Widget* t = f; t; t = t->parent()) {
        if (auto* sp = dynamic_cast<ScrollPane*>(t->parent())) {
            sp->reveal(*this, f->y, f->h);
        }
    }
}

void Ui::frame(float seconds) {
    graveyard_.clear();
    dt = std::min(0.1f, seconds);
    time += dt;
    animating = false;
    if (auto p = pressed_.lock()) {
        p->tick(*this);
    }
    auto hv = hover_.lock();
    if (hv && hv == tipWidget_.lock() && pressed_.expired()) {
        float before = tipRest_;
        tipRest_ += dt;
        float delay = theme.s[Theme::TOOLTIP_DELAY];
        if (before < delay && tipRest_ >= delay) {
            dirty_ = true;
        }
    }
    toasts_.erase(std::remove_if(toasts_.begin(), toasts_.end(), [&](auto& t) { return time - t.at > 3.2; }),
                  toasts_.end());
    if (!toasts_.empty()) {
        animating = true;
    }
    if (host) {
        auto c = pressed_.lock();
        if (!c) {
            c = hv;
        }
        host->setCursor(c ? c->cursor(mx, my) : Cursor::ARROW);
    }
}

void Ui::layout() {
    menuBar->setBounds(0, 0, width, barHeight());
    menuBar->layout(*this);
    if (background_) {
        float top = contentTop();
        background_->setBounds(0, top, width, height - top);
        background_->layout(*this);
    }
    for (auto& w : std::vector(windows_)) {
        w->place(*this);
    }
    for (auto& p : std::vector(popups_)) {
        p->place(*this);
    }
}

void Ui::draw(Painter& p) {
    layout();
    if (background_) {
        background_->draw(*this, p);
    }
    Window* modal = topModal();
    if (barVisible() && !barAutoHide_) {
        menuBar->draw(*this, p);
    }
    for (auto& w : std::vector(windows_)) {
        if (w.get() == modal) {
            Shapes::fill(p, 0, 0, width, height, theme.c[Theme::SHADE]);
        }
        w->draw(*this, p);
    }
    if (barVisible() && barAutoHide_) {
        menuBar->draw(*this, p);
    }
    for (auto& pp : std::vector(popups_)) {
        pp->draw(*this, p);
    }
    drawToasts(p);
    drawTooltip(p);
}

void Ui::drawToasts(Painter& p) {
    float ty0 = height - 12;
    Font& f = *theme.font;
    for (auto it = toasts_.rbegin(); it != toasts_.rend(); ++it) {
        double age = time - it->at;
        float a = float(std::min(1.0, std::min(age / 0.15, (3.2 - age) / 0.5)));
        float tw = f.width(it->text) + 24, th = f.lineHeight() + 10;
        float tx = (width - tw) / 2, ty = ty0 - th + (1 - a) * 8;
        float before = p.opacity();
        p.setOpacity(before * a);
        Skin::panel(p, theme, tx, ty, tw, th, theme.s[Theme::CUT], true);
        Shapes::fill(p, tx + 4, ty + th - 3, tw - 8, 1, Shapes::alpha(theme.c[Theme::ACCENT], 0.6f));
        f.draw(p, it->text, tx + 12, ty + 5, theme.c[Theme::TEXT_BRIGHT]);
        p.setOpacity(before);
        ty0 = ty - 4;
    }
}

void Ui::drawTooltip(Painter& p) {
    auto tw_ = tipWidget_.lock();
    float delay = theme.s[Theme::TOOLTIP_DELAY];
    if (!tw_ || !tw_->tooltip || !pressed_.expired() || tipRest_ < delay) {
        return;
    }
    std::u32string text = toU32(tw_->tooltip());
    if (text.empty()) {
        return;
    }
    Font& f = *theme.font;
    float a = std::min(1.0f, (tipRest_ - delay) / 0.12f);
    if (a < 1) {
        animating = true;
    }
    float tw = f.width(text) + 12, th = f.lineHeight() + 8;
    float tx = std::max(0.0f, std::min(tipX_ + 4, width - tw));
    float ty = tipY_ + 14;
    if (ty + th > height) {
        ty = tipY_ - th - 4;
    }
    float before = p.opacity();
    p.setOpacity(before * a);
    auto pts = Shapes::cut(tx, ty, tw, th, 3, 0, 3, 0);
    Shapes::ring(p, pts, Shapes::offset(pts, 5), theme.c[Theme::SHADOW], 0);
    Shapes::poly(p, pts, theme.c[Theme::TOOLTIP_TOP], theme.c[Theme::TOOLTIP_BOTTOM], ty, th);
    Shapes::outline(p, pts, 1, theme.c[Theme::ACCENT_SOFT]);
    f.draw(p, text, tx + 6, ty + 4, theme.c[Theme::TEXT_BRIGHT]);
    p.setOpacity(before);
}

Widget* Ui::hitAt(float hx, float hy) {
    for (auto it = popups_.rbegin(); it != popups_.rend(); ++it) {
        if (Widget* h = (*it)->hit(hx, hy)) {
            return h;
        }
    }
    Window* modal = topModal();
    if (barVisible() && (!modal || (barAutoHide_ && barShown_)) && hy < barHeight()) {
        if (Widget* h = menuBar->hit(hx, hy)) {
            return h;
        }
    }
    for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
        if (Widget* h = (*it)->hit(hx, hy)) {
            return h;
        }
        if (it->get() == modal) {
            return nullptr;
        }
    }
    return background_ ? background_->hit(hx, hy) : nullptr;
}

static Window* windowOf(Widget* w) {
    for (Widget* t = w; t; t = t->parent()) {
        if (auto* win = dynamic_cast<Window*>(t)) {
            return win;
        }
    }
    return nullptr;
}

static bool inPopup(Widget* w) {
    for (Widget* t = w; t; t = t->parent()) {
        if (dynamic_cast<Popup*>(t)) {
            return true;
        }
    }
    return false;
}

void Ui::mouseMoved(float x, float y) {
    if (std::abs(x - mx) > 2 || std::abs(y - my) > 2) {
        tipRest_ = 0;
        tipX_ = x;
        tipY_ = y;
    }
    mx = x;
    my = y;
    if (barAutoHide_) {
        bool show = y <= 1 || (barShown_ && (y < barHeight() || menuBar->isOpen()));
        if (show != barShown_) {
            barShown_ = show;
            dirty_ = true;
        }
    }
    Widget* h = hitAt(x, y);
    if (h != hover()) {
        hover_ = h ? h->weak_from_this() : std::weak_ptr<Widget>();
        Widget* t = h;
        while (t && !t->tooltip) {
            t = t->parent();
        }
        tipWidget_ = t ? t->weak_from_this() : std::weak_ptr<Widget>();
        tipRest_ = 0;
        tipX_ = x;
        tipY_ = y;
    }
    if (auto p = pressed_.lock()) {
        p->drag(*this, x, y);
    }
    for (Widget* t = h; t; t = t->parent()) {
        if (auto* mp = dynamic_cast<MenuPopup*>(t)) {
            mp->hoverAt(*this, x, y);
            break;
        }
    }
    menuBar->moved(*this, x, y);
    dirty_ = true;
}

void Ui::mousePressed(int button) {
    dirty_ = true;
    tipRest_ = -1000;
    if (time - lastPress_ < 0.4 && std::abs(mx - lastPressX_) < 4 && std::abs(my - lastPressY_) < 4) {
        clicks_++;
    } else {
        clicks_ = 1;
    }
    lastPress_ = time;
    lastPressX_ = mx;
    lastPressY_ = my;
    Widget* h = hitAt(mx, my);
    if (!popups_.empty() && !inPopup(h) && h != menuBar.get()) {
        closePopups();      // (the press only closes the popups)
        return;
    }
    Window* win = windowOf(h);
    if (win && win != topWindow() && topModal() != topWindow()) {
        auto it = std::find_if(windows_.begin(), windows_.end(), [&](auto& p) { return p.get() == win; });
        auto keep = *it;
        windows_.erase(it);
        windows_.push_back(keep);
    }
    if (!inPopup(h)) {
        Widget* f = h;
        while (f && !f->focusable()) {
            f = f->parent();
        }
        setFocus(f);
    }
    for (Widget* t = h; t; t = t->parent()) {
        if (t->isEnabled() && t->press(*this, mx, my, button, clicks_)) {
            pressed_ = t->weak_from_this();
            pressedButton_ = button;
            return;
        }
    }
}

void Ui::mouseReleased(int button) {
    dirty_ = true;
    if (auto p = pressed_.lock(); p && button == pressedButton_) {
        pressed_.reset();
        pressedButton_ = -1;
        p->release(*this, mx, my, button);
    }
    Widget* h = hitAt(mx, my);
    hover_ = h ? h->weak_from_this() : std::weak_ptr<Widget>();
}

void Ui::wheel(float dy) {
    dirty_ = true;
    for (Widget* t = hitAt(mx, my); t; t = t->parent()) {
        if (t->isEnabled() && t->wheel(*this, mx, my, dy)) {
            return;
        }
    }
}

void Ui::key(int key, int m) {
    dirty_ = true;
    if (!popups_.empty()) {
        auto top = popups_.back();
        if (top->key(*this, key, m)) {
            return;
        }
        if (key == Keys::ESCAPE || !focused() || top->owner != focused()) {
            if (key == Keys::ESCAPE) {
                closePopup(top.get());
            }
            return;
        }
        // (a list under a focused field, a combobox's: the other keys edit the field)
    }
    for (Widget* t = focused(); t; t = t->parent()) {
        if (t->isEnabled() && t->key(*this, key, m)) {
            return;
        }
    }
    Window* top = topWindow();
    if (key == Keys::TAB && (top || focused())) {
        focusNext((m & Keys::SHIFT) != 0);
        return;
    }
    if (top && !within(focused(), top) && top->key(*this, key, m)) {
        return;
    }
    if (Widget* f = focused(); f && f->takesText() && !(m & (Keys::CTRL | Keys::ALT)) && key < Keys::ESCAPE) {
        return;     // (typed into the field: not a shortcut or a key for the game)
    }
    if (!top || !top->isModal()) {
        if (key == Keys::F10 || ((m & Keys::ALT) && key >= Keys::A && key <= Keys::Z)) {
            if (menuBar->openByKey(*this, key)) {
                if (barAutoHide_) {
                    barShown_ = true;
                }
                return;
            }
        }
    }
    for (auto& [k, r] : shortcuts_) {
        if (k == key) {
            r();
            return;
        }
    }
    if (background_ && (!top || !top->isModal())) {
        background_->key(*this, key, m);
    }
}

void Ui::typed(char32_t cp) {
    dirty_ = true;
    Widget* f = focused();
    if (f && (popups_.empty() || popups_.back()->owner == f)) {
        f->typed(*this, cp);
    }
}

std::shared_ptr<Window> Ui::message(std::string title, std::string text, std::function<void()> done) {
    auto win = std::make_shared<std::weak_ptr<Window>>();
    auto ok = make<Button>("OK", [this, win, done] {
        if (auto w = win->lock()) {
            close(w.get());
        }
        if (done) {
            done();
        }
    });
    float tw = std::min(360.0f, theme.font->width(text) + 20);
    auto w = make<Window>(std::move(title), make<Label>(std::move(text)));
    w->width(tw).defaultButton(ok);
    *win = w;
    open(w);
    return w;
}

std::shared_ptr<Window> Ui::confirm(std::string title, std::string text, std::string yes, std::string no,
                                    std::function<void()> onYes, bool yesDefault) {
    auto win = std::make_shared<std::weak_ptr<Window>>();
    auto y = make<Button>(std::move(yes), [this, win, onYes] {
        if (auto w = win->lock()) {
            close(w.get());
        }
        onYes();
    });
    auto n = make<Button>(std::move(no), [this, win] {
        if (auto w = win->lock()) {
            close(w.get());
        }
    });
    float tw = std::min(360.0f, theme.font->width(text) + 20);
    auto w = make<Window>(std::move(title), make<Label>(std::move(text)));
    w->width(tw);
    if (yesDefault) {
        w->button(n).defaultButton(y);
    } else {
        w->button(y).defaultButton(n);
    }
    *win = w;
    open(w);
    return w;
}

std::shared_ptr<Window> Ui::ask(std::string title, std::string label, std::string initial,
                                std::function<void(const std::string&)> onOk) {
    return ask(std::move(title), std::move(label), std::move(initial), "OK", "Cancel", {}, std::move(onOk));
}

std::shared_ptr<Window> Ui::ask(std::string title, std::string label, std::string initial, std::string okLabel,
                                std::string cancelLabel, std::function<std::string(const std::string&)> check,
                                std::function<void(const std::string&)> onOk) {
    auto win = std::make_shared<std::weak_ptr<Window>>();
    auto field = make<TextField>(std::move(initial));
    auto warning = make<Label>("");
    warning->warning();
    warning->setVisible(false);
    std::weak_ptr<Label> ww = warning;
    std::weak_ptr<TextField> fw = field;
    auto ok = make<Button>(std::move(okLabel), [this, win, fw, ww, check, onOk] {
        auto f = fw.lock();
        if (!f) {
            return;
        }
        std::string text = f->text();
        std::string problem = check ? check(text) : std::string();
        if (!problem.empty()) {
            if (auto wl = ww.lock()) {
                wl->set(problem);
                wl->setVisible(true);
            }
            return;
        }
        if (auto w = win->lock()) {
            close(w.get());
        }
        onOk(text);
    });
    field->onChange([ww](const std::string&) {
        if (auto wl = ww.lock()) {
            wl->setVisible(false);
        }
    });
    auto cancel = make<Button>(std::move(cancelLabel), [this, win] {
        if (auto w = win->lock()) {
            close(w.get());
        }
    });
    auto w = make<Window>(std::move(title), Box::column({make<Label>(std::move(label)), field, warning}));
    w->width(260).button(cancel).defaultButton(ok);
    *win = w;
    open(w);
    setFocus(field.get());
    field->selectAll();
    return w;
}

}  // namespace slopui
