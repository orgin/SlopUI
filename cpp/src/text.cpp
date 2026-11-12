// SlopUI: TextField, TextArea, NumberField, ComboBox, DatePicker, CalendarPopup (Java: the classes of the same
// names).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cwctype>

#include "slopui/ui.hpp"

namespace slopui {

using namespace Shapes;

static bool letterOrDigit(char32_t c) {
    return (c >= U'0' && c <= U'9') || (c >= U'A' && c <= U'Z') || (c >= U'a' && c <= U'z') || c >= 0xC0;
}

// TextField

TextField::TextField(std::string initial) : text_(toU32(initial)) { caret_ = anchor_ = text_.size(); }

void TextField::set(std::string s) {
    text_ = toU32(s);
    caret_ = anchor_ = text_.size();
    changed();
}

void TextField::changed() {
    if (onChange_) {
        onChange_(text());
    }
}

float TextField::prefHeight(Ui& ui, float) { return ui.theme.s[Theme::ROW] + 3; }

void TextField::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool focus = ui.isFocused(this);
    float fo = animate(ui, 0, focus ? 1 : 0, t.s[Theme::HOVER_TIME]);
    float hv = animate(ui, 1, ui.isHover(this) ? 1 : 0, t.s[Theme::HOVER_TIME]);
    Skin::field(p, t, x, y, w, h, std::max(fo, hv * 0.35f), isEnabled());
    Font& f = *t.font;
    const float pad = 4;
    float inner = w - 2 * pad - rightInset_;
    float caretX = f.width(std::u32string_view(text_).substr(0, caret_));
    if (caretX - scrollX_ > inner - 2) {
        scrollX_ = caretX - inner + 2;
    }
    if (caretX - scrollX_ < 0) {
        scrollX_ = caretX;
    }
    scrollX_ = std::clamp(scrollX_, 0.0f, std::max(0.0f, f.width(text_) - inner + 2));
    float tx = x + pad - scrollX_, ty = y + (h - f.lineHeight()) / 2;
    p.pushClip(x + 2, y + 1, w - 4 - rightInset_, h - 2);
    if (caret_ != anchor_) {
        size_t a = std::min(caret_, anchor_), b = std::max(caret_, anchor_);
        std::u32string_view v(text_);
        fill(p, tx + f.width(v.substr(0, a)), ty - 1, f.width(v.substr(a, b - a)), f.lineHeight() + 2,
             focus ? t.c[Theme::SELECTION] : alpha(t.c[Theme::SELECTION], 0.5f));
    }
    if (text_.empty() && !placeholder_.empty()) {
        f.draw(p, placeholder_, tx, ty, t.c[Theme::TEXT_DIM]);
    } else {
        f.draw(p, text_, tx, ty, isEnabled() ? t.c[Theme::TEXT_BRIGHT] : t.c[Theme::TEXT_DISABLED]);
    }
    if (focus) {
        ui.animating = true;
        if (std::fmod(ui.time - lastEdit_, 1.0) < 0.6) {
            fill(p, tx + caretX, ty - 1, 1, f.lineHeight() + 2, t.c[Theme::ACCENT]);
        }
    }
    p.popClip();
}

size_t TextField::indexAt(Ui& ui, float mx) {
    Font& f = *ui.theme.font;
    float rel = mx - (x + 4 - scrollX_), acc = 0;
    for (size_t i = 0; i < text_.size(); i++) {
        float cw = f.advance(text_[i]);
        if (rel < acc + cw / 2) {
            return i;
        }
        acc += cw;
    }
    return text_.size();
}

bool TextField::press(Ui& ui, float mx, float, int button, int clicks) {
    if (button != 0) {
        return false;
    }
    size_t i = indexAt(ui, mx);
    if (clicks == 2) {
        anchor_ = wordStart(i);
        caret_ = wordEnd(i);
    } else if (clicks >= 3) {
        selectAll();
    } else {
        caret_ = i;
        if (!(ui.mods & Keys::SHIFT)) {
            anchor_ = i;
        }
    }
    selecting_ = true;
    lastEdit_ = ui.time;
    return true;
}

