// SlopUI: Dropdown, ListPopup, Table, SplitPane, Tabs (Java: the classes of the same names).
#include <algorithm>
#include <cmath>
#include <numeric>

#include "slopui/ui.hpp"

namespace slopui {

using namespace Shapes;

// ListPopup

ListPopup::ListPopup(Widget& o, std::vector<std::string> items, int selected, std::function<void(int)> onPick)
    : onPick_(std::move(onPick)), sel_(selected) {
    owner = &o;
    for (auto& s : items) {
        items_.push_back(toU32(s));
    }
    anchor(o.x, o.y, o.w, o.h, false);
}

static constexpr int MAX_ROWS = 10;

float ListPopup::rowH(Ui& ui) const { return ui.theme.s[Theme::ROW] + 1; }

float ListPopup::prefWidth(Ui& ui) {
    float m = 0;
    for (auto& s : items_) {
        m = std::max(m, ui.theme.font->width(s));
    }
    return m + 16;
}

float ListPopup::prefHeight(Ui& ui, float) { return std::min(int(items_.size()), MAX_ROWS) * rowH(ui) + 4; }

int ListPopup::at(Ui& ui, float my) const {
    int i = int(std::floor((my - y - 2 + scroll_) / rowH(ui)));
    return i >= 0 && i < int(items_.size()) ? i : -1;
}

void ListPopup::reveal(Ui& ui) {
    float rh = rowH(ui), view = h - 4;
    if (sel_ * rh < scroll_) {
        scroll_ = sel_ * rh;
    }
    if ((sel_ + 1) * rh > scroll_ + view) {
        scroll_ = (sel_ + 1) * rh - view;
    }
}

void ListPopup::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    float o = opening(ui);
    float before = p.opacity();
    p.setOpacity(before * o);
    Skin::panel(p, t, x, y, w, h * (0.6f + 0.4f * o), t.s[Theme::CUT], true);
    if (contains(ui.mx, ui.my) && ui.isHover(this)) {
        int i = at(ui, ui.my);
        if (i >= 0) {
            sel_ = i;
        }
    }
    Font& f = *t.font;
    float rh = rowH(ui);
    p.pushClip(x + 1, y + 2, w - 2, h - 4);
    for (int i = 0; i < int(items_.size()); i++) {
        float ry = y + 2 + i * rh - scroll_;
        if (ry + rh < y || ry > y + h) {
            continue;
        }
        if (i == sel_) {
            poly(p, cut(x + 2, ry, w - 4, rh, 3, 0, 3, 0), t.c[Theme::HOVER_TOP], t.c[Theme::HOVER_BOTTOM], ry, rh);
            fill(p, x + 2, ry, 2, rh, t.c[Theme::ACCENT]);
        }
        f.draw(p, items_[i], x + 8, ry + (rh - f.lineHeight()) / 2, i == sel_ ? t.c[Theme::TEXT_BRIGHT] : t.c[Theme::TEXT]);
    }
    p.popClip();
    float total = items_.size() * rh, view = h - 4;
    if (total > view) {
        float mh = std::max(8.0f, view * view / total), my = y + 2 + (view - mh) * scroll_ / (total - view);
        fill(p, x + w - 3, my, 2, mh, t.c[Theme::ACCENT_SOFT]);
    }
    p.setOpacity(before);
}

void ListPopup::release(Ui& ui, float mx, float my, int) {
    int i = contains(mx, my) ? at(ui, my) : -1;
    if (i >= 0) {
        auto pick = onPick_;
        ui.closePopup(this);
        pick(i);
    }
}

bool ListPopup::wheel(Ui& ui, float, float, float dy) {
    float max = std::max(0.0f, items_.size() * rowH(ui) - (h - 4));
    scroll_ = std::clamp(scroll_ - dy * rowH(ui) * 2, 0.0f, max);
    return true;
}

