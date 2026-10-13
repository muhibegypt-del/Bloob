#include "common.h"

#include <cstdlib>

void Backdrop::set(const std::string &url) {
  if (url == current_) return;
  // Whatever is on screen now stays underneath while the new picture fades in.
  if (!current_.empty() && ctx().images.get(current_, W, H).texture) previous_ = current_;
  current_ = url;
  fade_.rate = 9.0f;
  fade_.snap(0);
  fade_.to(1);
}

void Backdrop::update(double dt) {
  ImageRef ref = current_.empty() ? ImageRef{} : ctx().images.get(current_, W, H);
  // Hold the fade until the picture exists, so it fades in rather than popping.
  if (ref.texture) {
    if (fade_.step(dt)) ctx().animating = true;
    if (fade_.value >= 1.0f && ref.alpha == 255) previous_.clear();
  }
}

void Backdrop::draw(float dim) {
  Ctx &c = ctx();
  c.gfx.fill_rect(0, 0, W, H, theme::bg);
  if (!previous_.empty()) {
    ImageRef prev = c.images.get(previous_, W, H);
    if (prev.texture) c.gfx.texture(prev.texture, 0, 0, W, H, 255);
  }
  if (!current_.empty()) {
    ImageRef ref = c.images.get(current_, W, H);
    if (ref.texture) {
      int a = std::min<int>(ref.alpha, static_cast<int>(fade_.value * 255));
      c.gfx.texture(ref.texture, 0, 0, W, H, static_cast<Uint8>(a));
    }
  }
  if (dim > 0) c.gfx.fill_rect(0, 0, W, H, with_alpha(theme::bg, static_cast<int>(dim * 150)));
}

void draw_scrims(float left, int bottom_from, float bottom) {
  Gfx &g = ctx().gfx;
  auto a = [](float v) { return static_cast<Uint8>(std::max(0.0f, std::min(255.0f, v))); };
  if (left > 0) {
    // Solid behind the rail and text, then easing out across the picture.
    g.fill_rect(0, 0, 330, H, with_alpha(theme::bg, static_cast<int>(255 * left)));
    g.gradient_h(330, 0, 420, H, theme::bg, a(255 * left), a(225 * left));
    g.gradient_h(750, 0, 600, H, theme::bg, a(225 * left), a(30 * left));
    g.gradient_h(1350, 0, 570, H, theme::bg, a(30 * left), 0);
  }
  if (bottom > 0 && bottom_from < H) {
    g.gradient_v(0, bottom_from, W, H - bottom_from, theme::bg, 0, a(255 * bottom));
  }
}

int draw_title(const Item &item, int x, int top, int box_w, int box_h, int text_size, float alpha) {
  Ctx &c = ctx();
  int a = static_cast<int>(255 * alpha);
  if (!item.logo.empty()) {
    ImageRef logo = c.images.get(item.logo, box_w, box_h, 0, Fit::Contain);
    if (logo.texture) {
      c.gfx.texture(logo.texture, x, top + box_h - logo.h, logo.w, logo.h, static_cast<Uint8>(std::min<int>(a, logo.alpha)));
      return top + box_h;
    }
    if (!logo.failed) return top + box_h;  // still loading: hold the space, avoid a text flash
  }
  int line = text_size + text_size / 12;
  auto lines = wrap(c.fonts.heavy, item.name, text_size, box_w + 160, 2);
  int y = top + box_h - static_cast<int>(lines.size()) * line;
  for (auto &l : lines) {
    c.fonts.heavy.draw(l, x, y, text_size, with_alpha(theme::text, a));
    y += line;
  }
  return top + box_h;
}

std::string runtime_text(const std::string &runtime) {
  // Cinemeta gives "145 min".
  int minutes = std::atoi(runtime.c_str());
  if (minutes <= 0) return runtime;
  if (minutes < 60) return std::to_string(minutes) + " min";
  return std::to_string(minutes / 60) + "h " + std::to_string(minutes % 60) + "m";
}

std::string meta_line(const Item &item) {
  std::vector<std::string> parts;
  if (!item.year.empty()) parts.push_back(item.year);
  if (item.series()) parts.push_back("Series");
  else if (!item.runtime.empty()) parts.push_back(runtime_text(item.runtime));
  if (!item.genres.empty()) {
    std::string g = item.genres[0];
    if (item.genres.size() > 1) g += ", " + item.genres[1];
    parts.push_back(g);
  }
  if (!item.rating.empty()) parts.push_back("IMDb " + item.rating);
  return join(parts);
}
