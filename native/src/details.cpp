// Title page. Artwork, title art, essential context and one primary action that
// names exactly what it plays. For series the page continues downward: moving
// below the buttons slides the seasons and episodes up into view while the
// artwork softens, so episodes read as part of the title, not a new screen.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr int ESTRIDE = episode_w + card_gap;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

class DetailsScreen : public Screen {
 public:
  explicit DetailsScreen(const Item &item) {
    meta_.item = item;
    backdrop_.set(item.backdrop, true);
    opened_at_ = ctx().now;
    load();
  }

  const char *name() const override { return "details"; }

  void shown() override {
    // Back from playback: refresh what was watched and what plays next.
    if (loaded_) load_progress();
  }

  void load() {
    ctx().api.get("/api/meta?type=" + meta_.item.type + "&id=" + meta_.item.id, [this](const ApiResult &r) {
      if (!r.ok()) {
        error_ = r.error;
        return;
      }
      Item original = meta_.item;
      meta_ = parse_meta(r.json["meta"]);
      if (meta_.item.description.empty()) meta_.item.description = original.description;
      progress_ = parse_progress(r.json["progress"]);
      loaded_ = true;
      choose_start(true);
    }, alive_);
  }

  void load_progress() {
    ctx().api.get("/api/meta?type=" + meta_.item.type + "&id=" + meta_.item.id, [this](const ApiResult &r) {
      if (!r.ok()) return;
      progress_ = parse_progress(r.json["progress"]);
      choose_start(zone_ == Zone::Buttons);
    }, alive_);
  }

  // What the main button plays, and where the episode row starts.
  void choose_start(bool move_focus) {
    start_ = PlayRequest{};
    start_.item = meta_.item;
    start_label_ = "Play";
    has_resume_ = false;
    resume_note_.clear();
    if (!meta_.item.series()) {
      start_.video_id = meta_.item.id;
      auto it = progress_.find(meta_.item.id);
      if (it != progress_.end() && !it->second.watched && it->second.position >= 30000) {
        start_.resume_ms = it->second.position;
        start_label_ = "Resume";
        has_resume_ = true;
        resume_note_ = minutes_left(it->second.duration - it->second.position);
      }
      return;
    }
    if (meta_.seasons.empty()) return;
    const Episode *latest = nullptr;
    long long latest_at = -1;
    for (auto &s : meta_.seasons)
      for (auto &e : s.episodes) {
        auto it = progress_.find(e.id);
        if (it != progress_.end() && it->second.updated > latest_at) {
          latest = &e;
          latest_at = it->second.updated;
        }
      }
    const Episode *target = nullptr;
    if (latest) {
      const Watch &w = progress_[latest->id];
      if (!w.watched && w.position >= 30000) {
        target = latest;
        start_.resume_ms = w.position;
        start_label_ = "Resume " + episode_code(latest->season, latest->episode);
        resume_note_ = minutes_left(w.duration - w.position);
        has_resume_ = true;
      } else if (auto next = next_episode(meta_, latest->id)) {
        target = find_episode(next->video_id);
        start_label_ = "Play " + episode_code(next->season, next->episode);
      }
    }
    if (!target) {
      for (auto &s : meta_.seasons)
        if (s.season != 0) {
          target = &s.episodes[0];
          break;
        }
      if (!target) target = &meta_.seasons[0].episodes[0];
      start_label_ = "Play " + episode_code(target->season, target->episode);
    }
    fill_episode(start_, *target);
    if (!has_resume_) start_.resume_ms = 0;
    if (!move_focus) return;
    for (size_t i = 0; i < meta_.seasons.size(); i++)
      for (size_t k = 0; k < meta_.seasons[i].episodes.size(); k++)
        if (&meta_.seasons[i].episodes[k] == target) {
          season_ = static_cast<int>(i);
          episode_ = static_cast<int>(k);
          escroll_.snap(static_cast<float>(scroll_for(episode_)));
        }
  }

  const Episode *find_episode(const std::string &id) const {
    for (auto &s : meta_.seasons)
      for (auto &e : s.episodes)
        if (e.id == id) return &e;
    return nullptr;
  }

  void fill_episode(PlayRequest &req, const Episode &e) {
    req.item = meta_.item;
    req.video_id = e.id;
    req.season = e.season;
    req.episode = e.episode;
    req.episode_title = e.title;
    req.episode_thumbnail = e.thumbnail;
    req.next = next_episode(meta_, e.id);
  }