bool ListPopup::key(Ui& ui, int key, int) {
    int n = int(items_.size());
    switch (key) {
        case Keys::DOWN: sel_ = std::min(n - 1, sel_ + 1); break;
        case Keys::UP: sel_ = std::max(0, sel_ - 1); break;
        case Keys::PAGE_DOWN: sel_ = std::min(n - 1, sel_ + MAX_ROWS); break;
        case Keys::PAGE_UP: sel_ = std::max(0, sel_ - MAX_ROWS); break;
        case Keys::HOME: sel_ = 0; break;
        case Keys::END: sel_ = n - 1; break;
        case Keys::ENTER:
        case Keys::KP_ENTER:
        case Keys::SPACE: {
            auto pick = onPick_;
            int s = sel_;
            ui.closePopup(this);
            if (s >= 0) {
                pick(s);
            }
            return true;
        }
        case Keys::ESCAPE: ui.closePopup(this); return true;
        default: return false;
    }
    reveal(ui);
    return true;
}

// Dropdown

void Dropdown::select(int i) {
    if (i != selected_ && i >= 0 && i < int(items_.size())) {
        selected_ = i;
        if (onChange_) {
            onChange_(i);
        }
    }
}

float Dropdown::prefWidth(Ui& ui) {
    float m = 0;
    for (auto& s : items_) {
        m = std::max(m, ui.theme.font->width(s));
    }
    return m + 26;
}

float Dropdown::prefHeight(Ui& ui, float) { return ui.theme.s[Theme::BUTTON_HEIGHT]; }

void Dropdown::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool open = ui.popupOpen() && ui.popups().back()->owner == this;
    float hv = animate(ui, 0, isEnabled() && (ui.isHover(this) || open) ? 1 : 0, t.s[Theme::HOVER_TIME]);
    Skin::button(p, t, x, y, w, h, hv * 0.7f, down_ ? 0.5f : 0, isEnabled(), false, ui.isFocused(this));
    Font& f = *t.font;
    std::u32string s = selected_ >= 0 && selected_ < int(items_.size()) ? toU32(items_[selected_]) : U"";
    s = Skin::fit(f, s, w - 22);
    f.draw(p, s, x + 7, y + (h - f.lineHeight()) / 2, isEnabled() ? t.c[Theme::TEXT_BRIGHT] : t.c[Theme::TEXT_DISABLED]);
    fill(p, x + w - 15, y + 3, 1, h - 6, alpha(t.c[Theme::BUTTON_BORDER], 0.6f));
    Skin::chevron(p, x + w - 8, y + h / 2, 6, open ? 2 : 0, lerp(t.c[Theme::TEXT], t.c[Theme::ACCENT], hv));
}

void Dropdown::openList(Ui& ui) {
    ui.openPopup(make<ListPopup>(*this, items_, selected_, [this](int i) { select(i); }));
}

bool Dropdown::press(Ui& ui, float, float, int button, int) {
    if (button != 0) {
        return false;
    }
    down_ = true;
    openList(ui);
    return true;
}

bool Dropdown::key(Ui& ui, int key, int) {
    switch (key) {
        case Keys::DOWN: select(std::min(int(items_.size()) - 1, selected_ + 1)); break;
        case Keys::UP: select(std::max(0, selected_ - 1)); break;
        case Keys::SPACE:
        case Keys::ENTER:
        case Keys::KP_ENTER: openList(ui); break;
        default: return false;
    }
    return true;
}

// Table

Table::Table(std::vector<std::string> headers, std::vector<float> weights, int visibleRows)
    : headers_(std::move(headers)), weights_(std::move(weights)), visibleRows_(visibleRows) {}

void Table::setRows(std::vector<std::vector<std::string>> rows) {
    rows_ = std::move(rows);
    selected_ = -1;
    resort();
}

void Table::select(int row) {
    if (row != selected_) {
        selected_ = row;
        if (onSelect_) {
            onSelect_(row);
        }
    }
}

static int compareCells(const std::string& a, const std::string& b) {
    auto num = [](const std::string& s, double& v) {
        std::string t;
        for (char c : s) {
            if (c != ',') {
                t += c;
            }
        }
        try {
            size_t used = 0;
            v = std::stod(t, &used);
            return used == t.size() && !t.empty();
        } catch (...) {
            return false;
        }
    };
    double x, y;
    if (num(a, x) && num(b, y)) {
        return x < y ? -1 : x > y ? 1 : 0;
    }
    for (size_t i = 0; i < std::min(a.size(), b.size()); i++) {
        int ca = std::tolower(static_cast<unsigned char>(a[i])), cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) {
            return ca < cb ? -1 : 1;
        }
    }
    return a.size() < b.size() ? -1 : a.size() > b.size() ? 1 : 0;
}

