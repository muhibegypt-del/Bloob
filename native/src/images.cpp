#include "images.h"
#include "theme.h"

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
constexpr long long kRetryMs = 30000;
// A request nobody drew for this many frames is retired (study A6).
constexpr long long kRetireFrames = 30;
// A texture drawn in the last few frames is never evicted (study M7: the
// reference's default minimum age is three frames).
constexpr long long kMinAgeFrames = 3;
// Upload pacing (study G12): per frame at most this many textures or bytes,
// highest relevance first; the first is always allowed.
constexpr int kMaxUploadsPerFrame = 6;
constexpr size_t kMaxUploadBytesPerFrame = 12u * 1024 * 1024;
// Visible work that cannot fit the budget waits this many frames, then goes
// over (logged): an empty card is worse than a brief overrun.
constexpr long long kOverrunAfterFrames = 60;
// Decoded source pictures kept on the CPU for further variants.
constexpr long long kSourceBudget = 48ll * 1024 * 1024;

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

  // Title-on-art cards: Elements of A blends light artwork darker than dark
  // artwork (0.55 versus 0.2; TOKENS.md C6). Light or dark is judged from the
  // part under the title, the lower 45% of the crop.
  float shade_strength = 0.0f;
  if (shade) {
    double sum = 0;
    long n = 0;
    int ys = static_cast<int>(sy0 + (sy1 - sy0) * 0.55), ye = static_cast<int>(sy1);
    int step = std::max(1, static_cast<int>((sx1 - sx0) / 48));
    for (int y = ys; y < ye; y += step)
      for (int x = static_cast<int>(sx0); x < static_cast<int>(sx1); x += step) {
        const unsigned char *q = rgba + (static_cast<size_t>(y) * sw + x) * 4;
        sum += (0.2126 * q[0] + 0.7152 * q[1] + 0.0722 * q[2]) / 255.0;
        n++;
      }
    shade_strength = n && sum / n > theme::card_scrim_light_luma ? theme::card_scrim_light : theme::card_scrim_dark;
  }

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
      // Bottom 55%: ease from untouched to the blend strength at the bottom edge.
      float t = (y + 0.5f) / dh;
      if (t > 0.45f) {
        float k = (t - 0.45f) / 0.55f;
        darken = 1.0f - shade_strength * k * k * (3 - 2 * k);
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

float text_luma(const Pixels &p) {
  if (p.w <= 0 || p.h <= 0) return -1;
  const unsigned char *d = reinterpret_cast<const unsigned char *>(p.data.data());
  double sum = 0;
  long n = 0;
  int x1 = p.w * 55 / 100, y0 = p.h * 30 / 100;
  for (int y = y0; y < p.h; y += 4)
    for (int x = 0; x < x1; x += 4) {
      const unsigned char *px = d + (static_cast<size_t>(y) * p.w + x) * 4;
      sum += 0.2126 * px[0] + 0.7152 * px[1] + 0.0722 * px[2];
      n++;
    }
  return n ? static_cast<float>(sum / n / 255.0) : -1.0f;
}

Pixels frost_image(const unsigned char *rgba, int sw, int sh, int screen_w, int screen_h, GlassRect rect, int radius) {
  Pixels out;
  if (rect.w <= 0 || rect.h <= 0) return out;
  // The art as the backdrop draws it, at quarter size, softly blurred.
  Pixels full = scale_image(rgba, sw, sh, std::max(1, screen_w / 4), std::max(1, screen_h / 4), Fit::Cover, 0, false);
  blur_image(full, 12);
  // Half-resolution crop under the rect, sampled bilinearly from the quarter-size blur.
  out.w = std::max(1, rect.w / 2);
  out.h = std::max(1, rect.h / 2);
  out.data.resize(static_cast<size_t>(out.w) * out.h * 4);
  const unsigned char *src = reinterpret_cast<const unsigned char *>(full.data.data());
  auto at = [&](int x, int y, int c) {
    x = std::max(0, std::min(full.w - 1, x));
    y = std::max(0, std::min(full.h - 1, y));
    return static_cast<float>(src[(static_cast<size_t>(y) * full.w + x) * 4 + c]);
  };
  const float r = static_cast<float>(std::min(radius / 2, std::min(out.w, out.h) / 2));
  // First the blurred, slightly richer colour under each pixel, and how bright it is overall.
  std::vector<float> col(static_cast<size_t>(out.w) * out.h * 3);
  double luma_sum = 0;
  for (int y = 0; y < out.h; y++)
    for (int x = 0; x < out.w; x++) {
      float u = (rect.x + (x + 0.5f) * 2) / 4.0f - 0.5f, v = (rect.y + (y + 0.5f) * 2) / 4.0f - 0.5f;
      int x0 = static_cast<int>(std::floor(u)), y0 = static_cast<int>(std::floor(v));
      float fx = u - x0, fy = v - y0;
      float c[3];
      for (int k = 0; k < 3; k++)
        c[k] = (at(x0, y0, k) * (1 - fx) + at(x0 + 1, y0, k) * fx) * (1 - fy) + (at(x0, y0 + 1, k) * (1 - fx) + at(x0 + 1, y0 + 1, k) * fx) * fy;
      float l = 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
      float *o = &col[(static_cast<size_t>(y) * out.w + x) * 3];
      for (int k = 0; k < 3; k++) o[k] = std::max(0.0f, std::min(255.0f, l + (c[k] - l) * theme::frost_saturation));  // a little richer
      luma_sum += l;
    }
  // White text must stay readable on it: bright areas are pressed down towards a
  // mid tone (a soft knee on brightness) and get less white wash, so bright art
  // makes darker glass, as Apple's materials do.
  (void)luma_sum;
  const float kKnee = 84.0f;
  for (int y = 0; y < out.h; y++) {
    // Light gathers along the top edge, as on a pane of glass.
    float ty = static_cast<float>(y) / out.h;
    float light = ty < 0.35f ? theme::frost_light * (1 - ty / 0.35f) * (1 - ty / 0.35f) : 0.0f;
    for (int x = 0; x < out.w; x++) {
      const float *c = &col[(static_cast<size_t>(y) * out.w + x) * 3];
      unsigned char *p = reinterpret_cast<unsigned char *>(&out.data[(static_cast<size_t>(y) * out.w + x) * 4]);
      float l = 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
      float dim = l > kKnee ? (kKnee + (l - kKnee) * 0.2f) / l : 1.0f;
      float bright = std::min(1.0f, std::max(0.0f, (l - kKnee) / 120.0f));
      float tint = (theme::frost_tint + light) * (1.0f - 0.6f * bright);
      for (int k = 0; k < 3; k++) {
        float v2 = c[k] * dim * (1 - tint) + 255.0f * tint;  // frosted white
        p[k] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, v2 + 0.5f)));
      }
      float coverage = 1.0f;
      if (r > 0) {
        float px = x + 0.5f, py = y + 0.5f;
        float cx = px < r ? r : (px > out.w - r ? out.w - r : px);
        float cy = py < r ? r : (py > out.h - r ? out.h - r : py);
        float dx = px - cx, dy = py - cy;
        coverage = std::max(0.0f, std::min(1.0f, r - std::sqrt(dx * dx + dy * dy) + 0.5f));
      }
      p[3] = static_cast<unsigned char>(255 * coverage + 0.5f);
    }
  }
  return out;
}