void TextField::drag(Ui& ui, float mx, float) {
    if (selecting_) {
        caret_ = indexAt(ui, mx);
    }
}

size_t TextField::wordStart(size_t i) const {
    while (i > 0 && letterOrDigit(text_[i - 1])) {
        i--;
    }
    return i;
}

size_t TextField::wordEnd(size_t i) const {
    while (i < text_.size() && letterOrDigit(text_[i])) {
        i++;
    }
    return i;
}

size_t TextField::wordLeft(size_t i) const {
    while (i > 0 && !letterOrDigit(text_[i - 1])) {
        i--;
    }
    return wordStart(i);
}

size_t TextField::wordRight(size_t i) const {
    while (i < text_.size() && !letterOrDigit(text_[i])) {
        i++;
    }
    return wordEnd(i);
}

void TextField::deleteSelection() {
    size_t a = std::min(caret_, anchor_), b = std::max(caret_, anchor_);
    text_.erase(a, b - a);
    caret_ = anchor_ = a;
}

void TextField::insert(std::u32string_view s) {
    if (caret_ != anchor_) {
        deleteSelection();
    }
    std::u32string ok;
    for (char32_t c : s) {
        if (allowed_(c) && text_.size() + ok.size() < maxLength_) {
            ok.push_back(c);
        }
    }
    text_.insert(caret_, ok);
    caret_ += ok.size();
    anchor_ = caret_;
    changed();
}

bool TextField::typed(Ui& ui, char32_t cp) {
    lastEdit_ = ui.time;
    insert(std::u32string(1, cp));
    return true;
}

bool TextField::key(Ui& ui, int key, int mods) {
    bool shift = mods & Keys::SHIFT, ctrl = mods & Keys::CTRL;
    lastEdit_ = ui.time;
    switch (key) {
        case Keys::LEFT:
            caret_ = ctrl ? wordLeft(caret_) : (caret_ != anchor_ && !shift) ? std::min(caret_, anchor_) : caret_ > 0 ? caret_ - 1 : 0;
            break;
        case Keys::RIGHT:
            caret_ = ctrl ? wordRight(caret_)
                          : (caret_ != anchor_ && !shift) ? std::max(caret_, anchor_) : std::min(text_.size(), caret_ + 1);
            break;
        case Keys::HOME: caret_ = homeOf(caret_, ctrl); break;
        case Keys::END: caret_ = endOf(caret_, ctrl); break;
        case Keys::BACKSPACE:
            if (caret_ != anchor_) {
                deleteSelection();
            } else if (caret_ > 0) {
                size_t from = ctrl ? wordLeft(caret_) : caret_ - 1;
                text_.erase(from, caret_ - from);
                caret_ = anchor_ = from;
            }
            changed();
            return true;
        case Keys::DELETE_:
            if (caret_ != anchor_) {
                deleteSelection();
            } else if (caret_ < text_.size()) {
                size_t to = ctrl ? wordRight(caret_) : caret_ + 1;
                text_.erase(caret_, to - caret_);
            }
            changed();
            return true;
        case Keys::A:
            if (!ctrl) return false;
            selectAll();
            return true;
        case Keys::C:
        case Keys::X:
            if (!ctrl) return false;
            if (caret_ != anchor_ && ui.host) {
                size_t a = std::min(caret_, anchor_), b = std::max(caret_, anchor_);
                ui.host->setClipboard(toUtf8(std::u32string_view(text_).substr(a, b - a)));
                if (key == Keys::X) {
                    deleteSelection();
                    changed();
                }
            }
            return true;
        case Keys::V:
            if (!ctrl) return false;
            if (ui.host) {
                std::u32string c = toU32(ui.host->clipboard());
                if (!multiline()) {
                    std::replace(c.begin(), c.end(), U'\n', U' ');
                }
                insert(c);
            }
            return true;
        case Keys::ENTER:
        case Keys::KP_ENTER:
            if (multiline()) {
                insert(U"\n");
                return true;
            }
            if (onEnter_) {
                onEnter_(text());
                return true;
            }
            return false;
        default:
            return verticalKey(ui, key, shift);
    }
    if (!shift) {
        anchor_ = caret_;
    }
    return true;
}