void Table::resort() {
    order_.resize(rows_.size());
    std::iota(order_.begin(), order_.end(), 0);
    if (sortColumn_ >= 0) {
        std::stable_sort(order_.begin(), order_.end(), [&](int a, int b) {
            int c = compareCells(rows_[a][sortColumn_], rows_[b][sortColumn_]);
            return descending_ ? c > 0 : c < 0;
        });
    }
}

float Table::rowH(Ui& ui) const { return ui.theme.s[Theme::ROW] + 1; }
float Table::headH(Ui& ui) const { return ui.theme.s[Theme::ROW] + 3; }

float Table::prefWidth(Ui& ui) {
    float s = 0;
    for (size_t c = 0; c < headers_.size(); c++) {
        float m = ui.theme.font->width(headers_[c]) + 16;
        for (auto& r : rows_) {
            m = std::max(m, ui.theme.font->width(r[c]) + 10);
        }
        s += std::min(m, 220.0f);
    }
    return s + ui.theme.s[Theme::SCROLLBAR];
}

float Table::prefHeight(Ui& ui, float) { return headH(ui) + visibleRows_ * rowH(ui) + 2; }

std::vector<float> Table::colX(Ui& ui) const {
    float inner = w - ui.theme.s[Theme::SCROLLBAR] - 2;
    float sum = std::accumulate(weights_.begin(), weights_.end(), 0.0f);
    std::vector<float> cx(headers_.size() + 1);
    cx[0] = x + 1;
    for (size_t c = 0; c < headers_.size(); c++) {
        cx[c + 1] = cx[c] + inner * weights_[c] / sum;
    }
    return cx;
}

float Table::viewH(Ui& ui) const { return h - headH(ui) - 2; }

void Table::clampScroll(Ui& ui) {
    scroll_ = std::clamp(scroll_, 0.0f, std::max(0.0f, rows_.size() * rowH(ui) - viewH(ui)));
}

void Table::reveal(Ui& ui) {
    auto it = std::find(order_.begin(), order_.end(), selected_);
    if (it == order_.end()) {
        return;
    }
    int pos = int(it - order_.begin());
    float rh = rowH(ui);
    if (pos * rh < scroll_) {
        scroll_ = pos * rh;
    }
    if ((pos + 1) * rh > scroll_ + viewH(ui)) {
        scroll_ = (pos + 1) * rh - viewH(ui);
    }
    clampScroll(ui);
}

std::array<float, 2> Table::thumb(Ui& ui) const {
    float total = rows_.size() * rowH(ui), view = viewH(ui);
    float len = std::max(10.0f, view * view / total);
    return {y + headH(ui) + 1 + (view - len) * scroll_ / std::max(1.0f, total - view), len};
}

int Table::rowAt(Ui& ui, float mx, float my) const {
    float top = y + headH(ui) + 1;
    if (my < top || my >= y + h || mx >= x + w - ui.theme.s[Theme::SCROLLBAR] - 1) {
        return -1;
    }
    int pos = int(std::floor((my - top + scroll_) / rowH(ui)));
    return pos >= 0 && pos < int(order_.size()) ? order_[pos] : -1;
}

