// Player: LG's media pipeline in the hardware video plane under a transparent
// window, driven by the native Buna session. Minimal chrome: a timeline with a
// seek preview, a short control row, audio and subtitle menus listing the
// file's real tracks, resume, progress saving and "up next" for series.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "common.h"
#include "log.h"
#include "luna.h"
#include "media.h"
#include "screens.h"
#include "session.h"

namespace {

constexpr long long kHudTimeoutMs = 6000;
constexpr long long kSaveEveryMs = 15000;
constexpr long long kUpNextMs = 10000;

std::string language_name(const std::string &code) {
  static const std::pair<const char *, const char *> names[] = {
      {"en", "English"}, {"eng", "English"}, {"fr", "French"}, {"fre", "French"}, {"fra", "French"},
      {"de", "German"}, {"ger", "German"}, {"deu", "German"}, {"es", "Spanish"}, {"spa", "Spanish"},
      {"it", "Italian"}, {"ita", "Italian"}, {"pt", "Portuguese"}, {"por", "Portuguese"}, {"ru", "Russian"},
      {"rus", "Russian"}, {"ja", "Japanese"}, {"jpn", "Japanese"}, {"ko", "Korean"}, {"kor", "Korean"},
      {"zh", "Chinese"}, {"chi", "Chinese"}, {"zho", "Chinese"}, {"ar", "Arabic"}, {"ara", "Arabic"},
      {"hi", "Hindi"}, {"hin", "Hindi"}, {"nl", "Dutch"}, {"dut", "Dutch"}, {"nld", "Dutch"}, {"pl", "Polish"},
      {"pol", "Polish"}, {"tr", "Turkish"}, {"tur", "Turkish"}, {"sv", "Swedish"}, {"swe", "Swedish"},
      {"da", "Danish"}, {"dan", "Danish"}, {"no", "Norwegian"}, {"nor", "Norwegian"}, {"fi", "Finnish"},
      {"fin", "Finnish"}, {"el", "Greek"}, {"gre", "Greek"}, {"he", "Hebrew"}, {"heb", "Hebrew"}, {"ur", "Urdu"},
      {"urd", "Urdu"}, {"bn", "Bengali"}, {"ben", "Bengali"}, {"ta", "Tamil"}, {"tam", "Tamil"}, {"te", "Telugu"},
      {"tel", "Telugu"}, {"th", "Thai"}, {"tha", "Thai"}, {"vi", "Vietnamese"}, {"vie", "Vietnamese"},
      {"id", "Indonesian"}, {"ind", "Indonesian"}, {"ms", "Malay"}, {"may", "Malay"}, {"cs", "Czech"},
      {"cze", "Czech"}, {"hu", "Hungarian"}, {"hun", "Hungarian"}, {"ro", "Romanian"}, {"rum", "Romanian"},
      {"uk", "Ukrainian"}, {"ukr", "Ukrainian"}, {"fa", "Persian"}, {"per", "Persian"}};
  for (auto &n : names) if (code == n.first) return n.second;
  if (code.empty() || code == "und") return "Unknown language";
  return upper(code);
}

std::string codec_name(std::string codec) {
  codec = upper(codec);
  if (codec == "EAC3" || codec == "E-AC-3") return "Dolby Digital Plus";
  if (codec == "AC3" || codec == "AC-3") return "Dolby Digital";
  if (codec.find("TRUEHD") != std::string::npos) return "Dolby TrueHD";
  if (codec.find("DTS") != std::string::npos) return codec;
  return codec;
}

std::string audio_label(const MediaTrack &t) {
  std::string ch = t.channels >= 8 ? "7.1" : t.channels == 6 ? "5.1" : t.channels == 2 ? "Stereo" : t.channels == 1 ? "Mono"
                   : t.channels ? std::to_string(t.channels) + " ch" : "";
  return join({language_name(t.language), join({codec_name(t.codec), ch}, " ")});
}

class UmsPort : public PlayerPort {
 public:
  explicit UmsPort(UmsPlayer &player) : player_(player) {}
  void play() override { player_.play(); }
  void pause() override { player_.pause(); }
  void seek(long long ms) override { player_.seek(ms); }
  void select_track(const std::string &type, int index) override { player_.select_track(type, index); }
  void set_subtitles(bool enabled) override { player_.set_subtitles(enabled); }

