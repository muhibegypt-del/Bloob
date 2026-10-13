// Choose how to watch: one recommended source in plain language, the other
// options as a compact table, then pairing (once) and AllDebrid resolution
// with visible, cancellable progress. A question (which file, which quality)
// appears as a short list; failures say what happened and keep the list.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "pairing.h"
#include "screens.h"

namespace {

constexpr int AX = 96, AW = 520;   // aside
constexpr int MX = 700, MW = W - 96 - 700;  // main column
constexpr int ROW = 76;
constexpr int TABLE_TOP = 372;
constexpr int VISIBLE_ROWS = 8;
constexpr long long kPollMs = 700;

// Column positions inside the table.
constexpr int C_QUALITY = 28, C_PICTURE = 200, C_AUDIO = 430, C_SIZE = 720, C_AVAIL = 860;

class SourcesScreen : public Screen {
 public:
  SourcesScreen(const PlayRequest &request, bool autostart) : req_(request), autostart_(autostart) {
    focus_.rate = 16;
    scroll_.rate = 12;
    load();
    refresh_account();
  }

  const char *name() const override { return "sources"; }

  void shown() override {
    if (leave_after_play_) {
      leave_after_play_ = false;
      // Back from playback returns to where the viewer came from, unless they
      // asked the player for another source.
      if (!choose_another()) {
        ctx().pop();
        return;
      }
      choose_another() = false;
    }
    refresh_account();
  }

  // Called by the player when the viewer asks for another source.
  static bool &choose_another() {
    static bool flag = false;
    return flag;
  }

  // ---------------------------------------------------------------- data

  void load() {
    state_ = State::Loading;
    std::string body = "{\"type\":" + Json::quote(req_.item.type) + ",\"videoId\":" + Json::quote(req_.video_id) +
                       ",\"language\":" + Json::quote(ctx().language) + "}";
    ctx().api.post("/api/sources/ranked", body, [this](const ApiResult &r) {
      if (!r.ok()) {
        state_ = State::Error;
        error_ = r.error;
        return;
      }
      ranking_ = parse_ranking(r.json);
      options_.clear();
      if (ranking_.recommended) options_.push_back(*ranking_.recommended);
      for (auto &g : ranking_.groups)
        for (auto &o : g.items) options_.push_back(o);
      state_ = options_.empty() ? State::Empty : State::Ready;
      focus_index_ = 0;
      logf("sources", "%d sources, recommended=%d", ranking_.total, ranking_.recommended ? 1 : 0);
      if (autostart_ && ranking_.recommended) {
        autostart_ = false;
        choose(0);
      }
    }, alive_);
  }

  void refresh_account() {
    ctx().api.get("/api/account", [this](const ApiResult &r) {
      if (!r.ok()) return;
      ctx().paired = r.json["paired"].as_bool();
      account_known_ = true;
      durable_ = r.json["durable"].as_bool();
    }, alive_);
  }

  bool has_recommended() const { return ranking_.recommended.has_value(); }

  // ---------------------------------------------------------------- resolving

  void choose(int index) {
    if (index < 0 || index >= static_cast<int>(options_.size())) return;
    chosen_ = options_[static_cast<size_t>(index)];
    if (!ctx().paired) {
      pairing_.start([this](bool paired) {
        if (paired) start_job();
      });
      return;
    }
    start_job();
  }

  void start_job() {
    modal_ = Modal::Resolving;
    job_id_.clear();
    step_ = "starting";
    step_label_ = "Starting\xE2\x80\xA6";
    fraction_ = -1;
    saw_caching_ = false;
    std::string body = "{\"candidate\":" + Json::quote(chosen_.id);
    if (req_.item.series()) body += ",\"season\":" + std::to_string(req_.season) + ",\"episode\":" + std::to_string(req_.episode);
    body += "}";
    ctx().api.post("/api/resolve/start", body, [this](const ApiResult &r) { on_job(r); }, alive_);
  }