  int episode_count() const {
    return meta_.seasons.empty() ? 0 : static_cast<int>(meta_.seasons[static_cast<size_t>(season_)].episodes.size());
  }

  int scroll_for(int episode) const { return std::max(0, std::min(episode - 1, episode_count() - 4)); }

  // ---------------------------------------------------------------- input

  enum class Zone { Buttons, Seasons, Episodes };

  bool has_episodes() const { return meta_.item.series() && !meta_.seasons.empty(); }

  std::vector<ButtonSpec> buttons() const {
    std::vector<ButtonSpec> b;
    b.push_back({start_label_, kPlayIcon});
    if (has_resume_) b.push_back({"Start over", static_cast<int>(Icon::Restart)});
    return b;
  }

  void focus_moved() {
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
        else if (key == Key::Ok && !repeat) press_button(button_);
        else if (key == Key::Back) ctx().pop();
        else return true;
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
  }

  void press_button(int index) {
    if (start_.video_id.empty()) return;
    PlayRequest req = start_;
    if (index == 1) req.resume_ms = 0;
    ctx().push(make_sources(req));
  }

  void play_episode(const Episode &e) {
    PlayRequest req;
    fill_episode(req, e);
    auto it = progress_.find(e.id);
    if (it != progress_.end() && !it->second.watched && it->second.position >= 30000) req.resume_ms = it->second.position;
    ctx().push(make_sources(req));
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    backdrop_.update(dt);
    if (page_.step(dt) | escroll_.step(dt) | lift_.step(dt) | prev_lift_.step(dt)) ctx().animating = true;
  }

  void draw() override {
    float p = page_.value;  // 0: title page, 1: episodes
    backdrop_.draw(0.3f * p, p);
    draw_scrims(1.0f, static_cast<int>(lerp(500, 200, p)), 1.0f);

    const Item &it = meta_.item;
    // Title art shrinks and rises as the page continues into episodes.
    int box_top = static_cast<int>(lerp(150, 70, p));
    int box_h = static_cast<int>(lerp(230, 120, p));
    int box_w = static_cast<int>(lerp(760, 420, p));
    int y = draw_title(it, margin, box_top, box_w, box_h);

    float top_only = std::max(0.0f, 1.0f - 1.6f * p);
    if (top_only > 0.01f) {
      y += 26;
      std::vector<std::string> facts = {meta_line(it)};
      text(Text::Callout, join(facts), margin, y, fade(secondary, top_only), 1100);
      if (!it.rating.empty()) {
        int mw = measure(Text::Callout, join(facts));
        text(Text::Callout, "IMDb " + it.rating, margin + mw + 28, y, fade(tertiary, top_only));
      }
      y += 48;
      y = paragraph(Text::Body, it.description, margin, y, fade(secondary, top_only), 860, 3);
      draw_buttons(std::max(y + 40, 600), top_only);
      std::vector<std::string> credits;
      if (!meta_.cast.empty()) {
        std::string cast;
        for (size_t i = 0; i < std::min<size_t>(3, meta_.cast.size()); i++) cast += (i ? ", " : "") + meta_.cast[i];
        credits.push_back("Starring " + cast);
      }
      if (!meta_.director.empty()) credits.push_back("Directed by " + meta_.director[0]);
      if (!credits.empty() && !has_episodes()) text(Text::Caption, join(credits, "    "), margin, std::max(y + 40, 600) + button_h + 40, fade(tertiary, top_only), 1400);
    }
    if (has_episodes()) draw_episodes(p);
  }

  void draw_buttons(int by, float alpha) {
    if (!loaded_ && error_.empty()) {
      draw_spinner(margin + 36, by + button_h / 2, 40, secondary, opened_at_);
      return;
    }
    if (!error_.empty() && !loaded_) {
      paragraph(Text::Callout, error_, margin, by, fade(problem, alpha), 900, 2);
      return;
    }
    int bx = margin;
    auto list = buttons();
    for (size_t i = 0; i < list.size(); i++) {
      ButtonSpec b = list[i];
      b.focused = zone_ == Zone::Buttons && static_cast<int>(i) == button_;
      b.lift = lift_.value;
      b.alpha = alpha;
      bx += draw_button(bx, by, b) + 20;
    }
    if (has_resume_) text(Text::Callout, resume_note_, bx + 8, by + (button_h - line_for(Text::Callout)) / 2, fade(secondary, alpha));
  }