 private:
  UmsPlayer &player_;
};

enum class Ctl { Back10, Toggle, Fwd10, StartOver, Next, Audio, Subtitles };

class PlayerScreen : public Screen {
 public:
  PlayerScreen(const PlayRequest &req, const std::string &url, const std::string &source, const std::string &load_type)
      : req_(req), url_(url), source_(source), load_type_(load_type) {
    open();
  }
  ~PlayerScreen() override { close(); }

  const char *name() const override { return "player"; }
  bool over_video() const override { return session_.started() && session_.phase() != Phase::Error; }

  // ---------------------------------------------------------------- lifecycle

  void open() {
    Ctx &c = ctx();
    menu_ = Menu::None;
    zone_ = Zone::Timeline;
    control_ = 1;
    error_focus_ = 0;
    resumed_ = false;
    up_next_ = false;
    show_hud();
    session_.open(&port_, c.now);
    if (c.webos.create_exported_window) {
      const char *created = c.webos.create_exported_window(0);
      window_id_ = created ? created : "";
    }
    logf("player", "exported video window: %s", window_id_.empty() ? "(none)" : window_id_.c_str());
    place_video(0, 0);
    opened_ = true;
    ums_.load(url_, c.app_id, window_id_, load_type_, [this](const MediaEvent &event) { on_media(event); });
  }

  void close() {
    if (!opened_) return;
    save_progress();
    ums_.unload();
    session_.close();
    Ctx &c = ctx();
    if (!window_id_.empty() && c.webos.destroy_exported_window) c.webos.destroy_exported_window(window_id_.c_str());
    window_id_.clear();
    opened_ = false;
    if (c.refresh_home) c.refresh_home();
  }

  void exit(bool another = false) {
    player_wants_another_source() = another;
    close();
    ctx().pop();
  }

  void suspend() override {
    if (session_.intent_play()) session_.pause();
    save_progress();
  }

  void hidden() override { suspend(); }

  void on_media(const MediaEvent &event) {
    Ctx &c = ctx();
    long long t = c.now;
    bool had_frame = session_.first_frame_ms() >= 0;
    auto pending = session_.pending_ms();
    long long seek_started = session_.seek_started_ms();
    session_.on_event(event, ums_.info(), t);
    if (!had_frame && session_.first_frame_ms() >= 0) logf("player", "first frame after %lld ms", session_.first_frame_ms());
    if (pending && !session_.pending_ms()) {
      logf("player", "seek to %lld ms landed at %lld ms after %lld ms", *pending, session_.position_ms(t), t - seek_started);
    }
    if (event.kind == MediaEvent::VideoInfo || event.kind == MediaEvent::SourceInfo || event.kind == MediaEvent::Loaded) {
      place_video(ums_.info().width, ums_.info().height);
    }
    // Continue where the viewer left off, once the pipeline is playing.
    if (event.kind == MediaEvent::Playing && !resume_done_) {
      resume_done_ = true;
      long long d = session_.duration_ms();
      if (req_.resume_ms >= 30000 && (d <= 0 || req_.resume_ms < d * 95 / 100)) {
        session_.seek_to(req_.resume_ms, t);
        resumed_ = true;
        logf("player", "resuming at %lld ms", req_.resume_ms);
      }
    }
    if (event.kind == MediaEvent::Paused) save_progress();
    if (event.kind == MediaEvent::EndOfStream) {
      save_progress(true);
      if (req_.next) {
        up_next_ = true;
        up_next_at_ = t;
      }
      show_hud();
    }
  }

  void place_video(int video_w, int video_h) {
    Ctx &c = ctx();
    if (window_id_.empty() || !c.webos.set_exported_window) return;
    int dw = W, dh = H;
    SDL_GetWindowSize(c.window, &dw, &dh);
    SDL_Rect src{0, 0, video_w > 0 ? video_w : dw, video_h > 0 ? video_h : dh};
    SDL_Rect dst{0, 0, dw, dh};
    c.webos.set_exported_window(window_id_.c_str(), &src, &dst);
  }

