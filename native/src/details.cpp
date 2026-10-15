// Title page (Glass). Full-bleed artwork, title art, essential context and one
// primary action that names exactly what it plays and starts the best version
// directly. Films show "How to watch" on a frosted panel: the best version ticked
// and the alternatives one press away. For series the page continues downward:
// moving below the buttons slides the seasons and episodes up while the artwork
// softens, so episodes read as part of the title, not a new screen.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "play.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr int ESTRIDE = episode_w + card_gap;
constexpr int X0 = safe_x;
constexpr int kPanelY = 790, kPanelH = 214;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

class DetailsScreen : public Screen {
 public:
  explicit DetailsScreen(const Item &item) {
    meta_.item = item;
    backdrop_.set(item.backdrop, true);
    opened_at_ = ctx().now;
    if (const Json *cached = ctx().meta_cache.get(meta_key(item), ctx().now)) {
      apply_meta(*cached);  // complete on the first frame (fetched while focus rested on Home)
      logf("details", "ready from prefetch");
      load_progress();      // then only what changes between visits
    } else {
      load();
    }
  }

  const char *name() const override { return "details"; }
  void shell_glass(const GlassSpec &g) const override { backdrop_.glass(g); }

  std::string spoken() const override {
    if (!loaded_) return error_.empty() ? "Loading " + meta_.item.name : error_;
    switch (zone_) {
      case Zone::Buttons: {
        auto list = buttons();
        int i = std::min(button_, static_cast<int>(list.size()) - 1);
        std::string said = meta_.item.name + ". " + list[static_cast<size_t>(i)].label + ", button, " + position(i, static_cast<int>(list.size()));
        return note_.empty() ? said : said + ". " + note_;
      }
      case Zone::Versions: {
        if (versions_.empty()) return "";
        const SourceOption &o = versions_[static_cast<size_t>(version_)];
        return std::string(version_ == 0 ? "Best version, " : "") + o.headline + ", button, " + position(version_, static_cast<int>(versions_.size()));
      }
      case Zone::Seasons: {
        const Season &s = meta_.seasons[static_cast<size_t>(season_)];
        return s.label + ", " + position(season_, static_cast<int>(meta_.seasons.size()));
      }
      case Zone::Episodes: {
        const Season &s = meta_.seasons[static_cast<size_t>(season_)];
        if (s.episodes.empty()) return "";
        const Episode &e = s.episodes[static_cast<size_t>(episode_)];
        std::string number = e.season == 0 ? "Special" : "Episode " + std::to_string(e.episode);
        return number + ", " + e.title + ", " + position(episode_, static_cast<int>(s.episodes.size()));
      }
    }
    return "";
  }

  void shown() override {
    // Back from playback: refresh what was watched and what plays next.
    if (loaded_) load_progress();
  }

  void apply_meta(const Json &j) {
    Item original = meta_.item;
    meta_ = parse_meta(j["meta"]);
    if (meta_.item.description.empty()) meta_.item.description = original.description;
    read_state(j);
    loaded_ = true;
    choose_start(true);
  }

  // What changes between visits: progress, what Play does, the show's version.
  void read_state(const Json &j) {
    progress_ = parse_progress(j["progress"]);
    target_ = parse_target(j["target"]);
    intent_ = parse_intent(j["intent"]);
    dismissed_ = j["dismissed"].as_bool();
  }

  void load() {
    ctx().api.get("/api/meta?type=" + meta_.item.type + "&id=" + meta_.item.id, [this](const ApiResult &r) {
      if (!r.ok()) {
        error_ = r.error;
        return;
      }
      ctx().meta_cache.put(meta_key(meta_.item), r.json, ctx().now);
      apply_meta(r.json);
    }, alive_);
  }

  void load_progress() {
    ctx().api.get("/api/meta?type=" + meta_.item.type + "&id=" + meta_.item.id, [this](const ApiResult &r) {
      if (!r.ok()) return;
      read_state(r.json);
      choose_start(zone_ == Zone::Buttons);
    }, alive_);
  }