  void on_job(const ApiResult &r) {
    if (modal_ != Modal::Resolving && modal_ != Modal::ChooseFile && modal_ != Modal::ChooseQuality) return;
    if (!r.ok()) {
      if (r.status == 401) {  // the service lost or never had the pairing
        ctx().paired = false;
        modal_ = Modal::None;
        pairing_.start([this](bool paired) { if (paired) start_job(); });
        return;
      }
      fail(r.error);
      return;
    }
    const Json &j = r.json;
    job_id_ = j["id"].as_string(job_id_);
    std::string state = j["state"].as_string();
    step_ = j["step"].as_string(step_);
    step_label_ = j["label"].as_string(step_label_);
    fraction_ = j["fraction"].is_null() ? -1.0 : j["fraction"].as_number(-1);
    if (step_ == "caching") saw_caching_ = true;
    next_poll_ = ctx().now + kPollMs;
    if (state == "ready") {
      std::string url = j["url"].as_string();
      std::string file = j["fileName"].as_string();
      modal_ = Modal::None;
      logf("sources", "resolved (%s)", redact_uri(url).c_str());
      leave_after_play_ = true;
      choose_another() = false;
      ctx().push(make_player(req_, url, chosen_.headline));
    } else if (state == "error") {
      fail(j["error"].as_string("Could not get this source."));
    } else if (state == "choose-file") {
      modal_ = Modal::ChooseFile;
      choices_.clear();
      for (size_t i = 0; i < j["files"].size(); i++)
        choices_.push_back({std::to_string(j["files"][i]["index"].as_int()), j["files"][i]["name"].as_string(), j["files"][i]["size"].as_string()});
      choice_ = 0;
    } else if (state == "choose-quality") {
      modal_ = Modal::ChooseQuality;
      choices_.clear();
      for (size_t i = 0; i < j["qualities"].size(); i++)
        choices_.push_back({j["qualities"][i]["id"].as_string(), j["qualities"][i]["label"].as_string(), ""});
      choice_ = 0;
    }
  }

  void poll() {
    if (job_id_.empty()) return;
    next_poll_ = ctx().now + 60000;  // until the reply arrives
    ctx().api.get("/api/resolve/status?id=" + job_id_, [this](const ApiResult &r) { on_job(r); }, alive_);
  }

  void answer(int index) {
    const Choice &c = choices_[static_cast<size_t>(index)];
    std::string field = modal_ == Modal::ChooseFile ? "\"file\":" + c.id : "\"quality\":" + Json::quote(c.id);
    modal_ = Modal::Resolving;
    step_ = "unlocking";
    step_label_ = "Getting the stream\xE2\x80\xA6";
    ctx().api.post("/api/resolve/choose", "{\"id\":" + Json::quote(job_id_) + "," + field + "}", [this](const ApiResult &r) { on_job(r); }, alive_);
  }

  void cancel_job() {
    if (!job_id_.empty()) ctx().api.post("/api/resolve/cancel", "{\"id\":" + Json::quote(job_id_) + "}");
    job_id_.clear();
    modal_ = Modal::None;
  }