void Table::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    Font& f = *t.font;
    clampScroll(ui);
    Skin::field(p, t, x, y, w, h, ui.isFocused(this) ? 0.6f : 0, isEnabled());
    auto cx = colX(ui);
    float hh = headH(ui);
    size_t nc = headers_.size();
    vgradient(p, x + 1, y + 1, w - 2, hh - 1, t.c[Theme::BUTTON_TOP], t.c[Theme::BUTTON_BOTTOM]);
    fill(p, x + 1, y + hh, w - 2, 1, t.c[Theme::PANEL_BORDER]);
    for (size_t c = 0; c < nc; c++) {
        bool on = ui.isHover(this) && ui.my < y + hh && ui.mx >= cx[c] && ui.mx < cx[c + 1];
        if (on) {
            vgradient(p, cx[c], y + 1, cx[c + 1] - cx[c], hh - 1, t.c[Theme::HOVER_TOP], t.c[Theme::HOVER_BOTTOM]);
        }
        auto s = Skin::fit(f, toU32(headers_[c]), cx[c + 1] - cx[c] - 14);
        f.draw(p, s, cx[c] + 4, y + (hh - f.lineHeight()) / 2, int(c) == sortColumn_ ? t.c[Theme::ACCENT] : t.c[Theme::TEXT_BRIGHT]);
        if (int(c) == sortColumn_) {
            Skin::chevron(p, cx[c + 1] - 7, y + hh / 2, 5, descending_ ? 0 : 2, t.c[Theme::ACCENT]);
        }
        if (c > 0) {
            fill(p, cx[c], y + 2, 1, hh - 3, alpha(t.c[Theme::PANEL_BORDER], 0.7f));
        }
    }
    float rh = rowH(ui), top = y + hh + 1;
    p.pushClip(x + 1, top, w - 2, viewH(ui));
    int hot = rowAt(ui, ui.mx, ui.my);
    for (size_t pos = 0; pos < order_.size(); pos++) {
        float ry = top + pos * rh - scroll_;
        if (ry + rh < top || ry > y + h) {
            continue;
        }
        int r = order_[pos];
        float rw = cx[nc] - x - 1;
        if (r == selected_) {
            vgradient(p, x + 1, ry, rw, rh, t.c[Theme::SELECTION], lerp(t.c[Theme::SELECTION], 0xFF000000, 0.3f));
            fill(p, x + 1, ry, 2, rh, t.c[Theme::ACCENT]);
        } else if (r == hot && ui.isHover(this)) {
            fill(p, x + 1, ry, rw, rh, t.c[Theme::ROW_HOVER]);
        } else if (pos % 2 == 1) {
            fill(p, x + 1, ry, rw, rh, t.c[Theme::ROW_ALT]);
        }
        for (size_t c = 0; c < nc; c++) {
            auto s = Skin::fit(f, toU32(rows_[r][c]), cx[c + 1] - cx[c] - 8);
            f.draw(p, s, cx[c] + 4, ry + (rh - f.lineHeight()) / 2, r == selected_ ? t.c[Theme::SELECTION_TEXT] : t.c[Theme::TEXT]);
        }
    }
    if (rows_.empty()) {
        f.draw(p, "(empty)", x + (w - f.width("(empty)")) / 2, top + 6, t.c[Theme::TEXT_DIM]);
    }
    p.popClip();
    float total = rows_.size() * rh, view = viewH(ui), bw = t.s[Theme::SCROLLBAR], bx = x + w - bw - 1;
    vgradient(p, bx, top, bw, view, t.c[Theme::TRACK_TOP], t.c[Theme::TRACK_BOTTOM]);
    if (total > view) {
        auto th = thumb(ui);
        auto pts = cut(bx + 1, th[0], bw - 2, th[1], 0, 0, 0, 0);
        poly(p, pts, t.c[Theme::BUTTON_TOP], t.c[Theme::BUTTON_BOTTOM], th[0], th[1]);
        outline(p, pts, 1, t.c[Theme::BUTTON_BORDER]);
    }
}

bool Table::press(Ui& ui, float mx, float my, int button, int clicks) {
    if (button != 0) {
        return false;
    }
    if (mx >= x + w - ui.theme.s[Theme::SCROLLBAR] - 1 && my > y + headH(ui)) {
        dragThumb_ = true;
        dragFrom_ = my;
        dragStart_ = scroll_;
        return true;
    }
    if (my < y + headH(ui)) {
        auto cx = colX(ui);
        for (size_t c = 0; c < headers_.size(); c++) {
            if (mx >= cx[c] && mx < cx[c + 1]) {
                descending_ = sortColumn_ == int(c) && !descending_;
                sortColumn_ = int(c);
                resort();
            }
        }
        return true;
    }
    int r = rowAt(ui, mx, my);
    if (r >= 0) {
        select(r);
        if (clicks == 2 && onActivate_) {
            onActivate_(r);
        }
    }
    return true;
}

void Table::drag(Ui& ui, float, float my) {
    if (dragThumb_) {
        float total = rows_.size() * rowH(ui), view = viewH(ui), len = thumb(ui)[1];
        scroll_ = dragStart_ + (my - dragFrom_) * (total - view) / std::max(1.0f, view - len);
        clampScroll(ui);
    }
}

bool Table::wheel(Ui& ui, float, float, float dy) {
    scroll_ -= dy * rowH(ui) * 3;
    clampScroll(ui);
    return true;
}

