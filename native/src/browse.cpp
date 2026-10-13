// Home, Films and Series: a full-bleed backdrop that follows focus, the title
// block (logo art where available) and rows of landscape tiles. Focus starts
// on the hero's buttons; Down moves into the rows, which slide up under it.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "screens.h"

namespace {

constexpr int CL = 212;        // content left edge (right of the rail)
constexpr int TW = 384, TH = 216, TR = 18, GAP = 28;
constexpr int STRIDE = TW + GAP;
constexpr int ROW_H = 330;
constexpr long long kDwellMs = 280;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

class BrowseScreen : public Screen {
 public:
  explicit BrowseScreen(Browse kind) : kind_(kind) {
    mode_.rate = 10;
    vscroll_.rate = 11;
    focus_.rate = 16;
    load();
  }

  const char *name() const override { return kind_ == Browse::Home ? "home" : kind_ == Browse::Movies ? "movies" : "series"; }
  bool root() const override { return true; }
  void set_rail_focus(bool rail) override { rail_ = rail; }

  void shown() override {
    // Back from details or playback: pick up what was just watched.
    if (kind_ == Browse::Home && state_ == State::Ready) refresh_continue();
  }

  // ---------------------------------------------------------------- data

  void load() {
    state_ = State::Loading;
    if (!ctx().api.ready()) {
      waiting_for_service_ = true;
      return;
    }
    waiting_for_service_ = false;
    std::string path = kind_ == Browse::Home ? "/api/home" : kind_ == Browse::Movies ? "/api/browse?type=movie" : "/api/browse?type=series";
    ctx().api.get(path, [this](const ApiResult &r) {
      if (!r.ok()) {
        state_ = State::Error;
        error_ = r.error;
        return;
      }
      rows_.clear();
      const Json &cont = r.json["continue"];
      if (cont.size()) {
        Row row;
        row.id = "continue";
        row.title = "Continue watching";
        for (size_t i = 0; i < cont.size(); i++) row.resume.push_back(parse_resume(cont[i]));
        rows_.push_back(row);
      }
      const Json &rows = r.json["rows"];
      for (size_t i = 0; i < rows.size(); i++) {
        Row row;
        row.id = rows[i]["id"].as_string();
        row.title = rows[i]["title"].as_string();
        for (size_t k = 0; k < rows[i]["items"].size(); k++) row.items.push_back(parse_item(rows[i]["items"][k]));
        if (!row.items.empty()) rows_.push_back(row);
      }
      reset_focus();
      state_ = rows_.empty() ? State::Error : State::Ready;
      if (rows_.empty()) error_ = "Nothing to show right now.";
      logf("browse", "%s: %zu rows", name(), rows_.size());
    }, alive_);
  }

  void refresh_continue() {
    ctx().api.get("/api/progress", [this](const ApiResult &r) {
      if (!r.ok()) return;
      std::vector<Resume> items;
      for (size_t i = 0; i < r.json["items"].size(); i++) items.push_back(parse_resume(r.json["items"][i]));
      bool had = !rows_.empty() && rows_[0].continue_row();
      if (had && items.empty()) {
        rows_.erase(rows_.begin());
        cols_.erase(cols_.begin());
        hscroll_.erase(hscroll_.begin());
        row_ = std::max(0, row_ - 1);
      } else if (had) {
        rows_[0].resume = items;
        cols_[0] = std::min<int>(cols_[0], static_cast<int>(items.size()) - 1);
      } else if (!items.empty()) {
        Row row;
        row.id = "continue";
        row.title = "Continue watching";
        row.resume = items;
        rows_.insert(rows_.begin(), row);
        cols_.insert(cols_.begin(), 0);
        hscroll_.insert(hscroll_.begin(), Anim{});
        if (!hero_) row_++;
      }
      vscroll_.snap(static_cast<float>(row_));
      set_active(hero_ ? featured_item() : focused_item(), true);
    }, alive_);
  }

