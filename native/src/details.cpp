// Title details: backdrop, logo, facts and the main play button, which names
// exactly what it plays (resume point, next episode or the first episode).
// Series add a season selector and an episode row; focusing an episode shows
// its own summary in place of the series description.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "screens.h"

namespace {

constexpr int LM = 120;
constexpr int EW = 400, EH = 225, ER = 16, EGAP = 24;
constexpr int ESTRIDE = EW + EGAP;

class DetailsScreen : public Screen {
 public:
  explicit DetailsScreen(const Item &item) {
    meta_.item = item;
    backdrop_.set(item.backdrop);
    focus_.rate = 16;
    escroll_.rate = 12;
    load();
  }

  const char *name() const override { return "details"; }

  void shown() override {
    // Back from playback: refresh what was watched.
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
      choose_start();
    }, alive_);
  }

  void load_progress() {
    ctx().api.get("/api/meta?type=" + meta_.item.type + "&id=" + meta_.item.id, [this](const ApiResult &r) {
      if (!r.ok()) return;
      progress_ = parse_progress(r.json["progress"]);
      choose_start();
    }, alive_);
  }

  // Decide what the main button plays and where the episode row starts.
  void choose_start() {
    start_ = PlayRequest{};
    start_.item = meta_.item;
    start_label_ = "Play";
    has_resume_ = false;
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
    // Most recently watched episode decides.
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
    // Show that episode's season, scrolled to it.
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

  int scroll_for(int episode) const {
    int count = static_cast<int>(meta_.seasons.empty() ? 0 : meta_.seasons[static_cast<size_t>(season_)].episodes.size());
    return std::max(0, std::min(episode - 1, count - 4));
  }

  // ---------------------------------------------------------------- input

  enum class Zone { Buttons, Seasons, Episodes };

  std::vector<std::pair<std::string, int>> buttons() const {
    std::vector<std::pair<std::string, int>> b;
    b.push_back({start_label_, kPlayIcon});
    if (has_resume_) b.push_back({"Start over", static_cast<int>(Icon::Restart)});
    return b;
  }

  bool key(Key key, bool repeat) override {
    bool series = meta_.item.series() && !meta_.seasons.empty();
    switch (zone_) {
      case Zone::Buttons: {
        int count = static_cast<int>(buttons().size());
        if (key == Key::Left) button_ = std::max(0, button_ - 1);
        else if (key == Key::Right) button_ = std::min(count - 1, button_ + 1);
        else if (key == Key::Down && series) zone_ = meta_.seasons.size() > 1 ? Zone::Seasons : Zone::Episodes;
        else if (key == Key::Ok && !repeat) press_button(button_);
        else if (key == Key::Back) ctx().pop();
        else return key == Key::Up;
        break;
      }
      case Zone::Seasons: {
        int count = static_cast<int>(meta_.seasons.size());
        if (key == Key::Left && season_ > 0) select_season(season_ - 1);
        else if (key == Key::Right && season_ + 1 < count) select_season(season_ + 1);
        else if (key == Key::Up) zone_ = Zone::Buttons;
        else if (key == Key::Down || key == Key::Ok) zone_ = Zone::Episodes;
        else if (key == Key::Back) zone_ = Zone::Buttons;
        break;
      }
      case Zone::Episodes: {
        int count = static_cast<int>(meta_.seasons[static_cast<size_t>(season_)].episodes.size());
        if (key == Key::Left) {
          if (episode_ > 0) episode_--;
          else if (season_ > 0) {  // continue into the previous season's last episode
            select_season(season_ - 1);
            episode_ = static_cast<int>(meta_.seasons[static_cast<size_t>(season_)].episodes.size()) - 1;
          }
        } else if (key == Key::Right) {
          if (episode_ + 1 < count) episode_++;
          else if (season_ + 1 < static_cast<int>(meta_.seasons.size()) && meta_.seasons[static_cast<size_t>(season_) + 1].season != 0) {
            select_season(season_ + 1);
          }
        } else if (key == Key::Up) {
          zone_ = meta_.seasons.size() > 1 ? Zone::Seasons : Zone::Buttons;
        } else if (key == Key::Ok && !repeat) {
          play_episode(meta_.seasons[static_cast<size_t>(season_)].episodes[static_cast<size_t>(episode_)]);
        } else if (key == Key::Back) {
          zone_ = Zone::Buttons;
        }
        escroll_.to(static_cast<float>(scroll_for(episode_)));
        break;
      }
    }
    focus_.snap(0);
    focus_.to(1);
    return true;
  }

  void select_season(int index) {
    season_ = index;
    episode_ = 0;
    escroll_.snap(0);
  }

  void press_button(int index) {
    if (index == 0) {
      if (start_.video_id.empty()) return;
      ctx().push(make_sources(start_));
    } else {
      PlayRequest again = start_;
      again.resume_ms = 0;
      ctx().push(make_sources(again));
    }
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
    if (focus_.step(dt) | escroll_.step(dt)) ctx().animating = true;
  }

  void draw() override {
    Ctx &c = ctx();
    bool series = meta_.item.series();
    bool in_episodes = zone_ != Zone::Buttons && series && !meta_.seasons.empty();
    backdrop_.draw(in_episodes ? 0.25f : 0.0f);
    draw_scrims(1.0f, series ? 380 : 480, 1.0f);

    const Item &it = meta_.item;
    int y = draw_title(it, LM, 100, 760, series ? 190 : 230, 88);
    y += 24;

    if (in_episodes) {
      const Episode &e = meta_.seasons[static_cast<size_t>(season_)].episodes[static_cast<size_t>(episode_)];
      std::string when = e.released.size() >= 10 ? e.released.substr(0, 10) : "";
      c.fonts.bold.draw(upper(e.season == 0 ? "Special" : "Season " + std::to_string(e.season) + " \xC2\xB7 Episode " + std::to_string(e.episode)),
                        LM, y, 18, rgba(0x9fb7bd));
      draw_text(c.fonts.bold, e.title.empty() ? "Episode " + std::to_string(e.episode) : e.title, LM, y + 32, 36, theme::text, 1100);
      int py = draw_paragraph(c.fonts.regular, e.overview.empty() ? "No summary yet." : e.overview, LM, y + 90, 24, 36, rgba(0xd6d9dd), 900, 3);
      if (!when.empty()) c.fonts.regular.draw("Aired " + when, LM, py + 6, 20, theme::text3);
    } else {
      draw_text(c.fonts.semibold, meta_line(it), LM, y, 24, theme::text2, 1200);
      y += 48;
      y = draw_paragraph(c.fonts.regular, it.description, LM, y, 25, 38, rgba(0xd6d9dd), 880, series ? 3 : 4);
      std::vector<std::string> credits;
      if (!meta_.cast.empty()) {
        std::string cast;
        for (size_t i = 0; i < std::min<size_t>(3, meta_.cast.size()); i++) cast += (i ? ", " : "") + meta_.cast[i];
        credits.push_back("Starring " + cast);
      }
      if (!meta_.director.empty()) credits.push_back("Directed by " + meta_.director[0]);
      if (!credits.empty()) {
        draw_text(c.fonts.regular, join(credits, "   "), LM, y + 10, 21, theme::text3, 1100);
        y += 40;
      }
      text_bottom_ = y;
    }

    // Buttons: below the text for films, at a fixed line above the episodes for series.
    int by = series ? 560 : std::max(560, std::min(700, text_bottom_ + 48));
    if (!loaded_ && error_.empty()) {
      draw_spinner(LM + 40, by + 38, 44);
    } else if (!error_.empty() && !loaded_) {
      draw_paragraph(c.fonts.regular, error_, LM, by, 24, 34, theme::bad, 900, 2);
    } else {
      int bx = LM;
      auto list = buttons();
      for (size_t i = 0; i < list.size(); i++) {
        bool focused = zone_ == Zone::Buttons && static_cast<int>(i) == button_;
        bx += draw_button(bx, by, list[i].first, focused, i == 0, list[i].second) + 18;
      }
      if (has_resume_) c.fonts.regular.draw(resume_note_, bx + 8, by + 25, 22, theme::text2);
    }

    if (series && !meta_.seasons.empty()) draw_episodes();
  }

  void draw_episodes() {
    Ctx &c = ctx();
    // Season chips.
    int sy = 676;
    int sx = LM;
    if (meta_.seasons.size() > 1) {
      for (size_t i = 0; i < meta_.seasons.size(); i++) {
        bool selected = static_cast<int>(i) == season_;
        bool focused = zone_ == Zone::Seasons && selected;
        const std::string &label = meta_.seasons[i].label;
        int w = chip_width(label);
        if (sx + w > W - 60) break;
        draw_chip(sx, sy, label, focused, selected ? with_alpha(theme::white, 46) : with_alpha(theme::white, 16),
                  selected ? theme::text : theme::text3);
        sx += w + 12;
      }
    } else {
      c.fonts.bold.draw(meta_.seasons[0].label, LM, sy + 12, 26, theme::text);
    }

    const Season &season = meta_.seasons[static_cast<size_t>(season_)];
    int ey = 760;
    float hs = escroll_.value;
    int count = static_cast<int>(season.episodes.size());
    int first = std::max(0, static_cast<int>(std::floor(hs)) - 1), last = std::min(count - 1, static_cast<int>(std::ceil(hs)) + 5);
    for (int j = first; j <= last; j++) {
      const Episode &e = season.episodes[static_cast<size_t>(j)];
      int x = LM + static_cast<int>(std::lround((j - hs) * ESTRIDE));
      if (x > W || x + EW < 0) continue;
      bool focused = zone_ == Zone::Episodes && j == episode_;
      float s = focused ? 1.0f + 0.05f * focus_.value : 1.0f;
      int w = static_cast<int>(EW * s), h = static_cast<int>(EH * s);
      int tx = x - (w - EW) / 2, ty = ey - (h - EH) / 2;
      std::string art = e.thumbnail.empty() ? meta_.item.tile : e.thumbnail;
      ImageRef img = c.images.get(art, EW, EH, ER, Fit::Cover, true);
      c.gfx.fill_round(tx, ty, w, h, ER, theme::panel2);
      if (img.texture) c.gfx.texture(img.texture, tx, ty, w, h, img.alpha);
      std::string number = std::to_string(e.episode);
      c.fonts.heavy.draw(number, tx + 20, ty + h - 70, 30, theme::text);
      int nw = c.fonts.heavy.measure(number, 30);
      draw_text(c.fonts.semibold, e.title, tx + 30 + nw, ty + h - 64, 22, theme::text, w - 60 - nw);
      auto p = progress_.find(e.id);
      if (p != progress_.end()) {
        if (p->second.watched) {
          c.gfx.fill_circle(tx + w - 34, ty + 34, 20, with_alpha(theme::bg, 200));
          c.icons.draw(Icon::Check, tx + w - 48, ty + 20, 28, theme::ok);
        } else if (p->second.duration > 0) {
          c.gfx.fill_rect(tx + 18, ty + h - 18, w - 36, 6, with_alpha(theme::white, 60));
          c.gfx.fill_rect(tx + 18, ty + h - 18, static_cast<int>((w - 36) * std::min(1.0, static_cast<double>(p->second.position) / p->second.duration)), 6, theme::accent);
        }
      }
      if (focused) draw_focus_ring(tx, ty, w, h, ER, focus_.value);
    }
    if (count > 4 && hs + 4 < count) c.icons.draw(Icon::ChevronRight, W - 70, ey + EH / 2 - 20, 40, theme::text3);
  }

 private:
  Meta meta_;
  std::map<std::string, Watch> progress_;
  bool loaded_ = false;
  std::string error_;
  Backdrop backdrop_;
  Zone zone_ = Zone::Buttons;
  int button_ = 0;
  int season_ = 0, episode_ = 0;
  Anim focus_, escroll_;
  PlayRequest start_;
  std::string start_label_ = "Play", resume_note_;
  bool has_resume_ = false;
  int text_bottom_ = 560;
};

}  // namespace

std::unique_ptr<Screen> make_details(const Item &item) { return std::make_unique<DetailsScreen>(item); }
