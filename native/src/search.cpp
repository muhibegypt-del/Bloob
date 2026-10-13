// Search: an on-screen keyboard on the left, results on the right that update
// as you type (after a short pause). Right from the keyboard's edge moves into
// the results; Left from the first column comes back.
#include <algorithm>
#include <cmath>

#include "common.h"
#include "http.h"
#include "log.h"
#include "screens.h"

namespace {

constexpr int CL = 212;
constexpr int KW = 640;
constexpr int KCOLS = 6;
constexpr int KEY_H = 64, KEY_GAP = 10;
constexpr int RX = 940, RW = W - 96 - RX;
constexpr int RCOLS = 3, RGAP = 28;
constexpr int RTW = (RW - (RCOLS - 1) * RGAP) / RCOLS;
constexpr int RTH = RTW * 9 / 16;
constexpr int RROW = RTH + 86;
constexpr long long kDebounceMs = 450;

const char *kKeys = "abcdefghijklmnopqrstuvwxyz1234567890";

class SearchScreen : public Screen {
 public:
  SearchScreen() {
    focus_.rate = 16;
    scroll_.rate = 12;
  }

  const char *name() const override { return "search"; }
  bool root() const override { return true; }
  void set_rail_focus(bool rail) override { rail_ = rail; }

  int key_rows() const { return (static_cast<int>(std::string(kKeys).size()) + KCOLS - 1) / KCOLS; }

  // Keyboard positions: rows of letters/digits, then one row of [Space, Delete, Clear].
  int row_length(int row) const {
    if (row == key_rows()) return 3;
    int total = static_cast<int>(std::string(kKeys).size());
    return std::min(KCOLS, total - row * KCOLS);
  }

  void type(const std::string &s) {
    if (query_.size() >= 60) return;
    query_ += s;
    changed();
  }

  void changed() {
    due_at_ = ctx().now + kDebounceMs;
  }

  void run_search() {
    due_at_ = 0;
    std::string q = query_;
    while (!q.empty() && q.back() == ' ') q.pop_back();
    if (q.size() < 2) {
      results_.clear();
      searched_.clear();
      searching_ = false;
      return;
    }
    if (q == searched_) return;
    searching_ = true;
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

  std::string current_query() const {
    std::string q = query_;
    while (!q.empty() && q.back() == ' ') q.pop_back();
    return q;
  }

  void remember(const std::string &q) {
    auto &list = recent();
    if (!list.empty() && list[0] == q) return;
    list.erase(std::remove(list.begin(), list.end(), q), list.end());
    list.insert(list.begin(), q);
    if (list.size() > 6) list.resize(6);
    ctx().save_settings();
  }

  static std::vector<std::string> &recent() { return ctx().recent_searches; }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    if (zone_ == Zone::Keys) return keyboard_key(key, repeat);
    if (zone_ == Zone::Recent) return recent_key(key);
    return results_key(key, repeat);
  }

