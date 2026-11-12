// SlopUI: Label, Button, CheckBox, Toggle, RadioGroup, RadioButton, Separator, ProgressBar, Slider, ImageView,
// GroupBox (Java: the classes of the same names).
#include <algorithm>
#include <cmath>

#include "slopui/ui.hpp"

namespace slopui {

using namespace Shapes;

// Label

Font& Label::font(Ui& ui) const { return title_ ? ui.theme.titleFont() : *ui.theme.font; }

static std::vector<std::u32string> split(std::u32string_view s, char32_t sep) {
    std::vector<std::u32string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == sep) {
            out.emplace_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}

float Label::prefWidth(Ui& ui) {
    float m = 0;
    for (auto& line : split(toU32(text_()), U'\n')) {
        m = std::max(m, font(ui).width(line));
    }
    return m;
}

float Label::minWidth(Ui& ui) {
    float m = 0;
    for (auto& line : split(toU32(text_()), U'\n')) {
        for (auto& word : split(line, U' ')) {
            m = std::max(m, font(ui).width(word));
        }
    }
    return std::min(prefWidth(ui), std::max(m, 40.0f));
}

std::vector<std::u32string> Label::lines(Font& f, float width) const {
    std::vector<std::u32string> out;
    for (auto& para : split(toU32(text_()), U'\n')) {
        std::u32string line;
        for (auto& word : split(para, U' ')) {
            std::u32string tryLine = line.empty() ? word : line + U" " + word;
            if (!line.empty() && f.width(tryLine) > width) {
                out.push_back(line);
                line = word;
            } else {
                line = tryLine;
            }
        }
        out.push_back(line);
    }
    return out;
}

float Label::prefHeight(Ui& ui, float width) {
    return float(lines(font(ui), width).size()) * font(ui).lineHeight() * 1.25f;
}

void Label::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    Font& f = font(ui);
    argb c = colour_ ? colour_ : !isEnabled() ? t.c[Theme::TEXT_DISABLED] : title_ ? t.c[Theme::TEXT_BRIGHT] : t.c[Theme::TEXT];
    auto ls = lines(f, w);
    float lh = f.lineHeight() * 1.25f;
    float py = y + std::max(0.0f, (h - ls.size() * lh) / 2) + (lh - f.lineHeight()) / 2;
    for (auto& l : ls) {
        f.draw(p, l, x + (w - f.width(l)) * align_, py, c);
        py += lh;
    }
}

// Button

void Button::click(Ui&) {
    if (action_ && isEnabled()) {
        auto a = action_;       // (the action may close the window that owns this button)
        a();
    }
}

float Button::prefWidth(Ui& ui) {
    return std::max(44.0f, ui.theme.font->width(text_()) + 2 * ui.theme.s[Theme::PAD] + 8);
}

float Button::minWidth(Ui& ui) { return std::min(prefWidth(ui), ui.theme.font->width(text_()) + 10); }

float Button::prefHeight(Ui& ui, float) { return ui.theme.s[Theme::BUTTON_HEIGHT]; }

void Button::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool en = isEnabled(), over = ui.isHover(this);
    float hv = animate(ui, 0, en && (over || down_) ? 1 : 0, t.s[Theme::HOVER_TIME]);
    float pr = animate(ui, 1, down_ && over ? 1 : 0, t.s[Theme::PRESS_TIME]);
    float bh = std::min(h, t.s[Theme::BUTTON_HEIGHT]), by = y + (h - bh) / 2;
    Skin::button(p, t, x, by, w, bh, hv, pr, en, accent_, ui.isFocused(this));
    Font& f = *t.font;
    auto s = Skin::fit(f, toU32(text_()), w - 6);
    float tx = x + (w - f.width(s)) / 2, ty = by + (bh - f.lineHeight()) / 2 + pr;
    argb c = !en ? t.c[Theme::TEXT_DISABLED] : lerp(t.c[Theme::BUTTON_TEXT], t.c[Theme::TEXT_BRIGHT], hv);
    f.draw(p, s, tx + 0.5f, ty + 0.5f, 0x90000000);
    f.draw(p, s, tx, ty, c);
}

bool Button::press(Ui&, float, float, int button, int) {
    if (button != 0) {
        return false;
    }
    down_ = true;
    return true;
}