  void reset_focus() {
    hero_ = true;
    hero_button_ = 0;
    row_ = 0;
    cols_.assign(rows_.size(), 0);
    hscroll_.assign(rows_.size(), Anim{});
    for (auto &a : hscroll_) a.rate = 12;
    vscroll_.snap(0);
    mode_.snap(0);
    set_active(featured_item(), true);
  }

  // ---------------------------------------------------------------- what is shown

  struct Active {
    Item item;
    const Resume *resume = nullptr;
  };

  Active featured_item() const {
    Active a;
    for (auto &row : rows_) {
      if (row.continue_row()) {
        a.item = row.resume[0].item;
        a.resume = &row.resume[0];
        return a;
      }
      if (!row.items.empty()) {
        a.item = row.items[0];
        return a;
      }
    }
    return a;
  }

  Active focused_item() const {
    Active a;
    if (rows_.empty()) return a;
    const Row &row = rows_[static_cast<size_t>(row_)];
    int col = cols_[static_cast<size_t>(row_)];
    if (row.continue_row()) {
      a.item = row.resume[static_cast<size_t>(col)].item;
      a.resume = &row.resume[static_cast<size_t>(col)];
    } else {
      a.item = row.items[static_cast<size_t>(col)];
    }
    return a;
  }

  void set_active(const Active &a, bool now) {
    pending_ = a.item;
    pending_resume_ = a.resume ? *a.resume : Resume{};
    has_pending_resume_ = a.resume != nullptr;
    pending_at_ = now ? 0 : ctx().now;
    if (now) apply_active();
  }

  void apply_active() {
    shown_ = pending_;
    shown_resume_ = pending_resume_;
    has_shown_resume_ = has_pending_resume_;
    backdrop_.set(shown_.backdrop);
    pending_at_ = -1;
  }

  // ---------------------------------------------------------------- actions

  std::vector<std::pair<std::string, int>> hero_buttons() const {
    std::vector<std::pair<std::string, int>> b;
    if (has_shown_resume_) {
      const Resume &r = shown_resume_;
      std::string what = r.kind == "next" && r.next ? "Play " + episode_code(r.next->season, r.next->episode)
                         : r.item.series() ? "Resume " + episode_code(r.season, r.episode) : "Resume";
      b.push_back({what, kPlayIcon});
      b.push_back({r.item.series() ? "Episodes & info" : "Details", -1});
    } else if (shown_.series()) {
      b.push_back({"Episodes & info", kPlayIcon});
    } else {
      b.push_back({"Play", kPlayIcon});
      b.push_back({"Details", -1});
    }
    return b;
  }

  void play_resume(const Resume &r) {
    PlayRequest req;
    req.item = r.item;
    if (r.kind == "next" && r.next) {
      req.video_id = r.next->video_id;
      req.season = r.next->season;
      req.episode = r.next->episode;
      req.episode_title = r.next->title;
    } else {
      req.video_id = r.video_id;
      req.season = r.season;
      req.episode = r.episode;
      req.episode_title = r.episode_title;
      req.resume_ms = r.position;
      req.next = r.next;
    }
    ctx().push(make_sources(req));
  }

  void play_movie(const Item &item) {
    PlayRequest req;
    req.item = item;
    req.video_id = item.id;
    ctx().push(make_sources(req));
  }

  void hero_action(int index) {
    auto buttons = hero_buttons();
    if (index >= static_cast<int>(buttons.size())) return;
    const std::string &label = buttons[static_cast<size_t>(index)].first;
    if (label == "Details" || label == "Episodes & info") ctx().push(make_details(shown_));
    else if (has_shown_resume_) play_resume(shown_resume_);
    else play_movie(shown_);
  }

