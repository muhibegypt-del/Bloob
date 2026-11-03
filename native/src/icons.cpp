#include "icons.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Seg {
  float x0, y0, x1, y1;
};

struct Shape {
  std::vector<Seg> segs;
  float width = 1.8f;  // stroke width in grid units

  void poly(std::initializer_list<float> pts, bool closed = false) {
    std::vector<float> p(pts);
    for (size_t i = 0; i + 3 < p.size(); i += 2) segs.push_back({p[i], p[i + 1], p[i + 2], p[i + 3]});
    if (closed && p.size() >= 4) segs.push_back({p[p.size() - 2], p[p.size() - 1], p[0], p[1]});
  }
  void arc(float cx, float cy, float r, float a0, float a1) {
    const float rad = 3.14159265f / 180.0f;
    int steps = std::max(6, static_cast<int>(std::fabs(a1 - a0) / 8));
    for (int i = 0; i < steps; i++) {
      float t0 = (a0 + (a1 - a0) * i / steps) * rad, t1 = (a0 + (a1 - a0) * (i + 1) / steps) * rad;
      segs.push_back({cx + r * std::cos(t0), cy + r * std::sin(t0), cx + r * std::cos(t1), cy + r * std::sin(t1)});
    }
  }
  void circle(float cx, float cy, float r) { arc(cx, cy, r, 0, 360); }
};

Shape shape_for(Icon icon) {
  Shape s;
  switch (icon) {
    case Icon::Home:
      s.poly({3, 11.5f, 12, 4, 21, 11.5f});
      s.poly({5.5f, 9.8f, 5.5f, 20, 18.5f, 20, 18.5f, 9.8f});
      break;
    case Icon::Search:
      s.circle(10.5f, 10.5f, 6.5f);
      s.poly({15.5f, 15.5f, 20.5f, 20.5f});
      break;
    case Icon::Film:
      s.poly({4, 5, 20, 5, 20, 19, 4, 19}, true);
      s.poly({8, 5, 8, 19});
      s.poly({16, 5, 16, 19});
      s.poly({4, 9.7f, 8, 9.7f});
      s.poly({4, 14.3f, 8, 14.3f});
      s.poly({16, 9.7f, 20, 9.7f});
      s.poly({16, 14.3f, 20, 14.3f});
      break;
    case Icon::Tv:
      s.poly({3.5f, 7, 20.5f, 7, 20.5f, 18.5f, 3.5f, 18.5f}, true);
      s.poly({8.5f, 3, 12, 7, 15.5f, 3});
      break;
    case Icon::Settings: {
      s.circle(12, 12, 3.2f);
      for (int i = 0; i < 8; i++) {
        float a = i * 45.0f * 3.14159265f / 180.0f;
        s.segs.push_back({12 + 6.2f * std::cos(a), 12 + 6.2f * std::sin(a), 12 + 9.2f * std::cos(a), 12 + 9.2f * std::sin(a)});
      }
      s.circle(12, 12, 6.2f);
      break;
    }
    case Icon::Check:
      s.width = 2.4f;
      s.poly({5, 12.5f, 10, 17.5f, 19.5f, 7});
      break;
    case Icon::ChevronRight:
      s.width = 2.2f;
      s.poly({9, 5, 16, 12, 9, 19});
      break;
    case Icon::ChevronLeft:
      s.width = 2.2f;
      s.poly({15, 5, 8, 12, 15, 19});
      break;
    case Icon::ChevronDown:
      s.width = 2.2f;
      s.poly({5, 9, 12, 16, 19, 9});
      break;
    case Icon::Back10:
      s.arc(12, 12.5f, 8.5f, 200, 470);
      s.poly({3.2f, 5.2f, 4.2f, 9.6f, 8.6f, 8.8f});
      break;
    case Icon::Forward10:
      s.arc(12, 12.5f, 8.5f, -110, 160);
      s.poly({20.8f, 5.2f, 19.8f, 9.6f, 15.4f, 8.8f});
      break;
    case Icon::Audio:
      s.poly({4, 9.5f, 7.5f, 9.5f, 12, 5.5f, 12, 18.5f, 7.5f, 14.5f, 4, 14.5f}, true);
      s.arc(13, 12, 4, -50, 50);
      s.arc(13, 12, 7.5f, -50, 50);
      break;
    case Icon::Subtitles:
      s.poly({3, 5.5f, 21, 5.5f, 21, 18.5f, 3, 18.5f}, true);
      s.poly({6.5f, 12.5f, 11, 12.5f});
      s.poly({13.5f, 12.5f, 17.5f, 12.5f});
      s.poly({6.5f, 15.3f, 14, 15.3f});
      break;
    case Icon::Spinner:
      s.width = 2.4f;
      s.arc(12, 12, 9, 0, 270);
      break;
    case Icon::Restart:
      s.arc(12, 13, 7.5f, 200, 500);
      s.poly({3.5f, 6.5f, 4.6f, 10.8f, 9, 10});
      break;
    case Icon::Next:
      s.poly({5, 5, 15, 12, 5, 19}, true);
      s.poly({19, 5, 19, 19});
      break;
    case Icon::Link:
      s.arc(8.5f, 15.5f, 3.6f, 45, 315);
      s.arc(15.5f, 8.5f, 3.6f, 225, 495);
      s.poly({9.5f, 14.5f, 14.5f, 9.5f});
      break;
    case Icon::Warning:
      s.poly({12, 3.5f, 21, 19.5f, 3, 19.5f}, true);
      s.poly({12, 9.5f, 12, 14});
      s.poly({12, 16.6f, 12, 16.8f});
      break;
    case Icon::Info:
      s.circle(12, 12, 9);
      s.poly({12, 11, 12, 16.5f});
      s.poly({12, 7.6f, 12, 7.8f});
      break;
    case Icon::Close:
      s.width = 2.2f;
      s.poly({6, 6, 18, 18});
      s.poly({18, 6, 6, 18});
      break;
    case Icon::Bolt:
      s.poly({13, 3, 5, 13.5f, 11.5f, 13.5f, 10.5f, 21, 19, 10, 12.5f, 10}, true);
      break;
  }
  return s;
}

