#include "images.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "api.h"
#include "http.h"
#include "log.h"
#include "tasks.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wsign-compare"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "../third_party/stb/stb_image.h"
#pragma GCC diagnostic pop

namespace {

constexpr long long kFadeMs = 220;
constexpr long long kStaleMs = 2500;   // a queued image nobody drew for this long is skipped
constexpr long long kRetryMs = 30000;

// Premultiplied RGBA of source pixel (x, y).
inline void texel(const unsigned char *src, int sw, int x, int y, float out[4]) {
  const unsigned char *p = src + (static_cast<size_t>(y) * sw + x) * 4;
  float a = p[3] / 255.0f;
  out[0] = p[0] * a;
  out[1] = p[1] * a;
  out[2] = p[2] * a;
  out[3] = static_cast<float>(p[3]);
}

}  // namespace

Pixels scale_image(const unsigned char *rgba, int sw, int sh, int box_w, int box_h, Fit fit, int radius, bool shade) {
  Pixels result;
  if (sw <= 0 || sh <= 0 || box_w <= 0 || box_h <= 0) return result;
  double sx0 = 0, sy0 = 0, sx1 = sw, sy1 = sh;
  int dw = box_w, dh = box_h;
  if (fit == Fit::Cover) {
    double target = static_cast<double>(box_w) / box_h, aspect = static_cast<double>(sw) / sh;
    if (aspect > target) {  // wider: crop the sides
      double cw = sh * target;
      sx0 = (sw - cw) / 2;
      sx1 = sx0 + cw;
    } else {  // taller: crop top and bottom, favouring the upper part (faces, titles)
      double ch = sw / target;
      sy0 = (sh - ch) * 0.3;
      sy1 = sy0 + ch;
    }
  } else {
    double scale = std::min(static_cast<double>(box_w) / sw, static_cast<double>(box_h) / sh);
    dw = std::max(1, static_cast<int>(std::lround(sw * scale)));
    dh = std::max(1, static_cast<int>(std::lround(sh * scale)));
  }
  result.w = dw;
  result.h = dh;
  result.data.resize(static_cast<size_t>(dw) * dh * 4);
  const float r = static_cast<float>(std::min(radius, std::min(dw, dh) / 2));

  // Writes one premultiplied pixel, un-premultiplied, with rounded-corner coverage.
  auto emit = [&](int x, int y, const float px[4]) {
    unsigned char *p = reinterpret_cast<unsigned char *>(&result.data[(static_cast<size_t>(y) * dw + x) * 4]);
    float coverage = 1.0f;
    if (r > 0) {
      float fx = x + 0.5f, fy = y + 0.5f;
      float cx = fx < r ? r : (fx > dw - r ? dw - r : fx);
      float cy = fy < r ? r : (fy > dh - r ? dh - r : fy);
      float dx = fx - cx, dy = fy - cy;
      coverage = std::max(0.0f, std::min(1.0f, r - std::sqrt(dx * dx + dy * dy) + 0.5f));
    }
    float alpha = px[3] / 255.0f;
    float darken = 1.0f;
    if (shade) {
      // Bottom 55%: ease from untouched to 70% darker at the bottom edge.
      float t = (y + 0.5f) / dh;
      if (t > 0.45f) {
        float k = (t - 0.45f) / 0.55f;
        darken = 1.0f - 0.7f * k * k * (3 - 2 * k);
      }
    }
    for (int c = 0; c < 3; c++) {
      float v = (alpha > 0.0001f ? px[c] / alpha : 0.0f) * darken;
      p[c] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, v + 0.5f)));
    }
    p[3] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, px[3] * coverage + 0.5f)));
  };

  const double fx = (sx1 - sx0) / dw, fy = (sy1 - sy0) / dh;
  if (std::fabs(fx - 1.0) < 1e-6 && std::fabs(fy - 1.0) < 1e-6 && sx0 == std::floor(sx0) && sy0 == std::floor(sy0)) {
    // Already the right size (full-screen backdrops): copy.
    for (int y = 0; y < dh; y++) {
      const unsigned char *row = rgba + ((static_cast<size_t>(y) + static_cast<size_t>(sy0)) * sw + static_cast<size_t>(sx0)) * 4;
      if (r <= 0 && !shade) {
        std::copy(row, row + static_cast<size_t>(dw) * 4, reinterpret_cast<unsigned char *>(&result.data[static_cast<size_t>(y) * dw * 4]));
      } else {
        for (int x = 0; x < dw; x++) {
          float px[4];
          texel(row, 0, x, 0, px);
          emit(x, y, px);
        }
      }
    }
    return result;
  }
  if (fx <= 1.0 && fy <= 1.0) {
    // Upscaling: bilinear.
    for (int y = 0; y < dh; y++) {
      double v = sy0 + (y + 0.5) * fy - 0.5;
      int y0 = std::max(0, std::min(sh - 1, static_cast<int>(std::floor(v))));
      int y1 = std::min(sh - 1, y0 + 1);
      float ty = static_cast<float>(std::max(0.0, std::min(1.0, v - y0)));
      for (int x = 0; x < dw; x++) {
        double u = sx0 + (x + 0.5) * fx - 0.5;
        int x0 = std::max(0, std::min(sw - 1, static_cast<int>(std::floor(u))));
        int x1 = std::min(sw - 1, x0 + 1);
        float tx = static_cast<float>(std::max(0.0, std::min(1.0, u - x0)));
        float a[4], b[4], d[4], e[4], px[4];
        texel(rgba, sw, x0, y0, a);
        texel(rgba, sw, x1, y0, b);
        texel(rgba, sw, x0, y1, d);
        texel(rgba, sw, x1, y1, e);
        for (int c = 0; c < 4; c++) px[c] = (a[c] + (b[c] - a[c]) * tx) * (1 - ty) + (d[c] + (e[c] - d[c]) * tx) * ty;
        emit(x, y, px);
      }
    }
    return result;
  }
  // Downscaling: separable box filter with fractional edge weights. Only the
  // source rows inside the crop are filtered.
  int row0 = std::max(0, static_cast<int>(std::floor(sy0))), row1 = std::min(sh, static_cast<int>(std::ceil(sy1)));
  int rows_h = row1 - row0;
  std::vector<float> rows(static_cast<size_t>(dw) * rows_h * 4, 0.0f);
  for (int x = 0; x < dw; x++) {
    double a = sx0 + x * fx, b = a + fx;
    int i0 = std::max(0, static_cast<int>(std::floor(a))), i1 = std::min(sw, static_cast<int>(std::ceil(b)));
    for (int sy = row0; sy < row1; sy++) {
      float acc[4] = {0, 0, 0, 0};
      double total = 0;
      for (int i = i0; i < i1; i++) {
        double w = std::min(b, i + 1.0) - std::max(a, static_cast<double>(i));
        if (w <= 0) continue;
        float t[4];
        texel(rgba, sw, i, sy, t);
        for (int c = 0; c < 4; c++) acc[c] += static_cast<float>(t[c] * w);
        total += w;
      }
      float *o = &rows[(static_cast<size_t>(sy - row0) * dw + x) * 4];
      for (int c = 0; c < 4; c++) o[c] = total > 0 ? static_cast<float>(acc[c] / total) : 0;
    }
  }
  for (int y = 0; y < dh; y++) {
    double a = sy0 + y * fy, b = a + fy;
    int j0 = std::max(row0, static_cast<int>(std::floor(a))), j1 = std::min(row1, static_cast<int>(std::ceil(b)));
    for (int x = 0; x < dw; x++) {
      float acc[4] = {0, 0, 0, 0};
      double total = 0;
      for (int j = j0; j < j1; j++) {
        double w = std::min(b, j + 1.0) - std::max(a, static_cast<double>(j));
        if (w <= 0) continue;
        const float *p = &rows[(static_cast<size_t>(j - row0) * dw + x) * 4];
        for (int c = 0; c < 4; c++) acc[c] += static_cast<float>(p[c] * w);
        total += w;
      }
      float px[4];
      for (int c = 0; c < 4; c++) px[c] = total > 0 ? static_cast<float>(acc[c] / total) : 0;
      emit(x, y, px);
    }
  }
  return result;
}