  bool keyboard_key(Key key, bool repeat) {
    int rows = key_rows() + 1;
    switch (key) {
      case Key::Left:
        if (kcol_ == 0) return false;
        kcol_--;
        break;
      case Key::Right:
        if (kcol_ + 1 < row_length(krow_)) kcol_++;
        else if (!results_.empty()) {
          zone_ = Zone::Results;
          result_ = std::min(result_, static_cast<int>(results_.size()) - 1);
        }
        break;
      case Key::Up:
        if (krow_ > 0) {
          if (krow_ == key_rows()) kcol_ = kcol_ * KCOLS / 3 + 1;  // action row → the keys above it
          krow_--;
          kcol_ = std::min(kcol_, row_length(krow_) - 1);
        }
        break;
      case Key::Down:
        if (krow_ + 1 < rows) {
          krow_++;
          if (krow_ == key_rows()) kcol_ = std::min(2, kcol_ * 3 / KCOLS);  // into the action row
          else kcol_ = std::min(kcol_, row_length(krow_) - 1);
        } else if (!recent().empty() && query_.empty()) {
          zone_ = Zone::Recent;
          recent_index_ = 0;
        }
        break;
      case Key::Ok:
        if (repeat && krow_ == key_rows() && kcol_ != 1) return true;
        press_key();
        break;
      case Key::Back:
        return false;
      default:
        return false;
    }
    focus_.snap(0);
    focus_.to(1);
    return true;
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
    auto &list = recent();
    switch (key) {
      case Key::Left:
        if (recent_index_ == 0) return false;
        recent_index_--;
        break;
      case Key::Right: recent_index_ = std::min(static_cast<int>(list.size()) - 1, recent_index_ + 1); break;
      case Key::Up: zone_ = Zone::Keys; break;
      case Key::Ok:
        query_ = list[static_cast<size_t>(recent_index_)];
        run_search();
        zone_ = Zone::Keys;
        break;
      default: return false;
    }
    return true;
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
        return false;
    }
    int row = result_ / RCOLS;
    float target = scroll_.target;
    if (row < target) target = static_cast<float>(row);
    if (row > target + 2) target = static_cast<float>(row - 2);
    scroll_.to(target);
    focus_.snap(0);
    focus_.to(1);
    return true;
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    if (due_at_ && c.now >= due_at_) run_search();
    if (due_at_) c.redraw_at(due_at_);
    if (focus_.step(dt) | scroll_.step(dt)) c.animating = true;
    // Caret blink while the keyboard has focus.
    if (zone_ == Zone::Keys && !rail_) {
      caret_ = (c.now / 530) % 2 == 0;
      c.redraw_at((c.now / 530 + 1) * 530);
    }
  }

  void draw() override {
    Ctx &c = ctx();
    c.gfx.clear(theme::bg);
    c.gfx.fill_rect(0, 0, 900, H, rgba(0x0b0c0f));

    // Input.
    c.fonts.bold.draw(upper("Search films and series"), CL, 80, 18, theme::text3);
    int iy = 120;
    c.gfx.fill_round(CL, iy, KW, 84, 20, rgba(0x15171b));
    c.icons.draw(Icon::Search, CL + 26, iy + 25, 34, theme::text3);
    int qx = CL + 80;
    std::string shown = query_.empty() ? "" : query_;
    int qw = draw_text(c.fonts.bold, shown, qx, iy + 18, 38, theme::text, KW - 110);
    if (query_.empty()) c.fonts.bold.draw("Title", qx, iy + 18, 38, theme::text4);
    if (zone_ == Zone::Keys && !rail_ && caret_) c.gfx.fill_rect(qx + qw + 4, iy + 22, 3, 42, theme::accent);

    // Keyboard.
    int kw = (KW - (KCOLS - 1) * KEY_GAP) / KCOLS;
    int ky = 236;
    for (int r = 0; r < key_rows(); r++) {
      for (int k = 0; k < row_length(r); k++) {
        int x = CL + k * (kw + KEY_GAP), y = ky + r * (KEY_H + KEY_GAP);
        bool f = zone_ == Zone::Keys && !rail_ && krow_ == r && kcol_ == k;
        draw_key(x, y, kw, std::string(1, kKeys[r * KCOLS + k]), f);
      }
    }
    int ay = ky + key_rows() * (KEY_H + KEY_GAP);
    int aw = (KW - 2 * KEY_GAP) / 3;
    const char *actions[3] = {"Space", "Delete", "Clear"};
    for (int k = 0; k < 3; k++) {
      bool f = zone_ == Zone::Keys && !rail_ && krow_ == key_rows() && kcol_ == k;
      draw_key(CL + k * (aw + KEY_GAP), ay, aw, actions[k], f, 22);
    }
    // Recent searches.
    auto &list = recent();
    if (!list.empty()) {
      int ry = ay + KEY_H + 36;
      c.fonts.regular.draw("Recent", CL, ry + 14, 20, theme::text3);
      int rx = CL + 90;
      for (size_t i = 0; i < list.size(); i++) {
        int w = chip_width(list[i]);
        if (rx + w > CL + KW + 40) break;
        draw_chip(rx, ry, list[i], zone_ == Zone::Recent && static_cast<int>(i) == recent_index_ && !rail_, rgba(0x15171b), theme::text);
        rx += w + 10;
      }
    }

    draw_results();
  }