void Button::release(Ui& ui, float mx, float my, int) {
    bool on = down_ && contains(mx, my);
    down_ = false;
    if (on) {
        auto keep = shared_from_this();     // (alive until the action returns)
        click(ui);
    }
}

bool Button::key(Ui& ui, int key, int) {
    if (key == Keys::SPACE || Keys::isEnter(key)) {
        auto keep = shared_from_this();
        click(ui);
        return true;
    }
    return false;
}

// CheckBox, Toggle

void CheckBox::toggle() {
    on_ = !on_;
    if (onChange_) {
        onChange_(on_);
    }
}

float CheckBox::prefWidth(Ui& ui) { return 20 + ui.theme.font->width(text_); }
float CheckBox::prefHeight(Ui& ui, float) { return ui.theme.s[Theme::ROW]; }

void CheckBox::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool en = isEnabled();
    float hv = animate(ui, 0, en && ui.isHover(this) ? 1 : 0, t.s[Theme::HOVER_TIME]);
    float tk = animate(ui, 1, on_ ? 1 : 0, 0.14f);
    float s = 10, by = y + (h - s) / 2;
    Skin::field(p, t, x, by, s, s, std::max(hv * 0.6f, ui.isFocused(this) ? 1.0f : 0.0f), en);
    if (tk > 0) {
        Skin::tick(p, x, by, s, tk, en ? t.c[Theme::ACCENT] : t.c[Theme::TEXT_DISABLED]);
    }
    Font& f = *t.font;
    f.draw(p, text_, x + 14, y + (h - f.lineHeight()) / 2,
           !en ? t.c[Theme::TEXT_DISABLED] : lerp(t.c[Theme::TEXT], t.c[Theme::TEXT_BRIGHT], hv));
}

void CheckBox::release(Ui&, float mx, float my, int) {
    if (down_ && contains(mx, my)) {
        toggle();
    }
    down_ = false;
}

bool CheckBox::key(Ui&, int key, int) {
    if (key == Keys::SPACE) {
        toggle();
        return true;
    }
    return false;
}

float Toggle::prefWidth(Ui& ui) { return 32 + ui.theme.font->width(text_); }

void Toggle::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool en = isEnabled();
    float hv = animate(ui, 0, en && ui.isHover(this) ? 1 : 0, t.s[Theme::HOVER_TIME]);
    float k = animate(ui, 1, on_ ? 1 : 0, 0.12f);
    float tw = 20, th = 9, tx = x, ty = y + (h - th) / 2;
    auto track = cut(tx, ty, tw, th, 3, 0, 3, 0);
    poly(p, track, t.c[Theme::TRACK_TOP], t.c[Theme::TRACK_BOTTOM], ty, th);
    if (k > 0) {
        poly(p, track, alpha(t.c[Theme::FILL_TOP], k * (en ? 1 : 0.3f)), alpha(t.c[Theme::FILL_BOTTOM], k * (en ? 1 : 0.3f)), ty, th);
        glow(p, track, 3 * k, alpha(t.c[Theme::GLOW], 0.5f * k * (en ? 1 : 0)));
    }
    outline(p, track, 1, lerp(t.c[Theme::FIELD_BORDER], t.c[Theme::ACCENT_SOFT], std::max(hv, k * 0.6f)));
    float kw = 9, kx = tx + 1 + (tw - kw - 2) * k;
    Skin::button(p, t, kx, ty - 1, kw, th + 2, hv, 0, en, false, false);
    if (ui.isFocused(this)) {
        outline(p, offset(track, 2), 1, alpha(t.c[Theme::FOCUS], 0.8f));
    }
    Font& f = *t.font;
    f.draw(p, text_, x + 26, y + (h - f.lineHeight()) / 2,
           !en ? t.c[Theme::TEXT_DISABLED] : lerp(t.c[Theme::TEXT], t.c[Theme::TEXT_BRIGHT], hv));
}

bool Toggle::key(Ui&, int key, int) {
    if (key == Keys::SPACE || Keys::isEnter(key)) {
        toggle();
        return true;
    }
    return false;
}

// Radio buttons

