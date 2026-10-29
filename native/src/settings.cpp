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

// The TV's subtitle size levels (setSubtitleFontSize). Its own default is 2,
// which viewers found far too large; Bloob starts at the smallest.
const char *kSubtitleSizes[] = {"Small", "Medium", "Large (the TV's usual size)", "Larger", "Largest"};

const char *picture_name(const std::string &p) {
  return p == "sdr" ? "Prefer SDR" : p == "hdr10" ? "HDR10, skip Dolby Vision" : "Best available";
}
const char *picture_note(const std::string &p) {
  return p == "sdr" ? "SDR versions first, whatever their resolution"
       : p == "hdr10" ? "HDR10 first; Dolby Vision versions play as HDR10"
       : "Dolby Vision or HDR first, at the highest resolution";
}

const char *headphones_name(const std::string &h) {
  return h == "on" ? "Always" : h == "off" ? "Off" : "Automatic";
}
std::string headphones_note(const Ctx &c) {
  if (c.headphones == "off") return "Off: the film's sound goes to the TV as it is";
  std::string now = c.sound_output.empty() ? "" : c.sound_output.compare(0, 3, "bt_") == 0 ? " (on now: Bluetooth)" : " (off now: TV speakers)";
  if (c.headphones == "on") return "Always: every film rendered for headphones";
  return "Automatic: surround for two ears whenever the TV's sound goes to Bluetooth" + now;
}

#ifndef BLOOB_VERSION
#define BLOOB_VERSION "dev"
#endif

class SettingsScreen : public Screen {
 public:
  SettingsScreen() { refresh(); }
  const char *name() const override { return "settings"; }
  bool root() const override { return true; }
  void set_tabs_focus(bool tabs) override { tabs_ = tabs; }
  void shown() override {
    refresh();
    ctx().refresh_headphone_responses();
  }

  std::string spoken() const override {
    if (pairing_.active()) return pairing_.spoken();
    if (tabs_) return "";
    Ctx &c = ctx();
    auto list = rows();
    int i = std::min(row_, static_cast<int>(list.size()) - 1);
    std::string say;
    switch (list[static_cast<size_t>(i)]) {
      case Row::Account: say = "AllDebrid, " + (account_.empty() ? std::string("checking") : account_) + ", button"; break;
      case Row::Disconnect: say = confirm_ ? "Press OK again to forget the pairing" : "Disconnect AllDebrid, button"; break;
      case Row::Audio: say = "Preferred audio language, " + c.language + ", adjustable"; break;
      case Row::Subtitles: say = "Subtitles, " + c.subtitle_language + ", adjustable"; break;
      case Row::SubtitleSize: say = std::string("Subtitle size, ") + kSubtitleSizes[c.subtitle_size] + ", adjustable"; break;
      case Row::SubtitleBackground: say = std::string("Subtitle background, switch, ") + (c.subtitle_background ? "on" : "off"); break;
      case Row::Picture: say = std::string("Picture, ") + picture_name(c.picture) + ", adjustable"; break;
      case Row::Sources: say = std::string("Sources, ") + (c.sources == "torrentio" ? "Torrentio only" : "every index") + ", adjustable"; break;
      case Row::Headphones: say = std::string("Headphones, ") + headphones_name(c.headphones) + ", adjustable"; break;
      case Row::HeadphoneRoom: say = "Surround room, " + c.headphone_room_name(c.headphone_room) + ", adjustable"; break;
      case Row::HeadphoneBoost: say = "Headphone volume boost, " + std::to_string(c.headphone_boost) + " decibels, adjustable"; break;
      case Row::HeadphoneNight: say = std::string("Night listening, switch, ") + (c.headphone_night ? "on" : "off"); break;
      case Row::HeadphoneEq: say = std::string("Earbud correction, ") + (c.headphone_xm5 ? "Sony WF-1000XM5" : "off") + ", adjustable"; break;
      case Row::HeadphoneSync: say = "Headphone lip-sync, " + std::to_string(c.headphone_lead_ms) + " milliseconds, adjustable"; break;
      case Row::Autoplay: say = std::string("Play the next episode automatically, switch, ") + (c.autoplay_next ? "on" : "off"); break;
      case Row::Motion: say = std::string("Reduce motion, switch, ") + (c.reduce_motion ? "on" : "off"); break;
      case Row::Transparency: say = std::string("Reduce transparency, switch, ") + (c.reduce_transparency ? "on" : "off"); break;
      case Row::Contrast: say = std::string("Increase contrast, switch, ") + (c.high_contrast ? "on" : "off"); break;
      case Row::Calibrate: say = "Calibrate the picture with your meter, button"; break;
      case Row::About: say = "About Bloob, button"; break;
      case Row::Diagnostics: say = "Diagnostics, button"; break;
    }
    return say + ", " + position(i, static_cast<int>(list.size()));
  }

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

