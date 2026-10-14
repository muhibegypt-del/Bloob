// Search: an on-screen keyboard on the left, results on the right that update
// as you type (after a short pause). Right from the keyboard's edge moves into
// the results, and the keyboard gives way: it slides off, the query stays as a
// chip, and results widen to four columns. Left from their first column, or
// Back, brings it back. Up from the top row reaches the tab bar.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "http.h"
#include "log.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr int KX = safe_x, KW = 640;
constexpr int KCOLS = 6;
constexpr int KEY_H = 66, KEY_GAP = 10;
constexpr int RX = 840;
constexpr int RGAP = g_card_gap;
constexpr int TOP = 170;
constexpr long long kDebounceMs = 450;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

const char *kKeys = "abcdefghijklmnopqrstuvwxyz1234567890";

class SearchScreen : public Screen {
 public:
  const char *name() const override { return "search"; }
  bool root() const override { return true; }
  void set_tabs_focus(bool tabs) override { tabs_ = tabs; }

  std::string spoken() const override {
    if (tabs_) return "";
    if (zone_ == Zone::Keys) {
      if (krow_ == key_rows()) {
        const char *actions[3] = {"Space", "Delete", "Clear"};
        return std::string(actions[kcol_]) + ", button";
      }
      return std::string(1, kKeys[krow_ * KCOLS + kcol_]) + (query_.empty() ? "" : ". Search: " + query_);
    }
    if (zone_ == Zone::Recent) {
      auto &list = ctx().recent_searches;
      if (list.empty()) return "";
      int i = std::min(recent_index_, static_cast<int>(list.size()) - 1);
      return "Recent search, " + list[static_cast<size_t>(i)] + ", " + position(i, static_cast<int>(list.size()));
    }
    if (results_.empty()) return "";
    const Item &it = results_[static_cast<size_t>(result_)];
    return it.name + ", " + (it.series() ? "series" : "film") + (it.year.empty() ? "" : " " + it.year) + ", " +
           position(result_, static_cast<int>(results_.size()));
  }

  enum class Zone { Keys, Recent, Results };

  // Results layout follows the keyboard: beside it in 3 columns, full width in 4.
  // Beside the keyboard in 3 columns; on its own in the tvOS 5-column grid.
  int results_x() const { return static_cast<int>(lerp(static_cast<float>(safe_x), static_cast<float>(RX), kb_.value)); }
  int results_w() const { return W - safe_x - results_x(); }
  int results_cols() const { return kb_.target > 0.5f ? 3 : 5; }
  int tile_w() const { return (results_w() - (results_cols() - 1) * RGAP) / results_cols(); }
  int tile_h() const { return tile_w() * 9 / 16; }
  int row_h() const { return tile_h() + 96; }

  void set_zone(Zone z) {
    zone_ = z;
    float k = z == Zone::Results ? 0.0f : 1.0f;
    if (ctx().reduce_motion) kb_.snap(k);
    else kb_.to(k);
    keep_result_visible();
  }

  void keep_result_visible() {
    int row = result_ / results_cols();
    float target = scroll_.target;
    if (row < target) target = static_cast<float>(row);
    if (row > target + 2) target = static_cast<float>(row - 2);
    scroll_.to(target);
  }

  int key_rows() const { return (static_cast<int>(std::string(kKeys).size()) + KCOLS - 1) / KCOLS; }
  int row_length(int row) const {
    if (row == key_rows()) return 3;  // Space, Delete, Clear
    int total = static_cast<int>(std::string(kKeys).size());
    return std::min(KCOLS, total - row * KCOLS);
  }

  void type(const std::string &s) {
    if (query_.size() >= 60) return;
    query_ += s;
    changed();
  }

  void changed() { due_at_ = ctx().now + kDebounceMs; }

  std::string current_query() const {
    std::string q = query_;
    while (!q.empty() && q.back() == ' ') q.pop_back();
    return q;
  }

  void run_search() {
    due_at_ = 0;
    std::string q = current_query();
    if (q.size() < 2) {
      results_.clear();
      searched_.clear();
      searching_ = false;
      return;
    }
    if (q == searched_) return;
    searching_ = true;
    searching_since_ = ctx().now;
    std::string asked = q;
    ctx().api.get("/api/search?q=" + url_encode(q), [this, asked](const ApiResult &r) {
      if (asked != current_query()) return;  // a newer query is on its way
      searching_ = false;
      searched_ = asked;
      error_ = r.ok() ? "" : r.error;
      results_.clear();
      if (r.ok())
        for (size_t i = 0; i < r.json["items"].size(); i++) results_.push_back(parse_item(r.json["items"][i]));
      result_ = std::min(result_, std::max(0, static_cast<int>(results_.size()) - 1));
      if (results_.empty() && zone_ == Zone::Results) set_zone(Zone::Keys);
    }, alive_);
  }