void blur_image(Pixels &p, int radius) {
  if (radius <= 0 || p.w <= 0 || p.h <= 0) return;
  std::vector<int> buf(static_cast<size_t>(p.w) * p.h * 4);
  unsigned char *d = reinterpret_cast<unsigned char *>(&p.data[0]);
  for (size_t i = 0; i < buf.size(); i++) buf[i] = d[i];
  std::vector<int> tmp(buf.size());
  auto pass = [&](bool horizontal) {
    int n = horizontal ? p.w : p.h, m = horizontal ? p.h : p.w;
    for (int line = 0; line < m; line++) {
      for (int c = 0; c < 4; c++) {
        auto at = [&](int i) -> int & {
          int x = horizontal ? i : line, y = horizontal ? line : i;
          return buf[(static_cast<size_t>(y) * p.w + x) * 4 + c];
        };
        int sum = 0;
        for (int i = -radius; i <= radius; i++) sum += at(std::max(0, std::min(n - 1, i)));
        for (int i = 0; i < n; i++) {
          int x = horizontal ? i : line, y = horizontal ? line : i;
          tmp[(static_cast<size_t>(y) * p.w + x) * 4 + c] = sum / (2 * radius + 1);
          sum += at(std::min(n - 1, i + radius + 1)) - at(std::max(0, i - radius));
        }
      }
    }
    buf.swap(tmp);
  };
  for (int k = 0; k < 3; k++) {
    pass(true);
    pass(false);
  }
  for (size_t i = 0; i < buf.size(); i++) d[i] = static_cast<unsigned char>(std::max(0, std::min(255, buf[i])));
}

