// SlopUI's test program (Java: slopui.demo.Demo, MockGame, DemoWindows): the toolkit over a mockup of the game.
//   slopui_demo [--font FILE] [--title-font FILE] [--ui-size 0-4] [--theme N] [--open gallery|prefs|disks|load|editors]
// Fonts are not part of SlopUI: the demo uses the TrueType files given, else a system monospaced font (DejaVu Sans
// Mono, Noto Mono, Consolas).
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>

#include "slopui/sdl.hpp"

using namespace slopui;
using namespace slopui::Shapes;

namespace {

std::string fmt(const char* f, double v) {
    char b[64];
    std::snprintf(b, sizeof b, f, v);
    return b;
}

class Demo;

/** A mockup of the game under the UI: a 320 x 200 screen with the game's layout and three made-up screens. */
class MockGame : public Widget {
public:
    explicit MockGame(Demo& demo);
    void show(int s) { screen_ = s; }
    int screen() const { return screen_; }
    float prefWidth(Ui&) override { return 320; }
    float prefHeight(Ui&, float) override { return 200; }
    void draw(Ui& ui, Painter& p) override;
    bool press(Ui& ui, float mx, float my, int button, int clicks) override;
    void release(Ui& ui, float mx, float my, int button) override;
    bool key(Ui& ui, int key, int mods) override;

private:
    std::array<float, 3> frame() const {
        float gp = std::min(w / 320.0f, h / 200.0f);
        return {x + (w - 320 * gp) / 2, y + (h - 200 * gp) / 2, gp};
    }
    int iconAt(float mx, float my) const;
    Font& gameFont(float gp);
    void icon(Painter& p, int i, float cx, float cy, float g);
    void stationView(Ui& ui, Painter& p, float mx, float my, float g, Font& font);
    void starMap(Ui& ui, Painter& p, float mx, float my, float g, Font& font);
    void production(Painter& p, float mx, float my, float g, Font& font);
    Demo& demo_;
    int screen_ = 0, flash_ = -1;
    double flashAt_ = 0;
    std::vector<std::array<float, 4>> stars_;
    static constexpr const char* ICONS[12] = {"Station View", "Star Map", "Production", "Research", "Training",
                                              "Shuttle Bay", "Resource", "Mining Store", "Stocktaker", "Disk Access",
                                              "News", "Earth City"};
    static constexpr const char* SCREENS[3] = {"Station View", "Star Map", "Production"};
};

class Demo {
public:
    Demo(const std::filesystem::path& mono, const std::filesystem::path& bold, const std::filesystem::path& themes);
    void run();
    SdlHost host;
    Ui ui;
    std::vector<Theme> themes;
    int themeIndex = 0;
    std::vector<std::function<void()>> perFrame;
    std::shared_ptr<MockGame> game;

    std::shared_ptr<Font> font(bool bold, float size) {
        std::string k = (bold ? "b" : "m") + std::to_string(size);
        auto it = fonts_.find(k);
        if (it == fonts_.end()) {
            it = fonts_.emplace(k, std::make_shared<TtfFont>(bold ? bold_ : mono_, size)).first;
        }
        return it->second;
    }
    void setTheme(int i) {
        themeIndex = std::clamp(i, 0, int(themes.size()) - 1);
        ui.theme = themes[themeIndex];
        applyFonts();
    }
    std::vector<std::string> themeNames() const {
        std::vector<std::string> l;
        for (auto& t : themes) {
            l.push_back(t.name);
        }
        return l;
    }

    // windows
    std::shared_ptr<Window> gallery();
    std::shared_ptr<Window> preferences();
    std::shared_ptr<Window> loadGame();
    std::shared_ptr<Window> saveDisks();
    std::shared_ptr<Window> editors();
    std::shared_ptr<Window> floating(bool titleBar);
    std::shared_ptr<Window> busy();

private:
    void applyFonts() {
        for (Theme* t : {&ui.theme}) {
            t->font = font(false, 10);
            t->titleFontOverride = font(true, 10);
        }
        for (auto& t : themes) {
            t.font = font(false, 10);
            t.titleFontOverride = font(true, 10);
        }
    }
    void menus();
    WidgetPtr buttonsPage();
    WidgetPtr inputsPage();
    WidgetPtr progressPage();
    WidgetPtr tablePage();
    WidgetPtr textPage();
    WidgetPtr panesPage();
    std::vector<unsigned char> mono_, bold_;
    std::map<std::string, std::shared_ptr<Font>> fonts_;
};

std::vector<unsigned char> readFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot read " + p.string());
    }
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::string readText(const std::filesystem::path& p) {
    std::ifstream in(p);
    std::stringstream s;
    s << in.rdbuf();
    return s.str();
}

/** A weak handle to a window for its own buttons' actions (no cycle). */
using WinRef = std::shared_ptr<std::weak_ptr<Window>>;
WinRef winRef() { return std::make_shared<std::weak_ptr<Window>>(); }
std::function<void()> closer(Ui& ui, WinRef r) {
    return [&ui, r] {
        if (auto w = r->lock()) {
            ui.close(w.get());
        }
    };
}