float seg_distance(float px, float py, const Seg &s) {
  float dx = s.x1 - s.x0, dy = s.y1 - s.y0;
  float len2 = dx * dx + dy * dy;
  float t = len2 > 0 ? std::max(0.0f, std::min(1.0f, ((px - s.x0) * dx + (py - s.y0) * dy) / len2)) : 0;
  float ex = s.x0 + t * dx - px, ey = s.y0 + t * dy - py;
  return std::sqrt(ex * ex + ey * ey);
}

}  // namespace

void Icons::shutdown() { cache_.clear(); }

SDL_Texture *Icons::texture(Icon icon, int size) {
  auto key = std::make_pair(static_cast<int>(icon), size);
  if (SDL_Texture *found = cache_.find(key)) return found;
  Shape shape = shape_for(icon);
  const float scale = size / 24.0f;
  // Strokes never thinner than ~1.6 px so small icons stay legible.
  const float half = std::max(0.8f, shape.width * scale / 2);
  std::vector<Uint32> pixels(static_cast<size_t>(size) * size);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      float px = (x + 0.5f) / scale, py = (y + 0.5f) / scale;
      float best = 1e9f;
      for (auto &s : shape.segs) best = std::min(best, seg_distance(px, py, s));
      float coverage = std::max(0.0f, std::min(1.0f, half - best * scale + 0.5f));
      pixels[static_cast<size_t>(y) * size + x] = (static_cast<Uint32>(coverage * 255.0f + 0.5f) << 24) | 0x00FFFFFF;
    }
  }
  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels.data(), size, size, 32, size * 4, 0x00FF0000, 0x0000FF00,
                                                  0x000000FF, 0xFF000000);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
  SDL_FreeSurface(surface);
  if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  return cache_.put(key, texture, size, size);
}

void Icons::draw(Icon icon, int x, int y, int size, Color color) {
  SDL_Texture *t = texture(icon, size);
  if (!t) return;
  SDL_SetTextureColorMod(t, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(t, color.a);
  SDL_Rect dst{x, y, size, size};
  SDL_RenderCopy(renderer_, t, nullptr, &dst);
}

void Icons::spinner(int cx, int cy, int size, double angle, Color color) {
  SDL_Texture *t = texture(Icon::Spinner, size);
  if (!t) return;
  SDL_SetTextureColorMod(t, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(t, color.a);
  SDL_Rect dst{cx - size / 2, cy - size / 2, size, size};
  SDL_RenderCopyEx(renderer_, t, nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
}