  // How to watch: the ranked versions of what the main button plays, fetched as
  // the page opens so Play can start the best one directly.
  void load_versions() {
    if (start_.video_id.empty() || start_.video_id == versions_for_) return;
    versions_for_ = start_.video_id;
    versions_state_ = Versions::Loading;
    std::string body = "{\"type\":" + Json::quote(meta_.item.type) + ",\"videoId\":" + Json::quote(start_.video_id) +
                       ",\"language\":" + Json::quote(ctx().language);
    if (int minutes = runtime_minutes(meta_.item.runtime)) body += ",\"runtimeMinutes\":" + std::to_string(minutes);
    body += "}";
    std::string asked = start_.video_id;
    ctx().api.post("/api/sources/ranked", body, [this, asked](const ApiResult &r) {
      if (asked != versions_for_) return;
      if (!r.ok()) {
        versions_state_ = Versions::Failed;
        versions_error_ = r.error;
        return;
      }
      Ranking ranking = parse_ranking(r.json);
      versions_why_ = ranking.why;
      versions_.clear();
      if (ranking.recommended) versions_.push_back(*ranking.recommended);
      for (auto &g : ranking.groups)
        for (auto &o : g.items)
          if (versions_.size() < 3 && !o.excluded && (versions_.empty() || o.id != versions_[0].id)) versions_.push_back(o);
      versions_state_ = versions_.empty() ? Versions::Empty : Versions::Ready;
      version_ = std::min(version_, std::max(0, static_cast<int>(versions_.size())));
    }, alive_);
  }

  // What the main button plays (decided by the service's target, UX-PLAN X2),
  // its label, and where the episode row starts.
  void choose_start(bool move_focus) {
    std::string today = today_iso();
    start_ = play::title_default(meta_, progress_, target_, today);
    load_versions();
    note_.clear();
    if (target_.action == "caughtUp") {
      note_ = target_.upcoming ? "You\xE2\x80\x99re up to date. " + episode_code(target_.upcoming->season, target_.upcoming->episode) +
                                     " arrives " + air_day_text(target_.upcoming->released, today) + "."
                               : "You\xE2\x80\x99ve watched every episode.";
    }
    has_resume_ = start_.resume_ms > 0;
    resume_note_.clear();
    if (has_resume_) {
      auto it = progress_.find(start_.video_id);
      if (it != progress_.end()) resume_note_ = minutes_left(it->second.duration - it->second.position);
    }
    if (!meta_.item.series()) {
      start_label_ = has_resume_ ? "Resume" : target_.action == "again" ? "Play again" : "Play";
      return;
    }
    if (start_.video_id.empty()) {
      start_label_ = "Play";
      return;
    }
    start_label_ = (has_resume_ ? "Resume " : "Play ") + episode_code(start_.season, start_.episode);
    if (!move_focus) return;
    for (size_t i = 0; i < meta_.seasons.size(); i++)
      for (size_t k = 0; k < meta_.seasons[i].episodes.size(); k++)
        if (meta_.seasons[i].episodes[k].id == start_.video_id) {
          season_ = static_cast<int>(i);
          episode_ = static_cast<int>(k);
          escroll_.snap(static_cast<float>(scroll_for(episode_)));
          keep_season_visible(true);
        }
  }

  const Episode *find_episode(const std::string &id) const {
    for (auto &s : meta_.seasons)
      for (auto &e : s.episodes)
        if (e.id == id) return &e;
    return nullptr;
  }

  int episode_count() const {
    return meta_.seasons.empty() ? 0 : static_cast<int>(meta_.seasons[static_cast<size_t>(season_)].episodes.size());
  }

  int scroll_for(int episode) const { return std::max(0, std::min(episode - 1, episode_count() - 4)); }

  // ---------------------------------------------------------------- input

  enum class Zone { Buttons, Versions, Seasons, Episodes };
  enum class Versions { Loading, Ready, Empty, Failed };

  bool has_episodes() const { return meta_.item.series() && !meta_.seasons.empty(); }

  std::vector<ButtonSpec> buttons() const {
    std::vector<ButtonSpec> b;
    b.push_back({start_label_, kPlayIcon});
    if (has_resume_) b.push_back({"Start over", static_cast<int>(Icon::Restart)});
    b.push_back({"All versions", -1});
    if (in_continue()) b.push_back({"Remove from Continue", -1});
    return b;
  }