  enum class Row { Account, Disconnect, Audio, Subtitles, SubtitleSize, SubtitleBackground, Picture, Sources, Headphones,
                   HeadphoneRoom, HeadphoneBoost, HeadphoneNight, HeadphoneEq, HeadphoneSync, Autoplay, Motion,
                   Transparency, Contrast, Calibrate, About, Diagnostics };

  std::vector<Row> rows() const {
    std::vector<Row> list = {Row::Account};
    if (ctx().paired) list.push_back(Row::Disconnect);
    for (Row r : {Row::Audio, Row::Subtitles, Row::SubtitleSize, Row::SubtitleBackground, Row::Picture, Row::Sources, Row::Headphones})
      list.push_back(r);
    // The headphone details only matter while Headphones mode can be on.
    if (ctx().headphones != "off")
      for (Row r : {Row::HeadphoneRoom, Row::HeadphoneBoost, Row::HeadphoneNight, Row::HeadphoneEq, Row::HeadphoneSync}) list.push_back(r);
    for (Row r : {Row::Autoplay, Row::Motion, Row::Transparency, Row::Contrast, Row::Calibrate, Row::About, Row::Diagnostics}) list.push_back(r);
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
        else if (r == Row::Calibrate) ctx().push(make_calibrate());
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
    else if (r == Row::SubtitleSize) c.subtitle_size = std::max(0, std::min(4, c.subtitle_size + d));
    else if (r == Row::SubtitleBackground) c.subtitle_background = !c.subtitle_background;
    else if (r == Row::Picture) {
      static const char *order[] = {"best", "hdr10", "sdr"};
      int i = c.picture == "sdr" ? 2 : c.picture == "hdr10" ? 1 : 0;
      c.picture = order[(i + d + 3) % 3];
    } else if (r == Row::Sources) c.sources = c.sources == "torrentio" ? "all" : "torrentio";
    else if (r == Row::Headphones) {
      static const char *order[] = {"auto", "on", "off"};
      int i = c.headphones == "off" ? 2 : c.headphones == "on" ? 1 : 0;
      c.headphones = order[(i + d + 3) % 3];
    } else if (r == Row::HeadphoneRoom) {
      auto list = c.headphone_rooms();
      int n = static_cast<int>(list.size()), i = 0;
      for (int k = 0; k < n; k++) if (c.headphone_room == list[static_cast<size_t>(k)].first) i = k;
      c.headphone_room = list[static_cast<size_t>((i + d + n) % n)].first;
    } else if (r == Row::HeadphoneBoost) c.headphone_boost = std::max(0, std::min(15, c.headphone_boost + 3 * d));
    else if (r == Row::HeadphoneNight) c.headphone_night = !c.headphone_night;
    else if (r == Row::HeadphoneEq) c.headphone_xm5 = !c.headphone_xm5;
    else if (r == Row::HeadphoneSync) c.headphone_lead_ms = std::max(0, std::min(500, c.headphone_lead_ms + 20 * d));
    else if (r == Row::Autoplay) c.autoplay_next = !c.autoplay_next;
    else if (r == Row::Motion) c.reduce_motion = !c.reduce_motion;
    else if (r == Row::Transparency) c.reduce_transparency = !c.reduce_transparency;
    else if (r == Row::Contrast) c.high_contrast = !c.high_contrast;
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

  // List buttons [EoA R1-R5]: multiline 120 high, radius 33, padding 30, 14 apart.
  static constexpr int kRowH = 120, kGap = 14, kRowR = 33, kRowPad = 30;
  static int row_top(int i) { return 270 + i * (kRowH + kGap); }

  void draw() override {
    Ctx &c = ctx();
    using namespace theme;
    c.gfx.clear(plain);
    int x = safe_x, w = 1180;
    int off = static_cast<int>(std::lround(scroll_.value));
    text(Text::Title2, "Settings", x, 172 - off, label(Label::Secondary));  // Emphasized.Title2, secondary [EoA R6]
    auto list = rows();
    for (size_t i = 0; i < list.size(); i++) {
      int y = row_top(static_cast<int>(i)) - off;
      if (y + kRowH < 0 || y > H) continue;
      bool f = row_ == static_cast<int>(i) && !tabs_ && !pairing_.active();
      // Glass rows: white with a soft lift when focused, a faint white fill at rest.
      if (f) {
        if (!c.lite()) c.gfx.shadow(x, y, w, kRowH, kRowR, theme::shadow_blur, theme::shadow_dy, theme::shadow_alpha);
        c.gfx.fill_round(x, y, w, kRowH, kRowR, theme::button_focus);  // White90
      } else {
        c.gfx.fill_round(x, y, w, kRowH, kRowR, theme::settings_platter);
      }
      std::string title, value, action;
      bool choice = false;
      int toggle = -1;  // a switch: 0 off, 1 on
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
        case Row::SubtitleSize:
          title = "Subtitle size";
          value = kSubtitleSizes[c.subtitle_size];
          choice = true;
          break;
        case Row::SubtitleBackground:
          title = "Subtitle background";
          value = c.subtitle_background ? "A dark box behind the words" : "Off";
          toggle = c.subtitle_background;
          break;
        case Row::Picture:
          title = "Picture";
          value = std::string(picture_name(c.picture)) + " \xC2\xB7 " + picture_note(c.picture);
          choice = true;
          break;
        case Row::Sources:
          title = "Sources";
          value = c.sources == "torrentio" ? "Torrentio only: fastest, fewer versions"
                                           : "Torrentio, Comet, Meteor, StremThru, Zilean, TorrentsDB, Knaben, Bitmagnet, The Pirate Bay and EZTV; anime adds SeaDex and Nyaa";
          choice = true;
          break;
        case Row::Headphones:
          title = "Headphones";
          value = headphones_note(c);
          choice = true;
          break;
        case Row::HeadphoneRoom: {
          title = "Surround room";
          std::string id = c.headphone_room;
          value = id.compare(0, 5, "file:") == 0 ? c.headphone_room_name(id) + ": a measured response from Bloob's data folder"
                  : id == "studio" ? "Studio: close and clear, a small treated room"
                  : id == "cinema" ? "Cinema: big and enveloping, speakers far away"
                                   : "Lounge: a living room with speakers around you";
          choice = true;
          break;
        }
        case Row::HeadphoneBoost:
          title = "Headphone volume boost";
          value = c.headphone_boost ? "+" + std::to_string(c.headphone_boost) + " dB, with a limiter so peaks never clip" : "None";
          choice = true;
          break;
        case Row::HeadphoneNight:
          title = "Night listening";
          value = c.headphone_night ? "Loud scenes held back, quiet ones lifted" : "Off: the film's full dynamics";
          toggle = c.headphone_night;
          break;
        case Row::HeadphoneEq:
          title = "Earbud correction";
          value = c.headphone_xm5 ? "Sony WF-1000XM5 (AutoEQ, oratory1990's measurement)" : "Off";
          choice = true;
          break;
        case Row::HeadphoneSync:
          title = "Headphone lip-sync";
          value = c.headphone_lead_ms ? "Sound sent " + std::to_string(c.headphone_lead_ms) + " ms ahead of the picture"
                                      : "None: raise it if voices come after lips";
          choice = true;
          break;
        case Row::Autoplay:
          title = "Play the next episode automatically";
          value = c.autoplay_next ? "Starts after a 10 second countdown" : "Off";
          toggle = c.autoplay_next;
          break;
        case Row::Motion:
          title = "Reduce motion";
          value = c.reduce_motion ? "No sliding, zooming or slideshow" : "Off";
          toggle = c.reduce_motion;
          break;
        case Row::Transparency:
          title = "Reduce transparency";
          value = c.reduce_transparency ? "Panels are solid instead of frosted glass" : "Off";
          toggle = c.reduce_transparency;
          break;
        case Row::Contrast:
          title = "Increase contrast";
          value = c.high_contrast ? "Secondary text is brighter" : "Off";
          toggle = c.high_contrast;
          break;
        case Row::Calibrate:
          title = "Calibrate";
          value = "Your Spyder5 measures the screen; Bloob corrects Expert 1";
          action = "Open";
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
      Color fg = f ? rgba(0x000000) : label(Label::Primary);
      Color fg2 = f ? rgba(0x000000, 166) : label(Label::Secondary);
      // Title Regular.Body, subtitle Regular.Caption1, 5 apart, centred as a block.
      int block = line_for(Text::BodySemi) + 5 + line_for(Text::Caption1Semi);
      int ty = y + (kRowH - block) / 2;
      text(Text::BodySemi, title, x + kRowPad, ty, fg);
      text(Text::Caption1Semi, value, x + kRowPad, ty + line_for(Text::BodySemi) + 5, fg2, 860);
      if (toggle >= 0) {
        // A switch: white knob, filled track when on.
        int sw = 84, sh = 48, sx = x + w - kRowPad - sw, sy = y + (kRowH - sh) / 2;
        Color track = toggle ? (f ? rgba(0x000000) : rgba(0xffffff)) : (f ? rgba(0x000000, 40) : rgba(0xffffff, 50));
        c.gfx.fill_round(sx, sy, sw, sh, sh / 2, track);
        int kx = toggle ? sx + sw - sh / 2 : sx + sh / 2;
        Color knob = toggle ? (f ? rgba(0xffffff) : rgba(0x000000)) : rgba(0xffffff);
        c.gfx.fill_circle(kx, sy + sh / 2, sh / 2 - 5, knob);
      } else if (choice) {
        c.icons.draw(Icon::ChevronLeft, x + w - kRowPad - 110, y + (kRowH - 36) / 2, 36, f ? fg : label(Label::Tertiary));
        c.icons.draw(Icon::ChevronRight, x + w - kRowPad - 36, y + (kRowH - 36) / 2, 36, f ? fg : label(Label::Tertiary));
      } else {
        text_right(Text::Body, action, x + w - kRowPad, text_y_centered(Text::Body, y + kRowH / 2), f ? fg : label(Label::Tertiary));
      }
    }
    int y = row_top(static_cast<int>(list.size())) - off;
    text(Text::Caption1, "The preferred language decides which versions are recommended first and which sound track starts.", x, y + 8,
         label(Label::Tertiary), w);
    if (!ctx().paired && !attempts_.empty())
      text(Text::Caption1, "Where the pairing could not be saved: " + attempts_, x, y + 48, label(Label::Tertiary), w);
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
    c.gfx.clear(plain);
    int x = safe_x, w = W - 2 * safe_x;
    text(Text::Title1, "Bloob", x, 110, label(Label::Primary));
    text(Text::Callout, std::string("Version ") + BLOOB_VERSION + " \xC2\xB7 " + c.tv_model, x, 210, label(Label::Secondary));
    int y = 280;
    y = paragraph(Text::Body,
                   "Bloob is an independent app. It is not made or endorsed by LG, AllDebrid or any catalogue it shows. "
                   "Play only what you have the right to watch.",
                   x, y, label(Label::Secondary), w, 3) + 36;
    text(Text::Headline, "Credits and licences", x, y, label(Label::Primary));
    y += 56;
    const std::pair<const char *, const char *> credits[] = {
        {"Inter typeface", "Rasmus Andersson and contributors \xC2\xB7 SIL Open Font License 1.1"},
        {"stb_truetype, stb_image", "Sean Barrett \xC2\xB7 public domain or MIT, at your choice"},
        {"SDL2", "Sam Lantinga and contributors \xC2\xB7 zlib licence (the TV's own copy)"},
        {"Big Buck Bunny (test clip)", "Blender Foundation, peach.blender.org \xC2\xB7 Creative Commons Attribution 3.0"},
        {"Headphones correction", "AutoEq by Jaakko Pasanen (MIT), from oratory1990's WF-1000XM5 measurement"},
        {"Headphones rooms", "Bloob's own, after Brown and Duda's structural model of binaural sound (1998)"},
        {"FFmpeg", "The FFmpeg developers \xC2\xB7 LGPL 2.1 or later (the engine and Headphones mode)"},
        {"Titles and artwork", "Cinemeta and Metahub catalogue services \xC2\xB7 rights stay with their owners"},
        {"Sources", "Found by public indexes (Torrentio, Comet, Meteor, StremThru, Zilean, TorrentsDB, Knaben, Bitmagnet, The Pirate Bay, EZTV; for anime SeaDex and Nyaa) and prepared by your own AllDebrid account"},
    };
    for (auto &cr : credits) {
      text(Text::Callout, cr.first, x, y, label(Label::Primary), 560);
      text(Text::Callout, cr.second, x + 600, y, label(Label::Secondary), w - 600);
      y += 52;
    }
    text(Text::Caption1, "Back returns", x, H - safe_y - 32, label(Label::Tertiary));
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
    c.gfx.clear(theme::plain);
    text(Text::Title2, "Diagnostics", theme::safe_x, 72, label(Label::Primary));
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
    char mem[160];
    std::snprintf(mem, sizeof mem, "%.1f of %.0f MB GPU (%.1f MB art), %s", gpu_budget().used() / 1048576.0,
                  gpu_budget().limit() / 1048576.0, c.images.bytes() / 1048576.0, memory_policy_name(c.memory_policy));
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
                          (c.reduce_transparency ? " \xC2\xB7 solid glass" : "") +
                          (c.perf_line.empty() ? "" : "  \xC2\xB7  " + c.perf_line)},
        {"Network", !c.online ? "offline" : c.network.empty() ? "connected" : c.network},
        {"App", c.app_id + "  \xC2\xB7  " + c.dir},
        {"Last key", c.last_key},
    };
    int y = 170;
    for (auto &row : rows) {
      text(Text::Caption1, row.first, theme::safe_x, y, label(Label::Tertiary));
      text(Text::Caption1, row.second, 520, y, label(Label::Primary), W - 600);
      y += 44;
    }
    text(Text::Headline, "Recent log", theme::safe_x, y + 12, label(Label::Secondary));
    y += 64;
    for (auto &line : log_tail(8)) {
      text(Text::Caption2, line, theme::safe_x, y, label(Label::Tertiary), W - 2 * theme::safe_x);
      y += 30;
    }
    text(Text::Caption1, "Press any remote key to see its code  \xC2\xB7  Back returns", theme::safe_x, 1010, label(Label::Tertiary));
    c.redraw_at(c.now + 1000);
  }
};

}  // namespace

std::unique_ptr<Screen> make_settings() { return std::make_unique<SettingsScreen>(); }
std::unique_ptr<Screen> make_diagnostics() { return std::make_unique<DiagnosticsScreen>(); }
std::unique_ptr<Screen> make_about() { return std::make_unique<AboutScreen>(); }