  void fail(const std::string &message) {
    if (!job_id_.empty()) ctx().api.post("/api/resolve/cancel", "{\"id\":" + Json::quote(job_id_) + "}");
    job_id_.clear();
    modal_ = Modal::Failed;
    failure_ = message;
    fail_focus_ = 0;
    logf("sources", "resolve failed: %s", message.c_str());
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    if (pairing_.active()) return pairing_.key(key);
    switch (modal_) {
      case Modal::Resolving:
        if (key == Key::Back || key == Key::Ok) cancel_job();
        return true;
      case Modal::ChooseFile:
      case Modal::ChooseQuality: {
        int n = static_cast<int>(choices_.size());
        if (key == Key::Up) choice_ = std::max(0, choice_ - 1);
        if (key == Key::Down) choice_ = std::min(n - 1, choice_ + 1);
        if (key == Key::Ok && !repeat && n) answer(choice_);
        if (key == Key::Back) cancel_job();
        return true;
      }
      case Modal::Failed:
        if (key == Key::Left || key == Key::Right) fail_focus_ = 1 - fail_focus_;
        if (key == Key::Ok && !repeat) {
          modal_ = Modal::None;
          if (fail_focus_ == 1) start_job();
        }
        if (key == Key::Back) modal_ = Modal::None;
        return true;
      case Modal::None:
        break;
    }
    if (state_ != State::Ready) {
      if (key == Key::Back) ctx().pop();
      else if (key == Key::Ok && state_ != State::Loading) load();
      return true;
    }
    int n = static_cast<int>(options_.size());
    switch (key) {
      case Key::Up: focus_index_ = std::max(0, focus_index_ - 1); break;
      case Key::Down: focus_index_ = std::min(n - 1, focus_index_ + 1); break;
      case Key::Ok:
        if (!repeat) choose(focus_index_);
        return true;
      case Key::Play:
      case Key::PlayPause:
        choose(focus_index_);
        return true;
      case Key::Back:
        ctx().pop();
        return true;
      default:
        return true;
    }
    focus_.snap(0);
    focus_.to(1);
    int table_index = focus_index_ - (has_recommended() ? 1 : 0);
    float target = scroll_.target;
    if (table_index >= 0) {
      if (table_index < target) target = static_cast<float>(table_index);
      if (table_index >= target + VISIBLE_ROWS) target = static_cast<float>(table_index - VISIBLE_ROWS + 1);
    } else {
      target = 0;
    }
    scroll_.to(target);
    return true;
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    pairing_.update();
    if (modal_ == Modal::Resolving && !job_id_.empty() && c.now >= next_poll_) poll();
    if (focus_.step(dt) | scroll_.step(dt)) c.animating = true;
    if (modal_ == Modal::Resolving) c.animating = true;
  }

  void draw() override {
    Ctx &c = ctx();
    c.gfx.clear(theme::bg);
    draw_aside();
    switch (state_) {
      case State::Loading:
        draw_spinner(MX + MW / 2, 420, 56);
        draw_text_center(c.fonts.regular, "Finding sources\xE2\x80\xA6", MX + MW / 2, 480, 26, theme::text2);
        break;
      case State::Error:
      case State::Empty: {
        std::string title = state_ == State::Empty ? "No sources found" : "Couldn\xE2\x80\x99t search for sources";
        std::string body = state_ == State::Empty ? "Nothing is available for this yet. Try again later." : error_;
        c.fonts.heavy.draw(title, MX, 120, 44, theme::text);
        draw_paragraph(c.fonts.regular, body, MX, 190, 26, 38, theme::text2, MW, 3);
        draw_button(MX, 320, "Try again", true, true);
        break;
      }
      case State::Ready:
        draw_options();
        break;
    }
    draw_modal();
    pairing_.draw();
  }