void Images::init(SDL_Renderer *renderer, Tasks *tasks, Api *api) {
  renderer_ = renderer;
  tasks_ = tasks;
  api_ = api;
}

void Images::shutdown() {
  for (auto &e : entries_) if (e.second.texture) SDL_DestroyTexture(e.second.texture);
  entries_.clear();
  bytes_ = 0;
}

ImageRef Images::get(const std::string &url, int w, int h, int radius, Fit fit, bool shade) {
  ImageRef ref;
  if (url.empty() || w <= 0 || h <= 0) {
    ref.failed = true;
    return ref;
  }
  std::string key = std::to_string(w) + "x" + std::to_string(h) + "r" + std::to_string(radius) + (fit == Fit::Cover ? "c" : "f") +
                    (shade ? "s" : "") + url;
  Entry &e = entries_[key];
  e.used_at = now_;
  e.shared->wanted.store(now_);
  switch (e.state) {
    case Entry::Ready: {
      long long age = now_ - e.ready_at;
      ref.texture = e.texture;
      ref.w = e.w;
      ref.h = e.h;
      ref.alpha = age >= kFadeMs ? 255 : static_cast<Uint8>(255 * age / kFadeMs);
      if (age < kFadeMs) fading_ = true;
      return ref;
    }
    case Entry::Failed:
      if (now_ - e.failed_at < kRetryMs) {
        ref.failed = true;
        return ref;
      }
      e.state = Entry::Idle;
      // fall through
    case Entry::Idle:
      e.state = Entry::Loading;
      load(key, url, w, h, radius, fit, shade);
      return ref;
    case Entry::Loading:
      return ref;
  }
  return ref;
}

