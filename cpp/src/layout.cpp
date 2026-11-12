// SlopUI: Widget, Container, Box, Grid, ScrollPane (Java: the classes of the same names).
#include <algorithm>
#include <cmath>

#include "slopui/ui.hpp"

namespace slopui {

bool Widget::isEnabled() const {
    for (const Widget* t = this; t; t = t->parent_) {
        if (!t->enabled_) {
            return false;
        }
    }
    return true;
}

float Widget::animate(Ui& ui, int slot, float target, float time) {
    float v = anims_[slot];
    if (v != target) {
        float step = time <= 0 ? 1 : ui.dt / time;
        v = target > v ? std::min(target, v + step) : std::max(target, v - step);
        anims_[slot] = v;
        ui.animating = true;
    }
    return v;
}

Theme& Widget::theme(Ui& ui) const { return ui.theme; }

// Container

Container& Container::add(WidgetPtr w) {
    w->parent_ = this;
    kids_.push_back(std::move(w));
    return *this;
}

void Container::remove(Widget* w) {
    auto it = std::find_if(kids_.begin(), kids_.end(), [&](const WidgetPtr& k) { return k.get() == w; });
    if (it != kids_.end()) {
        (*it)->parent_ = nullptr;
        kids_.erase(it);
    }
}

void Container::clear() {
    for (auto& k : kids_) {
        k->parent_ = nullptr;
    }
    kids_.clear();
}

void Container::children(std::vector<Widget*>& out) {
    for (auto& k : kids_) {
        out.push_back(k.get());
    }
}

Widget* Container::hit(float px, float py) {
    if (!visible_ || !contains(px, py)) {
        return nullptr;
    }
    for (auto it = kids_.rbegin(); it != kids_.rend(); ++it) {
        if ((*it)->isVisible()) {
            if (Widget* h = (*it)->hit(px, py)) {
                return h;
            }
        }
    }
    return this;
}

void Container::draw(Ui& ui, Painter& p) {
    for (auto& k : kids_) {
        if (k->isVisible()) {
            k->draw(ui, p);
        }
    }
}

// Box

std::shared_ptr<Box> Box::row(std::vector<WidgetPtr> ws) {
    auto b = make<Box>();
    b->horizontal_ = true;
    for (auto& w : ws) {
        b->add(w);
    }
    return b;
}

std::shared_ptr<Box> Box::column(std::vector<WidgetPtr> ws) {
    auto b = make<Box>();
    for (auto& w : ws) {
        b->add(w);
    }
    return b;
}

float Box::gapOf(Ui& ui) const { return gap_ >= 0 ? gap_ : ui.theme.s[Theme::GAP]; }

std::vector<Widget*> Box::shown() const {
    std::vector<Widget*> l;
    for (auto& k : kids_) {
        if (k->isVisible()) {
            l.push_back(k.get());
        }
    }
    return l;
}

bool Box::asRow(Ui& ui, float width) {
    if (!horizontal_) {
        return false;
    }
    if (!responsive_) {
        return true;
    }
    auto l = shown();
    float need = 2 * pad_ + std::max(0, int(l.size()) - 1) * gapOf(ui);
    for (Widget* k : l) {
        need += k->minWidth(ui);
    }
    return need <= width + 0.01f;
}

float Box::prefWidth(Ui& ui) {
    auto l = shown();
    float s = 0;
    for (Widget* k : l) {
        s = horizontal_ ? s + k->prefWidth(ui) : std::max(s, k->prefWidth(ui));
    }
    if (horizontal_) {
        s += std::max(0, int(l.size()) - 1) * gapOf(ui);
    }
    return s + 2 * pad_;
}

float Box::minWidth(Ui& ui) {
    auto l = shown();
    float s = 0;
    bool stackable = responsive_ || !horizontal_;
    for (Widget* k : l) {
        s = stackable ? std::max(s, k->minWidth(ui)) : s + k->minWidth(ui);
    }
    if (!stackable) {
        s += std::max(0, int(l.size()) - 1) * gapOf(ui);
    }
    return s + 2 * pad_;
}

std::vector<float> Box::widths(Ui& ui, const std::vector<Widget*>& l, float inner) {
    size_t n = l.size();
    std::vector<float> ws(n);
    float avail = inner - std::max(0, int(n) - 1) * gapOf(ui);
    float sum = 0, growSum = 0;
    for (size_t i = 0; i < n; i++) {
        ws[i] = l[i]->prefWidth(ui);
        sum += ws[i];
        growSum += l[i]->grow;
    }
    if (sum <= avail) {
        if (growSum > 0) {
            for (size_t i = 0; i < n; i++) {
                ws[i] += (avail - sum) * l[i]->grow / growSum;
            }
        }
    } else {
        float slack = 0;
        std::vector<float> mins(n);
        for (size_t i = 0; i < n; i++) {
            mins[i] = std::min(ws[i], l[i]->minWidth(ui));
            slack += ws[i] - mins[i];
        }
        float over = sum - avail;
        for (size_t i = 0; i < n; i++) {
            ws[i] -= slack > 0 ? std::min(1.0f, over / slack) * (ws[i] - mins[i]) : 0;
        }
    }
    return ws;
}

std::vector<std::vector<Widget*>> Box::lines(Ui& ui, const std::vector<Widget*>& l, float inner) {
    std::vector<std::vector<Widget*>> out;
    std::vector<Widget*> line;
    float used = 0, g = gapOf(ui);
    for (Widget* k : l) {
        float kw = std::min(inner, k->prefWidth(ui));
        if (!line.empty() && used + g + kw > inner) {
            out.push_back(line);
            line.clear();
            used = 0;
        }
        used += (line.empty() ? 0 : g) + kw;
        line.push_back(k);
    }
    if (!line.empty()) {
        out.push_back(line);
    }
    return out;
}

float Box::prefHeight(Ui& ui, float width) {
    auto l = shown();
    float inner = width - 2 * pad_, s = 0, g = gapOf(ui);
    if (asRow(ui, width)) {
        auto ws = widths(ui, l, inner);
        for (size_t i = 0; i < l.size(); i++) {
            s = std::max(s, l[i]->prefHeight(ui, ws[i]));
        }
    } else if (horizontal_) {
        for (auto& line : lines(ui, l, inner)) {
            auto ws = widths(ui, line, inner);
            float lh = 0;
            for (size_t i = 0; i < line.size(); i++) {
                lh = std::max(lh, line[i]->prefHeight(ui, ws[i]));
            }
            s += lh + g;
        }
        s -= g;
    } else {
        for (Widget* k : l) {
            s += k->prefHeight(ui, inner);
        }
        s += std::max(0, int(l.size()) - 1) * g;
    }
    return s + 2 * pad_;
}

void Box::layout(Ui& ui) {
    auto l = shown();
    float inner = w - 2 * pad_, innerH = h - 2 * pad_, g = gapOf(ui);
    if (asRow(ui, w)) {
        auto ws = widths(ui, l, inner);
        float used = 0, growSum = 0;
        for (size_t i = 0; i < l.size(); i++) {
            used += ws[i];
            growSum += l[i]->grow;
        }
        used += std::max(0, int(l.size()) - 1) * g;
        float px = x + pad_ + (growSum > 0 ? 0 : std::max(0.0f, inner - used) * justify_);
        for (size_t i = 0; i < l.size(); i++) {
            l[i]->setBounds(px, y + pad_, ws[i], innerH);
            l[i]->layout(ui);
            px += ws[i] + g;
        }
    } else if (horizontal_) {
        float py = y + pad_;
        for (auto& line : lines(ui, l, inner)) {
            auto ws = widths(ui, line, inner);
            float sum = (line.size() - 1) * g;
            for (float v : ws) {
                sum += v;
            }
            float extra = std::max(0.0f, inner - sum) / line.size();
            float lh = 0;
            for (size_t i = 0; i < line.size(); i++) {
                ws[i] += extra;
                lh = std::max(lh, line[i]->prefHeight(ui, ws[i]));
            }
            float px = x + pad_;
            for (size_t i = 0; i < line.size(); i++) {
                line[i]->setBounds(px, py, ws[i], lh);
                line[i]->layout(ui);
                px += ws[i] + g;
            }
            py += lh + g;
        }
    } else {
        size_t n = l.size();
        std::vector<float> hs(n);
        float sum = 0, growSum = 0;
        for (size_t i = 0; i < n; i++) {
            hs[i] = l[i]->prefHeight(ui, inner);
            sum += hs[i];
            growSum += l[i]->grow;
        }
        float avail = innerH - std::max(0, int(n) - 1) * g;
        if (growSum > 0 && avail > sum) {
            for (size_t i = 0; i < n; i++) {
                hs[i] += (avail - sum) * l[i]->grow / growSum;
            }
        }
        float py = y + pad_;
        for (size_t i = 0; i < n; i++) {
            l[i]->setBounds(x + pad_, py, inner, hs[i]);
            l[i]->layout(ui);
            py += hs[i] + g;
        }
    }
}

// Grid

Grid::Grid(int columns) : columns_(columns), colGrow_(columns, 0.0f) { colGrow_[columns - 1] = 1; }

Grid& Grid::columnGrow(std::vector<float> g) {
    for (size_t i = 0; i < g.size() && i < colGrow_.size(); i++) {
        colGrow_[i] = g[i];
    }
    return *this;
}

float Grid::gx(Ui& ui) const { return ui.theme.s[Theme::PAD]; }
float Grid::gy(Ui& ui) const { return ui.theme.s[Theme::GAP]; }

std::vector<float> Grid::colPref(Ui& ui, bool min) {
    std::vector<float> c(columns_, 0.0f);
    for (size_t i = 0; i < kids_.size(); i++) {
        c[i % columns_] = std::max(c[i % columns_], min ? kids_[i]->minWidth(ui) : kids_[i]->prefWidth(ui));
    }
    return c;
}

bool Grid::stacked(Ui& ui, float width) {
    float need = (columns_ - 1) * gx(ui);
    for (float v : colPref(ui, true)) {
        need += v;
    }
    return need > width + 0.01f;
}

float Grid::prefWidth(Ui& ui) {
    float s = (columns_ - 1) * gx(ui);
    for (float v : colPref(ui, false)) {
        s += v;
    }
    return s;
}

float Grid::minWidth(Ui& ui) {
    float m = 0;
    for (float v : colPref(ui, true)) {
        m = std::max(m, v);
    }
    return m;
}

std::vector<float> Grid::widths(Ui& ui, float width) {
    auto c = colPref(ui, false);
    float sum = (columns_ - 1) * gx(ui), g = 0;
    for (int i = 0; i < columns_; i++) {
        sum += c[i];
        g += colGrow_[i];
    }
    float spare = width - sum;
    if (spare > 0 && g > 0) {
        for (int i = 0; i < columns_; i++) {
            c[i] += spare * colGrow_[i] / g;
        }
    } else if (spare < 0) {
        auto m = colPref(ui, true);
        float slack = 0;
        for (int i = 0; i < columns_; i++) {
            slack += c[i] - m[i];
        }
        for (int i = 0; i < columns_; i++) {
            c[i] -= slack > 0 ? std::min(1.0f, -spare / slack) * (c[i] - m[i]) : 0;
        }
    }
    return c;
}

float Grid::prefHeight(Ui& ui, float width) {
    if (stacked(ui, width)) {
        float s = 0;
        for (auto& k : kids_) {
            s += k->prefHeight(ui, width);
        }
        return s + std::max(0, int(kids_.size()) - 1) * gy(ui) * 0.5f;
    }
    auto c = widths(ui, width);
    float s = 0;
    for (int r = 0; r < rows(); r++) {
        float rh = 0;
        for (int col = 0; col < columns_ && size_t(r * columns_ + col) < kids_.size(); col++) {
            rh = std::max(rh, kids_[r * columns_ + col]->prefHeight(ui, c[col]));
        }
        s += rh;
    }
    return s + std::max(0, rows() - 1) * gy(ui);
}

void Grid::layout(Ui& ui) {
    if (stacked(ui, w)) {
        float py = y;
        for (auto& k : kids_) {
            float kh = k->prefHeight(ui, w);
            k->setBounds(x, py, w, kh);
            k->layout(ui);
            py += kh + gy(ui) * 0.5f;
        }
        return;
    }
    auto c = widths(ui, w);
    float py = y;
    for (int r = 0; r < rows(); r++) {
        float rh = 0;
        for (int col = 0; col < columns_ && size_t(r * columns_ + col) < kids_.size(); col++) {
            rh = std::max(rh, kids_[r * columns_ + col]->prefHeight(ui, c[col]));
        }
        float px = x;
        for (int col = 0; col < columns_ && size_t(r * columns_ + col) < kids_.size(); col++) {
            auto& k = kids_[r * columns_ + col];
            k->setBounds(px, py, c[col], rh);
            k->layout(ui);
            px += c[col] + gx(ui);
        }
        py += rh + gy(ui);
    }
}

// ScrollPane

ScrollPane::ScrollPane(WidgetPtr content, bool bars) : content_(std::move(content)), bars_(bars) { adopt(*content_); }

float ScrollPane::bar(Ui& ui) const { return bars_ ? ui.theme.s[Theme::SCROLLBAR] : 0; }

float ScrollPane::prefWidth(Ui& ui) { return content_->prefWidth(ui) + bar(ui); }

float ScrollPane::minWidth(Ui& ui) { return std::min(content_->minWidth(ui), 40.0f) + bar(ui); }

float ScrollPane::prefHeight(Ui& ui, float width) {
    float ch = content_->prefHeight(ui, std::max(content_->minWidth(ui), width - bar(ui)));
    return viewHeight_ > 0 ? std::min(ch, viewHeight_) : ch;
}

float ScrollPane::viewW(Ui& ui) const { return w - (contentH_ > h ? bar(ui) : 0); }

float ScrollPane::viewH(Ui& ui) const { return h - (contentW_ > viewW(ui) + 0.5f ? bar(ui) : 0); }

void ScrollPane::layout(Ui& ui) {
    barW_ = bar(ui);
    float vw = w - bar(ui);
    contentW_ = std::max(content_->minWidth(ui), vw);
    contentH_ = content_->prefHeight(ui, contentW_);
    if (contentH_ <= h) {
        contentW_ = std::max(content_->minWidth(ui), w);
        contentH_ = content_->prefHeight(ui, contentW_);
    }
    clampTargets(ui);
    float k = std::min(1.0f, ui.dt * 18);
    sx_ += (targetX_ - sx_) * k;
    sy_ += (targetY_ - sy_) * k;
    if (std::abs(targetX_ - sx_) < 0.05f) {
        sx_ = targetX_;
    }
    if (std::abs(targetY_ - sy_) < 0.05f) {
        sy_ = targetY_;
    }
    if (sx_ != targetX_ || sy_ != targetY_) {
        ui.animating = true;
    }
    content_->setBounds(x - std::round(sx_), y - std::round(sy_), contentW_, std::max(contentH_, viewH(ui)));
    content_->layout(ui);
}

void ScrollPane::clampTargets(Ui& ui) {
    targetX_ = std::max(0.0f, std::min(targetX_, contentW_ - viewW(ui)));
    targetY_ = std::max(0.0f, std::min(targetY_, contentH_ - viewH(ui)));
}

void ScrollPane::reveal(Ui& ui, float ry, float rh) {
    float top = ry - content_->y;
    if (top < targetY_) {
        targetY_ = top;
    } else if (top + rh > targetY_ + viewH(ui)) {
        targetY_ = top + rh - viewH(ui);
    }
    clampTargets(ui);
}

Widget* ScrollPane::hit(float px, float py) {
    if (!visible_ || !contains(px, py)) {
        return nullptr;
    }
    if (px < x + w - (contentH_ > h ? barW_ : 0) && py < y + h - (contentW_ > w ? barW_ : 0)) {
        if (Widget* c = content_->hit(px, py)) {
            return c;
        }
    }
    return this;
}

std::array<float, 2> ScrollPane::thumbV(Ui& ui) const {
    float th = viewH(ui);
    float len = std::max(12.0f, th * th / contentH_);
    float range = contentH_ - th;
    return {y + (range <= 0 ? 0 : (th - len) * sy_ / range), len};
}

std::array<float, 2> ScrollPane::thumbH(Ui& ui) const {
    float tw = viewW(ui);
    float len = std::max(12.0f, tw * tw / contentW_);
    float range = contentW_ - tw;
    return {x + (range <= 0 ? 0 : (tw - len) * sx_ / range), len};
}

void ScrollPane::draw(Ui& ui, Painter& p) {
    p.pushClip(x, y, viewW(ui), viewH(ui));
    content_->draw(ui, p);
    p.popClip();
    if (!bars_) {
        return;
    }
    Theme& t = ui.theme;
    float b = bar(ui);
    if (contentH_ > h) {
        float tx = x + w - b, th = viewH(ui);
        Shapes::vgradient(p, tx, y, b, th, t.c[Theme::TRACK_TOP], t.c[Theme::TRACK_BOTTOM]);
        auto thumb = thumbV(ui);
        bool on = dragging_ == 1 || (ui.hover() == this && ui.mx >= tx);
        float hv = animate(ui, 0, on ? 1 : 0, t.s[Theme::HOVER_TIME]);
        auto pts = Shapes::cut(tx + 1, thumb[0], b - 2, thumb[1], 0, 0, 0, 0);
        Shapes::poly(p, pts, Shapes::lerp(t.c[Theme::BUTTON_TOP], t.c[Theme::HOVER_TOP], hv),
                     Shapes::lerp(t.c[Theme::BUTTON_BOTTOM], t.c[Theme::HOVER_BOTTOM], hv), thumb[0], thumb[1]);
        Shapes::outline(p, pts, 1, Shapes::lerp(t.c[Theme::BUTTON_BORDER], t.c[Theme::ACCENT_SOFT], hv));
    }
    if (contentW_ > viewW(ui) + 0.5f) {
        float ty = y + h - b, tw = viewW(ui);
        Shapes::vgradient(p, x, ty, tw, b, t.c[Theme::TRACK_TOP], t.c[Theme::TRACK_BOTTOM]);
        auto thumb = thumbH(ui);
        auto pts = Shapes::cut(thumb[0], ty + 1, thumb[1], b - 2, 0, 0, 0, 0);
        Shapes::poly(p, pts, t.c[Theme::BUTTON_TOP], t.c[Theme::BUTTON_BOTTOM], ty, b);
        Shapes::outline(p, pts, 1, t.c[Theme::BUTTON_BORDER]);
    }
}

bool ScrollPane::press(Ui& ui, float mx, float my, int button, int) {
    float b = bar(ui);
    if (button == 2) {
        dragging_ = 3;
        dragFrom_ = my;
        dragStart_ = targetY_;
        dragFromX_ = mx;
        dragStartX_ = targetX_;
        return true;
    }
    if (button != 0 || !bars_) {
        return false;
    }
    if (contentH_ > h && mx >= x + w - b) {
        auto t = thumbV(ui);
        if (my < t[0] || my >= t[0] + t[1]) {
            targetY_ += (my < t[0] ? -1 : 1) * viewH(ui) * 0.9f;
            clampTargets(ui);
        }
        dragging_ = 1;
        dragFrom_ = my;
        dragStart_ = targetY_;
        return true;
    }
    if (contentW_ > viewW(ui) + 0.5f && my >= y + h - b) {
        dragging_ = 2;
        dragFrom_ = mx;
        dragStart_ = targetX_;
        return true;
    }
    return false;
}

void ScrollPane::drag(Ui& ui, float mx, float my) {
    if (dragging_ == 1) {
        float th = viewH(ui), len = thumbV(ui)[1];
        targetY_ = dragStart_ + (my - dragFrom_) * (contentH_ - th) / std::max(1.0f, th - len);
        clampTargets(ui);
        sy_ = targetY_;
    } else if (dragging_ == 2) {
        float tw = viewW(ui), len = thumbH(ui)[1];
        targetX_ = dragStart_ + (mx - dragFrom_) * (contentW_ - tw) / std::max(1.0f, tw - len);
        clampTargets(ui);
        sx_ = targetX_;
    } else if (dragging_ == 3) {
        targetY_ = dragStart_ - (my - dragFrom_);
        targetX_ = dragStartX_ - (mx - dragFromX_);
        clampTargets(ui);
        sx_ = targetX_;
        sy_ = targetY_;
    }
}

bool ScrollPane::wheel(Ui& ui, float, float, float dy) {
    float before = targetY_;
    targetY_ -= dy * ui.theme.s[Theme::ROW] * 3;
    clampTargets(ui);
    return targetY_ != before || contentH_ > h;
}

}  // namespace slopui