  void draw_aside() {
    Ctx &c = ctx();
    std::string art = !req_.episode_thumbnail.empty() ? req_.episode_thumbnail : req_.item.backdrop;
    ImageRef img = c.images.get(art, AW, 292, 22, Fit::Cover);
    c.gfx.fill_round(AX, 80, AW, 292, 22, theme::panel2);
    if (img.texture) c.gfx.texture(img.texture, AX, 80, AW, 292, img.alpha);
    int y = 404;
    c.fonts.bold.draw(upper("Choose how to watch"), AX, y, 18, theme::text3);
    y += 36;
    auto lines = wrap(c.fonts.heavy, req_.item.name, 54, AW, 2);
    for (auto &l : lines) {
      c.fonts.heavy.draw(l, AX, y, 54, theme::text);
      y += 60;
    }
    y += 8;
    if (req_.item.series()) {
      draw_text(c.fonts.semibold, req_.label(), AX, y, 24, theme::text2, AW);
      y += 40;
    }
    std::vector<std::string> facts;
    if (!req_.item.series() && !req_.item.runtime.empty()) facts.push_back(runtime_text(req_.item.runtime));
    if (!req_.item.year.empty()) facts.push_back(req_.item.year);
    draw_text(c.fonts.regular, join(facts), AX, y, 21, theme::text3, AW);
    y += 36;
    if (req_.resume_ms > 0) {
      c.fonts.regular.draw("Continues from " + clock_text(req_.resume_ms), AX, y, 21, theme::accent);
      y += 36;
    }
    // Account status.
    int ay = 940;
    c.gfx.fill_round(AX, ay, AW, 60, 16, theme::panel);
    bool paired = ctx().paired;
    c.gfx.fill_circle(AX + 28, ay + 30, 6, !account_known_ ? theme::text4 : paired ? theme::ok : theme::warn);
    std::string text = !account_known_ ? "Checking AllDebrid\xE2\x80\xA6"
                       : paired ? (durable_ ? "AllDebrid connected \xC2\xB7 saved on this TV" : "AllDebrid connected \xC2\xB7 until restart")
                                : "AllDebrid not connected \xC2\xB7 you\xE2\x80\x99ll pair on Play";
    draw_text(c.fonts.regular, text, AX + 48, ay + 18, 20, theme::text2, AW - 64);
  }

  void draw_options() {
    Ctx &c = ctx();
    int index = 0;
    int y = 80;
    if (has_recommended()) {
      const SourceOption &r = *ranking_.recommended;
      bool focused = focus_index_ == 0 && modal_ == Modal::None;
      int h = 236;
      Color bg = focused ? rgba(0xf4f5f7) : with_alpha(theme::white, 22);
      Color fg = focused ? rgba(0x0b0d12) : theme::text;
      Color fg2 = focused ? theme::ink2 : theme::text2;
      if (focused) c.gfx.fill_round(MX - 6, y - 6, MW + 12, h + 12, 32, with_alpha(theme::white, 46));
      c.gfx.fill_round(MX, y, MW, h, 26, bg);
      c.fonts.heavy.draw(upper("Recommended"), MX + 40, y + 32, 17, with_alpha(fg, 150));
      draw_text(c.fonts.heavy, r.headline, MX + 40, y + 62, 50, fg, MW - 340);
      // Play pill.
      int pw = 200, ph = 72, px = MX + MW - 40 - pw, py = y + 56;
      c.gfx.fill_round(px, py, pw, ph, ph / 2, focused ? rgba(0x0b0d12) : with_alpha(theme::white, 40));
      c.gfx.triangle_right(px + 40, py + 22, 28, theme::white);
      c.fonts.heavy.draw("Play", px + 84, py + 20, 26, theme::white);
      draw_text(c.fonts.regular, r.facts, MX + 40, y + 140, 22, fg2, MW - 80);
      std::string why = ranking_.why;
      if (!r.warnings.empty()) why += "  " + r.warnings[0];
      draw_text(c.fonts.regular, why, MX + 40, y + 180, 20, focused ? with_alpha(fg, 150) : theme::text3, MW - 80);
      y += h + 40;
      index = 1;
    }
    // Table header.
    int ty = has_recommended() ? TABLE_TOP : 80;
    int rows_total = static_cast<int>(options_.size()) - index;
    if (rows_total > 0) {
      const char *heads[] = {"Quality", "Picture", "Audio", "Size", "Availability"};
      const int cols[] = {C_QUALITY, C_PICTURE, C_AUDIO, C_SIZE, C_AVAIL};
      for (int k = 0; k < 5; k++) c.fonts.bold.draw(upper(heads[k]), MX + cols[k], ty, 16, theme::text4);
    }
    int top = ty + 36;
    float s = scroll_.value;
    for (int i = 0; i < rows_total; i++) {
      float ry = top + (i - s) * ROW;
      if (ry < top - ROW / 2 || ry > H - ROW) continue;
      const SourceOption &o = options_[static_cast<size_t>(i + index)];
      bool focused = focus_index_ == i + index && modal_ == Modal::None;
      int yy = static_cast<int>(ry);
      if (focused) c.gfx.fill_round(MX, yy, MW, ROW - 4, 16, rgba(0xf4f5f7));
      else c.gfx.fill_rect(MX + 28, yy, MW - 56, 1, theme::line);
      Color fg = focused ? rgba(0x0b0d12) : (o.excluded ? theme::text4 : theme::text);
      Color fg2 = focused ? theme::ink2 : (o.excluded ? theme::text4 : theme::text2);
      int text_y = yy + (ROW - 4) / 2 - 15;
      c.fonts.heavy.draw(o.resolution, MX + C_QUALITY, text_y - 1, 24, fg);
      draw_text(c.fonts.regular, o.picture, MX + C_PICTURE, text_y, 21, fg2, C_AUDIO - C_PICTURE - 20);
      draw_text(c.fonts.regular, o.audio, MX + C_AUDIO, text_y, 21, fg2, C_SIZE - C_AUDIO - 20);
      draw_text(c.fonts.regular, o.size, MX + C_SIZE, text_y, 21, fg2, C_AVAIL - C_SIZE - 20);
      std::string avail = o.availability;
      bool warn = !o.cautions.empty();
      if (warn) avail = o.cautions[0];
      draw_text(c.fonts.regular, avail, MX + C_AVAIL, text_y, 21, warn ? (focused ? theme::warn_ink : theme::warn) : fg2, MW - C_AVAIL - 70);
      c.icons.draw(Icon::ChevronRight, MX + MW - 56, yy + (ROW - 4) / 2 - 14, 28, with_alpha(fg, 120));
    }
    // The focused option's raw release name, for those who want it.
    if (focus_index_ < static_cast<int>(options_.size())) {
      draw_text(c.fonts.regular, options_[static_cast<size_t>(focus_index_)].release, MX + 28, H - 52, 18, theme::text4, MW - 56);
    }
  }