  void save_progress(bool finished = false) {
    if (!session_.started()) return;
    long long duration = session_.duration_ms();
    long long position = finished ? duration : session_.position_ms(ctx().now);
    if (duration <= 0 || position <= 0) return;
    std::string body = "{\"videoId\":" + Json::quote(req_.video_id) + ",\"id\":" + Json::quote(req_.item.id) +
                       ",\"type\":" + Json::quote(req_.item.type) + ",\"name\":" + Json::quote(req_.item.name) +
                       ",\"season\":" + std::to_string(req_.season) + ",\"episode\":" + std::to_string(req_.episode) +
                       ",\"episodeTitle\":" + Json::quote(req_.episode_title) + ",\"position\":" + std::to_string(position) +
                       ",\"duration\":" + std::to_string(duration);
    if (req_.next) {
      body += ",\"next\":{\"videoId\":" + Json::quote(req_.next->video_id) + ",\"season\":" + std::to_string(req_.next->season) +
              ",\"episode\":" + std::to_string(req_.next->episode) + ",\"title\":" + Json::quote(req_.next->title) + "}";
    }
    body += "}";
    ctx().api.post("/api/progress", body);
    last_save_ = ctx().now;
  }

  void play_next() {
    if (!req_.next) return;
    PlayRequest next;
    next.item = req_.item;
    next.video_id = req_.next->video_id;
    next.season = req_.next->season;
    next.episode = req_.next->episode;
    next.episode_title = req_.next->title;
    // The episode after that is unknown here; Details works it out next time.
    close();
    ctx().replace(make_sources(next, true));
  }

  // ---------------------------------------------------------------- HUD

  enum class Zone { Timeline, Controls };
  enum class Menu { None, Audio, Subtitles };

  void show_hud() {
    hud_ = true;
    hud_until_ = ctx().now + kHudTimeoutMs;
  }

  bool hud_pinned() const {
    Phase p = session_.phase();
    return menu_ != Menu::None || p == Phase::Paused || p == Phase::Ended || p == Phase::Loading || p == Phase::Error ||
           session_.preview_ms().has_value() || session_.pending_ms().has_value();
  }

  std::vector<Ctl> controls() const {
    std::vector<Ctl> list = {Ctl::Back10, Ctl::Toggle, Ctl::Fwd10};
    if (resumed_) list.push_back(Ctl::StartOver);
    if (req_.next) list.push_back(Ctl::Next);
    list.push_back(Ctl::Audio);
    list.push_back(Ctl::Subtitles);
    return list;
  }

  void run_control(Ctl ctl) {
    long long t = ctx().now;
    switch (ctl) {
      case Ctl::Back10: session_.nudge(-1, t); break;
      case Ctl::Toggle: session_.toggle(); break;
      case Ctl::Fwd10: session_.nudge(1, t); break;
      case Ctl::StartOver:
        session_.seek_to(0, t);
        resumed_ = false;
        control_ = 1;
        break;
      case Ctl::Next: play_next(); return;
      case Ctl::Audio: menu_ = Menu::Audio; menu_index_ = selected_index(Menu::Audio); break;
      case Ctl::Subtitles: menu_ = Menu::Subtitles; menu_index_ = selected_index(Menu::Subtitles); break;
    }
    show_hud();
  }

  int selected_index(Menu which) const {
    const MediaInfo &info = ums_.info();
    if (which == Menu::Audio) {
      for (size_t i = 0; i < info.audio.size(); i++) if (info.audio[i].selected) return static_cast<int>(i);
      return 0;
    }
    return session_.subtitle_index() + 1;  // row 0 = Off
  }