  void open_focused() {
    Active a = focused_item();
    if (a.resume) play_resume(*a.resume);
    else ctx().push(make_details(a.item));
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    if (state_ != State::Ready) {
      if (state_ == State::Error && key == Key::Ok) load();
      return key == Key::Ok;
    }
    if (hero_) {
      int count = static_cast<int>(hero_buttons().size());
      switch (key) {
        case Key::Left:
          if (hero_button_ == 0) return false;
          hero_button_--;
          return true;
        case Key::Right:
          hero_button_ = std::min(count - 1, hero_button_ + 1);
          return true;
        case Key::Down:
          enter_rows(0);
          return true;
        case Key::Ok:
          if (!repeat) hero_action(hero_button_);
          return true;
        default:
          return false;  // Back → rail
      }
    }
    int &col = cols_[static_cast<size_t>(row_)];
    int count = static_cast<int>(rows_[static_cast<size_t>(row_)].size());
    switch (key) {
      case Key::Left:
        if (col == 0) return false;
        col--;
        break;
      case Key::Right:
        if (col + 1 >= count) return true;
        col++;
        break;
      case Key::Up:
        if (row_ == 0) {
          hero_ = true;
          hero_button_ = 0;
          mode_.to(0);
          vscroll_.to(0);
          set_active(featured_item(), false);
          return true;
        }
        row_--;
        break;
      case Key::Down:
        if (row_ + 1 >= static_cast<int>(rows_.size())) return true;
        row_++;
        break;
      case Key::Ok:
        if (!repeat) open_focused();
        return true;
      case Key::Back:
        // First Back returns to the top, the next one reaches the rail.
        hero_ = true;
        hero_button_ = 0;
        row_ = 0;
        for (size_t i = 0; i < cols_.size(); i++) cols_[i] = 0;
        mode_.to(0);
        vscroll_.to(0);
        set_active(featured_item(), false);
        return true;
      default:
        return false;
    }
    after_move();
    return true;
  }

  void enter_rows(int row) {
    hero_ = false;
    row_ = row;
    mode_.to(1);
    after_move();
  }

  void after_move() {
    vscroll_.to(static_cast<float>(row_));
    int col = cols_[static_cast<size_t>(row_)];
    int count = static_cast<int>(rows_[static_cast<size_t>(row_)].size());
    int max_scroll = std::max(0, count - 4);
    hscroll_[static_cast<size_t>(row_)].to(static_cast<float>(std::min(col, max_scroll)));
    focus_.snap(0);
    focus_.to(1);
    set_active(focused_item(), false);
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    if (waiting_for_service_ && c.api.ready()) load();
    if (waiting_for_service_ && !c.service_error.empty()) {
      waiting_for_service_ = false;
      state_ = State::Error;
      error_ = c.service_error;
    }
    if (pending_at_ >= 0 && c.now - pending_at_ >= kDwellMs) apply_active();
    else if (pending_at_ >= 0) c.redraw_at(pending_at_ + kDwellMs);
    bool moving = mode_.step(dt) | vscroll_.step(dt) | focus_.step(dt);
    for (auto &h : hscroll_) moving |= h.step(dt);
    backdrop_.update(dt);
    if (moving) c.animating = true;
  }

  void draw() override {
    Ctx &c = ctx();
    if (state_ != State::Ready) {
      c.gfx.clear(theme::bg);
      if (state_ == State::Loading) {
        draw_spinner(CL + (W - CL) / 2, H / 2, 64);
      } else {
        draw_text_center(c.fonts.bold, "Couldn\xE2\x80\x99t load this page", CL + (W - CL) / 2, 420, 44, theme::text);
        draw_paragraph(c.fonts.regular, error_, CL + (W - CL) / 2 - 450, 490, 26, 38, theme::text2, 900, 3);
        draw_button(CL + (W - CL) / 2 - button_width("Try again", false) / 2, 600, "Try again", !rail_, true);
      }
      return;
    }
    float m = mode_.value;
    backdrop_.draw(0.35f * m);
    draw_scrims(1.0f, static_cast<int>(lerp(470, 330, m)), 1.0f);

    draw_hero(m);
    draw_rows(m);
  }