// TextArea

TextArea::TextArea(std::string initial, int rows) : TextField(std::move(initial)), rows_(rows) {
    allow([](char32_t c) { return c >= 32 || c == U'\n'; });
    maxLength(size_t(1) << 20);
}

float TextArea::lineH(Ui& ui) const { return ui.theme.font->lineHeight() + 3; }
float TextArea::prefHeight(Ui& ui, float) { return rows_ * lineH(ui) + 6; }

size_t TextArea::lineStart(size_t i) const {
    while (i > 0 && text_[i - 1] != U'\n') {
        i--;
    }
    return i;
}

size_t TextArea::lineEnd(size_t i) const {
    while (i < text_.size() && text_[i] != U'\n') {
        i++;
    }
    return i;
}

int TextArea::lineOf(size_t i) const { return int(std::count(text_.begin(), text_.begin() + i, U'\n')); }

size_t TextArea::startOfLine(int line) const {
    size_t i = 0;
    for (int n = 0; n < line && i <= text_.size(); n++) {
        i = lineEnd(i) + 1;
    }
    return std::min(i, text_.size());
}

size_t TextArea::columnAt(Ui& ui, size_t start, float relX) {
    Font& f = *ui.theme.font;
    size_t end = lineEnd(start);
    float acc = 0;
    for (size_t i = start; i < end; i++) {
        float cw = f.advance(text_[i]);
        if (relX < acc + cw / 2) {
            return i;
        }
        acc += cw;
    }
    return end;
}

bool TextArea::verticalKey(Ui& ui, int key, bool shift) {
    int line = lineOf(caret_);
    int page = std::max(1, int((h - 6) / lineH(ui)) - 1);
    int target;
    switch (key) {
        case Keys::UP: target = line - 1; break;
        case Keys::DOWN: target = line + 1; break;
        case Keys::PAGE_UP: target = line - page; break;
        case Keys::PAGE_DOWN: target = line + page; break;
        default: return false;
    }
    Font& f = *ui.theme.font;
    if (goalX_ < 0) {
        size_t ls = lineStart(caret_);
        goalX_ = f.width(std::u32string_view(text_).substr(ls, caret_ - ls));
    }
    if (target < 0) {
        caret_ = 0;
    } else if (target >= lineCount()) {
        caret_ = text_.size();
    } else {
        caret_ = columnAt(ui, startOfLine(target), goalX_);
    }
    if (!shift) {
        anchor_ = caret_;
    }
    return true;
}

bool TextArea::key(Ui& ui, int key, int mods) {
    if (key != Keys::UP && key != Keys::DOWN && key != Keys::PAGE_UP && key != Keys::PAGE_DOWN) {
        goalX_ = -1;
    }
    if (key == Keys::TAB) {
        return false;
    }
    return TextField::key(ui, key, mods);
}

