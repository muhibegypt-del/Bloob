// Settings (AllDebrid connection, preferred audio language, diagnostics) and
// the Diagnostics page (display, renderer, LG extensions, remote key codes, log).
#include <algorithm>

#include "common.h"
#include "log.h"
#include "pairing.h"
#include "screens.h"

namespace {

const char *kLanguages[] = {"English", "French", "Spanish", "German", "Italian", "Portuguese", "Hindi", "Urdu",
                            "Arabic", "Turkish", "Japanese", "Korean"};
constexpr int kLanguageCount = sizeof kLanguages / sizeof kLanguages[0];

class SettingsScreen : public Screen {
 public:
  SettingsScreen() { refresh(); }
  const char *name() const override { return "settings"; }
  bool root() const override { return true; }
  void set_tabs_focus(bool tabs) override { tabs_ = tabs; }
  void shown() override { refresh(); }

  void refresh() {
    if (!ctx().api.ready()) return;
    ctx().api.get("/api/account", [this](const ApiResult &r) {
      if (!r.ok()) {
        account_ = r.error;
        return;
      }
      ctx().paired = r.json["paired"].as_bool();
      std::string storage = r.json["storage"].as_string();
      bool durable = r.json["durable"].as_bool();
      if (ctx().paired) account_ = durable ? "Connected \xC2\xB7 saved on this TV" : "Connected \xC2\xB7 saved until the TV restarts";
      else account_ = "Not connected";
      // Where the pairing could not be saved, and why (no key material).
      attempts_.clear();
      const Json &list = r.json["attempts"];
      for (size_t i = 0; i < list.size(); i++) {
        // "none" and "empty" just mean nothing was saved there yet; only real errors are worth showing.
        std::string result = list[i]["result"].as_string();
        if (result == "none" || result == "empty") continue;
        attempts_ += (attempts_.empty() ? "" : ", ") + list[i]["location"].as_string() + " " + result;
      }
      (void)storage;
    }, alive_);
  }

  bool key(Key key, bool repeat) override {
    if (pairing_.active()) return pairing_.key(key);
    switch (key) {
      case Key::Up:
        if (row_ == 0) return false;  // to the tab bar
        row_--;
        return true;
      case Key::Down: row_ = std::min(2, row_ + 1); return true;
      case Key::Left:
        if (row_ == 1) cycle(-1);
        return true;
      case Key::Right:
        if (row_ == 1) cycle(1);
        return true;
      case Key::Ok:
        if (repeat) return true;
        if (row_ == 0) pairing_.start([this](bool) { refresh(); });
        if (row_ == 1) cycle(1);
        if (row_ == 2) ctx().push(make_diagnostics());
        return true;
      default:
        return false;
    }
  }

  void cycle(int d) {
    int index = 0;
    for (int i = 0; i < kLanguageCount; i++) if (ctx().language == kLanguages[i]) index = i;
    ctx().language = kLanguages[(index + d + kLanguageCount) % kLanguageCount];
    ctx().save_settings();
  }

  void update(double) override {
    ctx().show_tabs(true);
    pairing_.update();
  }

  void draw() override {
    Ctx &c = ctx();
    using namespace theme;
    c.gfx.clear(ground);
    int x = margin, w = 1180;
    text(Text::Title1, "Settings", x, 170, primary);
    struct Row {
      std::string title, value, action;
    };
    std::vector<Row> rows = {
        {"AllDebrid", account_.empty() ? "Checking\xE2\x80\xA6" : account_, ctx().paired ? "Reconnect" : "Connect"},
        {"Preferred audio language", ctx().language, ""},
        {"Diagnostics", "Display, remote keys and the recent log", "Open"},
    };
    int y = 270;
    for (int i = 0; i < 3; i++) {
      bool f = row_ == i && !tabs_ && !pairing_.active();
      if (f) draw_row_focus(x, y, w, 120);
      else c.gfx.fill_round(x, y, w, 120, 18, raised);
      const Row &r = rows[static_cast<size_t>(i)];
      text(Text::Headline, r.title, x + 40, y + 24, f ? ink : primary);
      text(Text::Callout, r.value, x + 40, y + 66, f ? ink2 : secondary, 800);
      if (i == 1) {
        c.icons.draw(Icon::ChevronLeft, x + w - 150, y + 42, 36, f ? ink : tertiary);
        c.icons.draw(Icon::ChevronRight, x + w - 76, y + 42, 36, f ? ink : tertiary);
      } else {
        text_right(Text::Headline, r.action, x + w - 40, y + 42, f ? ink : tertiary);
      }
      y += 140;
    }
    text(Text::Caption, "The preferred language decides which versions are recommended first.", x, y + 8, tertiary);
    if (!ctx().paired && !attempts_.empty()) text(Text::Footnote, "Where the pairing could not be saved: " + attempts_, x, y + 44, tertiary, w);
    text(Text::Footnote, "Bloob 1.1 \xC2\xB7 " + ctx().tv_model, x, H - 80, tertiary);
    pairing_.draw();
  }

