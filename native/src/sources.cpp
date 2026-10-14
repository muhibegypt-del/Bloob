// Choose how to watch. One best match, stated in plain words (what you will
// see and hear, and why it was picked), then other versions as short lines.
// Release names, sizes and seeders stay out of the way until the viewer asks
// (Right on any option). After OK: pairing if needed, then a short, cancellable
// progress panel, and any question (which file, which quality) as a list.
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "common.h"
#include "log.h"
#include "pairing.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr long long kPollMs = 700;
constexpr int LX = margin, LW = 600;           // title column
constexpr int RX = 760, RW = W - margin - 760;  // options column
constexpr int ROW = 84, ROW_GAP = 10;
constexpr int LIST_TOP = 480;
constexpr int VISIBLE = 5;

// Plain words for a version's picture and how easy it is to get.
std::string picture_words(const SourceOption &o) {
  if (o.picture == "SDR" || o.picture.empty()) return "Standard picture";
  return o.picture;
}

struct Availability {
  std::string words;
  Color dot;
};

Availability availability(const SourceOption &o) {
  if (!o.cautions.empty()) return {o.cautions[0], caution};
  if (o.availability == "Direct link") return {"Direct link", good};
  int seeders = std::atoi(o.availability.c_str());
  if (seeders >= 20) return {"Readily available", good};
  if (seeders >= 5) return {"Available", good};
  if (o.availability.find("seeder") != std::string::npos) return {"Slow to prepare", caution};
  return {"Availability unknown", tertiary};
}

class SourcesScreen : public Screen {
 public:
  SourcesScreen(const PlayRequest &request, bool autostart) : req_(request), autostart_(autostart) {
    backdrop_.set(request.item.backdrop, true);
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

  // Set by the player when the viewer asks for another source.
  static bool &choose_another() {
    static bool flag = false;
    return flag;
  }

  // ---------------------------------------------------------------- data

  void load() {
    state_ = State::Loading;
    loading_since_ = ctx().now;
    std::string body = "{\"type\":" + Json::quote(req_.item.type) + ",\"videoId\":" + Json::quote(req_.video_id) +
                       ",\"language\":" + Json::quote(ctx().language);
    // The service judges bitrate from size over running time.
    if (int minutes = runtime_minutes(req_.item.runtime)) body += ",\"runtimeMinutes\":" + std::to_string(minutes);
    body += "}";
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
      lift_.snap(0);
      lift_.to(1);
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
    modal_since_ = ctx().now;
    details_ = false;
    job_id_.clear();
    step_ = "starting";
    step_label_ = "Starting\xE2\x80\xA6";
    fraction_ = -1;
    saw_caching_ = false;
    // Up next already started this one during the credits: carry on with that job.
    Ctx::Prepared &prep = ctx().prepared_next;
    if (!prep.job_id.empty() && prep.video_id == req_.video_id && chosen_.headline == prep.headline &&
        ctx().now - prep.at < 10 * 60 * 1000) {
      job_id_ = prep.job_id;
      prep = Ctx::Prepared{};
      logf("sources", "continuing the job prepared during the credits");
      poll();
      return;
    }
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
          else if (focus_index_ + 1 < static_cast<int>(options_.size())) {
            move_to(focus_index_ + 1);  // the next best version, straight away
            choose(focus_index_);
          }
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
      case Key::Up:
        if (focus_index_ > 0) move_to(focus_index_ - 1);
        return true;
      case Key::Down:
        if (focus_index_ + 1 < n) move_to(focus_index_ + 1);
        return true;
      case Key::Right:
        details_ = true;  // release details, on request
        return true;
      case Key::Left:
        details_ = false;
        return true;
      case Key::Ok:
      case Key::Play:
      case Key::PlayPause:
        if (!repeat) choose(focus_index_);
        return true;
      case Key::Back:
        if (details_) details_ = false;
        else ctx().pop();
        return true;
      default:
        return true;
    }
  }