void TextArea::draw(Ui& ui, Painter& p) {
    Theme& t = ui.theme;
    bool focus = ui.isFocused(this);
    Skin::field(p, t, x, y, w, h, focus ? 1 : 0, isEnabled());
    Font& f = *t.font;
    float lh = lineH(ui), innerW = w - 8, innerH = h - 6;
    std::u32string_view v(text_);
    int cl = lineOf(caret_);
    size_t cls = lineStart(caret_);
    float cx = f.width(v.substr(cls, caret_ - cls));
    bool moved = caret_ != shownCaret_ || text_.size() != shownLength_;
    shownCaret_ = caret_;
    shownLength_ = text_.size();
    if (moved && cl * lh - scrollY_ < 0) {
        scrollY_ = cl * lh;
    }
    if (moved && (cl + 1) * lh - scrollY_ > innerH) {
        scrollY_ = (cl + 1) * lh - innerH;
    }
    if (cx - scrollX_ > innerW - 2) {
        scrollX_ = cx - innerW + 2;
    }
    if (cx - scrollX_ < 0) {
        scrollX_ = cx;
    }
    scrollY_ = std::clamp(scrollY_, 0.0f, std::max(0.0f, lineCount() * lh - innerH));
    p.pushClip(x + 2, y + 2, w - 4, h - 4);
    size_t a = std::min(caret_, anchor_), b = std::max(caret_, anchor_);
    size_t start = 0;
    for (int line = 0; start <= text_.size(); line++) {
        size_t end = lineEnd(start);
        float ly = y + 3 + line * lh - scrollY_;
        if (ly > y + h) {
            break;
        }
        if (ly + lh >= y) {
            float lx = x + 4 - scrollX_;
            if (a != b && b >= start && a <= end) {
                size_t s0 = std::max(a, start), s1 = std::min(b, end);
                float sx = f.width(v.substr(start, s0 - start)), sw = f.width(v.substr(s0, s1 - s0));
                if (b > end) {
                    sw += 3;
                }
                fill(p, lx + sx, ly, sw, lh, focus ? t.c[Theme::SELECTION] : alpha(t.c[Theme::SELECTION], 0.5f));
            }
            f.draw(p, v.substr(start, end - start), lx, ly + 1.5f, isEnabled() ? t.c[Theme::TEXT_BRIGHT] : t.c[Theme::TEXT_DISABLED]);
        }
        if (end >= text_.size()) {
            break;
        }
        start = end + 1;
    }
    if (focus) {
        ui.animating = true;
        if (std::fmod(ui.time, 1.0) < 0.6) {
            fill(p, x + 4 - scrollX_ + cx, y + 3 + cl * lh - scrollY_, 1, lh, t.c[Theme::ACCENT]);
        }
    }
    p.popClip();
    float total = lineCount() * lh;
    if (total > innerH) {
        float mh = std::max(8.0f, innerH * innerH / total), my = y + 3 + (innerH - mh) * scrollY_ / (total - innerH);
        fill(p, x + w - 3, my, 2, mh, alpha(t.c[Theme::ACCENT_SOFT], 0.6f));
    }
}

size_t TextArea::indexAt(Ui& ui, float mx) {
    float lh = lineH(ui);
    int line = int(std::floor((ui.my - y - 3 + scrollY_) / lh));
    line = std::clamp(line, 0, lineCount() - 1);
    return columnAt(ui, startOfLine(line), mx - (x + 4 - scrollX_));
}

bool TextArea::wheel(Ui& ui, float, float, float dy) {
    float lh = lineH(ui);
    float max = std::max(0.0f, lineCount() * lh - (h - 6));
    float before = scrollY_;
    scrollY_ = std::clamp(scrollY_ - dy * lh * 3, 0.0f, max);
    return scrollY_ != before || max > 0;
}

// NumberField

NumberField::NumberField(double value, double min, double max, double step, int decimals)
    : TextField(""), min_(min), max_(max), step_(step), decimals_(decimals) {
    rightInset_ = 12;
    allow([](char32_t c) { return (c >= U'0' && c <= U'9') || c == U'.' || c == U'-'; });
    maxLength(16);
    setValue(value);
}

std::string NumberField::format(double v) const {
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.*f", decimals_, v);
    return buf;
}

void NumberField::setValue(double v) {
    value_ = std::clamp(v, min_, max_);
    setting_ = true;
    set(format(value_));
    setting_ = false;
    if (onValue_) {
        onValue_(value_);
    }
}

void NumberField::changed() {
    TextField::changed();
    if (setting_) {
        return;
    }
    try {
        size_t used = 0;
        double v = std::stod(text(), &used);
        if (used == text().size() && v >= min_ && v <= max_) {
            value_ = v;
            if (onValue_) {
                onValue_(value_);
            }
        }
    } catch (...) {
        // (being typed: kept until it parses)
    }
}

float NumberField::prefWidth(Ui& ui) {
    return std::max(ui.theme.font->width(format(max_)), ui.theme.font->width(format(min_))) + 26;
}