// ---------------------------------------------------------------- the cache

// One decoded source picture, shared read-only by every variant made from it.
struct Images::Decoded {
  unsigned char *rgba = nullptr;
  int w = 0, h = 0;
  ~Decoded() {
    if (rgba) stbi_image_free(rgba);
    cpu_domains().decoded_sources -= static_cast<long long>(w) * h * 4;
  }
};

void Images::init(SDL_Renderer *renderer, Tasks *decode_tasks, Tasks *fetch_tasks, Api *api) {
  renderer_ = renderer;
  decode_tasks_ = decode_tasks;
  fetch_tasks_ = fetch_tasks;
  api_ = api;
}

void Images::shutdown() {
  for (auto &e : entries_)
    if (e.second.texture) {
      SDL_DestroyTexture(e.second.texture);
      gpu_budget().remove(GpuClass::Images, e.second.ev.bytes);
    }
  entries_.clear();
  sources_.clear();
  pending_.clear();
}

std::string Images::key_for(const std::string &url, int w, int h, int radius, Fit fit, bool shade, int blur) {
  return std::to_string(w) + "x" + std::to_string(h) + "r" + std::to_string(radius) + (fit == Fit::Cover ? "c" : "f") +
         (shade ? "s" : "") + (blur ? "b" + std::to_string(blur) : "") + url;
}

bool Images::settled(const std::string &url, int w, int h, int radius, Fit fit, bool shade, int blur) {
  if (url.empty()) return true;
  ImageRef ref = get(url, w, h, radius, fit, shade, blur);
  return ref.texture != nullptr || ref.failed;
}

ImageRef Images::get(const std::string &url, int w, int h, int radius, Fit fit, bool shade, int blur, Relevance relevance) {
  if (url.empty() || w <= 0 || h <= 0) {
    ImageRef ref;
    ref.failed = true;
    return ref;
  }
  return lookup(key_for(url, w, h, radius, fit, shade, blur), url,
                [=](const unsigned char *rgba, int sw, int sh) {
                  if (blur > 0) {
                    // Blurred art carries no detail: a quarter-size texture is enough.
                    Pixels p = scale_image(rgba, sw, sh, std::max(1, w / 4), std::max(1, h / 4), fit, 0, false);
                    blur_image(p, std::max(1, blur / 4));
                    return p;
                  }
                  return scale_image(rgba, sw, sh, w, h, fit, radius, shade);
                },
                relevance);
}

ImageRef Images::frost(const std::string &url, GlassRect rect, int radius) {
  if (url.empty() || rect.w <= 0 || rect.h <= 0) {
    ImageRef ref;
    ref.failed = true;
    return ref;
  }
  std::string key = "frost" + std::to_string(rect.x) + "," + std::to_string(rect.y) + "," + std::to_string(rect.w) + "," +
                    std::to_string(rect.h) + "r" + std::to_string(radius) + url;
  return lookup(
      key, url, [=](const unsigned char *rgba, int sw, int sh) { return frost_image(rgba, sw, sh, 1920, 1080, rect, radius); },
      Relevance::Visible);
}