  void remember(const std::string &q) {
    auto &list = ctx().recent_searches;
    if (!list.empty() && list[0] == q) return;
    list.erase(std::remove(list.begin(), list.end(), q), list.end());
    list.insert(list.begin(), q);
    if (list.size() > 6) list.resize(6);
    ctx().save_settings();
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    lift_.snap(0);
    lift_.to(1);
    if (zone_ == Zone::Keys) return keyboard_key(key, repeat);
    if (zone_ == Zone::Recent) return recent_key(key);
    return results_key(key, repeat);
  }

  bool keyboard_key(Key key, bool repeat) {
    int rows = key_rows() + 1;
    switch (key) {
      case Key::Left:
        if (kcol_ > 0) kcol_--;
        return true;
      case Key::Right:
        if (kcol_ + 1 < row_length(krow_)) kcol_++;
        else if (!results_.empty()) set_zone(Zone::Results);
        return true;
      case Key::Up:
        if (krow_ == 0) return false;  // to the tab bar
        if (krow_ == key_rows()) kcol_ = kcol_ * KCOLS / 3 + 1;
        krow_--;
        kcol_ = std::min(kcol_, row_length(krow_) - 1);
        return true;
      case Key::Down:
        if (krow_ + 1 < rows) {
          krow_++;
          if (krow_ == key_rows()) kcol_ = std::min(2, kcol_ * 3 / KCOLS);
          else kcol_ = std::min(kcol_, row_length(krow_) - 1);
        } else if (!ctx().recent_searches.empty() && current_query().empty()) {
          set_zone(Zone::Recent);
          recent_index_ = 0;
        }
        return true;
      case Key::Ok:
        if (repeat && krow_ == key_rows() && kcol_ != 1) return true;
        press_key();
        return true;
      default:
        return false;  // Back: the tab bar
    }
  }

  void press_key() {
    if (krow_ == key_rows()) {
      if (kcol_ == 0 && !query_.empty() && query_.back() != ' ') type(" ");
      if (kcol_ == 1 && !query_.empty()) {
        query_.pop_back();
        changed();
      }
      if (kcol_ == 2) {
        query_.clear();
        changed();
      }
      return;
    }
    type(std::string(1, kKeys[krow_ * KCOLS + kcol_]));
  }

  bool recent_key(Key key) {
    auto &list = ctx().recent_searches;
    switch (key) {
      case Key::Left: recent_index_ = std::max(0, recent_index_ - 1); return true;
      case Key::Right: recent_index_ = std::min(static_cast<int>(list.size()) - 1, recent_index_ + 1); return true;
      case Key::Up: set_zone(Zone::Keys); return true;
      case Key::Ok:
        query_ = list[static_cast<size_t>(recent_index_)];
        run_search();
        set_zone(Zone::Keys);
        return true;
      case Key::Back: set_zone(Zone::Keys); return true;
      default: return true;
    }
  }

  bool results_key(Key key, bool repeat) {
    int n = static_cast<int>(results_.size());
    int cols = results_cols();
    int col = result_ % cols;
    switch (key) {
      case Key::Left:
        if (col == 0) set_zone(Zone::Keys);
        else result_--;
        break;
      case Key::Right:
        if (col + 1 < cols && result_ + 1 < n) result_++;
        break;
      case Key::Up:
        if (result_ >= cols) result_ -= cols;
        break;
      case Key::Down:
        if (result_ + cols < n) result_ += cols;
        else if (result_ / cols < (n - 1) / cols) result_ = n - 1;
        break;
      case Key::Ok:
        if (!repeat && n) {
          remember(current_query());  // what the viewer actually used
          ctx().push(make_details(results_[static_cast<size_t>(result_)]));
        }
        return true;
      case Key::Back:
        set_zone(Zone::Keys);
        return true;
      default:
        return true;
    }
    keep_result_visible();
    return true;
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    c.show_tabs(true);
    if (due_at_ && c.now >= due_at_) run_search();
    if (due_at_) c.redraw_at(due_at_);
    if (lift_.step(dt) | scroll_.step(dt) | kb_.step(dt)) c.animating = true;
    if (zone_ == Zone::Keys && !tabs_) {
      caret_ = (c.now / 530) % 2 == 0;
      c.redraw_at((c.now / 530 + 1) * 530);
    }
  }