void Images::load(const std::string &key, const std::string &url, int w, int h, int radius, Fit fit, bool shade) {
  std::shared_ptr<Shared> shared = entries_[key].shared;
  Api *api = api_;
  tasks_->run(
      [=]() -> std::function<void()> {
        struct Result {
          Pixels pixels;
          bool stale = false;
          bool failed = false;
        };
        auto result = std::make_shared<Result>();
        // Skip work for an image that scrolled away before its turn came.
        auto now = [] {
          return static_cast<long long>(SDL_GetTicks());
        };
        if (now() - shared->wanted.load() > kStaleMs) {
          result->stale = true;
        } else {
          int status = 0;
          std::string bytes = api->fetch_bytes("/api/image?u=" + url_encode(url), &status);
          int sw = 0, sh = 0, channels = 0;
          unsigned char *decoded = bytes.empty() ? nullptr
              : stbi_load_from_memory(reinterpret_cast<const unsigned char *>(bytes.data()), static_cast<int>(bytes.size()), &sw, &sh, &channels, 4);
          if (!decoded) {
            result->failed = true;
            if (status != 404) logf("images", "image failed (status %d)", status);
          } else {
            result->pixels = scale_image(decoded, sw, sh, w, h, fit, radius, shade);
            stbi_image_free(decoded);
          }
        }
        return [this, key, result] {
          auto it = entries_.find(key);
          if (it == entries_.end()) return;
          Entry &e = it->second;
          if (result->stale) {
            e.state = Entry::Idle;
            return;
          }
          if (result->failed || result->pixels.w == 0) {
            e.state = Entry::Failed;
            e.failed_at = now_;
            return;
          }
          SDL_Texture *texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
                                                   result->pixels.w, result->pixels.h);
          if (!texture) {
            e.state = Entry::Failed;
            e.failed_at = now_;
            return;
          }
          SDL_UpdateTexture(texture, nullptr, result->pixels.data.data(), result->pixels.w * 4);
          SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
          e.texture = texture;
          e.w = result->pixels.w;
          e.h = result->pixels.h;
          e.bytes = result->pixels.data.size();
          e.state = Entry::Ready;
          // An image already on screen before its first frame fades in; one prefetched does not need to.
          e.ready_at = e.used_at >= now_ - 50 ? now_ : now_ - kFadeMs;
          bytes_ += e.bytes;
          loaded_ = true;
        };
      },
      alive_);
}

bool Images::end_frame() {
  bool fading = fading_;
  fading_ = false;
  if (bytes_ > budget_) {
    std::vector<std::pair<long long, std::string>> old;
    for (auto &e : entries_)
      if (e.second.state == Entry::Ready && e.second.used_at < now_ - 1000) old.push_back({e.second.used_at, e.first});
    std::sort(old.begin(), old.end());
    for (auto &o : old) {
      if (bytes_ <= budget_ * 3 / 4) break;
      Entry &e = entries_[o.second];
      SDL_DestroyTexture(e.texture);
      bytes_ -= e.bytes;
      entries_.erase(o.second);
    }
  }
  // Forget bookkeeping for images requested long ago and never loaded.
  if (entries_.size() > 2000) {
    for (auto it = entries_.begin(); it != entries_.end();) {
      if (it->second.state != Entry::Ready && it->second.state != Entry::Loading && it->second.used_at < now_ - 60000) it = entries_.erase(it);
      else ++it;
    }
  }
  bool any = loaded_;
  loaded_ = false;
  return fading || any;
}