// MockGame

MockGame::MockGame(Demo& demo) : demo_(demo) {
    std::mt19937 r(1977);
    std::uniform_real_distribution<float> u(0, 1);
    for (int i = 0; i < 160; i++) {
        stars_.push_back({u(r) * 256, u(r) * 168, u(r), u(r) * 6.28f});
    }
}

Font& MockGame::gameFont(float gp) {
    float s = std::max(0.25f, std::round(gp * 4) / 4);
    return *demo_.font(false, 8 * 1.11f * s);
}

int MockGame::iconAt(float mx, float my) const {
    auto f = frame();
    float gx = (mx - f[0]) / f[2], gy = (my - f[1]) / f[2];
    if (gx < 0 || gx >= 48 || gy < 88 || gy >= 184) {
        return -1;
    }
    int i = int((gy - 88) / 16) * 2 + int(gx / 24);
    return i < 12 ? i : -1;
}

void MockGame::icon(Painter& p, int i, float cx, float cy, float g) {
    argb c = 0xFFD8D8C8;
    switch (i % 4) {
        case 0: outline(p, {cx - 5 * g, cy, cx, cy - 4 * g, cx + 5 * g, cy, cx, cy + 4 * g}, g, c); break;
        case 1:
            fill(p, cx - 5 * g, cy - 3 * g, 10 * g, g, c);
            fill(p, cx - 5 * g, cy, 10 * g, g, c);
            fill(p, cx - 5 * g, cy + 3 * g, 10 * g, g, c);
            break;
        case 2:
            line(p, cx - 5 * g, cy + 4 * g, cx, cy - 4 * g, g, c);
            line(p, cx, cy - 4 * g, cx + 5 * g, cy + 4 * g, g, c);
            break;
        default: outline(p, cut(cx - 5 * g, cy - 4 * g, 10 * g, 8 * g, 2 * g, 0, 2 * g, 0), g, c);
    }
}

void MockGame::draw(Ui& ui, Painter& p) {
    fill(p, x, y, w, h, 0xFF000000);
    auto f = frame();
    float ox = f[0], oy = f[1], g = f[2];
    Font& font = gameFont(g);
    int hot = ui.isHover(this) ? iconAt(ui.mx, ui.my) : -1;
    std::string hoverText = hot >= 0 ? ICONS[hot] : "";
    std::string top = "MOCKUP - SLOPUI DEMO (C++)";
    if (!hoverText.empty()) {
        top = hoverText;
        std::transform(top.begin(), top.end(), top.begin(), ::toupper);
    }
    font.draw(p, top, ox + 2 * g, oy + 4 * g, 0xFFFFFFFF);
    auto sun = cut(ox + 232 * g, oy + 1 * g, 86 * g, 14 * g, 2 * g, 2 * g, 2 * g, 2 * g);
    poly(p, sun, 0xFF2A3420, 0xFF141A10, oy, 16 * g);
    outline(p, sun, g, 0xFF8A9A70);
    font.draw(p, "THE SUN", ox + 248 * g, oy + 4 * g, 0xFFFFF060);
    vgradient(p, ox, oy + 16 * g, 48 * g, 72 * g, 0xFF101810, 0xFF000000);
    fill(p, ox + 4 * g, oy + 22 * g, 30 * g, 8 * g, 0xFF2040A0);
    for (int i = 0; i < 12; i++) {
        float ix = ox + (i % 2) * 24 * g, iy = oy + (88 + i / 2 * 16) * g;
        bool lit = i == flash_ && ui.time - flashAt_ < 0.25;
        if (lit) {
            ui.animating = true;
        }
        auto r = cut(ix + g, iy + g, 22 * g, 14 * g, 0, 0, 0, 0);
        poly(p, r, lit ? 0xFF8AA060 : i == hot ? 0xFF3A4A2A : 0xFF1E2618, lit ? 0xFF4A6030 : 0xFF0A0E08, iy, 16 * g);
        outline(p, r, std::max(1.0f, g * 0.6f), i == screen_ ? 0xFFE8D860 : 0xFFB0B0A0);
        icon(p, i, ix + 12 * g, iy + 8 * g, g);
    }
    float mx = ox + 48 * g, my = oy + 16 * g;
    p.pushClip(mx, my, 272 * g, 168 * g);
    if (screen_ == 0) {
        stationView(ui, p, mx, my, g, font);
    } else if (screen_ == 1) {
        starMap(ui, p, mx, my, g, font);
    } else {
        production(p, mx, my, g, font);
    }
    p.popClip();
    float sy = oy + 184 * g;
    for (auto b : {cut(ox + 48 * g, sy + g, 112 * g, 14 * g, 3 * g, 3 * g, 3 * g, 3 * g),
                   cut(ox + 162 * g, sy + g, 156 * g, 14 * g, 3 * g, 3 * g, 3 * g, 3 * g)}) {
        poly(p, b, 0xFF242C1C, 0xFF0C100A, sy, 16 * g);
        outline(p, b, g, 0xFF8A9A70);
    }
    font.draw(p, "3100 580.00", ox + 60 * g, sy + 5 * g, 0xFFFFFFFF);
    std::string name = SCREENS[screen_];
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    font.draw(p, name, ox + 172 * g, sy + 5 * g, 0xFFFFFFFF);
}