  void move_to(int index) {
    prev_index_ = focus_index_;
    prev_lift_.snap(lift_.value);
    prev_lift_.to(0);
    focus_index_ = index;
    lift_.snap(0);
    lift_.to(1);
    int list_index = focus_index_ - (has_recommended() ? 1 : 0);
    float target = scroll_.target;
    if (list_index >= 0) {
      if (list_index < target) target = static_cast<float>(list_index);
      if (list_index >= target + VISIBLE) target = static_cast<float>(list_index - VISIBLE + 1);
    } else {
      target = 0;
    }
    scroll_.to(target);
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    pairing_.update();
    backdrop_.update(dt);
    if (modal_ == Modal::Resolving && !job_id_.empty() && c.now >= next_poll_) poll();
    if (modal_ == Modal::Resolving) c.redraw_at(next_poll_);
    bool moving = lift_.step(dt) | prev_lift_.step(dt) | scroll_.step(dt);
    details_anim_.to(details_ ? 1.0f : 0.0f);
    moving |= details_anim_.step(dt);
    modal_anim_.to(modal_ != Modal::None || pairing_.active() ? 1.0f : 0.0f);
    moving |= modal_anim_.step(dt);
    if (moving) c.animating = true;
  }

  void draw() override {
    backdrop_.draw(0.55f, 1.0f);
    draw_scrims(0.6f, 600, 0.8f);
    draw_title_column();
    switch (state_) {
      case State::Loading:
        draw_spinner(RX + RW / 2, 420, 56, secondary, loading_since_);
        if (ctx().now - loading_since_ > spinner_delay_ms) text_center(Text::Callout, "Finding versions\xE2\x80\xA6", RX + RW / 2, 480, secondary);
        break;
      case State::Error:
      case State::Empty: {
        text(Text::Title2, state_ == State::Empty ? "Nothing to play yet" : "Couldn\xE2\x80\x99t find versions", RX, 140, primary);
        paragraph(Text::Body, state_ == State::Empty ? "No versions of this are available right now. Try again later." : error_, RX, 200,
                  secondary, RW, 3);
        ButtonSpec b{"Try again", -1, true};
        draw_button(RX, 340, b);
        break;
      }
      case State::Ready:
        draw_options();
        break;
    }
    draw_details();
    draw_modal();
    pairing_.draw();
  }

  void draw_title_column() {
    Ctx &c = ctx();
    int y = draw_title(req_.item, LX, 120, LW, 150);
    y += 34;
    text(Text::Eyebrow, upper("Choose how to watch"), LX, y, accent);
    y += 36;
    if (req_.item.series()) {
      text(Text::Title2, episode_code(req_.season, req_.episode), LX, y, primary);
      y += line_for(Text::Title2) + 4;
      if (!req_.episode_title.empty()) y = paragraph(Text::Callout, req_.episode_title, LX, y, secondary, LW, 2);
    } else {
      text(Text::Callout, meta_line(req_.item), LX, y, secondary, LW);
      y += line_for(Text::Callout);
    }
    if (req_.resume_ms > 0) text(Text::Callout, "Continues from " + clock_text(req_.resume_ms), LX, y + 16, accent);
    // AllDebrid connection, quietly.
    int ay = H - 112;
    bool paired = c.paired;
    c.gfx.fill_circle(LX + 8, ay + 14, 6, !account_known_ ? tertiary : paired ? good : caution);
    std::string status = !account_known_ ? "Checking AllDebrid\xE2\x80\xA6"
                         : paired ? (durable_ ? "AllDebrid connected" : "AllDebrid connected until restart")
                                  : "AllDebrid not connected. You\xE2\x80\x99ll pair once when you press Play.";
    text(Text::Caption, status, LX + 28, ay, secondary, LW);
  }