  int menu_rows() const {
    const MediaInfo &info = ums_.info();
    return menu_ == Menu::Audio ? static_cast<int>(info.audio.size()) : static_cast<int>(info.text.size()) + 1;
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    long long t = ctx().now;
    Phase phase = session_.phase();
    if (phase == Phase::Error) {
      if (key == Key::Left) error_focus_ = std::max(0, error_focus_ - 1);
      if (key == Key::Right) error_focus_ = std::min(2, error_focus_ + 1);
      if (key == Key::Ok && !repeat) {
        if (error_focus_ == 0) {
          close();
          open();
        } else {
          exit(error_focus_ == 1);
        }
      }
      if (key == Key::Back || key == Key::Stop) exit();
      return true;
    }
    if (up_next_) {
      if (key == Key::Ok && !repeat) play_next();
      else if (key == Key::Back) up_next_ = false;
      return true;
    }
    if (menu_ != Menu::None) {
      int rows = menu_rows();
      if (key == Key::Up) menu_index_ = std::max(0, menu_index_ - 1);
      if (key == Key::Down) menu_index_ = std::min(std::max(0, rows - 1), menu_index_ + 1);
      if (key == Key::Ok && rows > 0 && !repeat) {
        if (menu_ == Menu::Audio) session_.select_audio(menu_index_);
        else session_.select_text(menu_index_ - 1);
        menu_ = Menu::None;
      }
      if (key == Key::Back || key == Key::Left || key == Key::Right) menu_ = Menu::None;
      show_hud();
      return true;
    }
    switch (key) {
      case Key::Play: session_.play(); show_hud(); return true;
      case Key::Pause: session_.pause(); show_hud(); return true;
      case Key::PlayPause: session_.toggle(); show_hud(); return true;
      case Key::Forward: session_.nudge(1, t); show_hud(); return true;
      case Key::Rewind: session_.nudge(-1, t); show_hud(); return true;
      case Key::Stop: exit(); return true;
      default: break;
    }
    if (key == Key::Back) {
      if (session_.cancel_preview()) { show_hud(); return true; }
      if (hud_ && session_.phase() == Phase::Playing) { hud_ = false; return true; }
      exit();
      return true;
    }
    auto list = controls();
    int count = static_cast<int>(list.size());
    if (!hud_) {
      if (key == Key::Left || key == Key::Right) session_.nudge(key == Key::Right ? 1 : -1, t);
      if (key == Key::Ok && !repeat) { session_.toggle(); zone_ = Zone::Controls; control_ = 1; }
      if (key == Key::Down) { zone_ = Zone::Controls; control_ = 1; }
      if (key == Key::Up) zone_ = Zone::Timeline;
      show_hud();
      return true;
    }
    show_hud();
    if (zone_ == Zone::Timeline) {
      if (key == Key::Left || key == Key::Right) session_.nudge(key == Key::Right ? 1 : -1, t);
      if (key == Key::Ok && !repeat) { if (!session_.commit_preview(t)) session_.toggle(); }
      if (key == Key::Down) { zone_ = Zone::Controls; control_ = std::min(1, count - 1); }
      return true;
    }
    if (key == Key::Up) zone_ = Zone::Timeline;
    if (key == Key::Left) control_ = std::max(0, control_ - 1);
    if (key == Key::Right) control_ = std::min(count - 1, control_ + 1);
    if (key == Key::Ok && !repeat) run_control(list[static_cast<size_t>(std::min(control_, count - 1))]);
    return true;
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    long long t = c.now;
    if (session_.tick(t)) c.animating = true;
    if (hud_ && t > hud_until_ && !hud_pinned()) hud_ = false;
    if (session_.phase() == Phase::Playing && t - last_save_ > kSaveEveryMs) save_progress();
    if (up_next_ && t - up_next_at_ >= kUpNextMs) {
      play_next();
      return;
    }
    bool show = hud_ || hud_pinned();
    // Redraw for a visible moving clock (4×/s), the seek preview and countdowns.
    if (session_.preview_ms() || up_next_ || session_.phase() == Phase::Buffering || !session_.started()) c.animating = true;
    else if (show) c.redraw_at((t / 250 + 1) * 250);
    if (hud_ && !hud_pinned()) c.redraw_at(hud_until_ + 1);
  }