void MockGame::stationView(Ui& ui, Painter& p, float mx, float my, float g, Font& font) {
    vgradient(p, mx, my, 272 * g, 168 * g, 0xFF020408, 0xFF0A1020);
    for (auto& s : stars_) {
        float tw = float(0.55 + 0.45 * std::sin(ui.time * (0.6 + s[2]) + s[3]));
        argb a = argb(255 * (0.3 + 0.7 * s[2]) * tw);
        float sz = s[2] > 0.85f ? 1.5f * g : g;
        fill(p, mx + s[0] * g, my + s[1] * g, sz, sz, a << 24 | 0xFFFFF0);
    }
    ui.animating = true;
    float pcx = mx + 196 * g, pcy = my + 120 * g, pr = 54 * g;
    Poly pts;
    for (int i = 0; i < 48; i++) {
        double a = i * M_PI * 2 / 48;
        pts.push_back(pcx + float(std::cos(a)) * pr);
        pts.push_back(pcy + float(std::sin(a)) * pr);
    }
    glow(p, pts, 8 * g, 0x502A60C0);
    poly(p, pts, 0xFF4A78B0, 0xFF0A1830, pcy - pr, 2 * pr);
    float sx = mx + 60 * g + float(std::sin(ui.time * 0.3)) * 3 * g, sy = my + 70 * g;
    fill(p, sx, sy, 90 * g, 6 * g, 0xFF8A8A80);
    fill(p, sx + 30 * g, sy - 14 * g, 30 * g, 34 * g, 0xFF6A6A60);
    fill(p, sx + 36 * g, sy - 10 * g, 18 * g, 4 * g, 0xFFE8D860);
    fill(p, sx - 20 * g, sy - 2 * g, 20 * g, 10 * g, 0xFF4A5A70);
    fill(p, sx + 90 * g, sy - 2 * g, 20 * g, 10 * g, 0xFF4A5A70);
    font.draw(p, "RIGHT-CLICK: CONTEXT MENU", mx + 6 * g, my + 156 * g, 0x90FFFFFF);
}

void MockGame::starMap(Ui& ui, Painter& p, float mx, float my, float g, Font& font) {
    fill(p, mx, my, 272 * g, 168 * g, 0xFF04060A);
    for (int i = 0; i <= 272; i += 34) {
        fill(p, mx + i * g, my, std::max(1.0f, g / 2), 168 * g, 0x302A6A3A);
    }
    for (int i = 0; i <= 168; i += 28) {
        fill(p, mx, my + i * g, 272 * g, std::max(1.0f, g / 2), 0x302A6A3A);
    }
    const char* names[] = {"SOL", "PROXIMA", "BARNARD", "SIRIUS", "LALANDE", "TAU CETI", "PROCYON", "CYGNI"};
    std::mt19937 r(7);
    std::uniform_real_distribution<float> u(0, 1);
    for (const char* nm : names) {
        float sx = mx + (20 + u(r) * 230) * g, sy = my + (14 + u(r) * 140) * g;
        float pulse = float(0.6 + 0.4 * std::sin(ui.time * 2 + sx));
        Poly d = {sx, sy - 3 * g, sx + 3 * g, sy, sx, sy + 3 * g, sx - 3 * g, sy};
        glow(p, d, 5 * g * pulse, 0x80FFE080);
        poly(p, d, 0xFFFFF0C0, 0xFFE0A040, sy - 3 * g, 6 * g);
        font.draw(p, nm, sx + 5 * g, sy - 3 * g, 0xFFB8C890);
    }
    ui.animating = true;
}

void MockGame::production(Painter& p, float mx, float my, float g, Font& font) {
    vgradient(p, mx, my, 272 * g, 168 * g, 0xFF181410, 0xFF080604);
    const char* items[] = {"MINING RIG", "SHUTTLE", "ORBITAL FACTORY", "DRONE", "PROBE", "SOLAR PANEL",
                           "CARGO POD", "MARINES", "LASER", "TORPEDO"};
    for (int i = 0; i < 10; i++) {
        float ry = my + (8 + i * 15) * g;
        auto row = cut(mx + 8 * g, ry, 256 * g, 13 * g, 3 * g, 0, 3 * g, 0);
        poly(p, row, i % 2 == 0 ? 0xFF2A2418 : 0xFF221E14, 0xFF100C08, ry, 13 * g);
        outline(p, row, std::max(1.0f, g / 2), 0xFF5A5040);
        font.draw(p, items[i], mx + 14 * g, ry + 3 * g, 0xFFE0D8C0);
        font.draw(p, std::to_string((i * 37 + 11) % 99), mx + 220 * g, ry + 3 * g, 0xFFE8D860);
    }
}