bool Table::key(Ui& ui, int key, int) {
    if (order_.empty()) {
        return false;
    }
    auto it = std::find(order_.begin(), order_.end(), selected_);
    int pos = it == order_.end() ? 0 : int(it - order_.begin());
    int page = std::max(1, int(viewH(ui) / rowH(ui)) - 1);
    switch (key) {
        case Keys::DOWN: pos = selected_ < 0 ? 0 : pos + 1; break;
        case Keys::UP: pos -= 1; break;
        case Keys::PAGE_DOWN: pos += page; break;
        case Keys::PAGE_UP: pos -= page; break;
        case Keys::HOME: pos = 0; break;
        case Keys::END: pos = int(order_.size()) - 1; break;
        case Keys::ENTER:
        case Keys::KP_ENTER:
            if (selected_ >= 0 && onActivate_) {
                onActivate_(selected_);
                return true;
            }
            return false;
        default: return false;
    }
    select(order_[std::clamp(pos, 0, int(order_.size()) - 1)]);
    reveal(ui);
    return true;
}

// SplitPane

SplitPane::SplitPane(WidgetPtr a, WidgetPtr b, bool vertical, float ratio)
    : a_(std::move(a)), b_(std::move(b)), vertical_(vertical), ratio_(ratio), initial_(ratio) {
    adopt(*a_);
    adopt(*b_);
}

float SplitPane::bar(Ui& ui) const { return ui.theme.s[Theme::SPLITTER]; }

float SplitPane::prefWidth(Ui& ui) {
    return vertical_ ? std::max(a_->prefWidth(ui), b_->prefWidth(ui)) : a_->prefWidth(ui) + b_->prefWidth(ui) + bar(ui);
}

float SplitPane::minWidth(Ui& ui) {
    return vertical_ ? std::max(a_->minWidth(ui), b_->minWidth(ui)) : a_->minWidth(ui) + b_->minWidth(ui) + bar(ui);
}

float SplitPane::prefHeight(Ui& ui, float width) {
    if (vertical_) {
        return a_->prefHeight(ui, width) + b_->prefHeight(ui, width) + bar(ui);
    }
    float aw = (width - bar(ui)) * ratio_;
    return std::max(a_->prefHeight(ui, aw), b_->prefHeight(ui, width - bar(ui) - aw));
}

float SplitPane::split(Ui& ui) {
    float total = (vertical_ ? h : w) - bar(ui);
    float s = total * ratio_;
    float minA = vertical_ ? 20 : a_->minWidth(ui), minB = vertical_ ? 20 : b_->minWidth(ui);
    if (minA + minB <= total) {
        s = std::clamp(s, minA, total - minB);
    }
    return std::round(s);
}

void SplitPane::layout(Ui& ui) {
    float s = split(ui), g = bar(ui);
    if (vertical_) {
        a_->setBounds(x, y, w, s);
        b_->setBounds(x, y + s + g, w, h - s - g);
    } else {
        a_->setBounds(x, y, s, h);
        b_->setBounds(x + s + g, y, w - s - g, h);
    }
    a_->layout(ui);
    b_->layout(ui);
}

bool SplitPane::onBar(float mx, float my) const {
    return vertical_ ? my >= a_->y + a_->h && my < b_->y : mx >= a_->x + a_->w && mx < b_->x;
}

Widget* SplitPane::hit(float px, float py) {
    if (!visible_ || !contains(px, py)) {
        return nullptr;
    }
    if (onBar(px, py)) {
        return this;
    }
    Widget* r = a_->hit(px, py);
    if (!r) {
        r = b_->hit(px, py);
    }
    return r ? r : this;
}

void SplitPane::draw(Ui& ui, Painter& p) {
    p.pushClip(a_->x, a_->y, a_->w, a_->h);
    a_->draw(ui, p);
    p.popClip();
    p.pushClip(b_->x, b_->y, b_->w, b_->h);
    b_->draw(ui, p);
    p.popClip();
    Theme& t = ui.theme;
    bool on = dragging_ || (ui.hover() == this && onBar(ui.mx, ui.my));
    float hv = animate(ui, 0, on ? 1 : 0, t.s[Theme::HOVER_TIME]);
    float g = bar(ui);
    argb lc = lerp(t.c[Theme::PANEL_BORDER], t.c[Theme::ACCENT], hv), dc = lerp(t.c[Theme::TEXT_DIM], t.c[Theme::ACCENT], hv);
    if (vertical_) {
        float by = a_->y + a_->h;
        hgradient(p, x, by + g / 2 - 0.5f, w / 2, 1, 0, lc);
        hgradient(p, x + w / 2, by + g / 2 - 0.5f, w / 2, 1, lc, 0);
        for (int i = -1; i <= 1; i++) {
            fill(p, x + w / 2 + i * 4 - 1, by + g / 2 - 1, 2, 2, dc);
        }
    } else {
        float bx = a_->x + a_->w;
        vgradient(p, bx + g / 2 - 0.5f, y, 1, h / 2, 0, lc);
        vgradient(p, bx + g / 2 - 0.5f, y + h / 2, 1, h / 2, lc, 0);
        for (int i = -1; i <= 1; i++) {
            fill(p, bx + g / 2 - 1, y + h / 2 + i * 4 - 1, 2, 2, dc);
        }
    }
}

