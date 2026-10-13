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

void Pairing::draw() {
  if (!active_) return;
  Ctx &c = ctx();
  c.gfx.fill_rect(0, 0, W, H, with_alpha(theme::bg, 225));
  int w = 1040, h = 600, x = (W - w) / 2, y = (H - h) / 2;
  c.gfx.fill_round(x, y, w, h, 32, rgba(0x121418));
  c.fonts.bold.draw(upper("Connect AllDebrid"), x + 64, y + 60, 18, theme::text3);
  c.fonts.heavy.draw("Pair this TV once", x + 64, y + 94, 48, theme::text);
  draw_paragraph(c.fonts.regular, "On your phone or computer, open the address below and enter the code. Bloob keeps the connection on this TV.",
                 x + 64, y + 170, 24, 36, theme::text2, w - 128, 2);
  int box_y = y + 270;
  c.gfx.fill_round(x + 64, box_y, w - 128, 170, 24, rgba(0x1b1e23));
  c.fonts.regular.draw("alldebrid.com/pin", x + 104, box_y + 36, 30, theme::text2);
  if (pin_.empty()) {
    if (error_.empty()) draw_spinner(x + w - 200, box_y + 85, 48);
  } else {
    int pw = c.fonts.heavy.measure(pin_, 84);
    c.fonts.heavy.draw(pin_, x + w - 104 - pw, box_y + 36, 84, theme::accent);
    long long left = std::max(0LL, (expires_at_ - c.now) / 1000);
    c.fonts.regular.draw("Code renews in " + std::to_string(left / 60) + ":" + (left % 60 < 10 ? "0" : "") + std::to_string(left % 60),
                         x + 104, box_y + 96, 22, theme::text3);
    c.redraw_at((c.now / 1000 + 1) * 1000);  // countdown
  }
  if (!error_.empty()) draw_paragraph(c.fonts.regular, error_, x + 64, box_y + 190, 22, 32, theme::bad, w - 128, 2);
  else if (!pin_.empty()) {
    draw_spinner(x + 84, box_y + 214, 26, theme::text3);
    c.fonts.regular.draw("Waiting for you to enter the code\xE2\x80\xA6", x + 112, box_y + 201, 22, theme::text3);
  }
  draw_button(x + 64, y + h - 110, "Cancel", true, false, -1, 64);
}