ImageRef Images::lookup(const std::string &key, const std::string &url, const Make &make, Relevance relevance) {
  ImageRef ref;
  Entry &e = entries_[key];
  if (e.url.empty()) e.url = url;
  // The most relevant use this frame wins.
  if (e.ev.last_frame != frame_ || relevance < e.ev.relevance) e.ev.relevance = relevance;
  e.ev.last_frame = frame_;
  e.wanted->store(frame_);
  switch (e.state) {
    case Entry::Ready: {
      long long age = now_ - e.ready_at;
      ref.texture = e.texture;
      ref.w = e.w;
      ref.h = e.h;
      ref.luma = e.luma;
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
      e.make = make;
      e.state = Entry::Waiting;
      request_source(url, key, static_cast<int>(relevance));
      return ref;
    case Entry::Waiting: {
      // Still wanted: keep the source's fetch or decode from being retired.
      auto s = sources_.find(url);
      if (s != sources_.end()) {
        s->second.last_frame = frame_;
        s->second.wanted->store(frame_);
      }
      return ref;
    }
    case Entry::Making:
    case Entry::Uploading:
      return ref;
  }
  return ref;
}

void Images::request_source(const std::string &url, const std::string &key, int lane) {
  Source &s = sources_[url];
  s.last_frame = frame_;
  s.wanted->store(frame_);
  switch (s.state) {
    case Source::Ready:
      make_variant(key, s.decoded);
      return;
    case Source::Failed:
      if (now_ - s.failed_at < kRetryMs) {
        Entry &e = entries_[key];
        e.state = Entry::Failed;
        e.failed_at = s.failed_at;
        return;
      }
      s.state = Source::Idle;
      // fall through
    case Source::Idle:
      if (std::find(s.waiters.begin(), s.waiters.end(), key) == s.waiters.end()) s.waiters.push_back(key);
      s.lane = std::min(s.lane, lane);
      if (!suspended_) start_fetch(url);
      return;
    case Source::Fetching:
    case Source::Decoding:
      if (std::find(s.waiters.begin(), s.waiters.end(), key) == s.waiters.end()) s.waiters.push_back(key);
      s.lane = std::min(s.lane, lane);
      return;
  }
}

// Stage 1: fetch the bytes (I/O pool). Skipped if nobody wants the picture any more.
void Images::start_fetch(const std::string &url) {
  Source &s = sources_[url];
  s.state = Source::Fetching;
  fetches_++;
  Api *api = api_;
  auto wanted = s.wanted;
  auto frame = frame_atomic_;
  fetch_tasks_->run(
      [=]() -> std::function<void()> {
        if (frame->load() - wanted->load() > kRetireFrames) return [this, url] { source_failed(url, true); };
        int status = 0;
        auto bytes = std::make_shared<std::string>(api->fetch_bytes("/api/image?u=" + url_encode(url), &status));
        if (bytes->empty()) {
          if (status != 404) logf_limited("images-fetch", 10000, "images", "image fetch failed (status %d)", status);
          return [this, url] { source_failed(url, false); };
        }
        return [this, url, bytes] { decode(url, bytes); };
      },
      alive_, s.lane, true);
}

// Stage 2: decode once (CPU pool). Checked again: the fetch may have been slow.
void Images::decode(const std::string &url, std::shared_ptr<std::string> bytes) {
  auto it = sources_.find(url);
  if (it == sources_.end()) return;
  Source &s = it->second;
  s.state = Source::Decoding;
  decodes_++;
  auto wanted = s.wanted;
  auto frame = frame_atomic_;
  decode_tasks_->run(
      [=]() -> std::function<void()> {
        if (frame->load() - wanted->load() > kRetireFrames) return [this, url] { source_failed(url, true); };
        auto decoded = std::make_shared<Decoded>();
        int channels = 0;
        decoded->rgba = stbi_load_from_memory(reinterpret_cast<const unsigned char *>(bytes->data()),
                                              static_cast<int>(bytes->size()), &decoded->w, &decoded->h, &channels, 4);
        if (!decoded->rgba) {
          decoded->w = decoded->h = 0;
          logf_limited("images-decode", 10000, "images", "image decode failed");
          return [this, url] { source_failed(url, false); };
        }
        cpu_domains().decoded_sources += static_cast<long long>(decoded->w) * decoded->h * 4;
        std::shared_ptr<const Decoded> done = decoded;
        return [this, url, done] {
          auto found = sources_.find(url);
          if (found == sources_.end()) return;
          found->second.decoded = done;
          found->second.bytes = static_cast<long long>(done->w) * done->h * 4;
          source_ready(url);
        };
      },
      alive_, s.lane, true);
}

void Images::source_ready(const std::string &url) {
  Source &s = sources_[url];
  s.state = Source::Ready;
  std::vector<std::string> waiters;
  waiters.swap(s.waiters);
  s.lane = 2;
  for (auto &key : waiters) make_variant(key, s.decoded);
}

void Images::source_failed(const std::string &url, bool cancelled) {
  auto it = sources_.find(url);
  if (it == sources_.end()) return;
  Source &s = it->second;
  std::vector<std::string> waiters;
  waiters.swap(s.waiters);
  s.lane = 2;
  if (cancelled) {
    s.state = Source::Idle;
    cancelled_++;
  } else {
    s.state = Source::Failed;
    s.failed_at = now_;
  }
  for (auto &key : waiters) {
    auto e = entries_.find(key);
    if (e == entries_.end() || e->second.state != Entry::Waiting) continue;
    if (cancelled) {
      e->second.state = Entry::Idle;
    } else {
      e->second.state = Entry::Failed;
      e->second.failed_at = now_;
    }
  }
}

// Stage 3: scale, blur or frost from the shared decoded source (CPU pool).
void Images::make_variant(const std::string &key, const std::shared_ptr<const Decoded> &decoded) {
  auto it = entries_.find(key);
  if (it == entries_.end() || it->second.state != Entry::Waiting || !decoded) return;
  Entry &e = it->second;
  e.state = Entry::Making;
  variants_++;
  Make make = e.make;
  auto wanted = e.wanted;
  auto frame = frame_atomic_;
  decode_tasks_->run(
      [=]() -> std::function<void()> {
        if (frame->load() - wanted->load() > kRetireFrames) {
          return [this, key] {
            auto found = entries_.find(key);
            if (found != entries_.end() && found->second.state == Entry::Making) found->second.state = Entry::Idle;
            cancelled_++;
          };
        }
        auto pixels = std::make_shared<Pixels>(make(decoded->rgba, decoded->w, decoded->h));
        float luma = pixels->w >= 1280 ? text_luma(*pixels) : -1;  // full-screen art: adaptive scrims
        cpu_domains().pixels_in_flight += static_cast<long long>(pixels->data.size());
        return [this, key, pixels, luma] {
          auto found = entries_.find(key);
          if (found == entries_.end() || found->second.state != Entry::Making) {
            cpu_domains().pixels_in_flight -= static_cast<long long>(pixels->data.size());
            return;
          }
          Entry &x = found->second;
          if (pixels->w == 0) {
            cpu_domains().pixels_in_flight -= static_cast<long long>(pixels->data.size());
            x.state = Entry::Failed;
            x.failed_at = now_;
            return;
          }
          x.pending = pixels;
          x.luma = luma;
          x.state = Entry::Uploading;
          x.deferred_since = -1;
          pending_.push_back(key);
        };
      },
      alive_, static_cast<int>(e.ev.relevance), true);
}

void Images::evict(const std::string &key) {
  auto it = entries_.find(key);
  if (it == entries_.end() || !it->second.texture) return;
  Entry &e = it->second;
  SDL_DestroyTexture(e.texture);
  gpu_budget().remove(GpuClass::Images, e.ev.bytes);
  e.texture = nullptr;
  e.ev.resident = false;
  e.ev.bytes = 0;
  e.state = Entry::Idle;
  e.seen = true;
  evictions_++;
}

int Images::upload() {
  if (pending_.empty()) return 0;
  // Highest relevance first, then the most recently wanted.
  std::stable_sort(pending_.begin(), pending_.end(), [this](const std::string &a, const std::string &b) {
    auto ia = entries_.find(a), ib = entries_.find(b);
    if (ia == entries_.end() || ib == entries_.end()) return ia != entries_.end();
    const Entry &x = ia->second, &y = ib->second;
    if (x.ev.relevance != y.ev.relevance) return x.ev.relevance < y.ev.relevance;
    return x.ev.last_frame > y.ev.last_frame;
  });
  GpuBudget &budget = gpu_budget();
  int count = 0;
  size_t bytes_this_frame = 0;
  std::vector<std::string> keep;
  for (size_t i = 0; i < pending_.size(); i++) {
    const std::string key = pending_[i];
    auto it = entries_.find(key);
    if (it == entries_.end() || it->second.state != Entry::Uploading || !it->second.pending) continue;
    Entry &e = it->second;
    size_t estimate = gpu_bytes(e.pending->w, e.pending->h);
    if (count > 0 && (count >= kMaxUploadsPerFrame || bytes_this_frame + estimate > kMaxUploadBytesPerFrame)) {
      keep.push_back(key);
      continue;
    }
    // Admission before allocation: evict until it fits, one at a time.
    while (!budget.fits(estimate)) {
      std::string victim = pick_victim(entries_, frame_, kMinAgeFrames);
      if (victim.empty()) break;
      evict(victim);
    }
    if (!budget.fits(estimate)) {
      if (e.deferred_since < 0) e.deferred_since = frame_;
      bool overdue = e.ev.relevance == Relevance::Visible && frame_ - e.deferred_since >= kOverrunAfterFrames;
      if (!overdue) {
        deferred_++;
        keep.push_back(key);
        continue;
      }
      budget.overruns++;
      logf_limited("images-overrun", 10000, "images", "over the GPU budget for a visible image (%zu of %zu bytes)",
                   budget.used(), budget.limit());
    }
    SDL_Texture *texture =
        SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, e.pending->w, e.pending->h);
    if (!texture) {
      // Allocation failure: reclaim one more and retry once (study M4).
      std::string victim = pick_victim(entries_, frame_, kMinAgeFrames);
      if (!victim.empty()) evict(victim);
      texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, e.pending->w, e.pending->h);
    }
    if (!texture) {
      logf_limited("images-create", 10000, "images", "texture %dx%d failed: %s", e.pending->w, e.pending->h, SDL_GetError());
      deferred_++;
      keep.push_back(key);
      break;  // the GPU is out of room this frame
    }
    SDL_UpdateTexture(texture, nullptr, e.pending->data.data(), e.pending->w * 4);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    cpu_domains().pixels_in_flight -= static_cast<long long>(e.pending->data.size());
    e.texture = texture;
    e.w = e.pending->w;
    e.h = e.pending->h;
    e.pending.reset();
    e.ev.bytes = estimate;
    e.ev.resident = true;
    budget.add(GpuClass::Images, estimate);
    e.state = Entry::Ready;
    // An image on screen fades in; one prefetched, or seen before an eviction, does not.
    e.ready_at = !e.seen && e.ev.last_frame >= frame_ - 1 && e.ev.relevance == Relevance::Visible ? now_ : now_ - kFadeMs;
    e.seen = true;
    loaded_ = true;
    count++;
    bytes_this_frame += estimate;
    uploads_++;
    upload_bytes_ += static_cast<long long>(estimate);
  }
  pending_.swap(keep);
  return count;
}