bool MockGame::press(Ui& ui, float mx, float my, int button, int) {
    if (button == 1) {
        auto m = std::make_shared<Menu>("Context");
        m->item("Station View", [this] { show(0); });
        m->item("Star Map", [this] { show(1); });
        m->item("Production", [this] { show(2); });
        m->separator();
        m->item("Toast here", [&ui] { ui.toast("A context menu item ran"); });
        ui.contextMenu(m);
        return true;
    }
    int i = iconAt(mx, my);
    if (button == 0 && i >= 0) {
        flash_ = i;
        flashAt_ = ui.time;
        return true;
    }
    return false;
}

void MockGame::release(Ui& ui, float mx, float my, int button) {
    int i = iconAt(mx, my);
    if (button == 0 && i >= 0 && i == flash_) {
        if (i < 3) {
            show(i);
        } else {
            ui.toast(std::string("The game got a click: ") + ICONS[i]);
        }
    }
}

bool MockGame::key(Ui& ui, int key, int mods) {
    if (key >= Keys::A && key <= Keys::Z && mods == 0) {
        ui.toast(std::string("The game got the key ") + char(key));
        return true;
    }
    return false;
}

// Demo

Demo::Demo(const std::filesystem::path& mono, const std::filesystem::path& bold, const std::filesystem::path& themeDir)
    : host("SlopUI demo (C++)", 1280, 824), ui(Theme(), &host) {
    mono_ = readFile(mono);
    bold_ = readFile(bold);
    Theme base;
    themes.push_back(base);
    for (auto& d : {themeDir / "dark.json", std::filesystem::path("themes/dark.json"), std::filesystem::path("../themes/dark.json")}) {
        if (std::filesystem::exists(d)) {
            themes.push_back(base.read(readText(d)));
            break;
        }
    }
    host.setUi(ui);
    applyFonts();
    game = make<MockGame>(*this);
    ui.setBackground(game);
    menus();
    ui.shortcut(Keys::F11, [this] { host.setFullscreen(!host.fullscreen()); });
    ui.shortcut(Keys::F1, [this] { ui.open(gallery()); });
}

void Demo::menus() {
    auto file = std::make_shared<Menu>("File");
    file->item("New Game", [this] {
        ui.confirm("New Game", "Start a new game? The game in progress is lost.", "New Game", "Cancel",
                   [this] { ui.toast("A new game"); });
    });
    file->item("Load Game...", [this] { ui.open(loadGame()); });
    file->item("Save Game...", [this] {
        ui.ask("Save Game", "Name of the save:", "my game", [this](const std::string& s) { ui.toast("Saved as " + s); });
    });
    file->item("Manage Save Disks...", [this] { ui.open(saveDisks()); });
    file->separator();
    file->item("Quit", [this] { ui.confirm("Quit", "Quit the demo?", "Quit", "Cancel", [this] { host.requestClose(); }); });

    auto display = std::make_shared<Menu>("Display");
    for (int k = 1; k <= 4; k++) {
        display->item(std::to_string(k) + "x" + std::to_string(k) + " pixels", [this, k] { host.setSize(320 * k, 200 * k + 24); });
    }
    display->item("720p", [this] { host.setSize(1280, 720); });
    display->item("1080p", [this] { host.setSize(1920, 1080); });
    display->item("Fullscreen", [this] { host.setFullscreen(!host.fullscreen()); })
        .check([this] { return host.fullscreen(); })
        .shortcut("F11");
    display->separator();
    auto size = std::make_shared<Menu>("UI size");
    const char* sizes[] = {"Auto", "1x", "2x", "3x", "4x"};
    for (int i = 0; i < 5; i++) {
        size->item(sizes[i], [this, i] { host.setUiScale(i); }).radioMark([this, i] { return host.uiScale() == i; });
    }
    display->add(MenuItem::sub(size));
    auto theme = std::make_shared<Menu>("Theme");
    std::weak_ptr<Menu> themeWeak = theme;
    theme->onOpen = [this, themeWeak] {
        auto t = themeWeak.lock();
        t->items.clear();
        for (int i = 0; i < int(themes.size()); i++) {
            t->item(themes[i].name, [this, i] { setTheme(i); }).radioMark([this, i] { return themeIndex == i; });
        }
    };
    display->add(MenuItem::sub(theme));

    auto widgets = std::make_shared<Menu>("Widgets");
    widgets->item("Widget gallery...", [this] { ui.open(gallery()); }).shortcut("F1");
    widgets->item("Preferences...", [this] { ui.open(preferences()); });
    widgets->item("Text editors...", [this] { ui.open(editors()); });
    widgets->item("Floating pane", [this] { ui.open(floating(true)); });
    auto fixed = std::make_shared<std::shared_ptr<Window>>();
    widgets->item("Fixed pane (no title bar)", [this, fixed] {
        if (*fixed && ui.isOpen(fixed->get())) {
            ui.close(fixed->get());
        } else {
            *fixed = floating(false);
            ui.open(*fixed);
        }
    }).check([this, fixed] { return *fixed && ui.isOpen(fixed->get()); });
    widgets->separator();
    auto dialogs = std::make_shared<Menu>("Dialogs");
    dialogs->item("Message", [this] {
        ui.message("Message", "Something happened. This is a longer message that wraps onto several lines when the window is narrow.");
    });
    dialogs->item("Question", [this] {
        ui.confirm("Question", "Overwrite the save \"quick\"?", "Overwrite", "Cancel", [this] { ui.toast("Overwritten"); });
    });
    dialogs->item("Ask for a name", [this] {
        ui.ask("Name", "Name of the station:", "Station 1", [this](const std::string& s) { ui.toast("Named " + s); });
    });
    dialogs->item("Busy (progress)", [this] { ui.open(busy()); });
    widgets->add(MenuItem::sub(dialogs));
    widgets->item("Toast", [this] { ui.toast("A toast: a short message that fades"); });
    auto screens = std::make_shared<Menu>("Game screen");
    const char* names[] = {"Station View", "Star Map", "Production"};
    for (int i = 0; i < 3; i++) {
        screens->item(names[i], [this, i] { game->show(i); }).radioMark([this, i] { return game->screen() == i; });
    }
    widgets->add(MenuItem::sub(screens));

    auto about = std::make_shared<Menu>("About");
    about->item("About...", [this] {
        ui.message("About", "SlopUI demo (C++)\n\nThe toolkit (slopui) draws with a Painter and has no game or GL code; "
                            "this demo runs it on SDL3 and OpenGL over a mockup of the game.");
    });
    ui.menuBar->add(file).add(display).add(widgets).add(about);
}

