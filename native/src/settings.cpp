// Settings (AllDebrid connection, languages, playback and motion preferences),
// About (version, credits, licences) and the Diagnostics page (display, renderer, LG extensions, remote key codes, log).
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "pairing.h"
#include "screens.h"

namespace {

const char *kLanguages[] = {"English", "French", "Spanish", "German", "Italian", "Portuguese", "Hindi", "Urdu",
                            "Arabic", "Turkish", "Japanese", "Korean"};
constexpr int kLanguageCount = sizeof kLanguages / sizeof kLanguages[0];

#ifndef BLOOB_VERSION
#define BLOOB_VERSION "dev"
#endif

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
    }, alive_);
  }

  enum class Row { Account, Disconnect, Audio, Subtitles, Autoplay, Motion, About, Diagnostics };

  std::vector<Row> rows() const {
    std::vector<Row> list = {Row::Account};
    if (ctx().paired) list.push_back(Row::Disconnect);
    for (Row r : {Row::Audio, Row::Subtitles, Row::Autoplay, Row::Motion, Row::About, Row::Diagnostics}) list.push_back(r);
    return list;
  }

  bool key(Key key, bool repeat) override {
    if (pairing_.active()) return pairing_.key(key);
    auto list = rows();
    int count = static_cast<int>(list.size());
    row_ = std::min(row_, count - 1);
    Row r = list[static_cast<size_t>(row_)];
    if (key != Key::Ok) confirm_ = false;
    switch (key) {
      case Key::Up:
        if (row_ == 0) return false;  // to the tab bar
        row_--;
        return true;
      case Key::Down: row_ = std::min(count - 1, row_ + 1); return true;
      case Key::Left:
      case Key::Right:
        change(r, key == Key::Right ? 1 : -1);
        return true;
      case Key::Ok:
        if (repeat) return true;
        if (r == Row::Account) pairing_.start([this](bool) { refresh(); });
        else if (r == Row::Disconnect) disconnect();
        else if (r == Row::About) ctx().push(make_about());
        else if (r == Row::Diagnostics) ctx().push(make_diagnostics());
        else change(r, 1);
        return true;
      default:
        return false;
    }
  }

  // Left/Right (and OK) step a setting; each change is saved by the service.
  void change(Row r, int d) {
    Ctx &c = ctx();
    if (r == Row::Audio) c.language = step(c.language, d, false);
    else if (r == Row::Subtitles) c.subtitle_language = step(c.subtitle_language, d, true);
    else if (r == Row::Autoplay) c.autoplay_next = !c.autoplay_next;
    else if (r == Row::Motion) c.reduce_motion = !c.reduce_motion;
    else return;
    c.save_settings();
  }

  // The next language in the list; subtitles also offer "Off" first.
  static std::string step(const std::string &current, int d, bool with_off) {
    std::vector<std::string> list;
    if (with_off) list.push_back("Off");
    for (auto *l : kLanguages) list.push_back(l);
    int n = static_cast<int>(list.size()), index = 0;
    for (int i = 0; i < n; i++) if (list[static_cast<size_t>(i)] == current) index = i;
    return list[static_cast<size_t>((index + d + n) % n)];
  }

  // Two presses: the first asks, the second forgets the pairing on this TV.
  void disconnect() {
    if (!confirm_) {
      confirm_ = true;
      return;
    }
    confirm_ = false;
    ctx().api.post("/api/account/unpair", "{}", [this](const ApiResult &r) {
      if (!r.ok()) {
        account_ = r.error;
        return;
      }
      ctx().paired = false;
      logf("settings", "AllDebrid disconnected on this TV");
      row_ = 0;
      refresh();
    }, alive_);
  }

  void update(double dt) override {
    ctx().show_tabs(true);
    pairing_.update();
    // Keep the focused row on screen.
    int top = row_top(row_), bottom = top + kRowH;
    float target = scroll_.target;
    if (top - target < 270) target = static_cast<float>(top - 270);
    if (bottom - target > H - 150) target = static_cast<float>(bottom - (H - 150));
    target = std::max(0.0f, target);
    if (ctx().reduce_motion) scroll_.snap(target);
    else scroll_.to(target);
    if (scroll_.step(dt)) ctx().animating = true;
  }

  static constexpr int kRowH = 112, kGap = 16;
  static int row_top(int i) { return 270 + i * (kRowH + kGap); }

  void draw() override {
    Ctx &c = ctx();
    using namespace theme;
    c.gfx.clear(ground);
    int x = margin, w = 1180;
    int off = static_cast<int>(std::lround(scroll_.value));
    text(Text::Title1, "Settings", x, 170 - off, primary);
    auto list = rows();
    for (size_t i = 0; i < list.size(); i++) {
      int y = row_top(static_cast<int>(i)) - off;
      if (y + kRowH < 0 || y > H) continue;
      bool f = row_ == static_cast<int>(i) && !tabs_ && !pairing_.active();
      if (f) draw_row_focus(x, y, w, kRowH);
      else c.gfx.fill_round(x, y, w, kRowH, 18, raised);
      std::string title, value, action;
      bool choice = false;
      switch (list[i]) {
        case Row::Account:
          title = "AllDebrid";
          value = account_.empty() ? "Checking\xE2\x80\xA6" : account_;
          action = ctx().paired ? "Reconnect" : "Connect";
          break;
        case Row::Disconnect:
          title = "Disconnect AllDebrid";
          value = confirm_ && f ? "Press OK again to forget the pairing on this TV" : "Forget the pairing on this TV";
          action = confirm_ && f ? "Confirm" : "Disconnect";
          break;
        case Row::Audio:
          title = "Preferred audio language";
          value = c.language;
          choice = true;
          break;
        case Row::Subtitles:
          title = "Subtitles";
          value = c.subtitle_language == "Off" ? "Off" : c.subtitle_language + " when the video has them";
          choice = true;
          break;
        case Row::Autoplay:
          title = "Play the next episode automatically";
          value = c.autoplay_next ? "On \xC2\xB7 starts after a 10 second countdown" : "Off";
          choice = true;
          break;
        case Row::Motion:
          title = "Reduce motion";
          value = c.reduce_motion ? "On \xC2\xB7 no sliding, zooming or slideshow" : "Off";
          choice = true;
          break;
        case Row::About:
          title = "About Bloob";
          value = "Version, credits and licences";
          action = "Open";
          break;
        case Row::Diagnostics:
          title = "Diagnostics";
          value = "Display, rendering, network, remote keys and the recent log";
          action = "Open";
          break;
      }
      text(Text::Headline, title, x + 40, y + 20, f ? ink : primary);
      text(Text::Callout, value, x + 40, y + 62, f ? ink2 : secondary, 860);
      if (choice) {
        c.icons.draw(Icon::ChevronLeft, x + w - 150, y + 38, 36, f ? ink : tertiary);
        c.icons.draw(Icon::ChevronRight, x + w - 76, y + 38, 36, f ? ink : tertiary);
      } else {
        text_right(Text::Headline, action, x + w - 40, y + 38, f ? ink : tertiary);
      }
    }
    int y = row_top(static_cast<int>(list.size())) - off;
    text(Text::Caption, "The preferred language decides which versions are recommended first and which sound track starts.", x, y + 8,
         tertiary, w);
    if (!ctx().paired && !attempts_.empty())
      text(Text::Caption, "Where the pairing could not be saved: " + attempts_, x, y + 48, tertiary, w);
    pairing_.draw();
  }

 private:
  int row_ = 0;
  bool tabs_ = false;
  bool confirm_ = false;
  std::string account_, attempts_;
  Anim scroll_{theme::scroll_rate};
  Pairing pairing_;
};