bool Images::end_frame() {
  bool fading = fading_;
  fading_ = false;
  // Retire requests nobody drew for a while (study A6): queued work is skipped
  // by the workers; decoded pixels awaiting upload are dropped.
  for (auto &kv : entries_) {
    Entry &e = kv.second;
    if (frame_ - e.ev.last_frame <= kRetireFrames) continue;
    if (e.state == Entry::Uploading) {
      if (e.pending) cpu_domains().pixels_in_flight -= static_cast<long long>(e.pending->data.size());
      e.pending.reset();
      e.state = Entry::Idle;
      cancelled_++;
    } else if (e.state == Entry::Waiting) {
      auto s = sources_.find(e.url);
      if (s != sources_.end()) {
        auto &w = s->second.waiters;
        w.erase(std::remove(w.begin(), w.end(), kv.first), w.end());
      }
      e.state = Entry::Idle;
      cancelled_++;
    }
  }
  // Keep within the budget (a lowered budget is reached here, one by one).
  GpuBudget &budget = gpu_budget();
  while (budget.used() > budget.limit()) {
    std::string victim = pick_victim(entries_, frame_, kMinAgeFrames);
    if (victim.empty()) break;
    evict(victim);
  }
  // Decoded sources: keep recent ones for further variants, within a CPU budget.
  long long total = 0;
  for (auto &kv : sources_) total += kv.second.state == Source::Ready ? kv.second.bytes : 0;
  while (total > kSourceBudget) {
    const std::string *oldest = nullptr;
    long long oldest_frame = 0;
    for (auto &kv : sources_)
      if (kv.second.state == Source::Ready && (!oldest || kv.second.last_frame < oldest_frame)) {
        oldest = &kv.first;
        oldest_frame = kv.second.last_frame;
      }
    if (!oldest) break;
    total -= sources_[*oldest].bytes;
    drop_source(*oldest);
  }
  // Forget bookkeeping for images requested long ago and never loaded.
  if (entries_.size() > 2000) {
    for (auto it = entries_.begin(); it != entries_.end();) {
      const Entry &e = it->second;
      bool idle = e.state == Entry::Idle || e.state == Entry::Failed;
      if (idle && !e.texture && frame_ - e.ev.last_frame > 3600) it = entries_.erase(it);
      else ++it;
    }
  }
  if (sources_.size() > 400) {
    for (auto it = sources_.begin(); it != sources_.end();) {
      const Source &s = it->second;
      bool idle = s.state == Source::Idle || s.state == Source::Failed;
      if (idle && s.waiters.empty() && frame_ - s.last_frame > 3600) it = sources_.erase(it);
      else ++it;
    }
  }
  frame_++;
  frame_atomic_->store(frame_);
  bool any = loaded_;
  loaded_ = false;
  return fading || any;
}