  void draw_options() {
    Ctx &c = ctx();
    int index = 0;
    if (has_recommended()) {
      draw_best(*ranking_.recommended, focus_index_ == 0);
      index = 1;
    }
    int others = static_cast<int>(options_.size()) - index;
    if (others > 0) {
      text(Text::Eyebrow, upper(has_recommended() ? "Other versions" : "Versions"), RX, LIST_TOP - 40, tertiary);
      float s = scroll_.value;
      for (int i = 0; i < others; i++) {
        float ry = LIST_TOP + (i - s) * (ROW + ROW_GAP);
        if (ry < LIST_TOP - ROW / 2 || ry > LIST_TOP + VISIBLE * (ROW + ROW_GAP) - ROW / 2) continue;
        int k = i + index;
        draw_version(options_[static_cast<size_t>(k)], static_cast<int>(ry), k == focus_index_);
      }
      if (s + VISIBLE < others) c.icons.draw(Icon::ChevronDown, RX + RW / 2 - 18, LIST_TOP + VISIBLE * (ROW + ROW_GAP) - 4, 36, tertiary);
    }
    // What the keys do here.
    int hx = RX;
    hx += draw_hint(hx, H - 82, "OK", "Play") + 32;
    draw_hint(hx, H - 82, "\xE2\x86\x92", details_ ? "Hide details" : "Release details");
  }

  void draw_best(const SourceOption &o, bool focused) {
    Ctx &c = ctx();
    int x = RX, y = 120, w = RW, h = 290;
    float lift = focused ? lift_.value : 0;
    if (focused) c.gfx.shadow(x, y, w, h, panel_radius, 30, 14, static_cast<Uint8>(150 * lift));
    c.gfx.fill_round(x, y, w, h, panel_radius, focused ? focus : glass);
    Color fg = focused ? ink : primary, fg2 = focused ? ink2 : secondary;
    // "Best match" with a small amber mark.
    c.gfx.fill_circle(x + 48, y + 54, 7, accent);
    text(Text::Eyebrow, upper("Best match"), x + 66, y + 42, focused ? ink2 : accent);
    text(Text::Title1, o.headline, x + 40, y + 80, fg, w - 260);
    text(Text::Body, picture_words(o) + " \xC2\xB7 " + o.audio, x + 40, y + 150, fg2, w - 260);
    paragraph(Text::Callout, ranking_.why, x + 40, y + 196, fg2, w - 260, 2);
    // Play affordance.
    int r = 52, cx = x + w - 40 - r, cy = y + h / 2;
    c.gfx.fill_circle(cx, cy, r, focused ? ink : glass_light);
    c.gfx.triangle_right(cx - 16, cy - 20, 40, focused ? focus : primary);
    if (!o.cautions.empty()) {
      c.icons.draw(Icon::Warning, x + 40, y + h - 46, 26, focused ? caution_ink : caution);
      text(Text::Caption, o.warnings[0], x + 76, y + h - 46, focused ? caution_ink : caution, w - 360);
    }
  }

  void draw_version(const SourceOption &o, int y, bool focused) {
    Ctx &c = ctx();
    int x = RX, w = RW;
    if (focused) draw_row_focus(x, y, w, ROW);
    else c.gfx.fill_round(x, y, w, ROW, 16, with_alpha(raised, 150));
    Color fg = focused ? ink : (o.excluded ? tertiary : primary);
    Color fg2 = focused ? ink2 : (o.excluded ? tertiary : secondary);
    int ty = y + (ROW - line_for(Text::Headline)) / 2;
    text(Text::Headline, o.resolution, x + 28, ty, fg);
    text(Text::Callout, picture_words(o) + " \xC2\xB7 " + o.audio, x + 170, y + (ROW - line_for(Text::Callout)) / 2, fg2, w - 170 - 330);
    Availability a = availability(o);
    int aw = measure(Text::Caption, a.words);
    int ax = x + w - 28 - aw;
    text(Text::Caption, a.words, ax, y + (ROW - line_for(Text::Caption)) / 2, focused && a.dot.r == caution.r ? caution_ink : fg2, 300);
    c.gfx.fill_circle(ax - 18, y + ROW / 2, 6, a.dot);
  }

