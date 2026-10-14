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
constexpr int LX = safe_x, LW = 600;           // title column
constexpr int RX = 760, RW = W - safe_x - 760;  // options column
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
  return {"Availability unknown", label(Label::Tertiary)};
}

class SourcesScreen : public Screen {
 public:
  SourcesScreen(const PlayRequest &request, bool autostart) : req_(request), autostart_(autostart) {
    backdrop_.set(request.item.backdrop, true);
    load();
    refresh_account();
  }

  const char *name() const override { return "sources"; }
  void shell_glass(const GlassSpec &g) const override { backdrop_.glass(g); }

  std::string spoken() const override {
    if (pairing_.active()) return pairing_.spoken();
    switch (modal_) {
      case Modal::Resolving: return "Starting " + chosen_.headline + ". " + step_label_ + " Cancel, button";
      case Modal::ChooseFile:
      case Modal::ChooseQuality: {
        if (choices_.empty()) return "";
        const Choice &c = choices_[static_cast<size_t>(choice_)];
        return std::string(modal_ == Modal::ChooseFile ? "Which file? " : "Which quality? ") + c.label + ", " + c.detail + ", " +
               position(choice_, static_cast<int>(choices_.size()));
      }
      case Modal::Failed:
        return "That version didn't work. " + failure_ + ". " + (fail_focus_ == 0 ? "Try the next best" : "Try again") + ", button";
      case Modal::None:
        break;
    }
    if (state_ == State::Loading) return "Finding versions";
    if (state_ != State::Ready) return state_ == State::Empty ? "Nothing to play yet" : error_;
    const SourceOption &o = options_[static_cast<size_t>(focus_index_)];
    std::string head = focus_index_ == 0 && has_recommended() ? "Best version, " : "";
    return head + o.headline + ", " + picture_words(o) + ", " + o.audio + ", " + position(focus_index_, static_cast<int>(options_.size()));
  }

  // A glass sheet over this screen's artwork (solid while the frost bakes).
  void sheet(int x, int y, int w, int h, float a) const {
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.alpha = a;
    backdrop_.glass(g);
  }

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
      direct_ = false;
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
        autostart_ = false;
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
      if (autostart_) {
        autostart_ = false;
        // The version picked on the title page, else the recommended one.
        int pick = ranking_.recommended ? 0 : -1;
        for (size_t i = 0; i < options_.size(); i++)
          if (!req_.candidate.empty() && options_[i].id == req_.candidate) pick = static_cast<int>(i);
        if (pick >= 0) {
          // Straight from the title page: only the progress shows, and
          // cancelling goes back there.
          direct_ = true;
          focus_index_ = pick;
          choose(pick);
        }
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
    if (direct_) ctx().pop();  // back to the title page it was started from
  }