  void draw_hero(float m) {
    Ctx &c = ctx();
    const Item &it = shown_;
    if (it.id.empty()) return;
    int eyebrow_y = static_cast<int>(lerp(120, 96, m));
    int box_top = static_cast<int>(lerp(156, 120, m));
    int box_h = static_cast<int>(lerp(220, 170, m));
    int box_w = static_cast<int>(lerp(760, 600, m));

    std::string eyebrow;
    if (has_shown_resume_) {
      const Resume &r = shown_resume_;
      eyebrow = r.kind == "next" ? "Up next" : r.item.series() ? "Continue season " + std::to_string(r.season) : "Continue watching";
    } else if (kind_ == Browse::Home && hero_) {
      eyebrow = "Most popular";
    }
    if (!eyebrow.empty()) c.fonts.bold.draw(upper(eyebrow), CL, eyebrow_y, 18, with_alpha(rgba(0x9fb7bd), 255));

    int y = draw_title(it, CL, box_top, box_w, box_h, static_cast<int>(lerp(96, 76, m)));
    y += 22;
    std::string meta = meta_line(it);
    if (has_shown_resume_ && shown_resume_.item.series()) {
      const Resume &r = shown_resume_;
      if (r.kind == "next" && r.next) meta = episode_code(r.next->season, r.next->episode) + " \xC2\xB7 " + r.next->title;
      else meta = episode_code(r.season, r.episode) + (r.episode_title.empty() ? "" : " \xC2\xB7 " + r.episode_title);
    }
    draw_text(c.fonts.semibold, meta, CL, y, 24, theme::text2, 1100);
    y += 46;
    y = draw_paragraph(c.fonts.regular, it.description, CL, y, 25, 38, rgba(0xd6d9dd), 820, 3);

    float buttons_alpha = std::max(0.0f, 1.0f - 2.0f * m);
    if (buttons_alpha > 0.02f) {
      int by = 600 + static_cast<int>(40 * m);
      int bx = CL;
      auto buttons = hero_buttons();
      for (size_t i = 0; i < buttons.size(); i++) {
        bool focused = hero_ && !rail_ && static_cast<int>(i) == hero_button_;
        bx += draw_button(bx, by, buttons[i].first, focused, i == 0, buttons[i].second, 76, 0, buttons_alpha) + 18;
      }
      if (has_shown_resume_ && shown_resume_.kind == "resume" && shown_resume_.duration > 0) {
        bx += 16;
        int a = static_cast<int>(255 * buttons_alpha);
        draw_progress(bx, by + 26, 220, 6, static_cast<double>(shown_resume_.position) / shown_resume_.duration,
                      with_alpha(theme::accent, a), with_alpha(theme::white, 50 * a / 255));
        c.fonts.regular.draw(minutes_left(shown_resume_.duration - shown_resume_.position), bx, by + 42, 18, with_alpha(rgba(0xaab0b8), a));
      }
    }
  }

  void draw_rows(float m) {
    Ctx &c = ctx();
    float base = lerp(760, 560, m);
    for (size_t i = 0; i < rows_.size(); i++) {
      float y = base + (static_cast<float>(i) - vscroll_.value) * ROW_H;
      if (y > H || y < -ROW_H) continue;
      // Rows that move above the focused one fade out under the title block.
      float alpha = std::max(0.0f, std::min(1.0f, (y - 330.0f) / 200.0f));
      if (alpha <= 0.01f) continue;
      bool focused_row = !hero_ && static_cast<int>(i) == row_;
      int hy = static_cast<int>(y);
      c.fonts.bold.draw(rows_[i].title, CL, hy, 28, with_alpha(focused_row ? theme::text : theme::text2, static_cast<int>(255 * alpha)));
      draw_row_tiles(i, hy + 52, alpha, focused_row);
    }
  }