  // Release details for the focused version: everything technical, on request.
  void draw_details() {
    float a = details_anim_.value;
    if (a <= 0.01f || state_ != State::Ready || options_.empty()) return;
    Ctx &c = ctx();
    const SourceOption &o = options_[static_cast<size_t>(focus_index_)];
    int w = 640, x = W - margin - w + static_cast<int>((1.0f - a) * 60), y = 96, h = H - 192;
    c.gfx.shadow(x, y, w, h, panel_radius, 36, 16, static_cast<Uint8>(170 * a));
    c.gfx.fill_round(x, y, w, h, panel_radius, fade(rgba(0x1c1e23, 252), a));
    int px = x + 44, pw = w - 88, py = y + 44;
    text(Text::Eyebrow, upper("Release details"), px, py, fade(tertiary, a));
    py += 34;
    text(Text::Title2, o.headline, px, py, fade(primary, a), pw);
    py += 62;
    auto row = [&](const std::string &label, const std::string &value, int lines = 1) {
      if (value.empty()) return;
      text(Text::Footnote, upper(label), px, py, fade(tertiary, a));
      py += 28;
      py = paragraph(Text::Callout, value, px, py, fade(primary, a), pw, lines) + 18;
    };
    row("Picture", picture_words(o));
    row("Sound", o.audio);
    row("File", join({o.size, o.availability}));
    row("Details", o.facts, 2);
    row("Release name", o.release, 3);
    for (auto &warning : o.warnings) {
      c.icons.draw(Icon::Warning, px, py, 26, fade(caution, a));
      py = paragraph(Text::Caption, warning, px + 38, py, fade(caution, a), pw - 38, 2) + 8;
    }
    paragraph(Text::Caption, "These details come from the release name, not from the file itself.", px, y + h - 92, fade(tertiary, a), pw, 2);
  }