  void draw() override {
    Ctx &c = ctx();
    long long t = c.now;
    Phase phase = session_.phase();

    if (!session_.started() && phase != Phase::Error) {
      draw_startup();
      return;
    }
    // Transparent: the hardware video plane under the window shows through.
    c.gfx.clear({0, 0, 0, 0});

    if (phase == Phase::Error) {
      draw_error();
      return;
    }
    if (up_next_) {
      draw_up_next();
      return;
    }
    bool show = hud_ || hud_pinned();
    if (!show) {
      if (phase == Phase::Buffering) {
        c.gfx.fill_circle(W / 2, H / 2, 56, with_alpha(theme::bg, 150));
        draw_spinner(W / 2, H / 2, 56, theme::white);
      }
      return;
    }

    c.gfx.gradient_v(0, 0, W, 260, rgba(0x000000), 190, 0);
    c.gfx.gradient_v(0, H - 420, W, 420, rgba(0x000000), 0, 215);
    draw_text(c.fonts.heavy, req_.item.name, 96, 60, 40, theme::text, 1300);
    std::string sub = req_.label();
    std::string stream = stream_line();
    draw_text(c.fonts.regular, join({sub, stream}), 96, 116, 22, theme::text2, 1500);

    long long display = session_.display_ms(t);
    long long duration = session_.duration_ms();
    long long position = session_.position_ms(t);
    std::string status = phase == Phase::Seeking ? "Seeking\xE2\x80\xA6" : phase == Phase::Buffering ? "Buffering\xE2\x80\xA6"
                         : phase == Phase::Ended ? "Finished" : phase == Phase::Paused ? "Paused" : "";
    if (!status.empty()) {
      int w = c.fonts.semibold.measure(status, 22) + 40;
      c.gfx.fill_round(96, H - 312, w, 46, 23, with_alpha(theme::white, 40));
      c.fonts.semibold.draw(status, 116, H - 301, 22, theme::text);
    }

    int tx = 96, tw = W - 192, ty = H - 214;
    bool timeline_focus = zone_ == Zone::Timeline;
    int th = timeline_focus ? 8 : 6;
    c.gfx.fill_round(tx, ty - th / 2, tw, th, th / 2, with_alpha(theme::white, 60));
    double frac = duration > 0 ? std::min(1.0, std::max(0.0, static_cast<double>(display) / duration)) : 0;
    c.gfx.fill_round(tx, ty - th / 2, std::max(th, static_cast<int>(tw * frac)), th, th / 2, theme::white);
    if (timeline_focus || session_.preview_ms() || session_.pending_ms()) c.gfx.fill_circle(tx + static_cast<int>(tw * frac), ty, 12, theme::white);
    if (session_.preview_ms() || session_.pending_ms()) {
      long long delta = display - session_.seek_origin_ms(position);
      long long s = (delta < 0 ? -delta : delta) / 1000;
      std::string d = std::string(delta < 0 ? "\xE2\x88\x92" : "+") + (s < 60 ? std::to_string(s) + "s" : clock_text(s * 1000));
      std::string label = clock_text(display) + "   " + d;
      int lw = c.fonts.bold.measure(label, 28) + 40;
      int lx = std::max(tx, std::min(tx + tw - lw, tx + static_cast<int>(tw * frac) - lw / 2));
      c.gfx.fill_round(lx, ty - 92, lw, 60, 14, theme::white);
      c.fonts.bold.draw(label, lx + 20, ty - 82, 28, theme::ink);
    }
    c.fonts.regular.draw(clock_text(display), tx, ty + 22, 24, theme::text2);
    draw_text_right(c.fonts.regular, duration > 0 ? "\xE2\x88\x92" + clock_text(std::max(0LL, duration - display)) : "--:--",
                    tx + tw, ty + 22, 24, theme::text2);

    draw_controls(phase);
    if (menu_ != Menu::None) draw_menu();
  }

  std::string stream_line() const {
    const MediaInfo &info = ums_.info();
    std::vector<std::string> parts;
    if (info.height >= 2000) parts.push_back("4K");
    else if (info.height >= 1000) parts.push_back("1080p");
    else if (info.height >= 700) parts.push_back("720p");
    else if (info.height > 0) parts.push_back(std::to_string(info.height) + "p");
    if (!info.hdr.empty() && upper(info.hdr) != "NONE" && upper(info.hdr) != "SDR") parts.push_back(info.hdr == "DolbyVision" ? "Dolby Vision" : info.hdr);
    if (!info.video_codec.empty()) parts.push_back(upper(info.video_codec));
    for (auto &a : info.audio) if (a.selected) parts.push_back(audio_label(a));
    return join(parts);
  }