  // On Home's Continue row: something watched, not removed, and not a finished film.
  bool in_continue() const {
    if (dismissed_ || progress_.empty()) return false;
    if (!meta_.item.series()) return target_.action == "resume" || !target_.valid();
    return target_.action != "caughtUp" || target_.upcoming.has_value();
  }

  bool has_versions_panel() const { return !meta_.item.series(); }
  int version_tiles() const { return versions_state_ == Versions::Ready ? static_cast<int>(versions_.size()) : 0; }

  void focus_moved() {
    // Each kind of control keeps its own focus timing: buttons 75 / 150 ms,
    // season pills 100 / 200, cards the shared 85 / 230 [EoA B6, P6, M1].
    lift_.motion = zone_ == Zone::Buttons ? button_in_rate : zone_ == Zone::Seasons ? pill_in_rate : focus_in_rate;
    prev_lift_.motion = zone_ == Zone::Buttons ? button_leave_rate : zone_ == Zone::Seasons ? pill_out_rate : focus_leave_rate;
    lift_.snap(0);
    lift_.to(1);
  }

  void set_zone(Zone z) {
    zone_ = z;
    page_.to(z == Zone::Buttons ? 0.0f : 1.0f);
    focus_moved();
  }

  bool key(Key key, bool repeat) override {
    switch (zone_) {
      case Zone::Buttons: {
        int count = static_cast<int>(buttons().size());
        if (key == Key::Left) button_ = std::max(0, button_ - 1);
        else if (key == Key::Right) button_ = std::min(count - 1, button_ + 1);
        else if (key == Key::Down && has_episodes()) set_zone(meta_.seasons.size() > 1 ? Zone::Seasons : Zone::Episodes);
        else if (key == Key::Down && has_versions_panel() && version_tiles() > 0) {
          zone_ = Zone::Versions;
          version_ = 0;
        }
        else if (key == Key::Ok && !repeat) press_button(button_);
        else if (key == Key::Back) ctx().pop();
        else return true;
        focus_moved();
        return true;
      }
      case Zone::Versions: {
        int count = version_tiles();
        if (key == Key::Left) version_ = std::max(0, version_ - 1);
        else if (key == Key::Right) version_ = std::min(count - 1, version_ + 1);
        else if (key == Key::Up || key == Key::Back) zone_ = Zone::Buttons;
        else if (key == Key::Ok && !repeat && count > 0) {
          PlayRequest req = start_;
          req.candidate = versions_[static_cast<size_t>(version_)].id;
          start_playback(req, PlayOrigin::Details, false, true);
        }
        focus_moved();
        return true;
      }
      case Zone::Seasons: {
        int count = static_cast<int>(meta_.seasons.size());
        if (key == Key::Left && season_ > 0) select_season(season_ - 1);
        else if (key == Key::Right && season_ + 1 < count) select_season(season_ + 1);
        else if (key == Key::Up || key == Key::Back) set_zone(Zone::Buttons);
        else if (key == Key::Down || key == Key::Ok) set_zone(Zone::Episodes);
        focus_moved();
        return true;
      }
      case Zone::Episodes: {
        int count = episode_count();
        int before = episode_;
        if (key == Key::Left) {
          if (episode_ > 0) episode_--;
          else if (season_ > 0 && meta_.seasons[static_cast<size_t>(season_) - 1].season != 0) {
            select_season(season_ - 1);
            episode_ = episode_count() - 1;
          }
        } else if (key == Key::Right) {
          if (episode_ + 1 < count) episode_++;
          else if (season_ + 1 < static_cast<int>(meta_.seasons.size()) && meta_.seasons[static_cast<size_t>(season_) + 1].season != 0) {
            select_season(season_ + 1);
          }
        } else if (key == Key::Up) {
          set_zone(meta_.seasons.size() > 1 ? Zone::Seasons : Zone::Buttons);
          return true;
        } else if (key == Key::Back) {
          set_zone(Zone::Buttons);
          return true;
        } else if (key == Key::Ok && !repeat) {
          play_episode(meta_.seasons[static_cast<size_t>(season_)].episodes[static_cast<size_t>(episode_)]);
          return true;
        }
        if (episode_ != before) {
          prev_episode_ = before;
          prev_lift_.snap(lift_.value);
          prev_lift_.to(0);
        }
        escroll_.to(static_cast<float>(scroll_for(episode_)));
        focus_moved();
        return true;
      }
    }
    return true;
  }

