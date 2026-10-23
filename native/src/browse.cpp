// Home, Films and Series (Glass). The featured title fills the screen: its art at
// full brightness under adaptive scrims, its title, one line and the actions on
// the left, the frosted tab bar above. Below, shelves of 320×180 cards on the
// tvOS grid. Moving into the shelves hides the tab bar, keeps the focused
// title's art and summary at the top, and sets the focused shelf at eye level.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "log.h"
#include "pairing.h"
#include "speech.h"
#include "play.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr int STRIDE = card_w + card_gap;
// Shelves [EoA H11-H14]: header block 72 (text 20 down), 14 to the cards, 55 below;
// the first shelf starts at 742 under the hero.
constexpr int kHeaderH = 72, kHeaderTop = 20, kHeaderToItems = 14, kShelfBottom = 55;
constexpr int ROW_H = kHeaderH + kHeaderToItems + card_h + kShelfBottom;  // 321
constexpr int kFirstShelf = 742;
// Hero text block [EoA H1-H8]: logo at most 420 × 126 (283 × 85 when compact),
// title → metadata 13, metadata → description 18, description 570 wide in 3
// lines, description → buttons 32. The block ends 32 above the first shelf.
constexpr int kLogoW = 420, kLogoH = 126, kLogoSmallW = 283, kLogoSmallH = 85;
constexpr int kTitleToMeta = 13, kMetaToBody = 18, kBodyToButtons = 32, kBodyW = 570, kBodyLines = 3;
constexpr int X0 = safe_x;
constexpr long long kAutoAdvanceMs = 9000;
constexpr int kFeatured = 6;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

class BrowseScreen : public Screen {
 public:
  explicit BrowseScreen(Browse kind) : kind_(kind) { load(); }