  void draw_controls(Phase phase) {
    Ctx &c = ctx();
    auto list = controls();
    int x = 96;
    int y = H - 140;
    // Transport on the left, extras on the right.
    std::vector<int> xs(list.size());
    int right = W - 96;
    for (int i = static_cast<int>(list.size()) - 1; i >= 0; i--) {
      Ctl ctl = list[static_cast<size_t>(i)];
      if (ctl == Ctl::Back10 || ctl == Ctl::Toggle || ctl == Ctl::Fwd10) continue;
      right -= 76;
      xs[static_cast<size_t>(i)] = right;
      right -= 18;
    }
    for (size_t i = 0; i < list.size(); i++) {
      Ctl ctl = list[i];
      bool f = zone_ == Zone::Controls && static_cast<int>(i) == control_;
      int size = ctl == Ctl::Toggle ? 88 : 76;
      int cx = (ctl == Ctl::Back10 || ctl == Ctl::Toggle || ctl == Ctl::Fwd10) ? x : xs[i];
      int cy = y + (88 - size) / 2;
      c.gfx.fill_circle(cx + size / 2, cy + size / 2, size / 2, f ? theme::white : with_alpha(theme::white, 38));
      Color fg = f ? theme::ink : theme::text;
      int is = 40;
      int ix = cx + (size - is) / 2, iy = cy + (size - is) / 2;
      const char *label = "";
      switch (ctl) {
        case Ctl::Back10: c.icons.draw(Icon::Back10, ix, iy, is, fg); c.fonts.bold.draw("10", cx + size / 2 - 9, cy + size / 2 - 8, 14, fg); label = "Back 10 s"; break;
        case Ctl::Fwd10: c.icons.draw(Icon::Forward10, ix, iy, is, fg); c.fonts.bold.draw("10", cx + size / 2 - 9, cy + size / 2 - 8, 14, fg); label = "Forward 10 s"; break;
        case Ctl::Toggle:
          if (session_.intent_play() && phase != Phase::Ended) { c.gfx.bars(cx + 30, cy + 29, 30, fg); label = "Pause"; }
          else { c.gfx.triangle_right(cx + 30, cy + 25, 38, fg); label = "Play"; }
          break;
        case Ctl::StartOver: c.icons.draw(Icon::Restart, ix, iy, is, fg); label = "Start over"; break;
        case Ctl::Next: c.icons.draw(Icon::Next, ix, iy, is, fg); label = "Next episode"; break;
        case Ctl::Audio: c.icons.draw(Icon::Audio, ix, iy, is, fg); label = "Audio"; break;
        case Ctl::Subtitles: c.icons.draw(Icon::Subtitles, ix, iy, is, fg); label = "Subtitles"; break;
      }
      if (f) draw_text_center(c.fonts.semibold, label, cx + size / 2, cy + size + 10, 20, theme::text);
      if (ctl == Ctl::Back10 || ctl == Ctl::Toggle || ctl == Ctl::Fwd10) x = cx + size + 22;
    }
  }

  void draw_menu() {
    Ctx &c = ctx();
    const MediaInfo &info = ums_.info();
    int w = 640, x = W - 96 - w, y = 170;
    int rows = menu_rows();
    int h = 132 + std::max(1, rows) * 68 + 56;
    c.gfx.fill_round(x, y, w, h, 28, rgba(0x121418, 245));
    c.fonts.heavy.draw(menu_ == Menu::Audio ? "Audio" : "Subtitles", x + 40, y + 36, 34, theme::text);
    int ry = y + 108;
    if (rows == 0) c.fonts.regular.draw("This video reports no audio tracks.", x + 40, ry + 14, 22, theme::text2);
    for (int i = 0; i < rows; i++) {
      bool f = i == menu_index_;
      std::string label;
      bool selected = false;
      if (menu_ == Menu::Audio) {
        label = audio_label(info.audio[static_cast<size_t>(i)]);
        selected = info.audio[static_cast<size_t>(i)].selected;
      } else if (i == 0) {
        label = "Off";
        selected = session_.subtitle_index() < 0;
      } else {
        const MediaTrack &tr = info.text[static_cast<size_t>(i - 1)];
        std::string kind = upper(tr.codec);
        if (kind == "TEXT" || kind == "SUBTITLE") kind.clear();  // generic: says nothing
        label = join({language_name(tr.language), kind});
        selected = session_.subtitle_index() == i - 1;
      }
      if (f) c.gfx.fill_round(x + 16, ry, w - 32, 60, 16, theme::white);
      draw_text(c.fonts.regular, label, x + 40, ry + 16, 24, f ? theme::ink : theme::text, w - 140);
      if (selected) c.icons.draw(Icon::Check, x + w - 76, ry + 14, 32, f ? theme::ink : theme::ok);
      ry += 68;
    }
    if (menu_ == Menu::Subtitles && info.text.empty()) c.fonts.regular.draw("This video has no built-in subtitles.", x + 40, ry + 4, 20, theme::text3);
  }