  void select_season(int index) {
    season_ = index;
    episode_ = 0;
    prev_episode_ = -1;
    escroll_.snap(0);
    keep_season_visible(false);
  }

  // Left edge of each season pill on the unscrolled strip, plus the strip's end.
  std::vector<int> season_xs() const {
    std::vector<int> xs;
    int x = 0;
    for (auto &s : meta_.seasons) {
      xs.push_back(x);
      GlassButton b;
      b.label = s.label;
      b.pill = true;
      x += glass_button_width(b) + theme::control_gap;
    }
    xs.push_back(x);
    return xs;
  }

  // Scroll the season strip so the selected pill is inside the safe area, with a
  // glimpse of the next one. Every season is reachable however many there are.
  void keep_season_visible(bool snap) {
    if (meta_.seasons.size() < 2) return;
    auto xs = season_xs();
    int view = W - 2 * X0;
    size_t i = static_cast<size_t>(season_);
    int look = i + 1 < meta_.seasons.size() ? (xs[i + 2] - xs[i + 1]) / 2 : 0;
    float target = sscroll_.target;
    if (xs[i] - target < 0) target = static_cast<float>(xs[i]);
    if (xs[i + 1] - 12 + look - target > view) target = static_cast<float>(xs[i + 1] - 12 + look - view);
    target = std::max(0.0f, std::min(target, static_cast<float>(std::max(0, xs.back() - 12 - view))));
    if (snap || ctx().reduce_motion) sscroll_.snap(target);
    else sscroll_.to(target);
  }

  void press_button(int index) {
    if (start_.video_id.empty()) return;
    auto list = buttons();
    const std::string &what = list[static_cast<size_t>(index)].label;
    if (what == "Remove from Continue") {
      dismissed_ = true;
      button_ = 0;
      ctx().api.post("/api/progress/dismiss", "{\"id\":" + Json::quote(meta_.item.id) + "}");
      ctx().toast("Removed from Continue. It returns when you play it.");
      return;
    }
    PlayRequest req = start_;
    if (what == "All versions") {
      start_playback(req, PlayOrigin::Details);  // the full chooser
      return;
    }
    if (what == "Start over") req.resume_ms = 0;
    start_playback(req, PlayOrigin::Details, false, true);  // straight to the best version
  }