 private:
  int row_ = 0;
  bool tabs_ = false;
  std::string account_, attempts_;
  Pairing pairing_;
};

class DiagnosticsScreen : public Screen {
 public:
  const char *name() const override { return "diagnostics"; }
  bool key(Key key, bool) override {
    if (key == Key::Back) ctx().pop();
    return true;
  }
  void draw() override {
    Ctx &c = ctx();
    c.gfx.clear(theme::ground);
    text(Text::Title1, "Diagnostics", 96, 72, theme::primary);
    SDL_DisplayMode mode{};
    SDL_GetCurrentDisplayMode(0, &mode);
    SDL_RendererInfo info{};
    SDL_GetRendererInfo(c.renderer, &info);
    int pw = 0, ph = 0, refresh = 0;
    if (c.webos.get_panel_resolution) c.webos.get_panel_resolution(&pw, &ph);
    if (c.webos.get_refresh_rate) c.webos.get_refresh_rate(&refresh);
    int ww = 0, wh = 0;
    SDL_GetWindowSize(c.window, &ww, &wh);
    std::string missing = c.webos.missing();
    char mem[64];
    std::snprintf(mem, sizeof mem, "%.1f MB in textures", c.images.bytes() / 1048576.0);
    std::vector<std::pair<std::string, std::string>> rows = {
        {"TV", c.tv_model},
        {"Service", c.api.ready() ? "running on port " + std::to_string(c.api.port()) : (c.service_error.empty() ? "starting" : c.service_error)},
        {"SDL video driver", SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "?"},
        {"Renderer", std::string(info.name ? info.name : "?") + ((info.flags & SDL_RENDERER_ACCELERATED) ? " (accelerated)" : " (software)")},
        {"Window / display", std::to_string(ww) + "\xC3\x97" + std::to_string(wh) + " / " + std::to_string(mode.w) + "\xC3\x97" +
                                 std::to_string(mode.h) + " @ " + std::to_string(mode.refresh_rate) + " Hz"},
        {"Panel", pw ? std::to_string(pw) + "\xC3\x97" + std::to_string(ph) + " @ " + std::to_string(refresh) + " Hz" : "unknown"},
        {"LG SDL extensions", missing.empty() ? "all present" : "missing: " + missing},
        {"Artwork", mem},
        {"App", c.app_id + "  \xC2\xB7  " + c.dir},
        {"Last key", c.last_key},
    };
    int y = 170;
    for (auto &row : rows) {
      text(Text::Callout, row.first, 96, y, theme::tertiary);
      text(Text::Callout, row.second, 520, y, theme::primary, W - 620);
      y += 44;
    }
    text(Text::Headline, "Recent log", 96, y + 16, theme::secondary);
    y += 60;
    for (auto &line : log_tail(8)) {
      text(Text::Footnote, line, 96, y, theme::tertiary, W - 192);
      y += 28;
    }
    text(Text::Caption, "Press any remote key to see its code  \xC2\xB7  Back returns", 96, 1024, theme::tertiary);
    c.redraw_at(c.now + 1000);
  }
};

}  // namespace

std::unique_ptr<Screen> make_settings() { return std::make_unique<SettingsScreen>(); }
std::unique_ptr<Screen> make_diagnostics() { return std::make_unique<DiagnosticsScreen>(); }