  void draw_row_tiles(size_t r, int y, float alpha, bool focused_row) {
    Ctx &c = ctx();
    const Row &row = rows_[r];
    float hs = hscroll_[r].value;
    int count = static_cast<int>(row.size());
    int first = std::max(0, static_cast<int>(std::floor(hs)) - 1);
    int last = std::min(count - 1, static_cast<int>(std::ceil(hs)) + 5);
    // Warm the next tiles off screen so scrolling right shows art immediately.
    for (int j = last + 1; j < std::min(count, last + 3); j++) {
      const Item &it = row.continue_row() ? row.resume[static_cast<size_t>(j)].item : row.items[static_cast<size_t>(j)];
      c.images.prefetch(it.tile, TW, TH, TR, Fit::Cover, true);
    }
    int focus_col = cols_[r];
    for (int j = first; j <= last; j++) {
      const Item &it = row.continue_row() ? row.resume[static_cast<size_t>(j)].item : row.items[static_cast<size_t>(j)];
      int x = CL + static_cast<int>(std::lround((j - hs) * STRIDE));
      if (x > W || x + TW < 0) continue;
      bool focused = focused_row && j == focus_col && !rail_;
      float s = focused ? 1.0f + 0.06f * focus_.value : 1.0f;
      int w = static_cast<int>(TW * s), h = static_cast<int>(TH * s);
      int tx = x - (w - TW) / 2, ty = y - (h - TH) / 2;
      ImageRef img = c.images.get(it.tile, TW, TH, TR, Fit::Cover, true);
      // Some titles have no landscape art: crop the poster instead.
      if (img.failed && !it.poster.empty()) img = c.images.get(it.poster, TW, TH, TR, Fit::Cover, true);
      Uint8 a = static_cast<Uint8>(255 * alpha);
      if (img.texture) {
        if (img.alpha < 255) c.gfx.fill_round(tx, ty, w, h, TR, with_alpha(theme::panel2, a));
        c.gfx.texture(img.texture, tx, ty, w, h, static_cast<Uint8>(img.alpha * alpha));
      } else {
        c.gfx.fill_round(tx, ty, w, h, TR, with_alpha(theme::panel2, a));
      }
      // Title on the tile's shaded lower edge.
      int name_size = focused ? 26 : 24;
      draw_text(c.fonts.heavy, it.name, tx + 22, ty + h - 30 - name_size, name_size, with_alpha(theme::text, a), w - 44);
      if (row.continue_row()) {
        const Resume &res = row.resume[static_cast<size_t>(j)];
        if (res.kind == "resume" && res.duration > 0) {
          c.gfx.fill_rect(tx + 18, ty + h - 18, w - 36, 6, with_alpha(theme::white, static_cast<int>(60 * alpha)));
          c.gfx.fill_rect(tx + 18, ty + h - 18, static_cast<int>((w - 36) * std::min(1.0, static_cast<double>(res.position) / res.duration)), 6,
                          with_alpha(theme::accent, a));
        }
      }
      if (focused) draw_focus_ring(tx, ty, w, h, TR, focus_.value);
      if (focused_row) {
        std::string label;
        if (row.continue_row()) {
          const Resume &res = row.resume[static_cast<size_t>(j)];
          if (res.kind == "next" && res.next) label = "Next: " + episode_code(res.next->season, res.next->episode) + " \xC2\xB7 " + res.next->title;
          else if (res.item.series()) label = join({episode_code(res.season, res.episode), minutes_left(res.duration - res.position)});
          else label = minutes_left(res.duration - res.position);
        } else {
          label = join({it.year, it.series() ? "Series" : runtime_text(it.runtime)});
        }
        draw_text(c.fonts.regular, label, x, y + TH + 18, 20, with_alpha(focused ? theme::text2 : theme::text3, a), TW);
      }
    }
  }

 private:
  enum class State { Loading, Ready, Error };
  Browse kind_;
  State state_ = State::Loading;
  std::string error_;
  bool waiting_for_service_ = false;
  std::vector<Row> rows_;
  bool rail_ = false;
  bool hero_ = true;
  int hero_button_ = 0;
  int row_ = 0;
  std::vector<int> cols_;
  std::vector<Anim> hscroll_;
  Anim mode_, vscroll_, focus_;
  Item pending_, shown_;
  Resume pending_resume_, shown_resume_;
  bool has_pending_resume_ = false, has_shown_resume_ = false;
  long long pending_at_ = -1;
  Backdrop backdrop_;
};

}  // namespace

std::unique_ptr<Screen> make_browse(Browse kind) { return std::make_unique<BrowseScreen>(kind); }
