// Search: an on-screen keyboard on the left, results on the right that update
// as you type (after a short pause). Right from the keyboard's edge moves into
// the results; Left from their first column comes back; Up from the top row
// reaches the tab bar.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "http.h"
#include "log.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr int KX = margin, KW = 640;
constexpr int KCOLS = 6;
constexpr int KEY_H = 66, KEY_GAP = 10;
constexpr int RX = 840, RW = W - margin - RX;
constexpr int RCOLS = 3, RGAP = 32;
constexpr int RTW = (RW - (RCOLS - 1) * RGAP) / RCOLS;
constexpr int RTH = RTW * 9 / 16;
constexpr int RROW = RTH + 90;
constexpr int TOP = 170;
constexpr long long kDebounceMs = 450;

const char *kKeys = "abcdefghijklmnopqrstuvwxyz1234567890";

class SearchScreen : public Screen {
 public:
  const char *name() const override { return "search"; }
  bool root() const override { return true; }
  void set_tabs_focus(bool tabs) override { tabs_ = tabs; }

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
      if (results_.empty() && zone_ == Zone::Results) zone_ = Zone::Keys;
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
        else if (!results_.empty()) zone_ = Zone::Results;
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
          zone_ = Zone::Recent;
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
      case Key::Up: zone_ = Zone::Keys; return true;
      case Key::Ok:
        query_ = list[static_cast<size_t>(recent_index_)];
        run_search();
        zone_ = Zone::Keys;
        return true;
      case Key::Back: zone_ = Zone::Keys; return true;
      default: return true;
    }
  }

  bool results_key(Key key, bool repeat) {
    int n = static_cast<int>(results_.size());
    int col = result_ % RCOLS;
    switch (key) {
      case Key::Left:
        if (col == 0) zone_ = Zone::Keys;
        else result_--;
        break;
      case Key::Right:
        if (col + 1 < RCOLS && result_ + 1 < n) result_++;
        break;
      case Key::Up:
        if (result_ >= RCOLS) result_ -= RCOLS;
        break;
      case Key::Down:
        if (result_ + RCOLS < n) result_ += RCOLS;
        else if (result_ / RCOLS < (n - 1) / RCOLS) result_ = n - 1;
        break;
      case Key::Ok:
        if (!repeat && n) {
          remember(current_query());  // what the viewer actually used
          ctx().push(make_details(results_[static_cast<size_t>(result_)]));
        }
        return true;
      case Key::Back:
        zone_ = Zone::Keys;
        return true;
      default:
        return true;
    }
    int row = result_ / RCOLS;
    float target = scroll_.target;
    if (row < target) target = static_cast<float>(row);
    if (row > target + 2) target = static_cast<float>(row - 2);
    scroll_.to(target);
    return true;
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    c.show_tabs(true);
    if (due_at_ && c.now >= due_at_) run_search();
    if (due_at_) c.redraw_at(due_at_);
    if (lift_.step(dt) | scroll_.step(dt)) c.animating = true;
    if (zone_ == Zone::Keys && !tabs_) {
      caret_ = (c.now / 530) % 2 == 0;
      c.redraw_at((c.now / 530 + 1) * 530);
    }
  }

  void draw() override {
    Ctx &c = ctx();
    c.gfx.clear(ground);
    // Field.
    int iy = TOP;
    c.gfx.fill_round(KX, iy, KW, 84, 20, raised);
    c.icons.draw(Icon::Search, KX + 26, iy + 25, 34, tertiary);
    int qx = KX + 80;
    int qw = text(Text::Title2, query_, qx, iy + 21, primary, KW - 110);
    if (query_.empty()) text(Text::Title2, "Films and series", qx, iy + 21, tertiary);
    if (zone_ == Zone::Keys && !tabs_ && caret_) c.gfx.fill_rect(qx + qw + 4, iy + 22, 3, 42, accent);

    // Keyboard.
    int kw = (KW - (KCOLS - 1) * KEY_GAP) / KCOLS;
    int ky = iy + 120;
    for (int r = 0; r < key_rows(); r++)
      for (int k = 0; k < row_length(r); k++) {
        bool f = zone_ == Zone::Keys && !tabs_ && krow_ == r && kcol_ == k;
        draw_key(KX + k * (kw + KEY_GAP), ky + r * (KEY_H + KEY_GAP), kw, std::string(1, kKeys[r * KCOLS + k]), f);
      }
    int ay = ky + key_rows() * (KEY_H + KEY_GAP);
    int aw = (KW - 2 * KEY_GAP) / 3;
    const char *actions[3] = {"Space", "Delete", "Clear"};
    for (int k = 0; k < 3; k++) {
      bool f = zone_ == Zone::Keys && !tabs_ && krow_ == key_rows() && kcol_ == k;
      draw_key(KX + k * (aw + KEY_GAP), ay, aw, actions[k], f, true);
    }
    // Recent searches.
    auto &list = ctx().recent_searches;
    if (!list.empty()) {
      int ry = ay + KEY_H + 40;
      text(Text::Eyebrow, upper("Recent"), KX, ry, tertiary);
      int rx = KX;
      for (size_t i = 0; i < list.size(); i++) {
        int w = pill_width(list[i]);
        if (rx + w > KX + KW + 40) break;
        rx += draw_pill(rx, ry + 34, list[i], zone_ == Zone::Recent && static_cast<int>(i) == recent_index_ && !tabs_) + 12;
      }
    }
    draw_results();
  }

  void draw_key(int x, int y, int w, const std::string &label, bool focused, bool small = false) {
    Ctx &c = ctx();
    float s = focused ? 1.0f + 0.08f * lift_.value : 1.0f;
    int gw = static_cast<int>(w * s), gh = static_cast<int>(KEY_H * s);
    int gx = x - (gw - w) / 2, gy = y - (gh - KEY_H) / 2;
    if (focused) c.gfx.shadow(gx, gy, gw, gh, 14, 16, 8, 140);
    c.gfx.fill_round(gx, gy, gw, gh, 14, focused ? focus : raised);
    Text style = small ? Text::Callout : Text::Title2;
    text_center(style, label, gx + gw / 2, gy + (gh - line_for(style)) / 2, focused ? ink : primary);
  }

  void draw_results() {
    Ctx &c = ctx();
    std::string q = current_query();
    if (q.size() < 2) {
      text(Text::Title1, "Find something to watch", RX, TOP + 4, primary);
      paragraph(Text::Body, "Type a few letters of a film or series. Results appear as you type.", RX, TOP + 76, secondary, RW, 2);
      return;
    }
    text(Text::Title2, "Results for \xE2\x80\x9C" + q + "\xE2\x80\x9D", RX, TOP + 18, primary, RW - 80);
    if (searching_ || due_at_) draw_spinner(RX + RW - 24, TOP + 40, 34, secondary, searching_ ? searching_since_ : 0);
    if (!error_.empty()) {
      paragraph(Text::Body, error_, RX, TOP + 90, problem, RW, 3);
      return;
    }
    if (results_.empty()) {
      if (!searching_ && !due_at_ && searched_ == q) text(Text::Body, "Nothing found. Try another spelling.", RX, TOP + 90, tertiary);
      return;
    }
    int top = TOP + 100;
    float s = scroll_.value;
    std::vector<size_t> order;
    for (size_t i = 0; i < results_.size(); i++) if (static_cast<int>(i) != result_) order.push_back(i);
    if (result_ < static_cast<int>(results_.size())) order.push_back(static_cast<size_t>(result_));
    for (size_t i : order) {
      int row = static_cast<int>(i) / RCOLS, col = static_cast<int>(i) % RCOLS;
      float y = top + (row - s) * RROW;
      if (y > H || y + RROW < top - 40) continue;
      const Item &it = results_[i];
      int x = RX + col * (RTW + RGAP);
      bool f = zone_ == Zone::Results && !tabs_ && static_cast<int>(i) == result_;
      CardSpec cs;
      cs.art = it.tile;
      cs.fallback_art = it.poster;
      cs.title = it.name;
      cs.w = RTW;
      cs.h = RTH;
      cs.focus = f ? lift_.value : 0;
      cs.focused = f;
      draw_card(x, static_cast<int>(y), cs);
      int ly = static_cast<int>(y) + RTH + (f ? 20 : 14);
      text(Text::Caption, it.name, x, ly, f ? primary : secondary, RTW);
      text(Text::Footnote, join({it.series() ? "Series" : "Film", it.year}), x, ly + 30, tertiary, RTW);
    }
    (void)c;
  }

 private:
  enum class Zone { Keys, Recent, Results };
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
  Anim lift_{focus_rate, 1}, scroll_{scroll_rate};
};

}  // namespace

std::unique_ptr<Screen> make_search() { return std::make_unique<SearchScreen>(); }