static std::vector<WidgetPtr> formRow(const std::string& l, WidgetPtr w) { return {make<Label>(l), std::move(w)}; }

static std::shared_ptr<Grid> form(std::vector<std::vector<WidgetPtr>> rows) {
    auto g = make<Grid>(2);
    for (auto& r : rows) {
        for (auto& w : r) {
            g->add(w);
        }
    }
    return g;
}

WidgetPtr Demo::buttonsPage() {
    auto disabled = make<Button>("Disabled", [] {});
    disabled->setEnabled(false);
    auto tip = make<Button>("With a tooltip", [this] { ui.toast("Clicked"); });
    tip->setTooltip("Buttons act on the release; move off before releasing to cancel.");
    auto def = make<Button>("Default", [this] { ui.toast("Default clicked"); });
    def->accent(true);
    auto buttons = Box::row({make<Button>("Normal", [this] { ui.toast("Normal clicked"); }), def, tip, disabled});
    buttons->responsive();
    auto c3 = make<CheckBox>("Disabled", true);
    c3->setEnabled(false);
    auto t3 = make<Toggle>("Disabled", false);
    t3->setEnabled(false);
    auto g = make<RadioGroup>(0);
    g->onChange([this](int i) { ui.toast("Radio " + std::to_string(i + 1)); });
    auto radios = Box::row({g->button("First"), g->button("Second"), g->button("Third")});
    radios->responsive();
    auto checks = make<GroupBox>("Check boxes", Box::column({make<CheckBox>("Check box", true), make<CheckBox>("Another one", false), c3}));
    auto toggles = make<GroupBox>("Toggles", Box::column({make<Toggle>("Toggle (3D effects)", true), make<Toggle>("Toggle (sound)", false), t3}));
    checks->grow = toggles->grow = 1;
    auto both = Box::row({checks, toggles});
    both->responsive();
    auto col = Box::column({make<GroupBox>("Buttons", buttons), both, make<GroupBox>("Radio buttons", radios), make<Separator>(),
                            make<Label>("Separators fade at their ends. Tab moves the focus; Space and Enter press.")});
    col->gap(6);
    return col;
}

WidgetPtr Demo::inputsPage() {
    auto name = make<TextField>("Station One");
    name->placeholder("Type a name");
    auto empty = make<TextField>("");
    empty->placeholder("Placeholder text");
    auto dd = make<Dropdown>(std::vector<std::string>{"Auto", "1x", "2x", "4x", "8x"}, 0);
    dd->onChange([this](int i) { ui.toast("Scale " + std::to_string(i)); });
    auto combo = make<ComboBox>("", std::vector<std::string>{"Alpha Centauri run", "Barnard outpost", "Before the war", "Cygni rigs",
                                                              "Earth only", "Last station", "Proxima trading", "Sirius battle",
                                                              "Tau Ceti mining"});
    combo->placeholder("Type to filter...");
    auto t = DatePicker::today();
    auto vol = make<Slider>(0, 100, 1, 70);
    vol->format([](double v) { return std::to_string(int(v)) + "%"; });
    auto hold = make<Slider>(0.1, 0.5, 0.05, 0.5);
    hold->format([](double v) { return fmt("%.2f s", v); });
    return form({formRow("Text field", name), formRow("Empty", empty), formRow("Number", make<NumberField>(25, 1, 100, 1, 0)),
                 formRow("Decimal", make<NumberField>(0.5, 0.1, 2.0, 0.1, 1)), formRow("Dropdown", dd), formRow("Combobox", combo),
                 formRow("Date picker", make<DatePicker>(t[0], t[1], t[2])), formRow("Volume", vol), formRow("Hold delay", hold)});
}

