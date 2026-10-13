// Settings (AllDebrid connection, preferred audio language, diagnostics) and
// the Diagnostics page (display, renderer, LG extensions, remote key codes, log).
#include <algorithm>

#include "common.h"
#include "log.h"
#include "pairing.h"
#include "screens.h"

namespace {

constexpr int CL = 212;
const char *kLanguages[] = {"English", "French", "Spanish", "German", "Italian", "Portuguese", "Hindi", "Urdu",
                            "Arabic", "Turkish", "Japanese", "Korean"};
constexpr int kLanguageCount = sizeof kLanguages / sizeof kLanguages[0];

class SettingsScreen : public Screen {
 public:
  SettingsScreen() { refresh(); }
  const char *name() const override { return "settings"; }
  bool root() const override { return true; }
  void set_rail_focus(bool rail) override { rail_ = rail; }
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
      for (size_t i = 0; i < list.size(); i++)
        attempts_ += (attempts_.empty() ? "" : ", ") + list[i]["location"].as_string() + " " + list[i]["result"].as_string();
      (void)storage;
    }, alive_);
  }

  bool key(Key key, bool repeat) override {
    if (pairing_.active()) return pairing_.key(key);
    switch (key) {
      case Key::Up: row_ = std::max(0, row_ - 1); return true;
      case Key::Down: row_ = std::min(2, row_ + 1); return true;
      case Key::Left:
        if (row_ == 1) { cycle(-1); return true; }
        return false;
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

  void update(double) override { pairing_.update(); }

  void draw() override {
    Ctx &c = ctx();
    c.gfx.clear(theme::bg);
    c.fonts.heavy.draw("Settings", CL, 80, 56, theme::text);
    struct Row {
      std::string title, value, action;
    };
    std::vector<Row> rows = {
        {"AllDebrid", account_.empty() ? "Checking\xE2\x80\xA6" : account_, ctx().paired ? "Reconnect" : "Connect"},
        {"Preferred audio language", ctx().language, "\xE2\x97\x80  \xE2\x96\xB6"},
        {"Diagnostics", "Display, remote keys and the recent log", "Open"},
    };
    int y = 200;
    for (int i = 0; i < 3; i++) {
      bool f = row_ == i && !rail_ && !pairing_.active();
      c.gfx.fill_round(CL, y, 1100, 120, 24, f ? theme::white : theme::panel);
      c.fonts.bold.draw(rows[static_cast<size_t>(i)].title, CL + 40, y + 26, 28, f ? theme::ink : theme::text);
      draw_text(c.fonts.regular, rows[static_cast<size_t>(i)].value, CL + 40, y + 70, 22, f ? theme::ink2 : theme::text2, 760);
      if (i == 1) {
        c.icons.draw(Icon::ChevronLeft, CL + 1100 - 160, y + 42, 36, f ? theme::ink : theme::text3);
        c.icons.draw(Icon::ChevronRight, CL + 1100 - 76, y + 42, 36, f ? theme::ink : theme::text3);
      } else {
        draw_text_right(c.fonts.bold, rows[static_cast<size_t>(i)].action, CL + 1100 - 40, y + 46, 24, f ? theme::ink : theme::text3);
      }
      y += 140;
    }
    c.fonts.regular.draw("The preferred language decides which sources are recommended first.", CL, y + 10, 21, theme::text3);
    if (!ctx().paired && !attempts_.empty()) c.fonts.regular.draw("Storage: " + attempts_, CL, y + 46, 18, theme::text4);
    c.fonts.regular.draw("Bloob 0.4 \xC2\xB7 Buna N2 \xC2\xB7 " + ctx().tv_model, CL, H - 70, 20, theme::text4);
    pairing_.draw();
  }

 private:
  int row_ = 0;
  bool rail_ = false;
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
    c.gfx.clear(theme::bg);
    c.fonts.heavy.draw("Diagnostics", 96, 72, 52, theme::text);
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
      c.fonts.regular.draw(row.first, 96, y, 24, theme::text3);
      draw_text(c.fonts.semibold, row.second, 520, y, 24, theme::text, W - 620);
      y += 44;
    }
    c.fonts.semibold.draw("Recent log", 96, y + 16, 24, theme::text2);
    y += 60;
    for (auto &line : log_tail(8)) {
      draw_text(c.fonts.regular, line, 96, y, 18, theme::text3, W - 192);
      y += 28;
    }
    c.fonts.regular.draw("Press any remote key to see its code  \xC2\xB7  Back returns", 96, 1024, 22, theme::text3);
    c.redraw_at(c.now + 1000);
  }
};

}  // namespace

std::unique_ptr<Screen> make_settings() { return std::make_unique<SettingsScreen>(); }
std::unique_ptr<Screen> make_diagnostics() { return std::make_unique<DiagnosticsScreen>(); }