std::shared_ptr<RadioButton> RadioGroup::button(std::string text) {
    auto b = make<RadioButton>(shared_from_this(), int(buttons_.size()), std::move(text));
    buttons_.push_back(b);
    return b;
}

void RadioGroup::select(int i) {
    if (i != selected_) {
        selected_ = i;
        if (onChange_) {
            onChange_(i);
        }
    }
}

float RadioButton::prefWidth(Ui& ui) { return 20 + ui.theme.font->width(text_); }
float RadioButton::prefHeight(Ui& ui, float) { return ui.theme.s[Theme::ROW]; }

void RadioButton::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool en = isEnabled(), sel = group_->selected() == index_;
    float hv = animate(ui, 0, en && ui.isHover(this) ? 1 : 0, t.s[Theme::HOVER_TIME]);
    float s = animate(ui, 1, sel ? 1 : 0, 0.12f);
    float cx = x + 5, cy = y + h / 2, r = 5;
    Poly d = {cx, cy - r, cx + r, cy, cx, cy + r, cx - r, cy};
    poly(p, d, t.c[Theme::FIELD_TOP], t.c[Theme::FIELD_BOTTOM], cy - r, 2 * r);
    outline(p, d, 1, lerp(t.c[Theme::FIELD_BORDER], t.c[Theme::FOCUS], ui.isFocused(this) ? 1 : hv * 0.6f));
    if (s > 0) {
        float q = 2.6f * s;
        Poly in = {cx, cy - q, cx + q, cy, cx, cy + q, cx - q, cy};
        argb c = en ? t.c[Theme::ACCENT] : t.c[Theme::TEXT_DISABLED];
        glow(p, in, 2.5f * s, alpha(t.c[Theme::GLOW], en ? 0.7f * s : 0));
        poly(p, in, c, c, cy - q, 2 * q);
    }
    Font& f = *t.font;
    f.draw(p, text_, x + 14, y + (h - f.lineHeight()) / 2,
           !en ? t.c[Theme::TEXT_DISABLED] : lerp(t.c[Theme::TEXT], t.c[Theme::TEXT_BRIGHT], hv));
}

void RadioButton::release(Ui&, float mx, float my, int) {
    if (down_ && contains(mx, my)) {
        group_->select(index_);
    }
    down_ = false;
}

bool RadioButton::key(Ui& ui, int key, int) {
    int n = int(group_->buttons_.size());
    if (key == Keys::SPACE) {
        group_->select(index_);
        return true;
    }
    if (key == Keys::DOWN || key == Keys::RIGHT || key == Keys::UP || key == Keys::LEFT) {
        int d = key == Keys::DOWN || key == Keys::RIGHT ? 1 : n - 1;
        int i = (group_->selected() + d) % n;
        group_->select(i);
        if (auto b = group_->buttons_[i].lock()) {
            ui.setFocus(b.get());
        }
        return true;
    }
    return false;
}

// Separator, ProgressBar

void Separator::draw(Ui& ui, Painter& p) {
    argb c = ui.theme.c[Theme::PANEL_BORDER];
    argb lite = alpha(ui.theme.c[Theme::ACCENT_SOFT], 0.25f);
    if (vertical_) {
        float cx = std::round(x + w / 2);
        vgradient(p, cx, y, 1, h / 2, 0, c);
        vgradient(p, cx, y + h / 2, 1, h / 2, c, 0);
        vgradient(p, cx + 1, y, 1, h / 2, 0, lite);
        vgradient(p, cx + 1, y + h / 2, 1, h / 2, lite, 0);
    } else {
        float cy = std::round(y + h / 2);
        hgradient(p, x, cy, w / 2, 1, 0, c);
        hgradient(p, x + w / 2, cy, w / 2, 1, c, 0);
        hgradient(p, x, cy + 1, w / 2, 1, 0, lite);
        hgradient(p, x + w / 2, cy + 1, w / 2, 1, lite, 0);
    }
}

float ProgressBar::prefHeight(Ui& ui, float) { return ui.theme.s[Theme::ROW]; }