WidgetPtr Demo::progressPage() {
    auto p1 = make<ProgressBar>(0);
    auto p2 = make<ProgressBar>(0.3f);
    std::weak_ptr<ProgressBar> w1 = p1;
    p1->text([w1] { auto p = w1.lock(); return std::to_string(p ? int(std::lround(p->value() * 100)) : 0) + "%"; });
    perFrame.push_back([this, w1] {
        if (auto p = w1.lock()) {
            p->set(float(std::fmod(ui.time * 0.15, 1.0)));
        }
    });
    std::weak_ptr<ProgressBar> w2 = p2;
    auto more = make<Button>("+10%", [w2] { if (auto p = w2.lock()) p->set(std::min(1.0f, p->value() + 0.1f)); });
    auto less = make<Button>("-10%", [w2] { if (auto p = w2.lock()) p->set(std::max(0.0f, p->value() - 0.1f)); });
    auto col = Box::column({make<Label>("Running:"), p1, make<Label>("Stepped (animates to the value):"), p2, Box::row({less, more}),
                            make<Label>("Busy (no amount known):"), make<ProgressBar>(-1)});
    col->gap(5);
    return col;
}

WidgetPtr Demo::tablePage() {
    auto table = make<Table>(std::vector<std::string>{"Name", "Date", "Stations", "Credits"}, std::vector<float>{3, 2, 1.2f, 1.6f}, 8);
    const char* names[] = {"Alpha Centauri run", "Barnard outpost", "Before the war", "Cygni rigs", "Earth only", "Last station",
                           "Proxima trading", "Sirius battle", "Tau Ceti mining", "The long one", "Tutorial",
                           "With a much too long name that will not fit"};
    std::vector<std::vector<std::string>> rows;
    for (int i = 0; i < 12; i++) {
        char d[32];
        std::snprintf(d, sizeof d, "21%02d-%02d-%02d", 10 + i * 3, 1 + i % 12, 1 + i * 2 % 28);
        rows.push_back({names[i], d, std::to_string(i % 7 + 1), std::to_string(1000 + i * 7919 % 90000)});
    }
    table->setRows(rows);
    std::weak_ptr<Table> tw = table;
    table->onActivate([this, tw](int r) { if (auto t = tw.lock()) ui.toast("Opened: " + t->row(r)[0]); });
    auto col = Box::column({table, make<Label>("Click a header to sort (again: reversed). Double click or Enter opens a row.")});
    col->gap(5);
    return col;
}

WidgetPtr Demo::textPage() {
    std::string lorem = "Long text wraps at the window's width. Make the window narrower (or the UI larger) and the lines break "
                        "again; rows of buttons wrap onto more lines when they no longer fit, and tabs wrap too.\n\nA new paragraph "
                        "after an empty line. Accents: åäö éèê ü ß ç ÅÄÖ.";
    auto t = make<Label>("A title label");
    t->title();
    auto col = Box::column({t, make<Label>(lorem), make<GroupBox>("Group box", make<Label>("A titled frame around related settings."))});
    col->gap(6);
    return col;
}

WidgetPtr Demo::panesPage() {
    auto list = make<Table>(std::vector<std::string>{"Item", "Qty"}, std::vector<float>{3, 1}, 6);
    std::vector<std::vector<std::string>> rows;
    for (std::string s : {"Mining rig", "Shuttle", "Factory", "Drone", "Probe", "Panel", "Pod", "Laser"}) {
        rows.push_back({s, std::to_string(s.size() * 3)});
    }
    list->setRows(rows);
    auto lots = Box::column(), lots2 = Box::column();
    for (int i = 1; i <= 20; i++) {
        lots->add(make<Label>("Scrolled line " + std::to_string(i)));
        lots2->add(make<Label>("No bars: wheel or middle-drag " + std::to_string(i)));
    }
    auto withBars = make<ScrollPane>(lots, true);
    withBars->viewHeight(60);
    auto noBars = make<ScrollPane>(lots2, false);
    noBars->viewHeight(60);
    auto split = make<SplitPane>(list, make<SplitPane>(withBars, noBars, true, 0.5f), false, 0.45f);
    auto col = Box::column({make<Label>("Drag the separators (double click: back)."), split});
    col->gap(4);
    return col;
}

std::shared_ptr<Window> Demo::gallery() {
    auto tabs = make<Tabs>();
    tabs->add("Buttons", buttonsPage()).add("Inputs", inputsPage()).add("Progress", progressPage()).add("Table", tablePage())
        .add("Text", textPage()).add("Panes", panesPage());
    auto r = winRef();
    auto w = make<Window>("Widget gallery", tabs);
    w->width(440).modal(false).resizable(true).defaultButton(make<Button>("Close", closer(ui, r)));
    *r = w;
    return w;
}