  void draw_modal() {
    if (modal_ == Modal::None) return;
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(theme::bg, 215));
    int w = 980, x = (W - w) / 2;
    if (modal_ == Modal::Resolving) {
      struct StepRow { const char *id; const char *label; };
      std::vector<StepRow> steps = {{"adding", "Send to AllDebrid"}};
      if (saw_caching_) steps.push_back({"caching", "AllDebrid downloads the release"});
      steps.push_back({"files", "Find the video file"});
      steps.push_back({"unlocking", "Get the stream"});
      int h = 360 + static_cast<int>(steps.size()) * 56 + (saw_caching_ ? 30 : 0), y = (H - h) / 2;
      c.gfx.fill_round(x, y, w, h, 32, theme::panel);
      c.fonts.bold.draw(upper("Getting ready"), x + 64, y + 56, 18, theme::text3);
      draw_text(c.fonts.heavy, chosen_.headline, x + 64, y + 88, 44, theme::text, w - 128);
      static const char *order[] = {"starting", "adding", "caching", "files", "unlocking", "preparing", "ready"};
      auto rank = [](const std::string &id) {
        for (int i = 0; i < 7; i++) if (id == order[i]) return i;
        return 0;
      };
      int now_rank = rank(step_);
      int sy = y + 176;
      for (auto &st : steps) {
        std::string id = st.id;
        int r = rank(id);
        bool preparing = id == "unlocking" && step_ == "preparing";
        bool done = now_rank > r && !preparing;
        bool current = now_rank == r || preparing;
        if (done) c.icons.draw(Icon::Check, x + 64, sy, 30, theme::ok);
        else if (current) draw_spinner(x + 79, sy + 15, 30, theme::text);
        else c.gfx.fill_circle(x + 79, sy + 15, 6, theme::text4);
        c.fonts.regular.draw(st.label, x + 116, sy, 26, done ? theme::text2 : current ? theme::text : theme::text4);
        if (id == "caching" && fraction_ >= 0 && current) {
          draw_progress(x + 116, sy + 44, 520, 8, fraction_);
          c.fonts.regular.draw(std::to_string(static_cast<int>(fraction_ * 100)) + "%", x + 656, sy + 34, 22, theme::text2);
        }
        sy += id == "caching" ? 86 : 56;
      }
      if (!step_label_.empty()) draw_text(c.fonts.regular, step_label_, x + 64, sy + 8, 21, theme::text3, w - 128);
      draw_button(x + 64, y + h - 96, "Cancel", true, false, -1, 60);
      return;
    }
    if (modal_ == Modal::ChooseFile || modal_ == Modal::ChooseQuality) {
      int n = static_cast<int>(choices_.size());
      int shown = std::min(n, 7);
      int h = 200 + shown * 72, y = (H - h) / 2;
      c.gfx.fill_round(x, y, w, h, 32, theme::panel);
      c.fonts.heavy.draw(modal_ == Modal::ChooseFile ? "Which file?" : "Which quality?", x + 64, y + 52, 40, theme::text);
      c.fonts.regular.draw(modal_ == Modal::ChooseFile ? "This release has several videos." : "AllDebrid offers these versions.", x + 64, y + 104, 22, theme::text3);
      int first = std::max(0, std::min(choice_ - 3, n - shown));
      int ry = y + 150;
      for (int i = first; i < first + shown; i++) {
        bool f = i == choice_;
        if (f) c.gfx.fill_round(x + 40, ry, w - 80, 64, 16, rgba(0xf4f5f7));
        draw_text(c.fonts.regular, choices_[static_cast<size_t>(i)].label, x + 64, ry + 17, 22, f ? theme::ink : theme::text, w - 300);
        draw_text_right(c.fonts.regular, choices_[static_cast<size_t>(i)].detail, x + w - 64, ry + 17, 22, f ? theme::ink2 : theme::text3);
        ry += 72;
      }
      return;
    }
    if (modal_ == Modal::Failed) {
      int h = 400, y = (H - h) / 2;
      c.gfx.fill_round(x, y, w, h, 32, theme::panel);
      c.icons.draw(Icon::Warning, x + 64, y + 56, 44, theme::warn);
      c.fonts.heavy.draw("That source didn\xE2\x80\x99t work", x + 128, y + 56, 40, theme::text);
      draw_paragraph(c.fonts.regular, failure_, x + 64, y + 140, 24, 36, theme::text2, w - 128, 3);
      int bx = x + 64;
      bx += draw_button(bx, y + h - 110, "Choose another", fail_focus_ == 0, true, -1, 64) + 16;
      draw_button(bx, y + h - 110, "Try again", fail_focus_ == 1, false, -1, 64);
    }
  }

 private:
  enum class State { Loading, Ready, Empty, Error };
  enum class Modal { None, Resolving, ChooseFile, ChooseQuality, Failed };
  struct Choice {
    std::string id, label, detail;
  };

  PlayRequest req_;
  bool autostart_;
  State state_ = State::Loading;
  std::string error_;
  Ranking ranking_;
  std::vector<SourceOption> options_;
  int focus_index_ = 0;
  Anim focus_, scroll_;
  bool account_known_ = false, durable_ = false;
  Pairing pairing_;
  Modal modal_ = Modal::None;
  SourceOption chosen_;
  std::string job_id_, step_, step_label_;
  double fraction_ = -1;
  bool saw_caching_ = false;
  long long next_poll_ = 0;
  std::vector<Choice> choices_;
  int choice_ = 0;
  std::string failure_;
  int fail_focus_ = 0;
  bool leave_after_play_ = false;
};

}  // namespace

bool &player_wants_another_source() { return SourcesScreen::choose_another(); }

std::unique_ptr<Screen> make_sources(const PlayRequest &request, bool autostart) {
  return std::make_unique<SourcesScreen>(request, autostart);
}