bool SplitPane::press(Ui&, float mx, float my, int button, int clicks) {
    if (button != 0 || !onBar(mx, my)) {
        return false;
    }
    if (clicks == 2) {
        ratio_ = initial_;
        return true;
    }
    dragging_ = true;
    return true;
}

void SplitPane::drag(Ui& ui, float mx, float my) {
    if (dragging_) {
        float total = (vertical_ ? h : w) - bar(ui);
        float pos = (vertical_ ? my - y : mx - x) - bar(ui) / 2;
        ratio_ = std::clamp(pos / std::max(1.0f, total), 0.02f, 0.98f);
    }
}

int SplitPane::cursor(float mx, float my) const {
    return dragging_ || onBar(mx, my) ? (vertical_ ? Cursor::RESIZE_V : Cursor::RESIZE_H) : Cursor::ARROW;
}

// Tabs

Tabs& Tabs::add(std::string title, WidgetPtr page) {
    titles_.push_back(toU32(title));
    adopt(*page);
    pages_.push_back(std::move(page));
    return *this;
}

void Tabs::select(int i) { selected_ = std::clamp(i, 0, int(pages_.size()) - 1); }

float Tabs::listW(Ui& ui) const {
    float m = 0;
    for (auto& s : titles_) {
        m = std::max(m, ui.theme.font->width(s));
    }
    return m + 20;
}

float Tabs::prefWidth(Ui& ui) {
    float m = 0, tabs = 0;
    for (auto& p : pages_) {
        m = std::max(m, p->prefWidth(ui));
    }
    for (auto& s : titles_) {
        tabs += ui.theme.font->width(s) + 18;
    }
    return std::max(m, tabs);
}

float Tabs::minWidth(Ui& ui) {
    float m = 0;
    for (auto& p : pages_) {
        m = std::max(m, p->minWidth(ui));
    }
    return std::max(m, listW(ui));
}

float Tabs::headerFor(Ui& ui, float width, float ox, float oy, std::vector<std::array<float, 4>>* out) const {
    Font& f = *ui.theme.font;
    float th = ui.theme.s[Theme::ROW] + 4;
    if (width >= sideWidth_) {
        float lw = listW(ui);
        for (size_t i = 0; i < titles_.size(); i++) {
            if (out) {
                out->push_back({ox, oy + i * (th + 1), lw, th});
            }
        }
        return 0;
    }
    float px = 0, py = 0;
    for (auto& s : titles_) {
        float tw = f.width(s) + 18;
        if (px > 0 && px + tw > width) {
            px = 0;
            py += th;
        }
        if (out) {
            out->push_back({ox + px, oy + py, tw, th});
        }
        px += tw;
    }
    return py + th + 3;
}

float Tabs::prefHeight(Ui& ui, float width) {
    float hh = headerFor(ui, width, 0, 0, nullptr);
    float m = 0;
    float pw = width >= sideWidth_ ? width - listW(ui) - 6 : width;
    for (auto& p : pages_) {
        m = std::max(m, p->prefHeight(ui, pw));
    }
    if (width >= sideWidth_) {
        m = std::max(m, titles_.size() * (ui.theme.s[Theme::ROW] + 5));
    }
    return hh + m;
}

void Tabs::layout(Ui& ui) {
    tabRects_.clear();
    float hh = headerFor(ui, w, x, y, &tabRects_);
    auto& p = pages_[selected_];
    if (side()) {
        float lw = listW(ui) + 6;
        p->setBounds(x + lw, y, w - lw, h);
    } else {
        p->setBounds(x, y + hh, w, h - hh);
    }
    p->layout(ui);
}