void NumberField::draw(Ui& ui, Painter& p) {
    TextField::draw(ui, p);
    Theme& t = ui.theme;
    float bx = x + w - rightInset_;
    fill(p, bx, y + 2, 1, h - 4, alpha(t.c[Theme::FIELD_BORDER], 0.8f));
    bool onUp = ui.isHover(this) && ui.mx >= bx && ui.my < y + h / 2;
    bool onDown = ui.isHover(this) && ui.mx >= bx && ui.my >= y + h / 2;
    if (held_) {
        fill(p, bx + 1, held_ > 0 ? y + 1 : y + h / 2, rightInset_ - 2, h / 2 - 1, alpha(t.c[Theme::ACCENT], 0.3f));
    }
    Skin::chevron(p, bx + rightInset_ / 2, y + h * 0.3f, 5, 2, onUp ? t.c[Theme::ACCENT] : t.c[Theme::TEXT]);
    Skin::chevron(p, bx + rightInset_ / 2, y + h * 0.72f, 5, 0, onDown ? t.c[Theme::ACCENT] : t.c[Theme::TEXT]);
}

bool NumberField::press(Ui& ui, float mx, float my, int button, int clicks) {
    if (button == 0 && mx >= x + w - rightInset_) {
        held_ = my < y + h / 2 ? 1 : -1;
        heldSince_ = lastRepeat_ = ui.time;
        setValue(value_ + held_ * step_);
        return true;
    }
    return TextField::press(ui, mx, my, button, clicks);
}

void NumberField::tick(Ui& ui) {
    if (held_ && ui.time - heldSince_ > 0.45 && ui.time - lastRepeat_ > 0.06) {
        lastRepeat_ = ui.time;
        setValue(value_ + held_ * step_);
    }
    if (held_) {
        ui.animating = true;
    }
}

void NumberField::release(Ui& ui, float mx, float my, int button) {
    held_ = 0;
    TextField::release(ui, mx, my, button);
}

bool NumberField::wheel(Ui&, float, float, float dy) {
    setValue(value_ + (dy > 0 ? 1 : dy < 0 ? -1 : 0) * step_);
    return true;
}

bool NumberField::key(Ui& ui, int key, int mods) {
    if (key == Keys::UP || key == Keys::DOWN) {
        setValue(value_ + (key == Keys::UP ? step_ : -step_));
        return true;
    }
    return TextField::key(ui, key, mods);
}

// ComboBox

