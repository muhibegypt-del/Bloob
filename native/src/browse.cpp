// Home, Films and Series. The top of the page is a featured title over its
// full-bleed artwork (pages through a few picks); below it, shelves of 16:9
// cards. Moving into the shelves hides the tab bar, keeps the focused title's
// artwork and summary at the top, and sets the focused shelf at eye level.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr int STRIDE = card_w + card_gap;
constexpr int ROW_H = 350;
constexpr long long kAutoAdvanceMs = 9000;
constexpr int kFeatured = 6;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

class BrowseScreen : public Screen {
 public:
  explicit BrowseScreen(Browse kind) : kind_(kind) { load(); }

  const char *name() const override { return kind_ == Browse::Home ? "home" : kind_ == Browse::Movies ? "films" : "series"; }
  bool root() const override { return true; }
  void set_tabs_focus(bool tabs) override {
    tabs_ = tabs;
    if (tabs && !hero_) go_top();
  }

  void shown() override {
    // Back from details or playback: pick up what was just watched.
    if (kind_ == Browse::Home && state_ == State::Ready) refresh_continue();
    idle_since_ = ctx().now;
  }

  // ---------------------------------------------------------------- data

  void load() {
    state_ = State::Loading;
    loading_since_ = ctx().now;
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
      std::vector<Resume> resume;
      for (size_t i = 0; i < r.json["continue"].size(); i++) resume.push_back(parse_resume(r.json["continue"][i]));
      if (!resume.empty()) {
        Row row;
        row.id = "continue";
        row.title = "Continue watching";
        row.resume = resume;
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
      state_ = rows_.empty() ? State::Error : State::Ready;
      if (rows_.empty()) error_ = "Nothing to show right now.";
      build_featured();
      reset_focus();
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
        hscroll_.insert(hscroll_.begin(), Anim(scroll_rate));
        if (!hero_) row_++;
      }
      vscroll_.snap(static_cast<float>(row_));
      build_featured();
      if (hero_) show_featured(true);
      else show_focused(true);
    }, alive_);
  }

  // The featured picks: what you were watching first, then popular titles with title art.
  void build_featured() {
    featured_.clear();
    for (auto &row : rows_) {
      if (row.continue_row()) {
        for (size_t i = 0; i < row.resume.size() && featured_.size() < 2; i++) featured_.push_back({row.resume[i].item, true, row.resume[i]});
        continue;
      }
      for (auto &item : row.items) {
        if (static_cast<int>(featured_.size()) >= kFeatured) break;
        bool dup = false;
        for (auto &f : featured_) dup |= f.item.id == item.id;
        if (!dup && !item.logo.empty() && !item.description.empty()) featured_.push_back({item, false, Resume{}});
        if (featured_.size() >= 4 && row.id.find("popular") == std::string::npos) break;
      }
      if (static_cast<int>(featured_.size()) >= kFeatured) break;
    }
    feature_ = std::min(feature_, std::max(0, static_cast<int>(featured_.size()) - 1));
  }

  void reset_focus() {
    hero_ = true;
    button_ = 0;
    row_ = 0;
    cols_.assign(rows_.size(), 0);
    hscroll_.assign(rows_.size(), Anim(scroll_rate));
    vscroll_.snap(0);
    mode_.snap(0);
    show_featured(true);
  }

  // ---------------------------------------------------------------- what the top of the page shows

  struct Shown {
    Item item;
    bool has_resume = false;
    Resume resume;
  };

  Shown featured() const {
    if (featured_.empty()) return {};
    return featured_[static_cast<size_t>(feature_)];
  }

  Shown focused() const {
    Shown s;
    if (rows_.empty()) return s;
    const Row &row = rows_[static_cast<size_t>(row_)];
    int col = cols_[static_cast<size_t>(row_)];
    if (row.continue_row()) {
      s.item = row.resume[static_cast<size_t>(col)].item;
      s.has_resume = true;
      s.resume = row.resume[static_cast<size_t>(col)];
    } else {
      s.item = row.items[static_cast<size_t>(col)];
    }
    return s;
  }

  void show_featured(bool now) {
    pending_ = featured();
    pending_at_ = now ? 0 : ctx().now;
    if (now) apply_pending(true);
  }

  void show_focused(bool now) {
    pending_ = focused();
    pending_at_ = now ? 0 : ctx().now;
    if (now) apply_pending(true);
  }

  void apply_pending(bool immediate = false) {
    shown_ = pending_;
    backdrop_.set(shown_.item.backdrop, immediate && backdrop_.url().empty());
    pending_at_ = -1;
    text_in_.snap(0);
    text_in_.to(1);
    // Warm the title art so it arrives with the backdrop.
    if (!shown_.item.logo.empty()) ctx().images.prefetch(shown_.item.logo, 760, 240, 0, Fit::Contain);
  }

  // ---------------------------------------------------------------- actions

  std::vector<ButtonSpec> hero_buttons() const {
    std::vector<ButtonSpec> b;
    const Shown &s = shown_;
    if (s.has_resume) {
      const Resume &r = s.resume;
      std::string what = r.kind == "next" && r.next ? "Play " + episode_code(r.next->season, r.next->episode)
                         : r.item.series() ? "Resume " + episode_code(r.season, r.episode) : "Resume";
      b.push_back({what, kPlayIcon});
      b.push_back({r.item.series() ? "Episodes" : "Details", -1});
    } else if (s.item.series()) {
      b.push_back({"Episodes", kPlayIcon});
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

  void hero_action(int index) {
    auto buttons = hero_buttons();
    if (index >= static_cast<int>(buttons.size())) return;
    const std::string &label = buttons[static_cast<size_t>(index)].label;
    if (label == "Details" || label == "Episodes") {
      ctx().push(make_details(shown_.item));
    } else if (shown_.has_resume) {
      play_resume(shown_.resume);
    } else {
      PlayRequest req;
      req.item = shown_.item;
      req.video_id = shown_.item.id;
      ctx().push(make_sources(req));
    }
  }

  void open_focused() {
    Shown s = focused();
    if (s.has_resume) play_resume(s.resume);
    else ctx().push(make_details(s.item));
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    idle_since_ = ctx().now;
    if (state_ != State::Ready) {
      if (state_ == State::Error && key == Key::Ok) load();
      return key == Key::Ok;
    }
    if (hero_) return hero_key(key, repeat);
    return rows_key(key, repeat);
  }

  bool hero_key(Key key, bool repeat) {
    int count = static_cast<int>(hero_buttons().size());
    int pages = static_cast<int>(featured_.size());
    switch (key) {
      case Key::Left:
        if (button_ > 0) button_--;
        else if (pages > 1) page_feature(-1);
        break;
      case Key::Right:
        if (button_ + 1 < count) button_++;
        else if (pages > 1) page_feature(1);
        break;
      case Key::Down:
        enter_rows();
        return true;
      case Key::Ok:
        if (!repeat) hero_action(button_);
        return true;
      default:
        return false;  // Up/Back: the tab bar
    }
    button_lift_.snap(0);
    button_lift_.to(1);
    return true;
  }

  void page_feature(int d) {
    int pages = static_cast<int>(featured_.size());
    feature_ = (feature_ + d + pages) % pages;
    button_ = d > 0 ? 0 : static_cast<int>(hero_buttons().size()) - 1;
    show_featured(true);
    button_ = std::min(button_, static_cast<int>(hero_buttons().size()) - 1);
  }

  bool rows_key(Key key, bool repeat) {
    int &col = cols_[static_cast<size_t>(row_)];
    int count = static_cast<int>(rows_[static_cast<size_t>(row_)].size());
    int before_row = row_, before_col = col;
    switch (key) {
      case Key::Left:
        if (col > 0) col--;
        break;
      case Key::Right:
        if (col + 1 < count) col++;
        break;
      case Key::Up:
        if (row_ == 0) {
          go_top();
          return true;
        }
        row_--;
        break;
      case Key::Down:
        if (row_ + 1 < static_cast<int>(rows_.size())) row_++;
        break;
      case Key::Ok:
        if (!repeat) open_focused();
        return true;
      case Key::Back:
        go_top();
        return true;
      default:
        return false;
    }
    if (row_ != before_row || cols_[static_cast<size_t>(row_)] != before_col) moved(before_row, before_col);
    return true;
  }

  void go_top() {
    hero_ = true;
    button_ = 0;
    row_ = 0;
    mode_.to(0);
    vscroll_.to(0);
    show_featured(false);
  }

  void enter_rows() {
    hero_ = false;
    row_ = 0;
    mode_.to(1);
    moved(-1, -1);
  }

  void moved(int from_row, int from_col) {
    prev_row_ = from_row;
    prev_col_ = from_col;
    prev_lift_.snap(lift_.value);
    prev_lift_.to(0);
    lift_.snap(0);
    lift_.to(1);
    vscroll_.to(static_cast<float>(row_));
    int col = cols_[static_cast<size_t>(row_)];
    int count = static_cast<int>(rows_[static_cast<size_t>(row_)].size());
    hscroll_[static_cast<size_t>(row_)].to(static_cast<float>(std::min(col, std::max(0, count - 4))));
    show_focused(false);
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
    c.show_tabs(hero_ || state_ != State::Ready);
    if (pending_at_ >= 0) {
      if (c.now - pending_at_ >= dwell_ms) apply_pending();
      else c.redraw_at(pending_at_ + dwell_ms);
    }
    // The featured title pages on by itself while nobody is using the remote.
    if (state_ == State::Ready && hero_ && featured_.size() > 1) {
      if (c.now - idle_since_ >= kAutoAdvanceMs) {
        idle_since_ = c.now;
        feature_ = (feature_ + 1) % static_cast<int>(featured_.size());
        button_ = 0;
        show_featured(false);
        pending_at_ = 0;
      } else {
        c.redraw_at(idle_since_ + kAutoAdvanceMs);
      }
    }
    bool moving = mode_.step(dt) | vscroll_.step(dt) | lift_.step(dt) | prev_lift_.step(dt) | text_in_.step(dt) | button_lift_.step(dt);
    for (auto &h : hscroll_) moving |= h.step(dt);
    backdrop_.update(dt);
    if (state_ == State::Loading) c.redraw_at(c.now + 50);  // skeleton pulse
    if (moving) c.animating = true;
  }

  void draw() override {
    Ctx &c = ctx();
    if (state_ == State::Loading) return draw_skeleton();
    if (state_ == State::Error) {
      c.gfx.clear(ground);
      text_center(Text::Title2, "Couldn\xE2\x80\x99t load this page", W / 2, 420, primary);
      int y = 480;
      for (auto &l : wrap(font_for(Text::Body), error_, size_for(Text::Body), 900, 3)) {
        text_center(Text::Body, l, W / 2, y, secondary);
        y += line_for(Text::Body);
      }
      ButtonSpec b{"Try again", -1, !tabs_};
      draw_button(W / 2 - button_width(b) / 2, y + 40, b);
      return;
    }
    float m = mode_.value;
    backdrop_.draw(0.25f * m);
    draw_scrims(1.0f, static_cast<int>(lerp(480, 300, m)), 1.0f);
    draw_top(m);
    draw_rows(m);
  }

  void draw_skeleton() {
    Ctx &c = ctx();
    c.gfx.clear(ground);
    float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(c.now / 1600.0 * 6.2831853));
    Color tone = mix(raised, raised2, pulse);
    c.gfx.fill_round(margin, 260, 520, 150, 16, tone);
    c.gfx.fill_round(margin, 440, 380, 26, 13, tone);
    c.gfx.fill_round(margin, 490, 760, 22, 11, tone);
    c.gfx.fill_round(margin, 526, 640, 22, 11, tone);
    c.gfx.fill_round(margin, 636, 200, button_h, button_h / 2, tone);
    c.gfx.fill_round(margin + 220, 636, 170, button_h, button_h / 2, tone);
    for (int i = 0; i < 5; i++) c.gfx.fill_round(margin + i * STRIDE, 900, card_w, card_h, card_radius, tone);
  }

  void draw_top(float m) {
    Ctx &c = ctx();
    const Shown &s = shown_;
    if (s.item.id.empty()) return;
    float t = text_in_.value;  // new title text eases in
    int box_top = static_cast<int>(lerp(232, 96, m));
    int box_h = static_cast<int>(lerp(220, 160, m));
    int box_w = static_cast<int>(lerp(760, 560, m));

    // Eyebrow: why this title is here.
    std::string eyebrow;
    if (s.has_resume) eyebrow = s.resume.kind == "next" ? "Up next" : "Continue watching";
    else if (hero_) eyebrow = kind_ == Browse::Series ? "Popular series" : kind_ == Browse::Movies ? "Popular film" : (s.item.series() ? "Popular series" : "Popular film");
    float hero_only = std::max(0.0f, 1.0f - 2.0f * m);
    if (!eyebrow.empty() && hero_only > 0.01f) text(Text::Eyebrow, upper(eyebrow), margin, box_top - 34, fade(accent, hero_only * t));

    int y = draw_title(s.item, margin, box_top, box_w, box_h, t);
    y += 24;
    std::string meta = meta_line(s.item);
    if (s.has_resume && s.item.series()) {
      const Resume &r = s.resume;
      if (r.kind == "next" && r.next) meta = episode_code(r.next->season, r.next->episode) + " \xC2\xB7 " + r.next->title;
      else meta = join({episode_code(r.season, r.episode), r.episode_title});
    }
    text(Text::Callout, meta, margin, y, fade(secondary, t), 1100);
    y += 44;
    paragraph(Text::Body, s.item.description, margin, y, fade(secondary, t), 780, 2);

    if (hero_only > 0.01f) {
      int by = 636;
      int bx = margin;
      auto buttons = hero_buttons();
      for (size_t i = 0; i < buttons.size(); i++) {
        ButtonSpec b = buttons[i];
        b.focused = hero_ && !tabs_ && static_cast<int>(i) == button_;
        b.lift = button_lift_.value;
        b.alpha = hero_only;
        bx += draw_button(bx, by, b) + 20;
      }
      if (s.has_resume && s.resume.kind == "resume" && s.resume.duration > 0) {
        bx += 12;
        draw_progress(bx, by + 24, 200, 6, static_cast<double>(s.resume.position) / s.resume.duration, fade(accent, hero_only),
                      fade(with_alpha(focus, 50), hero_only));
        text(Text::Footnote, minutes_left(s.resume.duration - s.resume.position), bx, by + 40, fade(secondary, hero_only));
      }
      // Page dots for the featured picks.
      int pages = static_cast<int>(featured_.size());
      int dx = margin;
      for (int i = 0; pages > 1 && i < pages; i++) {
        bool on = i == feature_;
        int w = on ? 28 : 10;
        c.gfx.fill_round(dx, 752, w, 10, 5, fade(on ? primary : tertiary, hero_only));
        dx += w + 10;
      }
    }
  }

  void draw_rows(float m) {
    float base = lerp(850, 520, m);
    for (size_t i = 0; i < rows_.size(); i++) {
      float y = base + (static_cast<float>(i) - vscroll_.value) * ROW_H;
      if (y > H || y < -ROW_H) continue;
      // Shelves that move above the focused one fade out under the title.
      float alpha = std::max(0.0f, std::min(1.0f, (y - 300.0f) / 170.0f));
      if (alpha <= 0.01f) continue;
      bool focused_row = !hero_ && static_cast<int>(i) == row_;
      int hy = static_cast<int>(y);
      text(Text::Headline, rows_[i].title, margin, hy, fade(focused_row ? primary : secondary, alpha));
      draw_shelf(i, hy + 50, alpha, focused_row);
    }
  }

  void draw_shelf(size_t r, int y, float alpha, bool focused_row) {
    Ctx &c = ctx();
    const Row &row = rows_[r];
    float hs = hscroll_[r].value;
    int count = static_cast<int>(row.size());
    int first = std::max(0, static_cast<int>(std::floor(hs)) - 1);
    int last = std::min(count - 1, static_cast<int>(std::ceil(hs)) + 5);
    for (int j = last + 1; j < std::min(count, last + 3); j++) {
      const Item &it = row.continue_row() ? row.resume[static_cast<size_t>(j)].item : row.items[static_cast<size_t>(j)];
      c.images.prefetch(it.tile, card_w, card_h, card_radius);
    }
    int focus_col = cols_[r];
    // Draw the focused card last so its lift and shadow sit above neighbours.
    std::vector<int> order;
    for (int j = first; j <= last; j++) if (!(focused_row && j == focus_col)) order.push_back(j);
    if (focused_row && focus_col >= first && focus_col <= last) order.push_back(focus_col);
    for (int j : order) {
      const Item &it = row.continue_row() ? row.resume[static_cast<size_t>(j)].item : row.items[static_cast<size_t>(j)];
      int x = margin + static_cast<int>(std::lround((j - hs) * STRIDE));
      if (x > W || x + card_w < 0) continue;
      bool focused = focused_row && j == focus_col && !tabs_;
      bool was = static_cast<int>(r) == prev_row_ && j == prev_col_;
      CardSpec cs;
      cs.art = it.tile;
      cs.fallback_art = it.poster;
      cs.title = it.name;
      cs.focus = focused ? lift_.value : was ? prev_lift_.value : 0;
      cs.focused = focused;
      cs.alpha = alpha;
      if (row.continue_row()) {
        const Resume &res = row.resume[static_cast<size_t>(j)];
        if (res.kind == "resume" && res.duration > 0) cs.progress = static_cast<double>(res.position) / res.duration;
      }
      draw_card(x, y, cs);
      if (focused_row) {
        std::string label = it.name, sub;
        if (row.continue_row()) {
          const Resume &res = row.resume[static_cast<size_t>(j)];
          if (res.kind == "next" && res.next) sub = "Next: " + episode_code(res.next->season, res.next->episode);
          else if (res.item.series()) sub = join({episode_code(res.season, res.episode), minutes_left(res.duration - res.position)});
          else sub = minutes_left(res.duration - res.position);
        }
        int ly = y + card_h + (focused ? 22 : 14);
        text(Text::Caption, label, x, ly, fade(focused ? primary : secondary, alpha), card_w);
        if (!sub.empty()) text(Text::Footnote, sub, x, ly + 30, fade(tertiary, alpha), card_w);
      }
    }
  }

 private:
  enum class State { Loading, Ready, Error };
  Browse kind_;
  State state_ = State::Loading;
  std::string error_;
  long long loading_since_ = 0;
  bool waiting_for_service_ = false;
  std::vector<Row> rows_;
  std::vector<Shown> featured_;
  int feature_ = 0;
  bool tabs_ = false;
  bool hero_ = true;
  int button_ = 0;
  int row_ = 0;
  std::vector<int> cols_;
  std::vector<Anim> hscroll_;
  Anim mode_{page_rate}, vscroll_{page_rate}, lift_{focus_rate}, prev_lift_{focus_rate}, text_in_{fade_rate, 1},
      button_lift_{focus_rate, 1};
  int prev_row_ = -1, prev_col_ = -1;
  Shown pending_, shown_;
  long long pending_at_ = -1;
  long long idle_since_ = 0;
  Backdrop backdrop_;
};

}  // namespace

std::unique_ptr<Screen> make_browse(Browse kind) { return std::make_unique<BrowseScreen>(kind); }