Widget* Tabs::hit(float px, float py) {
    if (!visible_ || !contains(px, py)) {
        return nullptr;
    }
    Widget* r = pages_[selected_]->hit(px, py);
    return r ? r : this;
}

void Tabs::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    Font& f = *t.font;
    bool sd = side();
    if (sd) {
        vgradient(p, x + listW(ui) + 2, y, 1, h, t.c[Theme::PANEL_BORDER], 0);
    } else if (!tabRects_.empty()) {
        auto& last = tabRects_.back();
        hgradient(p, x, last[1] + last[3], w, 1, t.c[Theme::PANEL_BORDER], alpha(t.c[Theme::PANEL_BORDER], 0.2f));
    }
    for (size_t i = 0; i < tabRects_.size(); i++) {
        auto& r = tabRects_[i];
        bool on = int(i) == selected_;
        bool hot = ui.isHover(this) && ui.mx >= r[0] && ui.mx < r[0] + r[2] && ui.my >= r[1] && ui.my < r[1] + r[3];
        if (on || hot) {
            auto pts = sd ? cut(r[0], r[1], r[2], r[3], 3, 0, 3, 0) : cut(r[0], r[1], r[2] - 1, r[3], 4, 0, 0, 0);
            poly(p, pts, on ? t.c[Theme::HOVER_TOP] : alpha(t.c[Theme::HOVER_TOP], 0.5f), on ? alpha(t.c[Theme::HOVER_BOTTOM], 0.6f) : 0,
                 r[1], r[3]);
        }
        f.draw(p, titles_[i], r[0] + (sd ? 10 : 9), r[1] + (r[3] - f.lineHeight()) / 2,
               on ? t.c[Theme::TEXT_BRIGHT] : hot ? t.c[Theme::TEXT] : t.c[Theme::TEXT_DIM]);
    }
    if (selected_ < int(tabRects_.size())) {
        auto& r = tabRects_[selected_];
        // (relative to the tabs: moving the window moves the mark with them; only a tab change slides it)
        float target = sd ? r[1] - y : r[0] - x, len = sd ? r[3] : r[2];
        if (std::isnan(markPos_)) {
            markPos_ = target;
            markLen_ = len;
        }
        float k = std::min(1.0f, ui.dt * 20);
        markPos_ += (target - markPos_) * k;
        markLen_ += (len - markLen_) * k;
        if (std::abs(target - markPos_) > 0.1f || std::abs(len - markLen_) > 0.1f) {
            ui.animating = true;
        }
        if (sd) {
            fill(p, r[0], y + markPos_, 2, markLen_, t.c[Theme::ACCENT]);
        } else {
            fill(p, x + markPos_ + 2, r[1] + r[3] - 2, markLen_ - 5, 2, t.c[Theme::ACCENT]);
        }
        if (ui.isFocused(this)) {
            outline(p, cut(r[0] - 1, r[1] - 1, r[2] + 1, r[3] + 2, 3, 0, 0, 0), 1, alpha(t.c[Theme::FOCUS], 0.7f));
        }
    }
    pages_[selected_]->draw(ui, p);
}

bool Tabs::press(Ui&, float mx, float my, int button, int) {
    if (button != 0) {
        return false;
    }
    for (size_t i = 0; i < tabRects_.size(); i++) {
        auto& r = tabRects_[i];
        if (mx >= r[0] && mx < r[0] + r[2] && my >= r[1] && my < r[1] + r[3]) {
            selected_ = int(i);
            return true;
        }
    }
    return false;
}

bool Tabs::key(Ui& ui, int key, int mods) {
    int n = int(pages_.size());
    if (key == Keys::TAB && (mods & Keys::CTRL)) {
        selected_ = (selected_ + ((mods & Keys::SHIFT) ? n - 1 : 1)) % n;
        return true;
    }
    if (ui.focused() != this) {
        return false;
    }
    bool sd = side();
    if (key == (sd ? Keys::DOWN : Keys::RIGHT)) {
        selected_ = (selected_ + 1) % n;
        return true;
    }
    if (key == (sd ? Keys::UP : Keys::LEFT)) {
        selected_ = (selected_ + n - 1) % n;
        return true;
    }
    return false;
}

}  // namespace slopui