std::shared_ptr<Window> Demo::preferences() {
    auto size = make<Dropdown>(std::vector<std::string>{"Auto", "1x", "2x", "3x", "4x"}, host.uiScale());
    size->onChange([this](int i) { host.setUiScale(i); });
    auto theme = make<Dropdown>(themeNames(), themeIndex);
    theme->onChange([this](int i) { setTheme(i); });
    auto full = make<Toggle>("Fullscreen", host.fullscreen());
    full->onChange([this](bool on) { host.setFullscreen(on); });
    auto gs = make<RadioGroup>(0);
    auto scale = Box::row({gs->button("Auto"), gs->button("1x"), gs->button("2x"), gs->button("4x"), gs->button("8x")});
    scale->responsive();
    auto display = Box::column({form({formRow("UI size", size), formRow("Theme", theme)}), full, make<CheckBox>("3D effects", true),
                                make<GroupBox>("Graphics scale", scale)});
    display->gap(5);
    auto hold = make<Slider>(0.1, 0.5, 0.05, 0.5);
    hold->format([](double v) { return fmt("%.2f s", v); });
    auto scroll = make<Slider>(1, 10, 1, 3);
    scroll->format([](double v) { return std::to_string(int(v)); });
    auto controls = Box::column({form({formRow("Hold delay", hold), formRow("Scroll speed", scroll),
                                       formRow("Double click", make<NumberField>(400, 100, 900, 50, 0))}),
                                 make<Label>("Hold delay: how long a held button waits before it repeats.")});
    controls->gap(5);
    auto music = make<Slider>(0, 100, 1, 80);
    music->format([](double v) { return std::to_string(int(v)) + "%"; });
    auto fx = make<Slider>(0, 100, 1, 100);
    fx->format([](double v) { return std::to_string(int(v)) + "%"; });
    auto sound = Box::column({form({formRow("Music", music), formRow("Effects", fx)}), make<Toggle>("Sound on", true),
                              make<Toggle>("Alarm while away", false)});
    sound->gap(5);
    auto lang = make<RadioGroup>(0);
    auto language = Box::column({lang->button("English"), lang->button("Français"), lang->button("Deutsch"), lang->button("Svenska")});
    language->gap(2);
    auto tabs = make<Tabs>();
    tabs->add("Display", display).add("Controls", controls).add("Sound", sound).add("Language", language).sideWhenWider(330);
    auto r = winRef();
    auto w = make<Window>("Preferences", tabs);
    w->width(420).button(make<Button>("Cancel", closer(ui, r))).button(make<Button>("Apply", [this] { ui.toast("Applied"); }));
    w->defaultButton(make<Button>("OK", closer(ui, r)));
    *r = w;
    return w;
}

std::shared_ptr<Window> Demo::loadGame() {
    auto t = make<Table>(std::vector<std::string>{"Name", "Saved", "Game date"}, std::vector<float>{3, 2.2f, 1.6f}, 8);
    const char* n[] = {"autosave", "before war", "checkpoint", "my game", "mining", "quick", "the long game"};
    std::vector<std::vector<std::string>> rows;
    for (int i = 0; i < 7; i++) {
        char saved[32], date[32];
        std::snprintf(saved, sizeof saved, "2026-11-%02d %02d:%02d", 1 + i * 2, 8 + i, i * 7 % 60);
        std::snprintf(date, sizeof date, "21%02d-%02d-01", 30 + i * 4, 1 + i);
        rows.push_back({n[i], saved, date});
    }
    t->setRows(rows);
    auto r = winRef();
    std::weak_ptr<Table> tw = t;
    auto load = make<Button>("Load", [this, r, tw] {
        auto tb = tw.lock();
        if (tb && tb->selected() >= 0) {
            std::string name = tb->row(tb->selected())[0];
            closer(ui, r)();
            ui.toast("Loaded " + name);
        }
    });
    std::weak_ptr<Button> lw = load;
    t->onActivate([this, lw](int) { if (auto b = lw.lock()) b->click(ui); });
    auto w = make<Window>("Load Game", t);
    w->width(380).button(make<Button>("Delete", [this, tw] {
        auto tb = tw.lock();
        if (tb && tb->selected() >= 0) {
            ui.confirm("Delete", "Delete \"" + tb->row(tb->selected())[0] + "\"?", "Delete", "Keep", [this] { ui.toast("Deleted"); });
        }
    }));
    w->button(make<Button>("Cancel", closer(ui, r))).defaultButton(load);
    *r = w;
    return w;
}

std::shared_ptr<Window> Demo::saveDisks() {
    auto t = make<Table>(std::vector<std::string>{"Disk", "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5"},
                         std::vector<float>{1.4f, 2, 2, 2, 2, 2}, 6);
    std::vector<std::vector<std::string>> rows;
    for (int d = 1; d <= 9; d++) {
        std::string ds = std::to_string(d);
        rows.push_back({"Save disk " + ds, "Game " + ds + "A", d % 2 == 0 ? "Empty" : "Game " + ds + "B", "Empty", d == 3 ? "War" : "Empty", "Empty"});
    }
    t->setRows(rows);
    auto r = winRef();
    auto col = Box::column({t, make<Label>("A wide window: it never leaves the screen; narrow it and the table truncates, the buttons wrap.")});
    col->gap(4);
    auto w = make<Window>("Manage Save Disks", col);
    w->width(640).button(make<Button>("New disk", [this] { ui.toast("New disk"); }));
    w->button(make<Button>("Rename", [this] {
        ui.ask("Rename", "Name of the disk:", "Save disk", [this](const std::string& s) { ui.toast("Renamed: " + s); });
    }));
    w->button(make<Button>("Delete", [this] { ui.toast("Delete"); })).button(make<Button>("Play", [this] { ui.toast("Play"); }));
    w->defaultButton(make<Button>("Close", closer(ui, r)));
    *r = w;
    return w;
}

