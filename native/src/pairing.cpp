#include "pairing.h"

#include "log.h"
#include "ui.h"

namespace {
constexpr long long kCheckMs = 5000;
}

void Pairing::start(std::function<void(bool)> done) {
  done_ = std::move(done);
  active_ = true;
  alive_ = std::make_shared<int>(0);
  request_pin();
}

void Pairing::cancel() {
  if (!active_) return;
  active_ = false;
  alive_.reset();
  auto done = done_;
  done_ = nullptr;
  if (done) done(false);
}

void Pairing::request_pin() {
  pin_.clear();
  error_.clear();
  checking_ = false;
  ctx().api.post("/api/pin/start", "{}", [this](const ApiResult &r) {
    if (!r.ok()) {
      error_ = r.error;
      return;
    }
    pin_ = r.json["pin"].as_string();
    url_ = r.json["url"].as_string();
    long long expires = r.json["expiresIn"].as_int(600);
    expires_at_ = ctx().now + expires * 1000;
    next_check_ = ctx().now + kCheckMs;
    logf("pairing", "PIN issued, expires in %llds", expires);
  }, alive_);
}

void Pairing::check() {
  checking_ = true;
  ctx().api.post("/api/pin/check", "{}", [this](const ApiResult &r) {
    checking_ = false;
    next_check_ = ctx().now + kCheckMs;
    if (!r.ok()) {
      // Expired PINs are renewed in update(); other errors show but keep trying.
      error_ = r.error;
      return;
    }
    error_.clear();
    if (r.json["paired"].as_bool()) {
      logf("pairing", "paired (storage=%s durable=%d)", r.json["storage"].as_string("none").c_str(), r.json["durable"].as_bool());
      ctx().paired = true;
      active_ = false;
      auto done = done_;
      done_ = nullptr;
      alive_.reset();
      if (done) done(true);
    }
  }, alive_);
}

bool Pairing::key(Key key) {
  if (!active_) return false;
  if (key == Key::Back || key == Key::Ok) cancel();
  return true;
}

void Pairing::update() {
  if (!active_) return;
  Ctx &c = ctx();
  if (!pin_.empty() && c.now >= expires_at_) {
    request_pin();  // renew silently
    return;
  }
  if (!pin_.empty() && !checking_ && c.now >= next_check_) check();
}

std::string Pairing::spoken() const {
  if (!active_) return "";
  if (!error_.empty()) return "Pair this TV once. " + error_ + ". Cancel, button";
  if (pin_.empty()) return "Pair this TV once. Getting a code";
  std::string spelled;
  for (char ch : pin_) spelled += std::string(spelled.empty() ? "" : " ") + ch;
  return "Pair this TV once. On your phone or computer, go to alldebrid.com/pin and enter " + spelled + ". Cancel, button";
}

// 4.1 Swiss pairing: the whole screen in black, the instruction as a headline,
// the code very large , the time left and the state beside it.
void Pairing::draw() {
  if (!active_) return;
  Ctx &c = ctx();
  using namespace theme;
  c.gfx.fill_rect(0, 0, W, H, rgba(0x000000, 245));
  int x = safe_x;
  text(Text::Caption1, "Connect AllDebrid, once", x, 80, label(Label::Secondary));
  Font &bold = c.fonts.bold;
  int hy = 130;
  for (auto &l : wrap(bold, "On your phone, open alldebrid.com/pin and enter this code.", 88, span_w(9), 3)) {
    bold.draw(l, x, hy, 88, rgba(0xffffff));
    hy += 92;
  }
  int ry = 560;
  if (pin_.empty()) {
    if (error_.empty()) {
      int bw = span_w(12) / 6;
      int pos = static_cast<int>(c.now % 1600 * (span_w(12) - bw) / 1600);
      c.gfx.fill_rect(x + pos, ry - 1, bw, 4, theme::signal);
      c.redraw_at(c.now + 16);
    }
  } else {
    bold.draw(pin_, x, ry + 20, 260, rgba(0xffffff));
    long long left = std::max(0LL, (expires_at_ - c.now) / 1000);
    std::string t = std::to_string(left / 60) + ":" + (left % 60 < 10 ? "0" : "") + std::to_string(left % 60);
    text(Text::Caption2, "New code in", col_x(7), ry + 40, label(Label::Secondary));
    bold.draw(t, col_x(7), ry + 70, 56, rgba(0xffffff));
    text(Text::Caption2, "Status", col_x(9), ry + 40, label(Label::Secondary));
    c.gfx.fill_rect(col_x(9), ry + 86, 14, 14, theme::signal);
    text(Text::Caption1, "Waiting for the code", col_x(9) + 26, ry + 76, label(Label::Primary), span_w(3) - 26);
    c.redraw_at((c.now / 1000 + 1) * 1000);
  }
  if (!error_.empty()) paragraph(Text::Callout, error_, x, ry + 60, theme::signal, span_w(9), 2);
  text(Text::Caption1, "Bloob keeps the connection on the TV. Your key never leaves it.", x, H - safe_y - 46, label(Label::Secondary), span_w(8));
  GlassButton cancel;
  cancel.label = "Cancel";
  cancel.focused = true;
  cancel.h = 60;
  draw_glass_button(W - safe_x - glass_button_width(cancel), H - safe_y - 60, cancel);
}