  // Started from the title page and still on the way: only the progress shows.
  bool quiet() const { return (autostart_ && state_ == State::Loading) || (direct_ && (modal_ == Modal::Resolving || pairing_.active())); }

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
        if (key == Key::Back) {
          modal_ = Modal::None;
          direct_ = false;  // a failure shows the other versions
        }
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
    // Pairing given up on the way from the title page: go back there.
    if (direct_ && modal_ == Modal::None && !pairing_.active() && !leave_after_play_) {
      direct_ = false;
      ctx().pop();
      return;
    }
    modal_anim_.to(modal_ != Modal::None || pairing_.active() ? 1.0f : 0.0f);
    moving |= modal_anim_.step(dt);
    if (moving) c.animating = true;
  }

  void draw() override {
    backdrop_.draw(0.4f, 1.0f);
    float sc = scrim_for(backdrop_.luma());
    draw_scrims(std::max(0.6f, sc), 600, 0.8f);
    if (quiet()) {
      if (modal_ == Modal::None && !pairing_.active()) draw_spinner(W / 2, H / 2, 56, label(Label::Secondary), loading_since_);
      draw_modal();
      pairing_.draw();
      return;
    }
    draw_title_column();
    switch (state_) {
      case State::Loading:
        draw_spinner(RX + RW / 2, 420, 56, label(Label::Secondary), loading_since_);
        if (ctx().now - loading_since_ > spinner_delay_ms) text_center(Text::Callout, "Finding versions\xE2\x80\xA6", RX + RW / 2, 480, label(Label::Secondary));
        break;
      case State::Error:
      case State::Empty: {
        text(Text::Title3, state_ == State::Empty ? "Nothing to play yet" : "Couldn\xE2\x80\x99t find versions", RX, 140, label(Label::Primary));
        paragraph(Text::Body, state_ == State::Empty ? "No versions of this are available right now. Try again later." : error_, RX, 210,
                  label(Label::Secondary), RW, 3);
        GlassButton b;
        b.label = "Try again";
        b.focused = true;
        b.lift = lift_.value;
        draw_glass_button(RX, 340, b);
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
    text(Text::Headline, "All versions", LX, y, label(Label::Primary));
    y += line_for(Text::Headline) + 8;
    if (req_.item.series()) {
      text(Text::Callout, episode_code(req_.season, req_.episode), LX, y, label(Label::Secondary));
      y += line_for(Text::Callout) + 4;
      if (!req_.episode_title.empty()) y = paragraph(Text::Callout, req_.episode_title, LX, y, label(Label::Secondary), LW, 2);
    } else {
      text(Text::Callout, meta_line(req_.item), LX, y, label(Label::Secondary), LW);
      y += line_for(Text::Callout);
    }
    if (req_.resume_ms > 0) text(Text::Callout, "Continues from " + clock_text(req_.resume_ms), LX, y + 16, label(Label::Primary));
    // AllDebrid connection, quietly.
    int ay = H - safe_y - 52;
    bool paired = c.paired;
    c.gfx.fill_circle(LX + 8, ay + 16, 6, !account_known_ ? label(Label::Tertiary) : paired ? good : caution);
    std::string status = !account_known_ ? "Checking AllDebrid\xE2\x80\xA6"
                         : paired ? (durable_ ? "AllDebrid connected" : "AllDebrid connected until restart")
                                  : "AllDebrid not connected. You\xE2\x80\x99ll pair once when you press Play.";
    text(Text::Caption1, status, LX + 28, ay, label(Label::Secondary), LW);
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
      text(Text::Caption1, has_recommended() ? "Other versions" : "Versions", RX, LIST_TOP - 44, label(Label::Secondary));
      float s = scroll_.value;
      for (int i = 0; i < others; i++) {
        float ry = LIST_TOP + (i - s) * (ROW + ROW_GAP);
        if (ry < LIST_TOP - ROW / 2 || ry > LIST_TOP + VISIBLE * (ROW + ROW_GAP) - ROW / 2) continue;
        int k = i + index;
        draw_version(options_[static_cast<size_t>(k)], static_cast<int>(ry), k == focus_index_);
      }
      if (s + VISIBLE < others) c.icons.draw(Icon::ChevronDown, RX + RW / 2 - 18, LIST_TOP + VISIBLE * (ROW + ROW_GAP) - 4, 36, label(Label::Tertiary));
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
    // Frosted at rest, white when focused (the frost stays put; the white lifts).
    sheet(x, y, w, h, 1.0f);
    if (focused) {
      float k = 1.0f + 0.02f * lift;
      int fw = static_cast<int>(w * k), fh = static_cast<int>(h * k);
      int fx = x - (fw - w) / 2, fy = y - (fh - h) / 2;
      c.gfx.shadow(fx, fy, fw, fh, glass_radius, 30, 14, static_cast<Uint8>(150 * lift));
      c.gfx.fill_round(fx, fy, fw, fh, glass_radius, rgba(0xffffff));
    }
    Color fg = focused ? rgba(0x000000) : label(Label::Primary);
    Color fg2 = focused ? rgba(0x000000, 166) : label(Label::Secondary);
    text(Text::Caption1, "Best version", x + 40, y + 36, fg2);
    text(Text::Title2, o.headline, x + 40, y + 72, fg, w - 260);
    text(Text::Body, picture_words(o) + " \xC2\xB7 " + o.audio, x + 40, y + 146, fg2, w - 260);
    paragraph(Text::Caption1, ranking_.why, x + 40, y + 190, fg2, w - 260, 2);
    // Play affordance.
    int r = 52, cx = x + w - 40 - r, cy = y + h / 2;
    c.gfx.fill_circle(cx, cy, r, focused ? rgba(0x000000) : candidate);
    c.gfx.triangle_right(cx - 16, cy - 20, 40, focused ? rgba(0xffffff) : label(Label::Primary));
    if (!o.cautions.empty()) {
      c.icons.draw(Icon::Warning, x + 40, y + h - 46, 26, focused ? caution_ink : caution);
      text(Text::Caption2, o.warnings[0], x + 76, y + h - 46, focused ? caution_ink : caution, w - 360);
    }
  }

  void draw_version(const SourceOption &o, int y, bool focused) {
    Ctx &c = ctx();
    int x = RX, w = RW;
    if (focused) {
      c.gfx.shadow(x, y, w, ROW, 20, 18, 8, 120);
      c.gfx.fill_round(x, y, w, ROW, 20, rgba(0xffffff));
    } else {
      c.gfx.fill_round(x, y, w, ROW, 20, rgba(0xffffff, 26));
    }
    Color fg = focused ? rgba(0x000000) : label(o.excluded ? Label::Disabled : Label::Primary);
    Color fg2 = focused ? rgba(0x000000, 166) : label(o.excluded ? Label::Disabled : Label::Secondary);
    int ty = y + (ROW - line_for(Text::Headline)) / 2;
    text(Text::Headline, o.resolution, x + 28, ty, fg);
    text(Text::Callout, picture_words(o) + " \xC2\xB7 " + o.audio, x + 170, y + (ROW - line_for(Text::Callout)) / 2, fg2, w - 170 - 330);
    Availability a = availability(o);
    int aw = measure(Text::Caption1, a.words);
    int ax = x + w - 28 - aw;
    text(Text::Caption1, a.words, ax, y + (ROW - line_for(Text::Caption1)) / 2, focused && a.dot.r == caution.r ? caution_ink : fg2, 300);
    c.gfx.fill_circle(ax - 18, y + ROW / 2, 6, a.dot);
  }

  // Release details for the focused version: everything technical, on request.
  void draw_details() {
    float a = details_anim_.value;
    if (a <= 0.01f || state_ != State::Ready || options_.empty()) return;
    Ctx &c = ctx();
    const SourceOption &o = options_[static_cast<size_t>(focus_index_)];
    int w = 640, x = W - safe_x - w + static_cast<int>((1.0f - a) * 60), y = 96, h = H - 192;
    // It slides, so it is solid glass (a moving frost would re-bake every frame).
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.kind = GlassKind::Solid;
    g.alpha = a;
    // Opaque underneath: it lies over text, which must not ghost through.
    c.gfx.fill_round(x, y, w, h, g.radius, fade(rgba(0x2c2c2e), a));
    draw_glass(g);
    int px = x + 44, pw = w - 88, py = y + 44;
    text(Text::Caption1, "Release details", px, py, label(Label::Secondary, a));
    py += 38;
    text(Text::Title3, o.headline, px, py, label(Label::Primary, a), pw);
    py += 70;
    auto row = [&](const std::string &name, const std::string &value, int lines = 1) {
      if (value.empty()) return;
      text(Text::Caption2, name, px, py, label(Label::Secondary, a));
      py += 30;
      py = paragraph(Text::Callout, value, px, py, label(Label::Primary, a), pw, lines) + 18;
    };
    row("Picture", picture_words(o));
    row("Sound", o.audio);
    row("File", join({o.size, o.availability}));
    row("Details", o.facts, 2);
    row("Release name", o.release, 3);
    for (auto &warning : o.warnings) {
      c.icons.draw(Icon::Warning, px, py, 26, fade(caution, a));
      py = paragraph(Text::Caption1, warning, px + 38, py, fade(caution, a), pw - 38, 2) + 8;
    }
    paragraph(Text::Caption2, "These details come from the release name, not from the file itself.", px, y + h - 92, label(Label::Tertiary, a), pw, 2);
  }

  void draw_modal() {
    float a = modal_anim_.value;
    if (modal_ == Modal::None || a <= 0.01f) return;
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(black, static_cast<int>(200 * a)));
    int w = 920, x = (W - w) / 2;
    if (modal_ == Modal::Resolving) {
      struct StepRow { const char *id; const char *label; };
      std::vector<StepRow> steps = {{"adding", "Sending to AllDebrid"}};
      if (saw_caching_) steps.push_back({"caching", "AllDebrid is fetching it"});
      steps.push_back({"files", "Finding the right file"});
      steps.push_back({"unlocking", "Getting the stream"});
      int h = 330 + static_cast<int>(steps.size()) * 58 + (saw_caching_ ? 34 : 0), y = (H - h) / 2;
      sheet(x, y, w, h, a);
      text(Text::Caption1, "Starting", x + 56, y + 48, label(Label::Secondary, a));
      text(Text::Title3, chosen_.headline, x + 56, y + 84, label(Label::Primary, a), w - 112);
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
        else if (current) draw_spinner(x + 71, sy + 15, 30, label(Label::Primary, a));
        else c.gfx.fill_circle(x + 71, sy + 15, 5, label(Label::Tertiary, a));
        text(Text::Body, st.label, x + 108, sy - 2, label(done ? Label::Secondary : current ? Label::Primary : Label::Tertiary, a));
        if (id == "caching" && fraction_ >= 0 && current) {
          draw_progress(x + 108, sy + 44, 520, 8, fraction_);
          text(Text::Callout, std::to_string(static_cast<int>(fraction_ * 100)) + "%", x + 648, sy + 32, label(Label::Secondary, a));
        }
        sy += id == "caching" ? 92 : 58;
      }
      GlassButton cancel;
      cancel.label = "Cancel";
      cancel.focused = true;
      cancel.h = 64;
      cancel.alpha = a;
      draw_glass_button(x + 56, y + h - 100, cancel);
      return;
    }
    if (modal_ == Modal::ChooseFile || modal_ == Modal::ChooseQuality) {
      int n = static_cast<int>(choices_.size());
      int shown = std::min(n, 7);
      int h = 210 + shown * 76, y = (H - h) / 2;
      sheet(x, y, w, h, a);
      text(Text::Title3, modal_ == Modal::ChooseFile ? "Which file?" : "Which quality?", x + 56, y + 46, label(Label::Primary, a));
      text(Text::Callout, modal_ == Modal::ChooseFile ? "This release holds several videos." : "AllDebrid offers these versions.", x + 56, y + 104,
           label(Label::Secondary, a));
      int first = std::max(0, std::min(choice_ - 3, n - shown));
      int ry = y + 160;
      for (int i = first; i < first + shown; i++) {
        bool f = i == choice_;
        if (f) {
          c.gfx.shadow(x + 32, ry, w - 64, 66, 20, 18, 8, 120);
          c.gfx.fill_round(x + 32, ry, w - 64, 66, 20, rgba(0xffffff));
        }
        text(Text::Callout, choices_[static_cast<size_t>(i)].label, x + 56, ry + 14, f ? rgba(0x000000) : label(Label::Primary, a), w - 320);
        text_right(Text::Callout, choices_[static_cast<size_t>(i)].detail, x + w - 56, ry + 14, f ? rgba(0x000000, 166) : label(Label::Secondary, a));
        ry += 76;
      }
      return;
    }
    if (modal_ == Modal::Failed) {
      int h = 420, y = (H - h) / 2;
      sheet(x, y, w, h, a);
      c.icons.draw(Icon::Warning, x + 56, y + 56, 40, fade(caution, a));
      text(Text::Title3, "That version didn\xE2\x80\x99t work", x + 112, y + 50, label(Label::Primary, a));
      paragraph(Text::Body, failure_, x + 56, y + 130, label(Label::Secondary, a), w - 112, 3);
      int bx = x + 56;
      bool has_next = focus_index_ + 1 < static_cast<int>(options_.size());
      GlassButton next;
      next.label = has_next ? "Try the next best" : "Back to versions";
      next.focused = fail_focus_ == 0;
      next.lift = lift_.value;
      next.alpha = a;
      bx += draw_glass_button(bx, y + h - 112, next) + 20;
      GlassButton again;
      again.label = "Try again";
      again.focused = fail_focus_ == 1;
      again.lift = lift_.value;
      again.alpha = a;
      draw_glass_button(bx, y + h - 112, again);
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
  Anim lift_{focus_in_rate, 1}, prev_lift_{focus_leave_rate}, scroll_{scroll_rate}, details_anim_{fade_rate}, modal_anim_{fade_rate};
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
  bool direct_ = false;
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
