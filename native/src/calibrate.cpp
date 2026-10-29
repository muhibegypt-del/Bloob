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
  const char *name() const override { return "calibrate"; }

  enum class Row { Check, Test, Calibrate, Thorough, Undo, Stop };

  std::vector<Row> rows() const {
    if (running()) return {Row::Stop};
    return {Row::Check, Row::Test, Row::Calibrate, Row::Thorough, Row::Undo};
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
      case Key::Back: ctx().pop(); return true;
      case Key::Up: row_ = std::max(0, row_ - 1); return true;
      case Key::Down: row_ = std::min(count - 1, row_ + 1); return true;
      case Key::Ok:
        if (repeat) return true;
        // Calibrate and Undo change the picture for good: OK twice.
        if ((r == Row::Calibrate || r == Row::Thorough || r == Row::Undo) && !confirm_) {
          confirm_ = true;
          return true;
        }
        confirm_ = false;
        if (r == Row::Stop) stop();
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
    c.gfx.clear(plain);
    int x = safe_x, w = 860;
    text(Text::Title2, "Calibrate", x, 90, label(Label::Primary));
    paragraph(Text::Callout,
              "Your Spyder5 measures the screen and Bloob corrects Expert 1, the picture mode you watch in, to " + std::to_string(kNits) +
                  " nits for a room with a small lamp on. Hang the meter flat in the middle of the screen first. While it works the screen "
                  "shows test squares instead of Bloob, and it carries on even if Bloob is closed.",
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
      case Row::Check: return "About 3 minutes. Reads test squares; changes nothing";
      case Row::Test: return "About 10 minutes. Changes Expert 2, then puts it back";
      case Row::Calibrate: return "1 to 1½ hours, like Calman. Saves your settings; undone if stopped";
      case Row::Thorough: return "3 to 4 hours. Reads everything more often, for the last bit";
      case Row::Undo: return "Expert 1 back to factory and your saved settings";
      case Row::Stop: return "Stops safely and puts back anything half-done";
    }
    return "";
  }

  bool running() const {
    if (status_["state"].as_string() != "running") return false;
    long long pid = status_["pid"].as_int();
    struct stat st;
    return pid > 0 && stat(("/proc/" + std::to_string(pid)).c_str(), &st) == 0;
  }

  void poll() {
    Ctx &c = ctx();
    next_poll_ = c.now + 1000;
    std::string text = slurp(cal_dir() + "/status.json");
    bool ok = false;
    Json s = Json::parse(text, &ok);
    if (ok) status_ = s;
  }

  void start(const std::string &action, const std::string &args = "") {
    std::string bin = std::getenv("BLOOB_CAL_BIN") ? std::getenv("BLOOB_CAL_BIN") : ctx().dir + "/bin/bloob-cal";
    std::string dir = cal_dir();
    // Fixed words only: the action and its arguments come from this screen's own list.
    std::string cmd = "mkdir -p '" + dir + "' && cd '" + dir + "' && '" + bin + "' " + action + (args.empty() ? "" : " " + args) +
                      " </dev/null >'" + dir + "/last-run.log' 2>&1 &";
    logf("calibrate", "start %s", action.c_str());
    started_ = action;
    Luna::instance().call(kExec, "{\"command\":" + Json::quote(cmd) + "}", [](const std::string &reply) {
      if (!Json::parse(reply)["returnValue"].as_bool()) logf("calibrate", "could not start: %s", reply.c_str());
    });
    next_poll_ = ctx().now + 1500;
    row_ = 0;
  }

  void stop() {
    long long pid = status_["pid"].as_int();
    if (pid <= 0) return;
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
    static const std::pair<const char *, const char *> names[] = {{"check", "Check"}, {"test", "Test on Expert 2"}, {"sdr", "Calibrate Expert 1"}, {"undo", "Undo"}};
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
  Json status_;
};

}  // namespace

std::unique_ptr<Screen> make_calibrate() { return std::make_unique<CalibrateScreen>(); }