  void draw_key(int x, int y, int w, const std::string &label, bool focused, int size = 28) {
    Ctx &c = ctx();
    float s = focused ? 1.0f + 0.08f * focus_.value : 1.0f;
    int gw = static_cast<int>(w * s), gh = static_cast<int>(KEY_H * s);
    int gx = x - (gw - w) / 2, gy = y - (gh - KEY_H) / 2;
    c.gfx.fill_round(gx, gy, gw, gh, 14, focused ? theme::white : rgba(0x15171b));
    Font &f = c.fonts.bold;
    draw_text_center(f, label, gx + gw / 2, gy + (gh - f.line_height(size)) / 2, size, focused ? theme::ink : theme::text);
  }

  void draw_results() {
    Ctx &c = ctx();
    std::string q = current_query();
    if (q.size() < 2) {
      c.fonts.heavy.draw("Find something to watch", RX, 80, 40, theme::text);
      draw_paragraph(c.fonts.regular, "Type a few letters of a film or series. Results appear as you type.", RX, 140, 24, 36, theme::text3, RW, 2);
      return;
    }
    std::string heading = "Results for \xE2\x80\x9C" + q + "\xE2\x80\x9D";
    draw_text(c.fonts.heavy, heading, RX, 80, 36, theme::text, RW - 80);
    if (searching_ || due_at_) draw_spinner(RX + RW - 24, 102, 34);
    if (!error_.empty()) {
      draw_paragraph(c.fonts.regular, error_, RX, 150, 24, 36, theme::bad, RW, 3);
      return;
    }
    if (results_.empty()) {
      if (!searching_ && !due_at_ && searched_ == q) c.fonts.regular.draw("Nothing found. Try another spelling.", RX, 150, 24, theme::text3);
      return;
    }
    c.fonts.regular.draw("Films and series", RX, 134, 20, theme::text3);
    int top = 190;
    float s = scroll_.value;
    for (size_t i = 0; i < results_.size(); i++) {
      int row = static_cast<int>(i) / RCOLS, col = static_cast<int>(i) % RCOLS;
      float y = top + (row - s) * RROW;
      if (y > H || y + RROW < top - 30) continue;
      const Item &it = results_[i];
      int x = RX + col * (RTW + RGAP);
      bool f = zone_ == Zone::Results && !rail_ && static_cast<int>(i) == result_;
      float sc = f ? 1.0f + 0.05f * focus_.value : 1.0f;
      int w = static_cast<int>(RTW * sc), h = static_cast<int>(RTH * sc);
      int tx = x - (w - RTW) / 2, ty = static_cast<int>(y) - (h - RTH) / 2;
      ImageRef img = c.images.get(it.tile, RTW, RTH, 14, Fit::Cover, true);
      if (img.failed && !it.poster.empty()) img = c.images.get(it.poster, RTW, RTH, 14, Fit::Cover, true);
      c.gfx.fill_round(tx, ty, w, h, 14, theme::panel2);
      if (img.texture) c.gfx.texture(img.texture, tx, ty, w, h, img.alpha);
      draw_text(c.fonts.heavy, it.name, tx + 16, ty + h - 44, 22, theme::text, w - 32);
      if (f) draw_focus_ring(tx, ty, w, h, 14, focus_.value);
      draw_text(c.fonts.regular, join({it.series() ? "Series" : "Film", it.year}), x, static_cast<int>(y) + RTH + 14, 19, f ? theme::text2 : theme::text3, RTW);
    }
  }

 private:
  enum class Zone { Keys, Recent, Results };
  std::string query_, searched_, error_;
  std::vector<Item> results_;
  bool searching_ = false;
  long long due_at_ = 0;
  Zone zone_ = Zone::Keys;
  int krow_ = 0, kcol_ = 0;
  int result_ = 0;
  int recent_index_ = 0;
  bool rail_ = false;
  bool caret_ = true;
  Anim focus_, scroll_;
};

}  // namespace

std::unique_ptr<Screen> make_search() { return std::make_unique<SearchScreen>(); }