void Images::drop_source(const std::string &url) {
  auto it = sources_.find(url);
  if (it == sources_.end()) return;
  it->second.decoded.reset();
  it->second.bytes = 0;
  if (it->second.state == Source::Ready) it->second.state = Source::Idle;
}

void Images::release_handles() {
  size_t released = 0;
  for (auto &kv : entries_) {
    Entry &e = kv.second;
    if (e.texture) {
      evict(kv.first);
      released++;
    }
    if (e.state == Entry::Uploading) {
      if (e.pending) cpu_domains().pixels_in_flight -= static_cast<long long>(e.pending->data.size());
      e.pending.reset();
      e.state = Entry::Idle;
    }
  }
  pending_.clear();
  for (auto &kv : sources_) drop_source(kv.first);
  logf("images", "released %zu textures and the decoded sources", released);
}

void Images::set_suspended(bool suspended) {
  if (suspended_ == suspended) return;
  suspended_ = suspended;
  if (suspended) return;
  // Back: start the fetches that waited, most relevant first.
  std::vector<std::pair<int, std::string>> waiting;
  for (auto &kv : sources_)
    if (kv.second.state == Source::Idle && !kv.second.waiters.empty()) waiting.push_back({kv.second.lane, kv.first});
  std::sort(waiting.begin(), waiting.end());
  for (auto &w : waiting) start_fetch(w.second);
}