  const char *name() const override { return kind_ == Browse::Home ? "home" : kind_ == Browse::Movies ? "films" : "series"; }
  bool root() const override { return true; }
  void shell_glass(const GlassSpec &g) const override {
    if (state_ == State::Ready) backdrop_.glass(g);
    else Screen::shell_glass(g);
  }
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
      if (kind_ == Browse::Home && state_ == State::Ready) ctx().mark("home-data");
    }, alive_);
  }

  // Identity of a card: the title, plus the episode for Continue watching.
  static std::string card_id(const Row &row, int col) {
    if (row.continue_row()) {
      const Resume &r = row.resume[static_cast<size_t>(col)];
      return r.item.id + "|" + r.video_id;
    }
    return row.items[static_cast<size_t>(col)].id;
  }
  // Where that card is now; the same title (a different episode) if the exact one has gone.
  static int find_card(const Row &row, const std::string &id, int fallback) {
    int count = static_cast<int>(row.size());
    for (int i = 0; i < count; i++)
      if (card_id(row, i) == id) return i;
    std::string title = id.substr(0, id.find('|')) + "|";
    for (int i = 0; i < count; i++)
      if (card_id(row, i).compare(0, title.size(), title) == 0) return i;
    return std::max(0, std::min(fallback, count - 1));
  }

  void refresh_continue() {
    ctx().api.get("/api/progress", [this](const ApiResult &r) {
      if (!r.ok()) return;
      // Remember what the viewer was on by identity, not position: the title just
      // watched moves to the front of the row.
      bool on_continue = !hero_ && row_ == 0 && !rows_.empty() && rows_[0].continue_row();
      std::string was = on_continue ? card_id(rows_[0], cols_[0]) : "";
      std::string feature_was = featured_.empty() ? "" : featured_[static_cast<size_t>(feature_)].item.id;
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
        int count = static_cast<int>(items.size());
        cols_[0] = was.empty() ? std::min(cols_[0], count - 1) : find_card(rows_[0], was, cols_[0]);
        hscroll_[0].snap(static_cast<float>(std::min(cols_[0], std::max(0, count - 4))));
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
      for (int i = 0; i < static_cast<int>(featured_.size()); i++)
        if (featured_[static_cast<size_t>(i)].item.id == feature_was) feature_ = i;
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
    return focused_shown();
  }

  std::string spoken() const override {
    if (pairing_.active()) return pairing_.spoken();
    if (tabs_) return "";
    if (welcome_) return std::string("Welcome to Bloob. ") + (welcome_button_ == 0 ? "Connect now" : "Later") + ", button, " +
                         position(welcome_button_, 2);
    if (state_ != State::Ready) return state_ == State::Loading ? "Loading" : error_;
    if (hero_) {
      auto list = hero_buttons();
      if (list.empty()) return "";
      int i = std::min(button_, static_cast<int>(list.size()) - 1);
      return shown_.item.name + ". " + list[static_cast<size_t>(i)].label + ", button, " + position(i, static_cast<int>(list.size()));
    }
    if (rows_.empty()) return "";
    const Row &row = rows_[static_cast<size_t>(row_)];
    int col = cols_[static_cast<size_t>(row_)];
    Shown s = focused_shown();
    return row.title + ". " + s.item.name + ", " + position(col, static_cast<int>(row.size()));
  }

  Shown focused_shown() const {
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
    if (!shown_.item.logo.empty()) ctx().images.prefetch(shown_.item.logo, 760, 240, 0, Fit::Contain, false, 0, Relevance::Near);
    // Focus has rested here: fetch the title's details so Details opens complete.
    prefetch_meta(shown_.item);
  }

  // ---------------------------------------------------------------- actions

  std::vector<ButtonSpec> hero_buttons() const {
    std::vector<ButtonSpec> b;
    const Shown &s = shown_;
    if (s.has_resume && s.resume.kind == "upcoming") {
      b.push_back({"Episodes", -1});  // caught up: nothing to play yet
    } else if (s.has_resume) {
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

  void play_resume(const Resume &r) { start_playback(play::continue_item(r), hero_ ? PlayOrigin::HomeHero : PlayOrigin::HomeRow, false, true); }

  void hero_action(int index) {
    auto buttons = hero_buttons();
    if (index >= static_cast<int>(buttons.size())) return;
    const std::string &label = buttons[static_cast<size_t>(index)].label;
    if (label == "Details" || label == "Episodes") {
      ctx().push(make_details(shown_.item));
    } else if (shown_.has_resume) {
      play_resume(shown_.resume);
    } else {
      start_playback(play::film(shown_.item, {}, false), PlayOrigin::HomeHero, false, true);
    }
  }

  void open_focused() {
    Shown s = focused();
    if (s.has_resume && s.resume.kind != "upcoming") play_resume(s.resume);
    else ctx().push(make_details(s.item));
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    idle_since_ = ctx().now;
    if (pairing_.active()) return pairing_.key(key);
    if (welcome_) return welcome_key(key, repeat);
    if (state_ != State::Ready) {
      if (state_ == State::Error && key == Key::Ok) load();
      return key == Key::Ok;
    }
    if (hero_) return hero_key(key, repeat);
    return rows_key(key, repeat);
  }

  // ---------------------------------------------------------------- first run

  // Once, on Home, while nothing is paired: what AllDebrid is and why Bloob needs it.
  void maybe_welcome() {
    Ctx &c = ctx();
    if (welcome_checked_ || state_ != State::Ready || !c.settings_loaded || !c.account_known) return;
    welcome_checked_ = true;
    welcome_ = kind_ == Browse::Home && !c.paired && !c.welcome_seen;
    welcome_button_ = 0;
  }

  void dismiss_welcome() {
    welcome_ = false;
    ctx().welcome_seen = true;
    ctx().save_settings();
  }

  bool welcome_key(Key key, bool repeat) {
    if (key == Key::Left) welcome_button_ = 0;
    if (key == Key::Right) welcome_button_ = 1;
    if (key == Key::Back) dismiss_welcome();
    if (key == Key::Ok && !repeat) {
      bool connect = welcome_button_ == 0;
      dismiss_welcome();
      if (connect) pairing_.start([](bool) {});
    }
    return true;
  }

  void draw_welcome() {
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(rgba(0x000000), 140));
    int w = 1080, h = 500, x = (W - w) / 2, y = (H - h) / 2;
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.radius = glass_radius;
    backdrop_.glass(g);
    text(Text::Title3, "Watch with your AllDebrid account", x + 56, y + 52, label(Label::Primary), w - 112);
    int ty = paragraph(Text::Body,
                       "Bloob finds the versions of a film or episode and prepares the one you pick through AllDebrid, "
                       "a paid service you sign up for at alldebrid.com. Connect once with a short code; this TV remembers it.",
                       x + 56, y + 132, label(Label::Primary, 0.86f), w - 112, 4);
    text(Text::Caption1, "You can connect any time from Settings.", x + 56, ty + 14, label(Label::Secondary), w - 112);
    GlassButton connect, later;
    connect.label = "Connect now";
    connect.focused = welcome_button_ == 0;
    later.label = "Later";
    later.focused = welcome_button_ == 1;
    connect.variant = later.variant = ButtonStyle::Default;  // on a panel [EoA B11]
    int bx = x + 56, by = y + h - 124;
    bx += draw_glass_button(bx, by, connect) + theme::stack_gap;
    draw_glass_button(bx, by, later);
    (void)c;
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
    pairing_.update();
    maybe_welcome();
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
    if (state_ == State::Ready && hero_ && featured_.size() > 1 && !c.reduce_motion && !speech().on()) {  // no slideshow while focus is read aloud
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
      c.gfx.clear(plain);
      text_center(Text::Title3, "Couldn\xE2\x80\x99t load this page", W / 2, 400, label(Label::Primary));
      int y = 476;
      for (auto &l : wrap(font_for(Text::Body), error_, size_for(Text::Body), 960, 3)) {
        text_center(Text::Body, l, W / 2, y, label(Label::Secondary));
        y += line_for(Text::Body);
      }
      GlassButton b;
      b.label = "Try again";
      b.focused = !tabs_;
      draw_glass_button(W / 2 - glass_button_width(b) / 2, y + 44, b);
      return;
    }
    float m = mode_.value;
    backdrop_.draw(0.22f * m);
    // Scrims follow the art: bright pictures get a stronger wash behind the text.
    float s = scrim_for(backdrop_.luma());
    draw_hero_overlay(backdrop_.luma());
    draw_scrims(s, static_cast<int>(lerp(560, 330, m)), std::min(1.0f, 0.55f + 0.45f * s + 0.2f * m));
    draw_top(m);
    draw_rows(m);
    if (welcome_) draw_welcome();
    pairing_.draw();
  }

  void draw_skeleton() {
    Ctx &c = ctx();
    c.gfx.clear(plain);
    float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(c.now / 1600.0 * 6.2831853));
    // The artwork placeholder grey, 29,29,29 [EoA C14], in the shapes of the hero below.
    Color tone = mix(rgba(0x1d1d1d), rgba(0x262627), pulse);
    c.gfx.fill_round(X0, 327, kLogoW, kLogoH, 18, tone);
    c.gfx.fill_round(X0, 466, 420, 26, 13, tone);
    c.gfx.fill_round(X0, 516, kBodyW, 24, 12, tone);
    c.gfx.fill_round(X0, 548, 520, 24, 12, tone);
    c.gfx.fill_round(X0, 580, 460, 24, 12, tone);
    c.gfx.fill_round(X0, 644, theme::title_button_min_w, theme::button_h, theme::button_h / 2, tone);
    c.gfx.fill_round(X0 + theme::title_button_min_w + theme::title_button_gap, 644, theme::title_button_min_w, theme::button_h, theme::button_h / 2, tone);
    for (int i = 0; i < 5; i++) c.gfx.fill_round(X0 + i * STRIDE, kFirstShelf + kHeaderH + kHeaderToItems, card_w, card_h, card_radius, tone);
  }

  void draw_top(float m) {
    Ctx &c = ctx();
    const Shown &s = shown_;
    if (s.item.id.empty()) return;
    float t = text_in_.value;  // new title text eases in
    std::string meta = meta_line(s.item);
    if (s.has_resume && s.item.series()) {
      const Resume &r = s.resume;
      if (r.kind == "upcoming" && r.next)
        meta = episode_code(r.next->season, r.next->episode) + " arrives " + air_day_text(r.next->released, today_iso());
      else if (r.kind == "next" && r.next) meta = episode_code(r.next->season, r.next->episode) + " \xC2\xB7 " + r.next->title;
      else meta = join({episode_code(r.season, r.episode), r.episode_title});
    }
    // Hero positions, laid out from the bottom so the block always ends in the same place.
    int lines = std::max(1, static_cast<int>(wrap(font_for(Text::Caption1), s.item.description, size_for(Text::Caption1), kBodyW, kBodyLines).size()));
    int line = line_for(Text::Caption1);
    int hero_by = kFirstShelf - kBodyToButtons - theme::button_h;
    int hero_box_top = hero_by - kBodyToButtons - lines * line - kMetaToBody - line - kTitleToMeta - kLogoH;
    int box_top = static_cast<int>(lerp(static_cast<float>(hero_box_top), 140, m));
    int box_h = static_cast<int>(lerp(kLogoH, kLogoSmallH, m));
    int box_w = static_cast<int>(lerp(kLogoW, kLogoSmallW, m));

    // Why this title is here, in plain words: Emphasized.Headline, secondary [EoA H9].
    std::string why;
    if (s.has_resume) why = s.resume.kind == "upcoming" ? "You\xE2\x80\x99re up to date"
                            : s.resume.reason == "new" ? "New episode"
                            : s.resume.kind == "next" ? "Up next" : "Continue watching";
    else if (hero_) why = kind_ == Browse::Series ? "Popular series" : kind_ == Browse::Movies ? "Popular film" : (s.item.series() ? "Popular series" : "Popular film");
    float hero_only = std::max(0.0f, 1.0f - 2.0f * m);
    if (!why.empty() && hero_only > 0.01f)
      text(Text::Headline, why, X0, box_top - 18 - line_for(Text::Headline), label(Label::Secondary, hero_only * t));

    int y = draw_title(s.item, X0, box_top, box_w, box_h, t);
    y += kTitleToMeta;
    // Metadata Standard.Caption1 primary; description Standard.Caption1 secondary [EoA H6, H7].
    text(Text::Caption1, meta, X0, y, label(Label::Primary, t), 1100);
    y += line + kMetaToBody;
    paragraph(Text::Caption1, s.item.description, X0, y, label(Label::Secondary, t), kBodyW, m > 0.5f ? 2 : kBodyLines);

    if (hero_only > 0.01f) {
      int by = hero_by;
      int bx = X0;
      auto buttons = hero_buttons();
      for (size_t i = 0; i < buttons.size(); i++) {
        GlassButton b;
        b.label = buttons[i].label;
        b.icon = buttons[i].icon;
        b.focused = hero_ && !tabs_ && static_cast<int>(i) == button_;
        b.lift = button_lift_.value;
        b.alpha = hero_only;
        b.min_w = theme::title_button_min_w;
        b.max_w = theme::title_button_max_w;
        bx += draw_glass_button(bx, by, b) + theme::title_button_gap;
      }
      if (s.has_resume && s.resume.kind == "resume" && s.resume.duration > 0) {
        bx += 14;
        draw_progress(bx, by + theme::button_h / 2 - 3, 200, 5, static_cast<double>(s.resume.position) / s.resume.duration, fade(rgba(0xffffff), hero_only),
                      fade(with_alpha(rgba(0xffffff), 72), hero_only));
        text(Text::Caption2, minutes_left(s.resume.duration - s.resume.position), bx, by + theme::button_h / 2 + 8, label(Label::Secondary, hero_only));
      }
      // Page dots for the featured picks, right-aligned on the button line.
      int pages = static_cast<int>(featured_.size());
      int dw = 0;
      for (int i = 0; i < pages; i++) dw += (i == feature_ ? 28 : 8) + 10;
      int dx = W - safe_x - dw + 10;
      for (int i = 0; pages > 1 && i < pages; i++) {
        bool on = i == feature_;
        int w = on ? 28 : 8;
        c.gfx.fill_round(dx, by + 36, w, 8, 4, fade(on ? rgba(0xffffff) : with_alpha(rgba(0xffffff), 102), hero_only));
        dx += w + 10;
      }
    }
  }

  void draw_rows(float m) {
    float base = lerp(kFirstShelf, 500, m);
    for (size_t i = 0; i < rows_.size(); i++) {
      float y = base + (static_cast<float>(i) - vscroll_.value) * ROW_H;
      if (y > H || y < -ROW_H) continue;
      // Shelves that move above the focused one fade out under the title.
      float alpha = std::max(0.0f, std::min(1.0f, (y - 320.0f) / 170.0f));
      if (alpha <= 0.01f) continue;
      bool focused_row = !hero_ && static_cast<int>(i) == row_;
      int hy = static_cast<int>(y);
      // Header: Emphasized.Callout, secondary [EoA H12]; the focused shelf's is primary.
      text(Text::CalloutBold, rows_[i].title, X0, hy + kHeaderTop, label(focused_row ? Label::Primary : Label::Secondary, alpha));
      draw_shelf(i, hy + kHeaderH + kHeaderToItems, alpha, focused_row);
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
      c.images.prefetch(it.tile, card_w, card_h, card_radius, Fit::Cover, row.continue_row());
    }
    int focus_col = cols_[r];
    // Draw the focused card last so its lift and shadow sit above neighbours.
    std::vector<int> order;
    for (int j = first; j <= last; j++) if (!(focused_row && j == focus_col)) order.push_back(j);
    if (focused_row && focus_col >= first && focus_col <= last) order.push_back(focus_col);
    // Unfocused cards in passes (shadows, bodies, text) so same-texture draws
    // batch; the focused card last, whole, so its lift sits above (R5a, G6).
    int unfocused = static_cast<int>(order.size()) - (focused_row && focus_col >= first && focus_col <= last ? 1 : 0);
    const CardPass passes[] = {CardPass::Shadow, CardPass::Body, CardPass::Text};
    for (int p = 0; p < 3; p++)
      for (int k = 0; k < unfocused; k++) draw_shelf_card(r, order[static_cast<size_t>(k)], y, alpha, focused_row, passes[p]);
    for (int k = unfocused; k < static_cast<int>(order.size()); k++)
      draw_shelf_card(r, order[static_cast<size_t>(k)], y, alpha, focused_row, CardPass::All);
  }

  void draw_shelf_card(size_t r, int j, int y, float alpha, bool focused_row, CardPass pass) {
    const Row &row = rows_[r];
    float hs = hscroll_[r].value;
    int focus_col = cols_[r];
    {
      const Item &it = row.continue_row() ? row.resume[static_cast<size_t>(j)].item : row.items[static_cast<size_t>(j)];
      int x = X0 + static_cast<int>(std::lround((j - hs) * STRIDE));
      if (x > W || x + card_w < 0) return;
      bool focused = focused_row && j == focus_col && !tabs_;
      bool was = static_cast<int>(r) == prev_row_ && j == prev_col_;
      CardSpec cs;
      cs.art = it.tile;
      cs.fallback_art = it.poster;
      cs.title = it.name;
      cs.w = card_w;
      cs.h = card_h;
      cs.focus = focused ? lift_.value : was ? prev_lift_.value : 0;
      cs.focused = focused;
      cs.alpha = alpha;
      std::string sub;
      if (row.continue_row()) {
        const Resume &res = row.resume[static_cast<size_t>(j)];
        if (res.kind == "resume" && res.duration > 0) cs.progress = static_cast<double>(res.position) / res.duration;
        if (res.kind == "upcoming" && res.next) sub = episode_code(res.next->season, res.next->episode) + " " + air_day_text(res.next->released, today_iso());
        else if (res.kind == "next" && res.next) sub = (res.reason == "new" ? "New: " : "Next: ") + episode_code(res.next->season, res.next->episode);
        else if (res.item.series()) sub = join({episode_code(res.season, res.episode), minutes_left(res.duration - res.position)});
        else sub = minutes_left(res.duration - res.position);
        // Continue watching: what and how long, on the picture itself.
        cs.title_on_art = true;
        // A new episode or a date is news, so it shows without focus too.
        cs.sub = !res.item.series() || res.kind == "upcoming" || res.reason == "new" ? sub : "";
      }
      cs.pass = pass;
      draw_card(x, y, cs);
      if (focused && (pass == CardPass::All || pass == CardPass::Text)) {
        int ly = y + card_h + 24;  // 14 for the focused card's growth, then 10 [EoA K8]
        text(Text::Caption1, it.name, x, ly, label(Label::Primary, alpha), card_w + 40);
        std::string under = row.continue_row() ? sub : join({it.year, it.series() ? "Series" : "Film"});
        if (!under.empty()) text(Text::Caption2, under, x, ly + 34, label(Label::Secondary, alpha), card_w + 40);
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
  Anim mode_{page_rate}, vscroll_{page_rate}, lift_{focus_in_rate}, prev_lift_{focus_leave_rate}, text_in_{fade_rate, 1},
      button_lift_{focus_in_rate, 1};
  int prev_row_ = -1, prev_col_ = -1;
  Shown pending_, shown_;
  long long pending_at_ = -1;
  long long idle_since_ = 0;
  Backdrop backdrop_;
  Pairing pairing_;
  bool welcome_ = false, welcome_checked_ = false;
  int welcome_button_ = 0;
};

}  // namespace

std::unique_ptr<Screen> make_browse(Browse kind) { return std::make_unique<BrowseScreen>(kind); }