  void draw_modal() {
    float a = modal_anim_.value;
    if (modal_ == Modal::None || a <= 0.01f) return;
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(ground, static_cast<int>(200 * a)));
    int w = 920, x = (W - w) / 2;
    if (modal_ == Modal::Resolving) {
      struct StepRow { const char *id; const char *label; };
      std::vector<StepRow> steps = {{"adding", "Sending to AllDebrid"}};
      if (saw_caching_) steps.push_back({"caching", "AllDebrid is fetching it"});
      steps.push_back({"files", "Finding the right file"});
      steps.push_back({"unlocking", "Getting the stream"});
      int h = 330 + static_cast<int>(steps.size()) * 58 + (saw_caching_ ? 34 : 0), y = (H - h) / 2;
      draw_panel(x, y, w, h, a);
      text(Text::Eyebrow, upper("Starting"), x + 56, y + 52, fade(accent, a));
      text(Text::Title2, chosen_.headline, x + 56, y + 84, fade(primary, a), w - 112);
      static const char *order[] = {"starting", "adding", "caching", "files", "unlocking", "preparing", "ready"};
      auto rank = [](const std::string &id) {
        for (int i = 0; i < 7; i++) if (id == order[i]) return i;
        return 0;
      };
      int now_rank = rank(step_);
      int sy = y + 162;
      for (auto &st : steps) {
        std::string id = st.id;
        int r = rank(id);
        bool preparing = id == "unlocking" && step_ == "preparing";
        bool done = now_rank > r && !preparing;
        bool current = now_rank == r || preparing;
        if (done) c.icons.draw(Icon::Check, x + 56, sy, 30, fade(good, a));
        else if (current) draw_spinner(x + 71, sy + 15, 30, primary);
        else c.gfx.fill_circle(x + 71, sy + 15, 5, fade(tertiary, a));
        text(Text::Body, st.label, x + 108, sy - 2, fade(done ? secondary : current ? primary : tertiary, a));
        if (id == "caching" && fraction_ >= 0 && current) {
          draw_progress(x + 108, sy + 44, 520, 8, fraction_);
          text(Text::Callout, std::to_string(static_cast<int>(fraction_ * 100)) + "%", x + 648, sy + 32, fade(secondary, a));
        }
        sy += id == "caching" ? 92 : 58;
      }
      ButtonSpec cancel{"Cancel", -1, true};
      cancel.h = 64;
      cancel.alpha = a;
      draw_button(x + 56, y + h - 100, cancel);
      return;
    }
    if (modal_ == Modal::ChooseFile || modal_ == Modal::ChooseQuality) {
      int n = static_cast<int>(choices_.size());
      int shown = std::min(n, 7);
      int h = 210 + shown * 76, y = (H - h) / 2;
      draw_panel(x, y, w, h, a);
      text(Text::Title2, modal_ == Modal::ChooseFile ? "Which file?" : "Which quality?", x + 56, y + 50, fade(primary, a));
      text(Text::Callout, modal_ == Modal::ChooseFile ? "This release holds several videos." : "AllDebrid offers these versions.", x + 56, y + 104,
           fade(secondary, a));
      int first = std::max(0, std::min(choice_ - 3, n - shown));
      int ry = y + 160;
      for (int i = first; i < first + shown; i++) {
        bool f = i == choice_;
        if (f) draw_row_focus(x + 32, ry, w - 64, 66);
        text(Text::Callout, choices_[static_cast<size_t>(i)].label, x + 56, ry + 17, f ? ink : primary, w - 320);
        text_right(Text::Callout, choices_[static_cast<size_t>(i)].detail, x + w - 56, ry + 17, f ? ink2 : secondary);
        ry += 76;
      }
      return;
    }
    if (modal_ == Modal::Failed) {
      int h = 420, y = (H - h) / 2;
      draw_panel(x, y, w, h, a);
      c.icons.draw(Icon::Warning, x + 56, y + 56, 40, fade(caution, a));
      text(Text::Title2, "That version didn\xE2\x80\x99t work", x + 112, y + 54, fade(primary, a));
      paragraph(Text::Body, failure_, x + 56, y + 130, fade(secondary, a), w - 112, 3);
      int bx = x + 56;
      bool has_next = focus_index_ + 1 < static_cast<int>(options_.size());
      ButtonSpec next{has_next ? "Try the next best" : "Back to versions", -1, fail_focus_ == 0};
      next.alpha = a;
      bx += draw_button(bx, y + h - 112, next) + 20;
      ButtonSpec again{"Try again", -1, fail_focus_ == 1};
      again.alpha = a;
      draw_button(bx, y + h - 112, again);
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
  Backdrop backdrop_;
  State state_ = State::Loading;
  std::string error_;
  long long loading_since_ = 0;
  Ranking ranking_;
  std::vector<SourceOption> options_;
  int focus_index_ = 0, prev_index_ = -1;
  Anim lift_{focus_rate, 1}, prev_lift_{focus_rate}, scroll_{scroll_rate}, details_anim_{fade_rate}, modal_anim_{fade_rate};
  bool details_ = false;
  bool account_known_ = false, durable_ = false;
  Pairing pairing_;
  Modal modal_ = Modal::None;
  long long modal_since_ = 0;
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

void start_playback(const PlayRequest &req, PlayOrigin origin, bool replace_top, bool autostart) {
  if (req.video_id.empty()) return;
  static const char *names[] = {"home-hero", "home-row", "details", "episodes", "up-next"};
  logf("play", "intent from %s: %s resume=%lld", names[static_cast<int>(origin)], req.label().c_str(), req.resume_ms);
  // Gates belong here and nowhere else (pairing, TV format profile, household limits).
  auto screen = make_sources(req, autostart);
  if (replace_top) ctx().replace(std::move(screen));
  else ctx().push(std::move(screen));
}