  void draw() override {
    Ctx &c = ctx();
    c.gfx.clear(plain);
    float k = kb_.value;
    int kx = KX - static_cast<int>((1.0f - k) * (KW + 120));
    if (k > 0.02f) draw_keyboard(kx, k);
    // Browsing results: the query stays in view as a compact glass chip.
    if (k < 0.98f) {
      GlassButton chip;
      chip.label = query_;
      chip.icon = static_cast<int>(Icon::Search);
      chip.h = 60;
      chip.style = Text::TVCallout;
      chip.alpha = 1.0f - k;
      draw_glass_button(safe_x, TOP - 84, chip);
    }
    draw_results();
  }

  void draw_keyboard(int x0, float a) {
    Ctx &c = ctx();
    // The keyboard sits on one solid glass panel.
    int pad = 28, iy = TOP;
    int kw = (KW - (KCOLS - 1) * KEY_GAP) / KCOLS;
    int ky = iy + 120;
    int ay = ky + key_rows() * (KEY_H + KEY_GAP);
    GlassSpec g;
    g.x = x0 - pad;
    g.y = iy - pad;
    g.w = KW + 2 * pad;
    g.h = ay + KEY_H + pad - g.y;
    g.kind = GlassKind::Solid;
    g.alpha = a;
    draw_glass(g);
    // Field.
    c.gfx.fill_round(x0, iy, KW, 84, 42, fade(rgba(0xffffff, 26), a));
    c.icons.draw(Icon::Search, x0 + 26, iy + 25, 34, label(Label::Secondary, a));
    int qx = x0 + 80;
    int qy = iy + (84 - line_for(Text::TVTitle3)) / 2;
    int qw = text(Text::TVTitle3, query_, qx, qy, label(Label::Primary, a), KW - 110);
    if (query_.empty()) text(Text::TVTitle3, "Films and series", qx, qy, label(Label::Tertiary, a));
    if (zone_ == Zone::Keys && !tabs_ && caret_) c.gfx.fill_rect(qx + qw + 4, iy + 20, 3, 44, fade(rgba(0xffffff), a));

    // Keys.
    for (int r = 0; r < key_rows(); r++)
      for (int k = 0; k < row_length(r); k++) {
        bool f = zone_ == Zone::Keys && !tabs_ && krow_ == r && kcol_ == k;
        draw_key(x0 + k * (kw + KEY_GAP), ky + r * (KEY_H + KEY_GAP), kw, std::string(1, kKeys[r * KCOLS + k]), f, false, a);
      }
    int aw = (KW - 2 * KEY_GAP) / 3;
    const char *actions[3] = {"Space", "Delete", "Clear"};
    for (int k = 0; k < 3; k++) {
      bool f = zone_ == Zone::Keys && !tabs_ && krow_ == key_rows() && kcol_ == k;
      draw_key(x0 + k * (aw + KEY_GAP), ay, aw, actions[k], f, true, a);
    }
    // Recent searches, under the panel.
    auto &list = ctx().recent_searches;
    if (!list.empty()) {
      int ry = ay + KEY_H + pad + 36;
      text(Text::TVCaption1, "Recent", x0, ry, label(Label::Secondary, a));
      int rx = x0;
      for (size_t i = 0; i < list.size(); i++) {
        GlassButton b;
        b.label = list[i];
        b.h = 60;
        b.style = Text::TVCallout;
        b.focused = zone_ == Zone::Recent && static_cast<int>(i) == recent_index_ && !tabs_;
        b.lift = lift_.value;
        b.alpha = a;
        int w = glass_button_width(b);
        if (rx + w > x0 + KW + 40) break;
        rx += draw_glass_button(rx, ry + 40, b) + 12;
      }
    }
  }

  void draw_key(int x, int y, int w, const std::string &name, bool focused, bool small = false, float a = 1.0f) {
    Ctx &c = ctx();
    float s = focused && !c.reduce_motion ? 1.0f + 0.08f * lift_.value : 1.0f;
    int gw = static_cast<int>(w * s), gh = static_cast<int>(KEY_H * s);
    int gx = x - (gw - w) / 2, gy = y - (gh - KEY_H) / 2;
    if (focused) c.gfx.shadow(gx, gy, gw, gh, 16, 16, 8, static_cast<Uint8>(140 * a));
    c.gfx.fill_round(gx, gy, gw, gh, 16, fade(focused ? rgba(0xffffff) : rgba(0xffffff, 20), a));
    Text style = small ? Text::TVCallout : Text::TVTitle3;
    text_center(style, name, gx + gw / 2, gy + (gh - line_for(style)) / 2, focused ? fade(rgba(0x000000), a) : label(Label::Primary, a));
  }