  void draw_episodes(float p) {
    Ctx &c = ctx();
    // Seasons row: peeks at the bottom of the title page, then rises.
    int sy = static_cast<int>(lerp(790, 320, p));
    if (meta_.seasons.size() > 1) {
      int sx = margin;
      for (size_t i = 0; i < meta_.seasons.size(); i++) {
        bool selected = static_cast<int>(i) == season_;
        bool focused = zone_ == Zone::Seasons && selected;
        int w = pill_width(meta_.seasons[i].label);
        if (sx + w > W - margin) break;
        draw_pill(sx, sy, meta_.seasons[i].label, focused, selected);
        sx += w + 12;
      }
    } else {
      text(Text::Headline, meta_.seasons[0].label, margin, sy + 10, primary);
    }

    const Season &season = meta_.seasons[static_cast<size_t>(season_)];
    int ey = sy + 92;
    float hs = escroll_.value;
    int count = static_cast<int>(season.episodes.size());
    int first = std::max(0, static_cast<int>(std::floor(hs)) - 1), last = std::min(count - 1, static_cast<int>(std::ceil(hs)) + 5);
    std::vector<int> order;
    for (int j = first; j <= last; j++) if (j != episode_) order.push_back(j);
    if (episode_ >= first && episode_ <= last) order.push_back(episode_);
    for (int j : order) {
      const Episode &e = season.episodes[static_cast<size_t>(j)];
      int x = margin + static_cast<int>(std::lround((j - hs) * ESTRIDE));
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
        c.gfx.fill_circle(box.x + box.w - 30, box.y + 30, 18, with_alpha(ground, 210));
        c.icons.draw(Icon::Check, box.x + box.w - 43, box.y + 17, 26, good);
      }
      // Under the card: number, title, then a short summary once the page has risen.
      int ty = ey + episode_h + (focused ? 22 : 16);
      text(Text::Footnote, e.season == 0 ? "Special" : "Episode " + std::to_string(e.episode), x, ty, tertiary);
      text(Text::Callout, e.title.empty() ? "Episode " + std::to_string(e.episode) : e.title, x, ty + 28, focused ? primary : secondary, episode_w);
      if (p > 0.05f) paragraph(Text::Caption, e.overview, x, ty + 66, fade(secondary, p * (focused ? 1.0f : 0.75f)), episode_w, 3);
    }
    if (count > 4 && hs + 4 < count) c.icons.draw(Icon::ChevronRight, W - 70, ey + episode_h / 2 - 20, 40, tertiary);

    // About the show, under the episodes once the page has risen.
    if (p > 0.6f) {
      float a = (p - 0.6f) / 0.4f;
      int ay = H - 150;
      c.gfx.fill_rect(margin, ay - 28, W - 2 * margin, 1, fade(hairline, a));
      std::vector<std::string> about = {meta_line(meta_.item)};
      if (!meta_.item.rating.empty()) about.push_back("IMDb " + meta_.item.rating);
      text(Text::Eyebrow, "ABOUT", margin, ay, fade(tertiary, a));
      text(Text::Callout, join(about), margin, ay + 34, fade(secondary, a), 700);
      std::vector<std::string> credits;
      if (!meta_.cast.empty()) {
        std::string cast;
        for (size_t i = 0; i < std::min<size_t>(4, meta_.cast.size()); i++) cast += (i ? ", " : "") + meta_.cast[i];
        credits.push_back("Starring " + cast);
      }
      int cx = margin + 820;
      for (size_t i = 0; i < credits.size(); i++)
        text(Text::Callout, credits[i], cx, ay + 34 + static_cast<int>(i) * 34, fade(secondary, a), W - margin - cx);
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
  Anim page_{page_rate}, escroll_{scroll_rate}, lift_{focus_rate, 1}, prev_lift_{focus_rate};
  PlayRequest start_;
  std::string start_label_ = "Play", resume_note_;
  bool has_resume_ = false;
};

}  // namespace

std::unique_ptr<Screen> make_details(const Item &item) { return std::make_unique<DetailsScreen>(item); }