  void draw_startup() {
    Ctx &c = ctx();
    c.gfx.clear(theme::bg);
    ImageRef bg = c.images.get(req_.item.backdrop, W, H);
    if (bg.texture) {
      c.gfx.texture(bg.texture, 0, 0, W, H, bg.alpha);
      c.gfx.fill_rect(0, 0, W, H, with_alpha(theme::bg, 170));
    }
    // Title art centred (or the name), then what is starting.
    int y = 330;
    ImageRef logo = req_.item.logo.empty() ? ImageRef{} : c.images.get(req_.item.logo, 720, 200, 0, Fit::Contain);
    if (logo.texture) c.gfx.texture(logo.texture, W / 2 - logo.w / 2, y + 200 - logo.h, logo.w, logo.h, logo.alpha);
    else if (logo.failed || req_.item.logo.empty()) draw_text_center(c.fonts.heavy, req_.item.name, W / 2, y + 120, 72, theme::text);
    y += 230;
    if (!req_.label().empty()) draw_text_center(c.fonts.semibold, req_.label(), W / 2, y, 26, theme::text2);
    draw_spinner(W / 2, y + 110, 52, theme::white);
    draw_text_center(c.fonts.regular, source_.empty() ? "Starting\xE2\x80\xA6" : "Starting \xC2\xB7 " + source_, W / 2, y + 160, 22, theme::text3);
  }

  void draw_error() {
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(theme::bg, 235));
    c.icons.draw(Icon::Warning, W / 2 - 28, 300, 56, theme::warn);
    draw_text_center(c.fonts.heavy, "Playback stopped", W / 2, 380, 52, theme::text);
    auto lines = wrap(c.fonts.regular, session_.error(), 26, 1000, 3);
    int y = 460;
    for (auto &l : lines) {
      draw_text_center(c.fonts.regular, l, W / 2, y, 26, theme::text2);
      y += 38;
    }
    const char *labels[3] = {"Try again", "Choose another source", "Exit"};
    int total = 0;
    for (auto *l : labels) total += button_width(l, false) + 20;
    int x = W / 2 - total / 2;
    for (int i = 0; i < 3; i++) x += draw_button(x, 600, labels[i], error_focus_ == i, i == 0) + 20;
  }

  void draw_up_next() {
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(theme::bg, 200));
    const NextEpisode &n = *req_.next;
    long long left = std::max(0LL, (kUpNextMs - (c.now - up_next_at_) + 999) / 1000);
    c.fonts.bold.draw(upper("Up next"), 160, 640, 18, rgba(0x9fb7bd));
    c.fonts.heavy.draw(episode_code(n.season, n.episode) + " \xC2\xB7 " + n.title, 160, 676, 48, theme::text);
    int bx = 160;
    bx += draw_button(bx, 780, "Play now", true, true, kPlayIcon) + 24;
    c.fonts.regular.draw("Starts in " + std::to_string(left) + " s  \xC2\xB7  Back to stay", bx, 803, 24, theme::text2);
    draw_progress(160, 900, 600, 6, static_cast<double>(c.now - up_next_at_) / kUpNextMs, theme::white);
  }

 private:
  PlayRequest req_;
  std::string url_, source_, load_type_;
  UmsPlayer ums_;
  UmsPort port_{ums_};
  Session session_;
  std::string window_id_;
  bool opened_ = false;
  bool hud_ = true;
  long long hud_until_ = 0;
  Zone zone_ = Zone::Timeline;
  int control_ = 1;
  Menu menu_ = Menu::None;
  int menu_index_ = 0;
  int error_focus_ = 0;
  long long last_save_ = 0;
  bool resume_done_ = false, resumed_ = false;
  bool up_next_ = false;
  long long up_next_at_ = 0;
};

}  // namespace

std::unique_ptr<Screen> make_player(const PlayRequest &request, const std::string &url, const std::string &source_label,
                                    const std::string &load_type) {
  return std::make_unique<PlayerScreen>(request, url, source_label, load_type);
}