  void play_episode(const Episode &e) { start_playback(play::episode(meta_, e, progress_, false), PlayOrigin::Episodes, false, true); }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    backdrop_.update(dt);
    if (page_.step(dt) | escroll_.step(dt) | sscroll_.step(dt) | lift_.step(dt) | prev_lift_.step(dt)) ctx().animating = true;
  }

  void draw() override {
    float p = page_.value;  // 0: title page, 1: episodes
    backdrop_.draw(0.3f * p, p);
    float sc = scrim_for(backdrop_.luma());
    draw_hero_overlay(backdrop_.luma());
    draw_scrims(sc, static_cast<int>(lerp(520, 200, p)), std::min(1.0f, 0.6f + 0.4f * sc));

    const Item &it = meta_.item;
    // Title page, the canonical lockup [EoA H1-H8]: logo at most 420 × 126,
    // title → metadata 13, metadata (Standard.Caption1, primary) → description 18,
    // description (Standard.Caption1, secondary) 667 wide in 4 lines → buttons 32.
    // Laid out from the buttons up so they always sit in the same place, above
    // the seasons peeking below (series) or How to watch (films).
    constexpr int kTitleToMeta = 13, kMetaToBody = 18, kBodyToButtons = 32, kBodyW = 667, kBodyLines = 4;
    int line = line_for(Text::Caption1);
    int lines = std::max(1, static_cast<int>(wrap(font_for(Text::Caption1), it.description, size_for(Text::Caption1), kBodyW, kBodyLines).size()));
    int by = kPanelY - kBodyToButtons - theme::button_h - (has_episodes() ? 46 : 0);
    int hero_top = by - kBodyToButtons - lines * line - kMetaToBody - line - kTitleToMeta - 126;
    // Title art shrinks and rises as the page continues into episodes.
    int box_top = static_cast<int>(lerp(static_cast<float>(hero_top), 80, p));
    int box_h = static_cast<int>(lerp(126, 85, p));
    int box_w = static_cast<int>(lerp(420, 283, p));
    int y = draw_title(it, X0, box_top, box_w, box_h);

    float top_only = std::max(0.0f, 1.0f - 1.6f * p);
    if (top_only > 0.01f) {
      y += kTitleToMeta;
      std::string facts = meta_line(it);
      int mw = text(Text::Caption1, facts, X0, y, label(Label::Primary, top_only), 1100);
      if (!it.rating.empty()) draw_glass_badge(X0 + mw + 18, y - 2, "IMDb " + it.rating, top_only);
      y += line + kMetaToBody;
      paragraph(Text::Caption1, it.description, X0, y, label(Label::Secondary, top_only), kBodyW, kBodyLines);
      draw_buttons(by, top_only);
      if (has_episodes()) draw_version_line(by + theme::button_h + 18, top_only);
      if (has_versions_panel()) draw_versions(top_only);
    }
    if (has_episodes()) draw_episodes(p);
  }

  void draw_buttons(int by, float alpha) {
    if (!loaded_ && error_.empty()) {
      draw_spinner(X0 + 40, by + 40, 40, label(Label::Secondary), opened_at_);
      return;
    }
    if (!error_.empty() && !loaded_) {
      paragraph(Text::Callout, error_, X0, by, fade(problem, alpha), 900, 2);
      return;
    }
    int bx = X0;
    auto list = buttons();
    for (size_t i = 0; i < list.size(); i++) {
      GlassButton b;
      b.label = list[i].label;
      b.icon = list[i].icon;
      b.focused = zone_ == Zone::Buttons && static_cast<int>(i) == button_;
      b.lift = lift_.value;
      b.alpha = alpha;
      b.min_w = theme::title_button_min_w;
      b.max_w = theme::title_button_max_w;
      bx += draw_glass_button(bx, by, b) + theme::title_button_gap;
    }
    if (has_resume_) text(Text::Caption1, resume_note_, bx, text_y_centered(Text::Caption1, by + theme::button_h / 2), label(Label::Secondary, alpha));
  }

  // Under the buttons of a show: what Play will look like and why, or that
  // you're caught up. One line; problems in the caution colour (UX-PLAN X3).
  void draw_version_line(int y, float alpha) {
    if (!loaded_) return;
    if (!note_.empty()) {
      text(Text::Callout, note_, X0, y, label(Label::Secondary, alpha), 1400);
      return;
    }
    if (versions_state_ != Versions::Ready || versions_.empty()) return;
    const SourceOption &best = versions_[0];
    std::string why = best.match == "same" ? "the same version as your last episode"
                      : best.match == "like" ? "like your last episode"
                      : intent_.known() ? "your last episode\xE2\x80\x99s version isn\xE2\x80\x99t offered" : "";
    std::string line = "In " + best.headline + (why.empty() ? "" : ", " + why);
    int w = text(Text::Callout, line, X0, y, label(Label::Secondary, alpha), 1400);
    if (!best.cautions.empty()) {
      int cx = X0 + w + 28;
      ctx().icons.draw(Icon::Warning, cx, y + 2, 26, fade(caution, alpha));
      text(Text::Callout, best.cautions[0], cx + 36, y, fade(caution, alpha), 600);
    }
  }

  // How to watch: a frosted panel along the bottom, best version ticked.
  void draw_versions(float alpha) {
    GlassSpec g;
    g.x = X0;
    g.y = kPanelY;
    g.w = W - 2 * X0;
    g.h = kPanelH;
    g.alpha = alpha;
    backdrop_.glass(g);
    int px = g.x + 36, py = g.y + 26;
    text(Text::Headline, "How to watch", px, py, label(Label::Primary, alpha));
    std::string status;
    if (versions_state_ == Versions::Loading) status = "Finding versions\xE2\x80\xA6";
    else if (versions_state_ == Versions::Empty) status = "No versions found right now. Try again later.";
    else if (versions_state_ == Versions::Failed) status = versions_error_;
    else status = ctx().paired ? "Play starts the best version" : "Play connects AllDebrid first, then starts the best version";
    text_right(Text::Caption1, status, g.x + g.w - 36, py + 8, label(Label::Secondary, alpha));
    if (versions_state_ != Versions::Ready) {
      if (versions_state_ == Versions::Loading) draw_spinner(px + 24, py + 110, 34, label(Label::Secondary), opened_at_);
      return;
    }
    int n = static_cast<int>(versions_.size());
    int gap = 18, tw = (g.w - 72 - gap * 2) / 3, th = 96, ty = py + 64;
    for (int i = 0; i < n; i++) {
      const SourceOption &o = versions_[static_cast<size_t>(i)];
      int tx = px + i * (tw + gap);
      bool focused = zone_ == Zone::Versions && i == version_;
      bool best = i == 0;
      if (focused) {
        ctx().gfx.shadow(tx, ty, tw, th, 20, 24, 10, static_cast<Uint8>(140 * alpha));
        ctx().gfx.fill_round(tx, ty, tw, th, 20, fade(rgba(0xffffff), alpha));
      } else if (best) {
        ctx().gfx.fill_round(tx, ty, tw, th, 20, fade(candidate, alpha));
      }
      Color main = focused ? fade(rgba(0x000000), alpha) : label(Label::Primary, alpha);
      Color sub = focused ? fade(rgba(0x000000, 166), alpha) : label(Label::Secondary, alpha);
      std::string kind = best ? "Best version" : !o.cautions.empty() ? o.cautions[0] : !o.size.empty() ? o.size : "Another version";
      text(Text::Caption2, kind, tx + 24, ty + 14, sub, tw - 80);
      text(Text::Callout, o.headline, tx + 24, ty + 46, main, tw - (best ? 84 : 48));
      if (best) ctx().icons.draw(Icon::Check, tx + tw - 56, ty + (th - 32) / 2, 32, main);
    }
  }

  void draw_episodes(float p) {
    Ctx &c = ctx();
    // Seasons row: peeks at the bottom of the title page, then rises.
    int sy = static_cast<int>(lerp(800, 330, p));
    if (meta_.seasons.size() > 1) {
      auto xs = season_xs();
      int off = static_cast<int>(std::lround(sscroll_.value));
      for (size_t i = 0; i < meta_.seasons.size(); i++) {
        int x = X0 + xs[i] - off;
        int w = xs[i + 1] - xs[i] - theme::control_gap;
        if (x + w < X0 - 60 || x > W - X0 + 60) continue;  // off screen
        bool selected = static_cast<int>(i) == season_;
        GlassButton b;
        b.label = meta_.seasons[i].label;
        b.pill = true;
        b.focused = zone_ == Zone::Seasons && selected;
        b.candidate = selected && !b.focused;
        b.lift = lift_.value;
        draw_glass_button(x, sy, b);
      }
      // More seasons either side: a quiet chevron in the margin.
      if (off > 0) c.icons.draw(Icon::ChevronLeft, X0 - 56, sy + 12, 36, label(Label::Tertiary));
      if (xs.back() - 12 - off > W - 2 * X0) c.icons.draw(Icon::ChevronRight, W - X0 + 20, sy + 12, 36, label(Label::Tertiary));
    } else {
      text(Text::CalloutBold, meta_.seasons[0].label, X0, sy + 16, label(Label::Primary));
    }

    const Season &season = meta_.seasons[static_cast<size_t>(season_)];
    int ey = sy + 96;
    float hs = escroll_.value;
    int count = static_cast<int>(season.episodes.size());
    int first = std::max(0, static_cast<int>(std::floor(hs)) - 1), last = std::min(count - 1, static_cast<int>(std::ceil(hs)) + 5);
    std::vector<int> order;
    for (int j = first; j <= last; j++) if (j != episode_) order.push_back(j);
    if (episode_ >= first && episode_ <= last) order.push_back(episode_);
    for (int j : order) {
      const Episode &e = season.episodes[static_cast<size_t>(j)];
      int x = X0 + static_cast<int>(std::lround((j - hs) * ESTRIDE));
      if (x > W || x + episode_w < 0) continue;
      bool focused = zone_ == Zone::Episodes && j == episode_;
      CardSpec cs;
      cs.art = e.thumbnail;
      cs.fallback_art = meta_.item.tile;
      cs.title = e.title;
      cs.w = episode_w;
      cs.h = episode_h;
      cs.focus = focused ? lift_.value : j == prev_episode_ ? prev_lift_.value : 0;
      cs.focused = focused;
      auto prog = progress_.find(e.id);
      bool watched = prog != progress_.end() && prog->second.watched;
      if (prog != progress_.end() && !watched && prog->second.duration > 0)
        cs.progress = static_cast<double>(prog->second.position) / prog->second.duration;
      SDL_Rect box = draw_card(x, ey, cs);
      if (watched) {
        c.gfx.fill_circle(box.x + box.w - 30, box.y + 30, 18, with_alpha(rgba(0x000000), 170));
        c.icons.draw(Icon::Check, box.x + box.w - 43, box.y + 17, 26, rgba(0xffffff));
      }
      // Under the card: number, title, then a short summary once the page has risen.
      // The 16:9 lockup with description [EoA K9, inferred]: eyebrow Caption3,
      // title Caption1, description Caption2 in up to 5 lines, 2 apart; text 4
      // below the card at rest, 24 when focused (growth 14 + separation 10).
      int ty = ey + episode_h + (focused ? 24 : 4);
      text(Text::Caption3, e.season == 0 ? "Special" : "Episode " + std::to_string(e.episode), x, ty, label(Label::Secondary));
      ty += line_for(Text::Caption3) + 2;
      text(Text::Caption1, e.title.empty() ? "Episode " + std::to_string(e.episode) : e.title, x, ty, label(focused ? Label::Primary : Label::Secondary), episode_w);
      if (p > 0.05f) paragraph(Text::Caption2, e.overview, x, ty + line_for(Text::Caption1) + 2, label(Label::Secondary, p * (focused ? 1.0f : 0.75f)), episode_w, 5);
    }
    if (count > 4 && hs + 4 < count) c.icons.draw(Icon::ChevronRight, W - 64, ey + episode_h / 2 - 20, 40, label(Label::Tertiary));

    // About the show, under the episodes once the page has risen.
    if (p > 0.6f) {
      float a = (p - 0.6f) / 0.4f;
      int ay = H - safe_y - 96;
      c.gfx.fill_rect(X0, ay - 26, W - 2 * X0, 1, fade(frost_hairline, a));
      std::vector<std::string> about = {meta_line(meta_.item)};
      if (!meta_.item.rating.empty()) about.push_back("IMDb " + meta_.item.rating);
      text(Text::Caption1, "About", X0, ay, label(Label::Secondary, a));
      text(Text::Callout, join(about), X0, ay + 38, label(Label::Primary, a), 760);
      if (!meta_.cast.empty()) {
        std::string cast;
        for (size_t i = 0; i < std::min<size_t>(4, meta_.cast.size()); i++) cast += (i ? ", " : "") + meta_.cast[i];
        int cx = X0 + 860;
        text(Text::Caption1, "Starring", cx, ay, label(Label::Secondary, a));
        text(Text::Callout, cast, cx, ay + 38, label(Label::Primary, a), W - X0 - cx);
      }
    }
  }

 private:
  Meta meta_;
  std::map<std::string, Watch> progress_;
  bool loaded_ = false;
  std::string error_;
  long long opened_at_ = 0;
  Backdrop backdrop_;
  Zone zone_ = Zone::Buttons;
  int button_ = 0;
  int season_ = 0, episode_ = 0, prev_episode_ = -1;
  Anim page_{page_rate}, escroll_{scroll_rate}, sscroll_{scroll_rate}, lift_{focus_in_rate, 1}, prev_lift_{focus_leave_rate};
  Versions versions_state_ = Versions::Loading;
  std::vector<SourceOption> versions_;
  std::string versions_for_, versions_error_, versions_why_;
  int version_ = 0;
  PlayRequest start_;
  std::string start_label_ = "Play", resume_note_, note_;
  Target target_;
  Intent intent_;
  bool dismissed_ = false;
  bool has_resume_ = false;
};

}  // namespace

std::unique_ptr<Screen> make_details(const Item &item) { return std::make_unique<DetailsScreen>(item); }