void ProgressBar::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    float bh = std::min(h, 10.0f), by = y + (h - bh) / 2;
    Skin::field(p, t, x, by, w, bh, 0, true);
    ui.animating = true;
    p.pushClip(x + 1, by + 1, w - 2, bh - 2);
    if (value_ < 0) {
        float period = 1.4f, f = float(std::fmod(ui.time, double(period)) / period);
        float sw = w * 0.3f, sx = x - sw + (w + sw) * f;
        hgradient(p, sx, by + 1, sw / 2, bh - 2, 0, t.c[Theme::FILL_TOP]);
        hgradient(p, sx + sw / 2, by + 1, sw / 2, bh - 2, t.c[Theme::FILL_TOP], 0);
    } else {
        shown_ += (value_ - shown_) * std::min(1.0f, ui.dt * 12);
        float fw = (w - 2) * std::clamp(shown_, 0.0f, 1.0f);
        if (fw > 0) {
            poly(p, cut(x + 1, by + 1, fw, bh - 2, 0, 0, std::min(3.0f, fw), 0), t.c[Theme::FILL_TOP], t.c[Theme::FILL_BOTTOM], by, bh);
            vgradient(p, x + 1, by + 1, fw, (bh - 2) / 2, 0x50FFFFFF, 0x00FFFFFF);
            // the sheen: a steady sweep along the whole bar (its own period, not the fill's width, which changes
            // every frame), shown only over the filled part
            float span = w + 40, sheen = float(std::fmod(ui.time * 0.7, 1.0)) * span - 20;
            p.pushClip(x + 1, by + 1, fw, bh - 2);
            hgradient(p, x + 1 + sheen, by + 1, 10, bh - 2, 0, 0x40FFFFFF);
            hgradient(p, x + 11 + sheen, by + 1, 10, bh - 2, 0x40FFFFFF, 0);
            p.popClip();
        }
    }
    p.popClip();
    if (text_) {
        Font& f = *t.font;
        std::string s = text_();
        float tx = x + (w - f.width(s)) / 2, ty = by + (bh - f.lineHeight()) / 2;
        f.draw(p, s, tx + 0.5f, ty + 0.5f, 0xC0000000);
        f.draw(p, s, tx, ty, t.c[Theme::TEXT_BRIGHT]);
    }
}

// Slider

void Slider::set(double v) {
    double s = step_ > 0 ? std::round((v - min_) / step_) * step_ + min_ : v;
    s = std::clamp(s, min_, max_);
    if (s != value_) {
        value_ = s;
        if (onChange_) {
            onChange_(value_);
        }
    }
}

float Slider::labelW(Ui& ui) {
    if (!format_) {
        return 0;
    }
    return std::max(ui.theme.font->width(format_(max_)), ui.theme.font->width(format_(min_))) + 6;
}

float Slider::prefWidth(Ui& ui) { return 120 + labelW(ui); }
float Slider::minWidth(Ui& ui) { return 50 + labelW(ui); }
float Slider::prefHeight(Ui& ui, float) { return ui.theme.s[Theme::ROW] + 2; }

void Slider::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool en = isEnabled();
    float hv = animate(ui, 0, en && (ui.isHover(this) || dragging_) ? 1 : 0, t.s[Theme::HOVER_TIME]);
    float tx = x + 4, tw = trackW(ui), cy = y + h / 2;
    float frac = float((value_ - min_) / (max_ - min_));
    Skin::field(p, t, tx, cy - 2.5f, tw, 5, ui.isFocused(this) ? 1 : 0, en);
    float fw = tw * frac;
    if (fw > 1) {
        hgradient(p, tx + 1, cy - 1.5f, fw - 1, 3, t.c[Theme::FILL_BOTTOM], en ? t.c[Theme::FILL_TOP] : t.c[Theme::TEXT_DISABLED]);
    }
    float kx = tx + fw, kh = 11, kw = 7;
    Poly knob = {kx - kw / 2, cy - kh / 2 + 2, kx - kw / 2 + 2, cy - kh / 2, kx + kw / 2, cy - kh / 2,
                 kx + kw / 2, cy + kh / 2 - 2, kx + kw / 2 - 2, cy + kh / 2, kx - kw / 2, cy + kh / 2};
    if (hv > 0) {
        glow(p, knob, 4 * hv, alpha(t.c[Theme::GLOW], hv));
    }
    poly(p, knob, lerp(t.c[Theme::BUTTON_TOP], t.c[Theme::HOVER_TOP], hv), lerp(t.c[Theme::BUTTON_BOTTOM], t.c[Theme::HOVER_BOTTOM], hv),
         cy - kh / 2, kh);
    outline(p, knob, 1, en ? lerp(t.c[Theme::BUTTON_BORDER], t.c[Theme::ACCENT], hv) : t.c[Theme::FIELD_BORDER]);
    fill(p, kx - 0.5f, cy - 2, 1, 4, alpha(t.c[Theme::ACCENT], en ? 0.8f : 0.2f));
    if (format_) {
        Font& f = *t.font;
        std::string s = format_(value_);
        f.draw(p, s, x + w - f.width(s), y + (h - f.lineHeight()) / 2, en ? t.c[Theme::TEXT_BRIGHT] : t.c[Theme::TEXT_DISABLED]);
    }
}

