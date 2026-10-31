// Settings → Calibrate (CALIBRATION-PLAN.md §8): the owner's Spyder5 measures the
// screen and Bloob corrects Expert 1. The work is done by bin/bloob-cal, started as
// root through the Homebrew Channel (it needs the meter, the TV's calibration
// service and its pattern generator); it runs on even if Bloob is closed, and writes
// its progress to a status file this screen reads every second.
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "common.h"
#include "json.h"
#include "log.h"
#include "luna.h"
#include "media.h"
#include "screens.h"

namespace {

const char *kExec = "luna://org.webosbrew.hbchannel.service/exec";
constexpr int kNits = 140;  // the owner's choice: a small lamp on in the room (29 October 2026)

std::string cal_dir() {
  const char *d = std::getenv("BLOOB_CAL_DATA");
  return d && *d ? d : "/media/internal/com.muhib.bloob/calibration";
}

std::string slurp(const std::string &path) {
  std::ifstream f(path);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

std::string duration_text(long long s) {
  if (s < 90) return std::to_string(std::max(0LL, s)) + " s";
  if (s < 5400) return std::to_string((s + 30) / 60) + " min";
  long long m = (s + 30) / 60;
  return std::to_string(m / 60) + " h " + std::to_string(m % 60) + " min";
}

class CalibrateScreen : public Screen {
 public:
  CalibrateScreen() { poll(); }
  ~CalibrateScreen() override { stop_black(); }
  const char *name() const override { return "calibrate"; }

  enum class Row { Pal, Check, Test, Calibrate, Thorough, Undo, Stop };

  // LG's pattern generator draws in the video plane, under Bloob's graphics (seen on the
  // owner's TV: the meter read Bloob's page, never a square). So while a run is going this
  // screen is see-through, with one small card in a corner, away from the meter.
  bool over_video() const override { return busy(); }

  std::vector<Row> rows() const {
    if (busy()) return {Row::Stop};
    return {Row::Pal, Row::Check, Row::Test, Row::Calibrate, Row::Thorough, Row::Undo};
  }

  std::string spoken() const override {
    auto list = rows();
    int i = std::min(row_, static_cast<int>(list.size()) - 1);
    return title(list[static_cast<size_t>(i)]) + ", button, " + position(i, static_cast<int>(list.size()));
  }

  bool key(Key key, bool repeat) override {
    auto list = rows();
    int count = static_cast<int>(list.size());
    row_ = std::min(row_, count - 1);
    Row r = list[static_cast<size_t>(row_)];
    if (key != Key::Ok) confirm_ = false;
    switch (key) {
      case Key::Back:
        // Leaving would stop the black video, and LG's squares with it.
        if (busy()) {
          back_note_until_ = ctx().now + 4000;
          return true;
        }
        ctx().pop();
        return true;
      case Key::Up: row_ = std::max(0, row_ - 1); return true;
      case Key::Down: row_ = std::min(count - 1, row_ + 1); return true;
      case Key::Ok:
        if (repeat) return true;
        // Calibrate, Pal and Undo change the picture for good: OK twice.
        if ((r == Row::Calibrate || r == Row::Thorough || r == Row::Undo || r == Row::Pal) && !confirm_) {
          confirm_ = true;
          return true;
        }
        confirm_ = false;
        if (r == Row::Stop) stop();
        else if (r == Row::Pal) start_pal();
        else if (r == Row::Calibrate || r == Row::Thorough) start("sdr", std::to_string(kNits) + (r == Row::Thorough ? " thorough" : " quick"));
        else start(r == Row::Check ? "check" : r == Row::Test ? "test" : "undo");
        return true;
      default: return true;
    }
  }

  void draw() override {
    Ctx &c = ctx();
    using namespace theme;
    if (c.now >= next_poll_) poll();
    tend_black();
    if (pal_hold_) {
      draw_pal_card();
      c.redraw_at(c.now + 1000);
      return;
    }
    if (busy()) {
      draw_running_card();
      c.redraw_at(c.now + 1000);
      return;
    }
    c.gfx.clear(plain);
    int x = safe_x, w = 860;
    text(Text::Title2, "Calibrate", x, 90, label(Label::Primary));
    paragraph(Text::Callout,
              "Your Spyder5 measures the screen and Bloob corrects Expert 1, the picture mode you watch in, to " + std::to_string(kNits) +
                  " nits for a room with a small lamp on. Hang the meter flat in the middle of the screen first. The TV draws its test squares "
                  "under Bloob's picture, so while a run goes this screen turns see-through with a small card in a corner: keep Bloob open on "
                  "it until the run finishes.",
              x, 160, label(Label::Secondary), W - 2 * safe_x, 3);
    auto list = rows();
    for (size_t i = 0; i < list.size(); i++) {
      int y = 300 + static_cast<int>(i) * (kRowH + kGap);
      bool f = row_ == static_cast<int>(i);
      if (f) {
        if (!c.lite()) c.gfx.shadow(x, y, w, kRowH, kRowR, theme::shadow_blur, theme::shadow_dy, theme::shadow_alpha);
        c.gfx.fill_round(x, y, w, kRowH, kRowR, theme::button_focus);
      } else {
        c.gfx.fill_round(x, y, w, kRowH, kRowR, theme::settings_platter);
      }
      Color fg = f ? rgba(0x000000) : label(Label::Primary);
      Color fg2 = f ? rgba(0x000000, 166) : label(Label::Secondary);
      int block = line_for(Text::BodySemi) + 5 + line_for(Text::Caption1Semi);
      int ty = y + (kRowH - block) / 2;
      text(Text::BodySemi, title(list[i]), x + kRowPad, ty, fg);
      std::string note = f && confirm_ ? "Press OK again to start" : subtitle(list[i]);
      text(Text::Caption1Semi, note, x + kRowPad, ty + line_for(Text::BodySemi) + 5, fg2, w - 2 * kRowPad - 110);
      text_right(Text::Body, f && confirm_ ? "Confirm" : list[i] == Row::Stop ? "Stop" : "Start", x + w - kRowPad, text_y_centered(Text::Body, y + kRowH / 2),
                 f ? fg : label(Label::Tertiary));
    }
    draw_status(x + w + 60, 300, W - safe_x - (x + w + 60));
    c.redraw_at(c.now + 1000);
  }

 private:
  static constexpr int kRowH = 120, kGap = 14, kRowR = 33, kRowPad = 30;

  static std::string title(Row r) {
    switch (r) {
      case Row::Pal: return "Pal session";
      case Row::Check: return "Check";
      case Row::Test: return "Test on Expert 2";
      case Row::Calibrate: return "Calibrate Expert 1";
      case Row::Thorough: return "Calibrate Expert 1, thorough";
      case Row::Undo: return "Undo calibration";
      case Row::Stop: return "Stop";
    }
    return "";
  }
  static std::string subtitle(Row r) {
    switch (r) {
      case Row::Pal: return "Pal calibrates from your computer; this screen shows the squares and Pal's progress";
      case Row::Check: return "About 3 minutes. Reads test squares; changes nothing";
      case Row::Test: return "About 10 minutes. Changes Expert 2, then puts it back";
      case Row::Calibrate: return "1½ to 2 hours, like Calman. Saves your settings; undone if stopped";
      case Row::Thorough: return "3 to 4 hours. Reads everything more often, for the last bit";
      case Row::Undo: return "Expert 1 back to factory and your saved settings";
      case Row::Stop: return "Stops safely and puts back anything half-done";
    }
    return "";
  }

  // Running, or just started and not yet reported (a second OK must not start a second run),
  // or holding a Pal session.
  bool busy() const { return pal_hold_ || running() || !pending_.empty() || ctx().now < starting_until_; }

  // ---- Pal session (native/PAL-PLAYBOOK.md) ----
  // Pal runs bloob-cal's Pal commands over SSH from the owner's computer; this screen only
  // holds the TV ready: see-through, so LG's test squares show (they are drawn under Bloob's
  // graphics, which hid them in 3.3.3), and the black video only if Pal asks for it
  // (bloob-cal pal-screen video on writes pal/screen.json). Whether the squares need a video
  // at all is what a session without it settles. OK ends the session: anything still running
  // stops, and an open session is undone (pal-end undo).
  void start_pal() {
    pal_hold_ = true;
    pal_ending_ = false;
    pal_since_ = static_cast<long long>(time(nullptr));
    row_ = 0;
    logf("calibrate", "Pal session held");
  }

  void stop_pal() {
    if (running()) {
      long long pid = status_["pid"].as_int();
      logf("calibrate", "stop Pal's command %lld", pid);
      Luna::instance().call(kExec, "{\"command\":\"kill -TERM " + std::to_string(pid) + "\"}", nullptr);
    }
    if (pal_session_open_ && !pal_ending_) {
      pal_ending_ = true;
      launch("pal-end", "undo", 2);  // after the stopped command has let go of the lock
      return;
    }
    if (!pal_session_open_) {
      pal_hold_ = false;
      logf("calibrate", "Pal session released");
    }
  }

  void draw_pal_card() {
    using namespace theme;
    Ctx &c = ctx();
    c.gfx.clear({0, 0, 0, 0});
    const int w = 820, h = 210, x = safe_x, y = H - 60 - h;
    c.gfx.fill_round(x, y, w, h, 30, rgba(0x1c1c1e, 235));
    static const std::pair<const char *, const char *> names[] = {
        {"pal-start", "getting the TV ready"}, {"grey-report", "measuring the greys"}, {"pal-propose", "working out a correction"},
        {"apply-1d", "loading a correction"}, {"pal-light", "setting the light"}, {"pal-end", "finishing"}, {"flashcheck", "checking for flicker"}};
    std::string action = status_["action"].as_string(), doing;
    bool ours = status_["updated"].as_int() >= pal_since_;
    for (auto &n : names)
      if (action == n.first) doing = n.second;
    int tx = x + 36, tw = w - 72, ty = y + 28;
    std::string head = pal_ending_ ? "Pal session: ending, putting Expert 1 back"
                       : running() && !doing.empty() ? "Pal: " + doing
                                                     : "Pal session: waiting for your computer";
    text(Text::Headline, head, tx, ty, label(Label::Primary), tw);
    ty += 54;
    long long readings = status_["readings"].as_int(), expected = status_["expected"].as_int();
    if (running() && expected > 0) {
      draw_progress(tx, ty, tw, 8, std::min(1.0, static_cast<double>(readings) / expected));
      ty += 24;
    }
    std::string line;
    const Json &res = status_["result"];
    if (running()) line = !status_["note"].as_string().empty() ? status_["note"].as_string() : status_["step"].as_string();
    else if (ours && res.is_object() && res["kind"].as_string() == "report")
      line = "Last report: average " + fixed(res["summary"]["avg"].as_number()) + ", worst " + fixed(res["summary"]["max"].as_number()) + " \xCE\x94" "E";
    else if (ours && res.is_object() && res["kind"].as_string() == "applied")
      line = "After the correction: average " + fixed(res["after"]["summary"]["avg"].as_number()) + ", worst " +
             fixed(res["after"]["summary"]["max"].as_number()) + " \xCE\x94" "E";
    else if (ours && res.is_object() && res.has("error")) line = res["error"].as_string();
    else line = "On your computer, ask Claude to run Pal's SDR grey flow";
    text(Text::Caption1, line, tx, ty, label(Label::Secondary), tw);
    ty += 44;
    std::string hint = c.now < back_note_until_ ? "A Pal session is open: OK ends it, then Back leaves"
                       : pal_session_open_     ? "OK ends the session and puts Expert 1 back"
                                               : "OK ends the session";
    text(Text::Caption1, hint + (pal_video_ ? "  \xC2\xB7  black video on" : ""), tx, ty, label(Label::Tertiary), tw);
  }

  static std::string fixed(double v) {
    char b[16];
    std::snprintf(b, sizeof b, "%.2f", v);
    return b;
  }

  // Over the patterns: transparent everywhere except a card in the bottom left.
  void draw_running_card() {
    using namespace theme;
    Ctx &c = ctx();
    c.gfx.clear({0, 0, 0, 0});
    const int w = 820, h = 210, x = safe_x, y = H - 60 - h;
    c.gfx.fill_round(x, y, w, h, 30, rgba(0x1c1c1e, 235));
    static const std::pair<const char *, const char *> names[] = {{"check", "Check"}, {"test", "Test on Expert 2"}, {"sdr", "Calibrate Expert 1"}, {"undo", "Undo"}};
    std::string action = running() ? status_["action"].as_string() : started_, what = action;
    for (auto &n : names)
      if (action == n.first) what = n.second;
    int tx = x + 36, tw = w - 72, ty = y + 28;
    text(Text::Headline, what + (running() ? ": running" : ": starting"), tx, ty, label(Label::Primary), tw);
    ty += 54;
    long long readings = status_["readings"].as_int(), expected = status_["expected"].as_int(), started = status_["started"].as_int();
    if (running() && expected > 0) {
      double frac = std::min(1.0, static_cast<double>(readings) / expected);
      draw_progress(tx, ty, tw, 8, frac);
      ty += 24;
      long long elapsed = static_cast<long long>(time(nullptr)) - started;
      std::string t = std::to_string(readings) + " of about " + std::to_string(expected) + " readings  \xC2\xB7  " + duration_text(elapsed) + " so far";
      if (readings > 20 && frac < 1) t += ", about " + duration_text(static_cast<long long>(elapsed * (1 - frac) / frac)) + " left";
      text(Text::Caption1, t, tx, ty, label(Label::Secondary), tw);
      ty += 44;
    } else {
      ty += 32;
    }
    std::string step = status_["step"].as_string(), note = status_["note"].as_string();
    if (running() && !note.empty()) step = note;  // e.g. a slow dark reading: say so, so it doesn't look frozen
    std::string hint = c.now < back_note_until_ ? "A run is going: OK stops it, then Back leaves" : "OK stops";
    if (!pending_.empty()) hint = "Starting the black video the TV needs for its squares";
    text(Text::Caption1, (running() && !step.empty() ? step + "  \xC2\xB7  " : std::string()) + hint, tx, ty, label(Label::Tertiary), tw);
  }

  // ---- The black video ----
  // LG's squares draw only while a video plays (owner's TV, 30 October 2026: with Bloob's
  // pages or LG Home on screen every pattern was accepted and none appeared; with a video
  // playing all five routes drew). So a run plays a 10-minute SDR black clip, looped, in
  // Bloob's own video window, and bloob-cal starts once it plays. The picture mode measured
  // is then the one films in Bloob use.
  void start_black() {
    if (black_) return;
    Ctx &c = ctx();
    if (window_.empty() && c.webos.create_exported_window) {
      const char *w = c.webos.create_exported_window(0);
      window_ = w ? w : "";
    }
    if (!window_.empty() && c.webos.set_exported_window) {
      int dw = W, dh = H;
      SDL_GetWindowSize(c.window, &dw, &dh);
      SDL_Rect src{0, 0, 1920, 1080}, dst{0, 0, dw, dh};
      c.webos.set_exported_window(window_.c_str(), &src, &dst);
    }
    black_.reset(new UmsPlayer());
    black_playing_ = black_failed_ = false;
    black_started_at_ = c.now;
    std::weak_ptr<int> alive = alive_;
    black_->load("file://" + c.dir + "/assets/media/black.mp4", c.app_id, window_, "media", [this, alive](const MediaEvent &e) {
      if (!alive.lock() || !black_) return;
      if (e.kind == MediaEvent::Loaded || e.kind == MediaEvent::VideoInfo || e.kind == MediaEvent::SourceInfo)
        place_black(black_->info().width, black_->info().height);  // as the player places films, once the size is known
      if (e.kind == MediaEvent::Loaded) {
        black_->play();
      } else if (e.kind == MediaEvent::Playing) {
        if (!black_playing_) logf("calibrate", "black video playing");
        black_playing_ = true;
        ctx().dirty = true;
      } else if (e.kind == MediaEvent::EndOfStream) {
        black_->seek(0);
        black_->play();
      } else if (e.kind == MediaEvent::Error || e.kind == MediaEvent::Lost) {
        logf("calibrate", "black video stopped: %s", e.text.c_str());
        black_failed_ = true;
        ctx().dirty = true;
      }
    });
    logf("calibrate", "black video loading");
  }

  void place_black(int vw, int vh) {
    Ctx &c = ctx();
    if (window_.empty() || !c.webos.set_exported_window) return;
    int dw = W, dh = H;
    SDL_GetWindowSize(c.window, &dw, &dh);
    SDL_Rect src{0, 0, vw > 0 ? vw : 1920, vh > 0 ? vh : 1080}, dst{0, 0, dw, dh};
    c.webos.set_exported_window(window_.c_str(), &src, &dst);
  }

  void stop_black() {
    if (!black_) return;
    logf("calibrate", "black video unloaded");
    black_->unload();
    black_.reset();
    black_playing_ = false;
    Ctx &c = ctx();
    if (!window_.empty() && c.webos.destroy_exported_window) {
      c.webos.destroy_exported_window(window_.c_str());
      window_.clear();
    }
  }

  // Every frame: the pending run starts once the video plays (or after 6 s: the square check
  // then says what the meter saw); a dropped video is restarted; no run, no video.
  void tend_black() {
    Ctx &c = ctx();
    if (!pending_.empty()) {
      if (black_playing_ || c.now - black_started_at_ > 6000) {
        if (!black_playing_) logf("calibrate", "black video not confirmed after 6 s; starting anyway");
        launch(pending_, pending_args_);
        pending_.clear();
      }
      c.redraw_at(c.now + 200);
      return;
    }
    if (pal_hold_) {
      if (pal_video_ && !black_) start_black();
      if (!pal_video_ && black_) stop_black();
      if (pal_video_ && black_failed_ && c.now - black_started_at_ > 5000) {
        stop_black();
        start_black();
      }
      return;
    }
    if (busy()) {
      if (black_failed_ && c.now - black_started_at_ > 5000) {
        stop_black();
        start_black();
      }
    } else if (black_) {
      stop_black();
    }
  }

  void background() override { stop_black(); }
  void foreground() override {
    if (busy() && (!pal_hold_ || pal_video_)) start_black();
  }

  // The status file's run is live only if its pid is still bloob-cal: a pid from an
  // earlier run may belong to another program by now.
  bool running() const {
    if (status_["state"].as_string() != "running") return false;
    long long pid = status_["pid"].as_int();
    if (pid <= 0) return false;
    return slurp("/proc/" + std::to_string(pid) + "/cmdline").find("bloob-cal") != std::string::npos;
  }

  void poll() {
    Ctx &c = ctx();
    next_poll_ = c.now + 1000;
    std::string text = slurp(cal_dir() + "/status.json");
    bool ok = false;
    Json s = Json::parse(text, &ok);
    if (ok) status_ = s;
    Json screen = Json::parse(slurp(cal_dir() + "/pal/screen.json"), &ok);
    pal_video_ = ok && screen["video"].as_bool();
    Json session = Json::parse(slurp(cal_dir() + "/pal/session.json"), &ok);
    pal_session_open_ = ok && session["open"].as_bool();
    // A session ends when pal-end has run after it was held (from the computer, or from OK here).
    if (pal_hold_ && status_["action"].as_string() == "pal-end" && status_["state"].as_string() != "running" &&
        status_["updated"].as_int() >= pal_since_ && !running()) {
      pal_hold_ = pal_ending_ = false;
      logf("calibrate", "Pal session ended");
    }
  }

  void start(const std::string &action, const std::string &args = "") {
    if (!pending_.empty()) return;
    pending_ = action;
    pending_args_ = args;
    started_ = action;
    starting_until_ = ctx().now + 30000;
    row_ = 0;
    start_black();
  }

  void launch(const std::string &action, const std::string &args, int delay_s = 0) {
    std::string bin = std::getenv("BLOOB_CAL_BIN") ? std::getenv("BLOOB_CAL_BIN") : ctx().dir + "/bin/bloob-cal";
    std::string dir = cal_dir();
    // Fixed words only: the action and its arguments come from this screen's own list.
    // bloob-cal writes last-run.log itself once it holds the lock; a refused start only
    // reaches start.log.
    std::string cmd = "mkdir -p '" + dir + "' && cd '" + dir + "' && " + (delay_s ? "sleep " + std::to_string(delay_s) + " && " : std::string()) + "'" +
                      bin + "' " + action + (args.empty() ? "" : " " + args) + " </dev/null >'" + dir + "/start.log' 2>&1 &";
    logf("calibrate", "start %s", action.c_str());
    started_ = action;
    starting_until_ = ctx().now + 8000;
    Luna::instance().call(kExec, "{\"command\":" + Json::quote(cmd) + "}", [](const std::string &reply) {
      if (!Json::parse(reply)["returnValue"].as_bool()) logf("calibrate", "could not start: %s", reply.c_str());
    });
    next_poll_ = ctx().now + 1500;
    row_ = 0;
  }

  void stop() {
    if (pal_hold_) {
      stop_pal();
      return;
    }
    if (!pending_.empty()) {  // still waiting for the black video: nothing has started
      pending_.clear();
      starting_until_ = 0;
      stop_black();
      return;
    }
    if (!running()) return;  // just launched: no pid of this run yet
    long long pid = status_["pid"].as_int();
    logf("calibrate", "stop %lld", pid);
    Luna::instance().call(kExec, "{\"command\":\"kill -TERM " + std::to_string(pid) + "\"}", nullptr);
  }

  void draw_status(int x, int y, int w) {
    using namespace theme;
    Ctx &c = ctx();
    std::string state = status_["state"].as_string();
    std::string action = status_["action"].as_string();
    if (state.empty()) {
      text(Text::Headline, "Not run yet", x, y, label(Label::Secondary));
      paragraph(Text::Callout, "Start with Check: it confirms the squares appear and the meter reads them.", x, y + 56, label(Label::Tertiary), w, 3);
      return;
    }
    static const std::pair<const char *, const char *> names[] = {{"check", "Check"}, {"test", "Test on Expert 2"}, {"sdr", "Calibrate Expert 1"}, {"undo", "Undo"},
                                                                   {"pal-start", "Pal"}, {"grey-report", "Pal"}, {"apply-1d", "Pal"}, {"pal-end", "Pal"}};
    std::string what = action;
    for (auto &n : names)
      if (action == n.first) what = n.second;
    bool live = running();
    std::string head = what + (live ? ": running" : state == "done" ? ": finished" : state == "stopped" ? ": stopped" : state == "running" ? ": interrupted" : ": did not finish");
    text(Text::Headline, head, x, y, label(Label::Primary), w);
    y += 58;
    long long readings = status_["readings"].as_int(), expected = status_["expected"].as_int();
    long long started = status_["started"].as_int(), updated = status_["updated"].as_int();
    if (live && expected > 0) {
      double frac = std::min(1.0, static_cast<double>(readings) / expected);
      draw_progress(x, y, w, 10, frac);
      y += 30;
      long long elapsed = static_cast<long long>(time(nullptr)) - started;
      std::string t = "Readings " + std::to_string(readings) + " of about " + std::to_string(expected) + "  \xC2\xB7  " + duration_text(elapsed) + " so far";
      if (readings > 20 && frac < 1) t += ", about " + duration_text(static_cast<long long>(elapsed * (1 - frac) / frac)) + " left";
      text(Text::Caption1, t, x, y, label(Label::Secondary), w);
      y += 48;
    } else if (updated > 0) {
      text(Text::Caption1, "Took " + duration_text(updated - started), x, y, label(Label::Tertiary), w);
      y += 44;
    }
    std::string step = status_["step"].as_string();
    if (!step.empty()) {
      text(Text::Callout, step, x, y, label(Label::Secondary), w);
      y += 48;
    }
    const Json &result = status_["result"];
    if (!live && result.is_object()) {
      if (action == "sdr" && result["ok"].as_bool()) {
        char b[200];
        std::snprintf(b, sizeof b, "Grey %.2f \xE2\x86\x92 %.2f  \xC2\xB7  Colour %.2f \xE2\x86\x92 %.2f  (\xCE\x94" "E, lower is better)",
                      result["before"]["grey_avg"].as_number(), result["after"]["grey_avg"].as_number(), result["before"]["colour_avg"].as_number(),
                      result["after"]["colour_avg"].as_number());
        text(Text::BodySemi, b, x, y, label(Label::Primary), w);
        y += 54;
        if (result["viewing"].is_object()) {
          std::snprintf(b, sizeof b, "As you watch it: grey %.2f, colour %.2f", result["viewing"]["grey_avg"].as_number(), result["viewing"]["colour_avg"].as_number());
          text(Text::Callout, b, x, y, label(Label::Secondary), w);
          y += 48;
        }
      } else if (action == "test") {
        text(Text::BodySemi, std::to_string(result["passed"].as_int()) + " passed, " + std::to_string(result["failed"].as_int()) + " failed", x, y,
             label(Label::Primary), w);
        y += 54;
      } else if (action == "sdr" && result.has("error")) {
        paragraph(Text::Callout, result["error"].as_string() + (result["undone"].as_bool() ? ". Expert 1 was put back." : ""), x, y,
                  label(Label::Secondary), w, 3);
        y += 120;
      }
    }
    // The last lines bloob-cal wrote.
    const Json &lines = status_["lines"];
    size_t n = lines.size(), from = n > 7 ? n - 7 : 0;
    for (size_t i = from; i < n; i++) {
      std::string line = lines[i].as_string();
      if (line.rfind("== ", 0) == 0) continue;  // the step, already shown
      text(Text::Caption2, line, x, y, label(Label::Tertiary), w);
      y += 30;
    }
    (void)c;
  }

  int row_ = 0;
  bool confirm_ = false;
  long long next_poll_ = 0;
  std::string started_;
  long long starting_until_ = 0;
  std::string pending_, pending_args_;  // a run waiting for the black video
  long long back_note_until_ = 0;
  std::unique_ptr<UmsPlayer> black_;
  std::string window_;
  bool black_playing_ = false, black_failed_ = false;
  long long black_started_at_ = 0;
  Json status_;
  bool pal_hold_ = false, pal_ending_ = false, pal_video_ = false, pal_session_open_ = false;
  long long pal_since_ = 0;
};

}  // namespace

std::unique_ptr<Screen> make_calibrate() { return std::make_unique<CalibrateScreen>(); }