class AboutScreen : public Screen {
 public:
  const char *name() const override { return "about"; }
  bool key(Key key, bool) override {
    if (key == Key::Back) ctx().pop();
    return true;
  }
  void draw() override {
    Ctx &c = ctx();
    using namespace theme;
    c.gfx.clear(ground);
    int x = margin, w = W - 2 * margin;
    text(Text::Title1, "Bloob", x, 120, primary);
    text(Text::Callout, std::string("Version ") + BLOOB_VERSION + " \xC2\xB7 " + c.tv_model, x, 196, secondary);
    int y = 270;
    y = paragraph(Text::Body,
                   "Bloob is an independent app. It is not made or endorsed by LG, AllDebrid or any catalogue it shows. "
                   "Play only what you have the right to watch.",
                   x, y, secondary, w, 3) + 36;
    text(Text::Headline, "Credits and licences", x, y, primary);
    y += 56;
    const std::pair<const char *, const char *> credits[] = {
        {"Inter typeface", "Rasmus Andersson and contributors \xC2\xB7 SIL Open Font License 1.1"},
        {"stb_truetype, stb_image", "Sean Barrett \xC2\xB7 public domain or MIT, at your choice"},
        {"SDL2", "Sam Lantinga and contributors \xC2\xB7 zlib licence (the TV's own copy)"},
        {"Big Buck Bunny (test clip)", "Blender Foundation, peach.blender.org \xC2\xB7 Creative Commons Attribution 3.0"},
        {"Titles and artwork", "Cinemeta and Metahub catalogue services \xC2\xB7 rights stay with their owners"},
        {"Sources", "Found by a public indexer and prepared by your own AllDebrid account"},
    };
    for (auto &cr : credits) {
      text(Text::Callout, cr.first, x, y, primary, 560);
      text(Text::Callout, cr.second, x + 600, y, secondary, w - 600);
      y += 52;
    }
    text(Text::Caption, "Back returns", x, H - 90, tertiary);
  }
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
        {"Rendering", std::string(c.lite() ? "Lite effects (measured slow)" : "Full effects") +
                          (c.perf_line.empty() ? "" : "  Â·  " + c.perf_line)},
        {"Network", !c.online ? "offline" : c.network.empty() ? "connected" : c.network},
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
std::unique_ptr<Screen> make_about() { return std::make_unique<AboutScreen>(); }