Images::Stats Images::stats() const {
  Stats s;
  s.entries = entries_.size();
  for (auto &kv : entries_) {
    const Entry &e = kv.second;
    if (e.texture) {
      s.resident++;
      s.resident_bytes[static_cast<int>(e.ev.relevance)] += e.ev.bytes;
    }
    switch (e.state) {
      case Entry::Waiting: s.waiting++; break;
      case Entry::Making: s.making++; break;
      case Entry::Uploading: s.uploading++; break;
      case Entry::Failed: s.failed++; break;
      default: break;
    }
  }
  s.sources = sources_.size();
  for (auto &kv : sources_) {
    if (kv.second.state == Source::Ready) s.sources_ready++;
    if (kv.second.state == Source::Fetching) s.sources_fetching++;
    if (kv.second.state == Source::Decoding) s.sources_decoding++;
    s.source_bytes += kv.second.bytes;
  }
  s.uploads = uploads_;
  s.upload_bytes = upload_bytes_;
  s.deferred = deferred_;
  s.evictions = evictions_;
  s.cancelled = cancelled_;
  s.overruns = static_cast<long long>(gpu_budget().overruns);
  s.fetches = fetches_;
  s.decodes = decodes_;
  s.variants = variants_;
  s.frame = frame_;
  return s;
}