void Slider::setFrom(Ui& ui, float mx) {
    float frac = (mx - x - 4) / trackW(ui);
    set(min_ + std::clamp(frac, 0.0f, 1.0f) * (max_ - min_));
}

bool Slider::press(Ui& ui, float mx, float, int button, int) {
    if (button != 0) {
        return false;
    }
    dragging_ = true;
    setFrom(ui, mx);
    return true;
}

bool Slider::wheel(Ui&, float, float, float dy) {
    set(value_ + (dy > 0 ? 1 : dy < 0 ? -1 : 0) * stepOr());
    return true;
}

bool Slider::key(Ui&, int key, int) {
    switch (key) {
        case Keys::LEFT:
        case Keys::DOWN: set(value_ - stepOr()); break;
        case Keys::RIGHT:
        case Keys::UP: set(value_ + stepOr()); break;
        case Keys::HOME: set(min_); break;
        case Keys::END: set(max_); break;
        default: return false;
    }
    return true;
}

// ImageView, GroupBox

void ImageView::draw(Ui&, Painter& p) {
    if (!image_ || owner_ != &p) {
        image_ = p.createImage(iw_, ih_, px_.data(), smooth_, false);
        owner_ = &p;
    }
    float s = std::min(w / iw_, h / ih_);
    float dw = iw_ * s, dh = ih_ * s;
    image(p, *image_, 0, 0, float(iw_), float(ih_), x + (w - dw) / 2, y + (h - dh) / 2, dw, dh, 0xFFFFFFFF);
}

GroupBox::GroupBox(std::string title, WidgetPtr content) : title_(toU32(title)), content_(std::move(content)) {
    adopt(*content_);
}

float GroupBox::top(Ui& ui) const { return ui.theme.font->lineHeight() + 4; }

float GroupBox::prefWidth(Ui& ui) {
    return std::max(content_->prefWidth(ui), ui.theme.font->width(title_) + 16) + 2 * ui.theme.s[Theme::PAD];
}

float GroupBox::minWidth(Ui& ui) { return content_->minWidth(ui) + 2 * ui.theme.s[Theme::PAD]; }

float GroupBox::prefHeight(Ui& ui, float width) {
    float pad = ui.theme.s[Theme::PAD];
    return top(ui) + content_->prefHeight(ui, width - 2 * pad) + pad;
}

void GroupBox::layout(Ui& ui) {
    float pad = ui.theme.s[Theme::PAD];
    content_->setBounds(x + pad, y + top(ui), w - 2 * pad, h - top(ui) - pad);
    content_->layout(ui);
}

Widget* GroupBox::hit(float px, float py) {
    if (!visible_ || !contains(px, py)) {
        return nullptr;
    }
    Widget* c = content_->hit(px, py);
    return c ? c : this;
}

void GroupBox::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    Font& f = *t.font;
    float ty = y + f.lineHeight() / 2;
    auto pts = cut(x, ty, w, h - (ty - y), 4, 0, 4, 0);
    poly(p, pts, 0x18000000, 0x30000000, ty, h);
    outline(p, pts, 1, alpha(t.c[Theme::PANEL_BORDER], 0.8f));
    float tw = f.width(title_) + 8;
    fill(p, x + 8, ty - 1, tw, 3, t.c[Theme::PANEL_BOTTOM]);
    f.draw(p, title_, x + 12, y, t.c[Theme::ACCENT_SOFT]);
    content_->draw(ui, p);
}

}  // namespace slopui