ComboBox::ComboBox(std::string initial, std::vector<std::string> items) : TextField(std::move(initial)), items_(std::move(items)) {
    rightInset_ = 15;
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

void ComboBox::showList(Ui& ui, bool all) {
    if (auto l = list_.lock()) {
        ui.closePopup(l.get());
    }
    std::string q = lower(text());
    std::vector<std::string> shown;
    for (auto& s : items_) {
        if (all || q.empty() || lower(s).find(q) != std::string::npos) {
            shown.push_back(s);
        }
    }
    if (shown.empty()) {
        list_.reset();
        return;
    }
    int sel = -1;
    if (all) {
        auto it = std::find(shown.begin(), shown.end(), text());
        sel = it == shown.end() ? 0 : int(it - shown.begin());
    }
    auto l = make<ListPopup>(*this, shown, sel, [this, shown](int i) {
        set(shown[i]);
        list_.reset();
    });
    list_ = l;
    ui.openPopup(l);
}

void ComboBox::draw(Ui& ui, Painter& p) {
    TextField::draw(ui, p);
    Theme& t = ui.theme;
    bool on = ui.isHover(this) && ui.mx >= x + w - rightInset_;
    fill(p, x + w - rightInset_, y + 3, 1, h - 6, alpha(t.c[Theme::FIELD_BORDER], 0.8f));
    Skin::chevron(p, x + w - 8, y + h / 2, 6, 0, on ? t.c[Theme::ACCENT] : t.c[Theme::TEXT]);
}

bool ComboBox::press(Ui& ui, float mx, float my, int button, int clicks) {
    if (button == 0 && mx >= x + w - rightInset_) {
        showList(ui, true);
        return true;
    }
    return TextField::press(ui, mx, my, button, clicks);
}

bool ComboBox::typed(Ui& ui, char32_t cp) {
    bool r = TextField::typed(ui, cp);
    showList(ui, false);
    return r;
}

bool ComboBox::key(Ui& ui, int key, int mods) {
    if (key == Keys::DOWN) {
        showList(ui, true);
        return true;
    }
    bool r = TextField::key(ui, key, mods);
    if (key == Keys::BACKSPACE || key == Keys::DELETE_ || ((mods & Keys::CTRL) && (key == Keys::X || key == Keys::V))) {
        showList(ui, false);
    }
    return r;
}

// DatePicker and its calendar

int DatePicker::daysIn(int y, int m) {
    return m == 2 ? (leap(y) ? 29 : 28) : (m == 4 || m == 6 || m == 9 || m == 11) ? 30 : 31;
}

static long long floorDiv(long long a, long long b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }

long long DatePicker::days(int y, int m, int d) {
    y -= m <= 2 ? 1 : 0;
    long long era = floorDiv(y, 400);
    long long yoe = y - era * 400;
    long long doy = (153LL * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

std::array<int, 3> DatePicker::civil(long long z) {
    z += 719468;
    long long era = floorDiv(z, 146097);
    long long doe = z - era * 146097;
    long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long y = yoe + era * 400;
    long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long long mp = (5 * doy + 2) / 153;
    long long d = doy - (153 * mp + 2) / 5 + 1;
    long long m = mp + (mp < 10 ? 3 : -9);
    return {int(y + (m <= 2 ? 1 : 0)), int(m), int(d)};
}

int DatePicker::weekday(int y, int m, int d) {
    long long z = days(y, m, d) + 3;
    return int(((z % 7) + 7) % 7);
}

std::array<int, 3> DatePicker::today() {
    using namespace std::chrono;
    long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    return civil(floorDiv(ms, 86400000LL));
}

DatePicker::DatePicker(int year, int month, int day) : TextField("") {
    rightInset_ = 14;
    allow([](char32_t c) { return (c >= U'0' && c <= U'9') || c == U'-'; });
    maxLength(10);
    setDate(year, month, day);
}

void DatePicker::setDate(int y, int m, int d) {
    year_ = y;
    month_ = m;
    day_ = std::min(d, daysIn(y, m));
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", year_, month_, day_);
    set(buf);
    if (onDate_) {
        onDate_(date());
    }
}

void DatePicker::changed() {
    TextField::changed();
    std::string s = text();
    if (s.size() == 10 && s[4] == '-' && s[7] == '-') {
        int y = 0, m = 0, d = 0;
        if (std::sscanf(s.c_str(), "%4d-%2d-%2d", &y, &m, &d) == 3 && m >= 1 && m <= 12 && d >= 1 && d <= daysIn(y, m)) {
            year_ = y;
            month_ = m;
            day_ = d;
            if (onDate_) {
                onDate_(date());
            }
        }
    }
}

float DatePicker::prefWidth(Ui& ui) { return ui.theme.font->width("0000-00-00") + 26; }

void DatePicker::draw(Ui& ui, Painter& p) {
    TextField::draw(ui, p);
    Theme& t = ui.theme;
    float bx = x + w - rightInset_;
    bool on = (ui.isHover(this) && ui.mx >= bx) || !calendar_.expired();
    fill(p, bx, y + 2, 1, h - 4, alpha(t.c[Theme::FIELD_BORDER], 0.8f));
    argb c = on ? t.c[Theme::ACCENT] : t.c[Theme::TEXT];
    float cx = bx + 3, cy = y + 3, cw = 8, ch = h - 6;
    outline(p, cut(cx, cy, cw, ch, 0, 0, 0, 0), 1, c);
    fill(p, cx, cy, cw, 2, c);
    fill(p, cx + 2, cy + 4, 1, 1, c);
    fill(p, cx + 5, cy + 4, 1, 1, c);
}

/** The month under a DatePicker: header with month/year arrows, the weekdays, six weeks of days. */
class CalendarPopup : public Popup {
public:
    explicit CalendarPopup(DatePicker& picker) : picker_(picker) {
        owner = &picker;
        year_ = picker.year_;
        month_ = picker.month_;
        day_ = picker.day_;
        matchAnchor_ = false;
        anchor(picker.x, picker.y, picker.w, picker.h, false);
    }

    float prefWidth(Ui& ui) override {
        measure(ui);
        return std::max(7 * cell_ + 8, ui.theme.font->width("September 0000") + 60);
    }
    float prefHeight(Ui& ui, float) override {
        measure(ui);
        return headH_ + daysH_ + 6 * cell_ + 6;
    }
    void layout(Ui& ui) override {
        measure(ui);
        cell_ = std::max(cell_, (w - 8) / 7);
    }

    void draw(Ui& ui, Painter& p) override {
        static const char* MONTHS[] = {"January", "February", "March", "April", "May", "June", "July", "August",
                                       "September", "October", "November", "December"};
        static const char* DAYS[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
        Theme& t = ui.theme;
        float o = opening(ui);
        float before = p.opacity();
        p.setOpacity(before * o);
        Skin::panel(p, t, x, y, w, h, t.s[Theme::CUT], true);
        Font& f = *t.font;
        std::string head = std::string(MONTHS[month_ - 1]) + " " + std::to_string(year_);
        f.draw(p, head, x + (w - f.width(head)) / 2, y + (headH_ - f.lineHeight()) / 2, t.c[Theme::TEXT_BRIGHT]);
        auto ax = arrowX();
        float ah = headH_ - 4, ay = y + 2;
        for (int i = 0; i < 4; i++) {
            bool on = ui.isHover(this) && ui.mx >= ax[i] && ui.mx < ax[i] + ah && ui.my >= ay && ui.my < ay + ah;
            if (on) {
                poly(p, cut(ax[i], ay, ah, ah, 3, 0, 3, 0), t.c[Theme::HOVER_TOP], t.c[Theme::HOVER_BOTTOM], ay, ah);
            }
            int dir = i >= 2 ? 1 : 3;
            float cy = ay + ah / 2, cx = ax[i] + ah / 2;
            argb col = on ? t.c[Theme::ACCENT] : t.c[Theme::TEXT];
            if (i == 0 || i == 3) {
                Skin::chevron(p, cx - 2, cy, 6, dir, col);
                Skin::chevron(p, cx + 2, cy, 6, dir, col);
            } else {
                Skin::chevron(p, cx, cy, 6, dir, col);
            }
        }
        float c = cell_;
        for (int d = 0; d < 7; d++) {
            f.draw(p, DAYS[d], x + 4 + d * c + (c - f.width(DAYS[d])) / 2, y + headH_ + 1,
                   d >= 5 ? t.c[Theme::ACCENT_SOFT] : t.c[Theme::TEXT_DIM]);
        }
        hgradient(p, x + 4, gridY() - 1, w - 8, 1, t.c[Theme::PANEL_BORDER], 0);
        int first = DatePicker::weekday(year_, month_, 1), n = DatePicker::daysIn(year_, month_);
        auto today = DatePicker::today();
        int hot = dayAt(ui.mx, ui.my);
        for (int i = 0; i < 42; i++) {
            int dnum = i - first + 1;
            if (dnum < 1 || dnum > n) {
                continue;
            }
            float cx = x + 4 + i % 7 * c, cy = gridY() + i / 7 * c;
            bool chosen = picker_.year_ == year_ && picker_.month_ == month_ && picker_.day_ == dnum;
            auto box = cut(cx + 1, cy + 1, c - 2, c - 2, 3, 0, 3, 0);
            if (chosen) {
                poly(p, box, t.c[Theme::FILL_TOP], t.c[Theme::FILL_BOTTOM], cy, c);
            } else if ((hot == dnum && ui.isHover(this)) || day_ == dnum) {
                poly(p, box, t.c[Theme::HOVER_TOP], t.c[Theme::HOVER_BOTTOM], cy, c);
            }
            if (today[0] == year_ && today[1] == month_ && today[2] == dnum) {
                outline(p, box, 1, t.c[Theme::ACCENT]);
            }
            std::string s = std::to_string(dnum);
            argb col = chosen ? 0xFF101008 : i % 7 >= 5 ? t.c[Theme::ACCENT_SOFT] : t.c[Theme::TEXT];
            f.draw(p, s, cx + (c - f.width(s)) / 2, cy + (c - f.lineHeight()) / 2, col);
        }
        p.setOpacity(before);
    }

    void release(Ui& ui, float mx, float my, int) override {
        float ah = headH_ - 4;
        if (my >= y + 2 && my < y + 2 + ah) {
            auto ax = arrowX();
            int steps[] = {-12, -1, 1, 12};
            for (int i = 0; i < 4; i++) {
                if (mx >= ax[i] && mx < ax[i] + ah) {
                    addMonths(steps[i]);
                }
            }
            return;
        }
        int d = dayAt(mx, my);
        if (d > 0) {
            DatePicker& picker = picker_;
            int y0 = year_, m0 = month_;
            ui.closePopup(this);
            picker.setDate(y0, m0, d);
        }
    }

    bool wheel(Ui&, float, float, float dy) override {
        addMonths(dy > 0 ? -1 : 1);
        return true;
    }

    bool key(Ui& ui, int key, int) override {
        long long z = DatePicker::days(year_, month_, day_);
        switch (key) {
            case Keys::LEFT: z -= 1; break;
            case Keys::RIGHT: z += 1; break;
            case Keys::UP: z -= 7; break;
            case Keys::DOWN: z += 7; break;
            case Keys::PAGE_UP: addMonths(-1); return true;
            case Keys::PAGE_DOWN: addMonths(1); return true;
            case Keys::ENTER:
            case Keys::KP_ENTER:
            case Keys::SPACE: {
                DatePicker& picker = picker_;
                int y0 = year_, m0 = month_, d0 = day_;
                ui.closePopup(this);
                picker.setDate(y0, m0, d0);
                return true;
            }
            case Keys::ESCAPE: ui.closePopup(this); return true;
            default: return false;
        }
        auto c = DatePicker::civil(z);
        year_ = c[0];
        month_ = c[1];
        day_ = c[2];
        return true;
    }

private:
    void measure(Ui& ui) {
        Font& f = *ui.theme.font;
        cell_ = std::max({f.width("00") + 7, f.width("Mo") + 7, f.lineHeight() + 6});
        headH_ = f.lineHeight() + 8;
        daysH_ = f.lineHeight() + 4;
    }
    float gridY() const { return y + headH_ + daysH_; }
    std::array<float, 4> arrowX() const {
        float ah = headH_ - 4;
        return {x + 3, x + 4 + ah, x + w - 4 - 2 * ah, x + w - 3 - ah};
    }
    int dayAt(float mx, float my) const {
        float c = cell_;
        if (mx < x + 4 || mx >= x + 4 + 7 * c || my < gridY() || my >= gridY() + 6 * c) {
            return -1;
        }
        int i = int((my - gridY()) / c) * 7 + int((mx - x - 4) / c);
        int d = i - DatePicker::weekday(year_, month_, 1) + 1;
        return d >= 1 && d <= DatePicker::daysIn(year_, month_) ? d : -1;
    }
    void addMonths(int n) {
        long long m = (long long)year_ * 12 + month_ - 1 + n;
        year_ = int(floorDiv(m, 12));
        month_ = int(m - (long long)year_ * 12) + 1;
        day_ = std::min(day_, DatePicker::daysIn(year_, month_));
    }
    DatePicker& picker_;
    int year_, month_, day_;
    float cell_ = 15, headH_ = 16, daysH_ = 11;
};

void DatePicker::openCalendar(Ui& ui) {
    auto c = make<CalendarPopup>(*this);
    calendar_ = c;
    ui.openPopup(c);
}

bool DatePicker::press(Ui& ui, float mx, float my, int button, int clicks) {
    if (button == 0 && mx >= x + w - rightInset_) {
        openCalendar(ui);
        return true;
    }
    return TextField::press(ui, mx, my, button, clicks);
}

bool DatePicker::key(Ui& ui, int key, int mods) {
    if (key == Keys::DOWN && (mods & Keys::ALT)) {
        openCalendar(ui);
        return true;
    }
    return TextField::key(ui, key, mods);
}

}  // namespace slopui
