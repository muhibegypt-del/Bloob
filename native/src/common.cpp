#include "common.h"

#include <cstdlib>

namespace {
constexpr int kBlur = 48;  // px at 1080p for the soft backdrop
}

void Backdrop::set(const std::string &url, bool immediate) {
  if (url == current_) return;
  // Whatever is on screen now stays underneath while the new picture fades in.
  if (!current_.empty() && ctx().images.get(current_, W, H).texture) previous_ = current_;
  current_ = url;
  if (immediate) {
    fade_.snap(1);
    previous_.clear();
  } else {
    fade_.snap(0);
    fade_.to(1);
  }
}

void Backdrop::update(double dt) {
  if (current_.empty()) return;
  Images &images = ctx().images;
  // A picture that never arrives leaves the previous one in place.
  ImageRef ref = images.get(current_, W, H);
  if (ref.failed) {
    if (!previous_.empty()) current_ = previous_;
    previous_.clear();
    fade_.snap(1);
    return;
  }
  // Hold the fade until the picture is decoded, so it never pops in half-drawn.
  if (images.settled(current_, W, H)) {
    if (fade_.step(dt)) ctx().animating = true;
    if (fade_.value >= 1.0f) previous_.clear();
  }
}

void Backdrop::draw(float dim, float blur) {
  Ctx &c = ctx();
  c.gfx.fill_rect(0, 0, W, H, theme::ground);
  auto layer = [&](const std::string &url, float alpha) {
    if (url.empty() || alpha <= 0.01f) return;
    if (blur < 0.99f) {
      ImageRef sharp = c.images.get(url, W, H);
      if (sharp.texture) c.gfx.texture(sharp.texture, 0, 0, W, H, static_cast<Uint8>(alpha * sharp.alpha));
    }
    if (blur > 0.01f) {
      ImageRef soft = c.images.get(url, W, H, 0, Fit::Cover, false, kBlur);
      if (soft.texture) c.gfx.texture(soft.texture, 0, 0, W, H, static_cast<Uint8>(alpha * blur * soft.alpha));
    }
  };
  layer(previous_, 1.0f);
  if (!current_.empty() && c.images.settled(current_, W, H)) layer(current_, fade_.value);
  if (dim > 0) c.gfx.fill_rect(0, 0, W, H, with_alpha(theme::ground, static_cast<int>(dim * 200)));
}

void draw_scrims(float left, int bottom_from, float bottom) {
  Gfx &g = ctx().gfx;
  auto a = [](float v) { return static_cast<Uint8>(std::max(0.0f, std::min(255.0f, v))); };
  if (left > 0) {
    // Dense where the text sits, gone by two-thirds across.
    g.gradient_h(0, 0, 420, H, theme::ground, a(240 * left), a(200 * left));
    g.gradient_h(420, 0, 520, H, theme::ground, a(200 * left), a(70 * left));
    g.gradient_h(940, 0, 360, H, theme::ground, a(70 * left), 0);
  }
  if (bottom > 0 && bottom_from < H) g.gradient_v(0, bottom_from, W, H - bottom_from, theme::ground, 0, a(255 * bottom));
}

int draw_title(const Item &item, int x, int top, int box_w, int box_h, float alpha, bool centred) {
  Ctx &c = ctx();
  if (!item.logo.empty()) {
    // Decoded once at the largest size used, then scaled: boxes animate, and a
    // new size per frame would mean a new decode per frame.
    ImageRef logo = c.images.get(item.logo, 760, 240, 0, Fit::Contain);
    if (logo.texture) {
      float scale = std::min(1.0f, std::min(static_cast<float>(box_w) / logo.w, static_cast<float>(box_h) / logo.h));
      int lw = static_cast<int>(logo.w * scale), lh = static_cast<int>(logo.h * scale);
      int lx = centred ? x + (box_w - lw) / 2 : x;
      c.gfx.texture(logo.texture, lx, top + box_h - lh, lw, lh, static_cast<Uint8>(std::min<float>(255 * alpha, logo.alpha)));
      return top + box_h;
    }
    if (!logo.failed) return top + box_h;  // still loading: hold the space rather than flash type
  }
  // Display type, sized down for long names.
  Font &f = c.fonts.bold;
  int size = box_h >= 200 ? 76 : box_h >= 140 ? 60 : 48;
  auto lines = wrap(f, item.name, size, box_w + 120, 2);
  if (lines.size() > 1 && size > 48) {
    size = size * 4 / 5;
    lines = wrap(f, item.name, size, box_w + 120, 2);
  }
  int line = size + size / 10;
  int y = top + box_h - static_cast<int>(lines.size()) * line;
  for (auto &l : lines) {
    int lx = centred ? x + (box_w - f.measure(l, size)) / 2 : x;
    f.draw(l, lx, y, size, fade(theme::primary, alpha));
    y += line;
  }
  return top + box_h;
}

std::string runtime_text(const std::string &runtime) {
  int minutes = std::atoi(runtime.c_str());
  if (minutes <= 0) return runtime;
  if (minutes < 60) return std::to_string(minutes) + " min";
  return std::to_string(minutes / 60) + "h " + std::to_string(minutes % 60) + "m";
}

std::string meta_line(const Item &item) {
  std::vector<std::string> parts;
  if (!item.year.empty()) parts.push_back(item.year);
  if (!item.genres.empty()) {
    std::string g = item.genres[0];
    if (item.genres.size() > 1) g += ", " + item.genres[1];
    parts.push_back(g);
  }
  if (item.series()) parts.push_back("Series");
  else if (!item.runtime.empty()) parts.push_back(runtime_text(item.runtime));
  return join(parts);
}