  // Four states: landing (type something), waiting (old results dimmed),
  // results, and empty.
  void draw_results() {
    Ctx &c = ctx();
    std::string q = current_query();
    int rx = results_x(), rw = results_w(), cols = results_cols(), tw = tile_w(), th = tile_h(), rh = row_h();
    if (q.size() < 2) {
      text(Text::TVTitle2, "Find something to watch", rx, TOP + 4, label(Label::Primary));
      paragraph(Text::TVBody, "Type a few letters of a film or series. Results appear as you type.", rx, TOP + 84, label(Label::Secondary), rw, 2);
      if (!ctx().recent_searches.empty())
        paragraph(Text::TVCallout, "Or press Down from the keyboard to search again for something recent.", rx, TOP + 172, label(Label::Tertiary), rw, 2);
      return;
    }
    bool waiting = searching_ || due_at_;
    text(Text::TVTitle3, "Results for \xE2\x80\x9C" + q + "\xE2\x80\x9D", rx, TOP + 14, label(Label::Primary), rw - 80);
    if (waiting) draw_spinner(rx + rw - 24, TOP + 40, 34, label(Label::Secondary), searching_ ? searching_since_ : 0);
    if (!error_.empty()) {
      paragraph(Text::TVBody, error_, rx, TOP + 90, problem, rw, 3);
      return;
    }
    if (results_.empty()) {
      if (!waiting && searched_ == q) {
        paragraph(Text::TVBody, "No titles match \xE2\x80\x98" + q + "\xE2\x80\x99.", rx, TOP + 90, label(Label::Secondary), rw, 2);
        text(Text::TVCallout, "Try fewer letters, or another spelling.", rx, TOP + 140, label(Label::Tertiary), rw);
      }
      return;
    }
    int top = TOP + 100;
    float s = scroll_.value;
    std::vector<size_t> order;
    for (size_t i = 0; i < results_.size(); i++) if (static_cast<int>(i) != result_) order.push_back(i);
    if (result_ < static_cast<int>(results_.size())) order.push_back(static_cast<size_t>(result_));
    for (size_t i : order) {
      int row = static_cast<int>(i) / cols, col = static_cast<int>(i) % cols;
      float y = top + (row - s) * rh;
      if (y > H || y + rh < top - 40) continue;
      const Item &it = results_[i];
      int x = rx + col * (tw + RGAP);
      bool f = zone_ == Zone::Results && !tabs_ && static_cast<int>(i) == result_;
      CardSpec cs;
      cs.art = it.tile;
      cs.fallback_art = it.poster;
      cs.title = it.name;
      cs.w = tw;
      cs.h = th;
      cs.focus = f ? lift_.value : 0;
      cs.focused = f;
      cs.glass = true;
      draw_card(x, static_cast<int>(y), cs);
      int ly = static_cast<int>(y) + th + (f ? 22 : 14);
      text(Text::TVCaption1, it.name, x, ly, label(f ? Label::Primary : Label::Secondary), tw);
      text(Text::TVCaption2, join({it.series() ? "Series" : "Film", it.year}), x, ly + 34, label(Label::Tertiary), tw);
    }
    // Still showing the previous query's results while the new ones come.
    if (waiting && searched_ != q) c.gfx.fill_rect(rx - 20, top - 20, rw + 40, H - top + 20, with_alpha(plain, 130));
  }

 private:
  std::string query_, searched_, error_;
  std::vector<Item> results_;
  bool searching_ = false;
  long long searching_since_ = 0;
  long long due_at_ = 0;
  Zone zone_ = Zone::Keys;
  int krow_ = 0, kcol_ = 0;
  int result_ = 0;
  int recent_index_ = 0;
  bool tabs_ = false;
  bool caret_ = true;
  Anim lift_{focus_in_rate, 1}, scroll_{scroll_rate};
  Anim kb_{page_rate, 1};  // 1 = keyboard open, 0 = browsing results
};

}  // namespace

std::unique_ptr<Screen> make_search() { return std::make_unique<SearchScreen>(); }