std::shared_ptr<Window> Demo::editors() {
    auto area = make<TextArea>("Several lines of text.\nEnter makes a new line, Tab moves on.\nCtrl+A, Ctrl+C, Ctrl+X, Ctrl+V; Ctrl+arrows by words;\n"
                               "Home/End per line, Ctrl+Home/End the text.\nShift+arrows select.\n\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11\nLine 12",
                               8);
    auto single = make<TextField>("One line");
    auto col = Box::column({make<Label>("Single line"), single, make<Label>("Multiline"), area});
    col->gap(4);
    auto r = winRef();
    auto w = make<Window>("Text editors", col);
    w->width(360).modal(false).resizable(true).defaultButton(make<Button>("Close", closer(ui, r)));
    *r = w;
    return w;
}

std::shared_ptr<Window> Demo::floating(bool titleBar) {
    std::vector<argb> px(24 * 16);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 24; x++) {
            bool edge = x == 0 || y == 0 || x == 23 || y == 15;
            px[y * 24 + x] = edge ? 0xFFB8C890 : ((x / 4 + y / 4) % 2 == 0 ? 0xFF3A4A2A : 0xFF1A2214);
        }
    }
    auto content = Box::column({make<Label>(titleBar ? "Drag me by the title bar. Not modal: the game still takes clicks."
                                                     : "A fixed pane without a title bar (closed from the Widgets menu)."),
                                make<ImageView>(24, 16, px, 72, 48, false)});
    auto w = make<Window>(titleBar ? "Floating pane" : "", content);
    w->width(200).modal(false).titleBar(titleBar);
    return w;
}

std::shared_ptr<Window> Demo::busy() {
    auto bar = make<ProgressBar>(0);
    std::weak_ptr<ProgressBar> bw = bar;
    bar->text([bw] { auto b = bw.lock(); return std::to_string(b ? int(std::lround(b->value() * 100)) : 0) + "%"; });
    auto r = winRef();
    double start = ui.time;
    perFrame.push_back([this, r, bw, start] {
        auto w = r->lock();
        auto b = bw.lock();
        if (w && b && ui.isOpen(w.get())) {
            float v = float(std::min(1.0, (ui.time - start) / 4));
            b->set(v);
            if (v >= 1) {
                ui.close(w.get());
                ui.toast("Extraction done");
            }
        }
    });
    auto w = make<Window>("Extracting", Box::column({make<Label>("Extracting the game's pictures..."), bar}));
    w->width(240).closable(false).button(make<Button>("Cancel", closer(ui, r)));
    *r = w;
    return w;
}

void Demo::run() {
    while (host.frame([this] {
        auto fs = perFrame;
        for (auto& f : fs) {
            f();
        }
    })) {
    }
}

}  // namespace

int main(int argc, char** argv) {
    int uiSize = 0, theme = 0;
    std::string open;
    std::filesystem::path font, titleFont;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--ui-size" && i + 1 < argc) {
            uiSize = std::atoi(argv[++i]);
        } else if (a == "--theme" && i + 1 < argc) {
            theme = std::atoi(argv[++i]);
        } else if (a == "--open" && i + 1 < argc) {
            open = argv[++i];
        } else if (a == "--font" && i + 1 < argc) {
            font = argv[++i];
        } else if (a == "--title-font" && i + 1 < argc) {
            titleFont = argv[++i];
        }
    }
    namespace fs = std::filesystem;
    fs::path exe = fs::path(argv[0]).parent_path(), mono = font, bold = titleFont.empty() ? font : titleFont;
    const char* sys[][2] = {{"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"},
                            {"/usr/share/fonts/truetype/noto/NotoMono-Regular.ttf", "/usr/share/fonts/truetype/noto/NotoMono-Regular.ttf"},
                            {"C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/consolab.ttf"}};
    for (auto& s : sys) {
        if (mono.empty() && fs::exists(s[0])) {
            mono = s[0];
            bold = !titleFont.empty() ? titleFont : fs::exists(s[1]) ? fs::path(s[1]) : mono;
        }
    }
    if (mono.empty()) {
        std::cerr << "slopui_demo: no font found: give a TrueType file with --font FILE\n";
        return 1;
    }
    try {
        Demo d(mono, bold, exe / "themes");
        d.host.setUiScale(uiSize);
        d.setTheme(theme);
        if (open == "gallery") {
            d.ui.open(d.gallery());
        } else if (open == "prefs") {
            d.ui.open(d.preferences());
        } else if (open == "disks") {
            d.ui.open(d.saveDisks());
        } else if (open == "load") {
            d.ui.open(d.loadGame());
        } else if (open == "editors") {
            d.ui.open(d.editors());
        }
        d.run();
    } catch (const std::exception& e) {
        std::cerr << "slopui_demo: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
