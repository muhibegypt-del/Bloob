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
  using namespace theme;
  c.gfx.fill_rect(0, 0, W, H, with_alpha(ground, 215));
  int w = 1000, h = 560, x = (W - w) / 2, y = (H - h) / 2;
  draw_panel(x, y, w, h);
  text(Text::Eyebrow, upper("Connect AllDebrid"), x + 64, y + 56, accent);
  text(Text::Title1, "Pair this TV once", x + 64, y + 88, primary);
  paragraph(Text::Body, "On your phone or computer, open the address below and enter the code. Bloob remembers the connection.",
            x + 64, y + 164, secondary, w - 128, 2);
  int by = y + 266;
  c.gfx.fill_round(x + 64, by, w - 128, 160, 22, with_alpha(raised2, 230));
  text(Text::Title2, "alldebrid.com/pin", x + 104, by + 36, primary);
  if (pin_.empty()) {
    if (error_.empty()) draw_spinner(x + w - 180, by + 80, 44);
  } else {
    Font &f = c.fonts.bold;
    int pw = f.measure(pin_, 84);
    f.draw(pin_, x + w - 104 - pw, by + 30, 84, accent);
    long long left = std::max(0LL, (expires_at_ - c.now) / 1000);
    text(Text::Caption, "New code in " + std::to_string(left / 60) + ":" + (left % 60 < 10 ? "0" : "") + std::to_string(left % 60),
         x + 104, by + 96, tertiary);
    c.redraw_at((c.now / 1000 + 1) * 1000);
  }
  if (!error_.empty()) {
    paragraph(Text::Callout, error_, x + 64, by + 186, problem, w - 128, 2);
  } else if (!pin_.empty()) {
    draw_spinner(x + 80, by + 202, 24, tertiary);
    text(Text::Callout, "Waiting for the code\xE2\x80\xA6", x + 106, by + 186, secondary);
  }
  ButtonSpec cancel{"Cancel", -1, true};
  cancel.h = 64;
  draw_button(x + w - 64 - button_width(cancel), y + h - 100, cancel);
}
