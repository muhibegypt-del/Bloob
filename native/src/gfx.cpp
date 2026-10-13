#include "gfx.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include "log.h"

// Vendored single-header library: silence its unused-function warnings.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../third_party/stb/stb_truetype.h"
#pragma GCC diagnostic pop

std::vector<unsigned> utf8_decode(const std::string &text) {
  std::vector<unsigned> out;
  for (size_t i = 0; i < text.size();) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    unsigned cp;
    int extra;
    if (c < 0x80) { cp = c; extra = 0; }
    else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
    else { i++; continue; }
    if (extra && i + static_cast<size_t>(extra) >= text.size()) break;  // truncated sequence
    for (int k = 1; k <= extra; k++) cp = (cp << 6) | (static_cast<unsigned char>(text[i + static_cast<size_t>(k)]) & 0x3F);
    out.push_back(cp);
    i += static_cast<size_t>(extra) + 1;
  }
  return out;
}

// Code points baked into every atlas: Latin-1 plus the punctuation and arrows the UI uses.
static std::vector<unsigned> atlas_codepoints() {
  std::vector<unsigned> cps;
  for (unsigned c = 32; c < 127; c++) cps.push_back(c);
  for (unsigned c = 160; c < 256; c++) cps.push_back(c);
  const unsigned extra[] = {0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x2212,
                            0x2190, 0x2191, 0x2192, 0x2193, 0x00D7, 0x2715, 0x2713};
  cps.insert(cps.end(), std::begin(extra), std::end(extra));
  return cps;
}

Font::~Font() {
  for (auto &entry : atlases_) SDL_DestroyTexture(entry.second.texture);
  delete static_cast<stbtt_fontinfo *>(info_);
}

bool Font::load(SDL_Renderer *renderer, const std::string &path) {
  renderer_ = renderer;
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    logf("gfx", "font missing: %s", path.c_str());
    return false;
  }
  data_.assign(std::istreambuf_iterator<char>(file), {});
  auto *info = new stbtt_fontinfo;
  if (!stbtt_InitFont(info, data_.data(), stbtt_GetFontOffsetForIndex(data_.data(), 0))) {
    delete info;
    logf("gfx", "font unreadable: %s", path.c_str());
    return false;
  }
  info_ = info;
  return true;
}

Font::Atlas *Font::atlas(int size) {
  auto found = atlases_.find(size);
  if (found != atlases_.end()) return &found->second;
  if (!info_) return nullptr;
  auto *info = static_cast<stbtt_fontinfo *>(info_);

  Atlas atlas;
  atlas.scale = stbtt_ScaleForPixelHeight(info, static_cast<float>(size));
  int ascent, descent, gap;
  stbtt_GetFontVMetrics(info, &ascent, &descent, &gap);
  atlas.ascent = ascent * atlas.scale;
  atlas.line = (ascent - descent + gap) * atlas.scale;

  std::vector<unsigned> cps = atlas_codepoints();
  std::vector<stbtt_packedchar> packed(cps.size());
  std::vector<int> cp_ints(cps.begin(), cps.end());
  // Smallest atlas that holds every glyph at this size (most sizes fit 512 or 1024).
  int dim = 256;
  std::vector<unsigned char> pixels;
  for (;; dim *= 2) {
    pixels.assign(static_cast<size_t>(dim) * dim, 0);
    stbtt_pack_context pack{};
    stbtt_PackBegin(&pack, pixels.data(), dim, dim, 0, 1, nullptr);
    // 2× horizontal oversampling keeps stems even at text sizes; large display
    // type does not need it.
    stbtt_PackSetOversampling(&pack, size <= 48 ? 2 : 1, 1);
    stbtt_pack_range range{};
    range.font_size = static_cast<float>(size);
    range.array_of_unicode_codepoints = cp_ints.data();
    range.num_chars = static_cast<int>(cps.size());
    range.chardata_for_range = packed.data();
    int ok = stbtt_PackFontRanges(&pack, data_.data(), 0, &range, 1);
    stbtt_PackEnd(&pack);
    if (ok || dim >= 4096) break;
  }

  // White glyphs with coverage in alpha; colour comes from texture colour mod.
  // Coverage is lifted slightly (gamma) because blending is done in linear
  // space: light text on dark grounds otherwise looks thin and stepped.
  Uint8 lift[256];
  for (int i = 0; i < 256; i++) lift[i] = static_cast<Uint8>(std::lround(255.0 * std::pow(i / 255.0, 1.0 / 1.35)));
  std::vector<Uint32> rgba_pixels(pixels.size());
  for (size_t i = 0; i < pixels.size(); i++) rgba_pixels[i] = (static_cast<Uint32>(lift[pixels[i]]) << 24) | 0x00FFFFFF;
  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(rgba_pixels.data(), dim, dim, 32, dim * 4, 0x00FF0000,
                                                  0x0000FF00, 0x000000FF, 0xFF000000);
  atlas.texture = SDL_CreateTextureFromSurface(renderer_, surface);
  SDL_FreeSurface(surface);
  SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);

  for (size_t i = 0; i < cps.size(); i++) {
    const stbtt_packedchar &p = packed[i];
    atlas.glyphs[cps[i]] = {{p.x0, p.y0, p.x1 - p.x0, p.y1 - p.y0}, p.xoff, p.yoff, p.xoff2, p.yoff2, p.xadvance};
  }
  return &(atlases_[size] = std::move(atlas));
}

int Font::line_height(int size) {
  Atlas *a = atlas(size);
  return a ? static_cast<int>(a->line) : size;
}

float Font::kern(Atlas *a, unsigned prev, unsigned cp) {
  if (!prev) return 0;
  unsigned long long key = (static_cast<unsigned long long>(prev) << 32) | cp;
  auto it = a->kerning.find(key);
  if (it != a->kerning.end()) return it->second;
  float value = stbtt_GetCodepointKernAdvance(static_cast<stbtt_fontinfo *>(info_), static_cast<int>(prev), static_cast<int>(cp)) * a->scale;
  a->kerning[key] = value;
  return value;
}

int Font::measure(const std::string &text, int size) {
  Atlas *a = atlas(size);
  if (!a) return 0;
  float x = 0;
  unsigned prev = 0;
  for (unsigned cp : utf8_decode(text)) {
    auto it = a->glyphs.find(cp);
    if (it == a->glyphs.end()) it = a->glyphs.find('?');
    x += kern(a, prev, cp) + it->second.advance;
    prev = cp;
  }
  return static_cast<int>(std::ceil(x));
}

int Font::draw(const std::string &text, int x, int y, int size, Color color) {
  Atlas *a = atlas(size);
  if (!a || !a->texture) return 0;
  SDL_SetTextureColorMod(a->texture, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(a->texture, color.a);
  float pen = static_cast<float>(x);
  float baseline = static_cast<float>(y) + a->ascent;
  unsigned prev = 0;
  for (unsigned cp : utf8_decode(text)) {
    auto it = a->glyphs.find(cp);
    if (it == a->glyphs.end()) it = a->glyphs.find('?');
    pen += kern(a, prev, cp);
    prev = cp;
    const Glyph &g = it->second;
    if (g.src.w > 0 && g.src.h > 0) {
      int x0 = static_cast<int>(std::lround(pen + g.xoff)), y0 = static_cast<int>(std::lround(baseline + g.yoff));
      SDL_Rect dst{x0, y0, static_cast<int>(std::lround(pen + g.xoff2)) - x0, static_cast<int>(std::lround(baseline + g.yoff2)) - y0};
      SDL_RenderCopy(renderer_, a->texture, &g.src, &dst);
    }
    pen += g.advance;
  }
  return static_cast<int>(pen) - x;
}

static SDL_Texture *texture_from_coverage(SDL_Renderer *renderer, const std::vector<float> &coverage, int size) {
  std::vector<Uint32> pixels(coverage.size());
  for (size_t i = 0; i < coverage.size(); i++) {
    float c = std::max(0.0f, std::min(1.0f, coverage[i]));
    pixels[i] = (static_cast<Uint32>(c * 255.0f + 0.5f) << 24) | 0x00FFFFFF;
  }
  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels.data(), size, size, 32, size * 4, 0x00FF0000, 0x0000FF00,
                                                  0x000000FF, 0xFF000000);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);
  if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  return texture;
}

bool Gfx::init(SDL_Renderer *renderer, int w, int h) {
  renderer_ = renderer;
  w_ = w;
  h_ = h;
  std::vector<Uint32> ramp(256);
  for (Uint32 i = 0; i < 256; i++) ramp[i] = (i << 24) | 0x00FFFFFF;
  for (int dir = 0; dir < 2; dir++) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(ramp.data(), dir ? 1 : 256, dir ? 256 : 1, 32, dir ? 4 : 256 * 4,
                                                    0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
    if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    (dir ? ramp_v_ : ramp_h_) = t;
  }

  canvas_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w_, h_);
  if (canvas_) {
    SDL_SetTextureBlendMode(canvas_, SDL_BLENDMODE_NONE);
  } else {
    logf("gfx", "no render-target support (%s); using logical scaling", SDL_GetError());
    SDL_RenderSetLogicalSize(renderer_, w_, h_);
  }
  return true;
}

void Gfx::begin_frame() {
  if (canvas_) SDL_SetRenderTarget(renderer_, canvas_);
  // end_frame() leaves blending off for the final copy; drawing needs it on.
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

void Gfx::end_frame() {
  if (!canvas_) return;
  SDL_SetRenderTarget(renderer_, nullptr);
  // Copy without blending so transparent pixels stay transparent and the
  // hardware video plane beneath the window remains visible.
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, canvas_, nullptr, nullptr);
}

void Gfx::shutdown() {
  for (auto &d : discs_) SDL_DestroyTexture(d.second);
  for (auto &t : triangles_) SDL_DestroyTexture(t.second);
  for (auto &t : shadows_) SDL_DestroyTexture(t.second);
  discs_.clear();
  triangles_.clear();
  shadows_.clear();
  if (canvas_) SDL_DestroyTexture(canvas_);
  if (layer_) SDL_DestroyTexture(layer_);
  layer_ = nullptr;
  if (ramp_h_) SDL_DestroyTexture(ramp_h_);
  if (ramp_v_) SDL_DestroyTexture(ramp_v_);
  for (auto &r : rings_) SDL_DestroyTexture(r.second);
  rings_.clear();
  canvas_ = ramp_h_ = ramp_v_ = nullptr;
}

void Gfx::clear(Color color) {
  // No blending: alpha 0 must reach the window so the video plane shows through.
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  SDL_RenderClear(renderer_);
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

void Gfx::fill_rect(int x, int y, int w, int h, Color color) {
  if (w <= 0 || h <= 0) return;
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  SDL_Rect rect{x, y, w, h};
  SDL_RenderFillRect(renderer_, &rect);
}

SDL_Texture *Gfx::disc(int radius) {
  radius = std::max(1, radius);
  auto it = discs_.find(radius);
  if (it != discs_.end()) return it->second;
  const int size = radius * 2;
  std::vector<float> coverage(static_cast<size_t>(size) * size);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      // 4×4 supersampled so small radii stay round.
      float sum = 0;
      for (int sy = 0; sy < 4; sy++)
        for (int sx = 0; sx < 4; sx++) {
          float dx = x + (sx + 0.5f) / 4 - radius, dy = y + (sy + 0.5f) / 4 - radius;
          sum += dx * dx + dy * dy <= static_cast<float>(radius * radius) ? 1.0f : 0.0f;
        }
      coverage[static_cast<size_t>(y) * size + x] = sum / 16.0f;
    }
  }
  SDL_Texture *t = texture_from_coverage(renderer_, coverage, size);
  discs_[radius] = t;
  return t;
}

SDL_Texture *Gfx::triangle(int size) {
  size = std::max(4, size);
  auto it = triangles_.find(size);
  if (it != triangles_.end()) return it->second;
  std::vector<float> tri(static_cast<size_t>(size) * size);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      int inside = 0;
      for (int sy = 0; sy < 4; sy++) {
        for (int sx = 0; sx < 4; sx++) {
          float px = (x + (sx + 0.5f) / 4) / size, py = (y + (sy + 0.5f) / 4) / size;
          // Vertices (0.12,0.04) (0.12,0.96) (0.92,0.5)
          bool above = py >= 0.04f + (0.46f / 0.80f) * (px - 0.12f);
          bool below = py <= 0.96f - (0.46f / 0.80f) * (px - 0.12f);
          if (px >= 0.12f && above && below) inside++;
        }
      }
      tri[static_cast<size_t>(y) * size + x] = inside / 16.0f;
    }
  }
  SDL_Texture *t = texture_from_coverage(renderer_, tri, size);
  triangles_[size] = t;
  return t;
}

void Gfx::fill_circle(int cx, int cy, int radius, Color color) {
  SDL_Texture *t = disc(radius);
  if (!t) return;
  SDL_SetTextureColorMod(t, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(t, color.a);
  SDL_Rect dst{cx - radius, cy - radius, radius * 2, radius * 2};
  SDL_RenderCopy(renderer_, t, nullptr, &dst);
}

void Gfx::fill_round(int x, int y, int w, int h, int radius, Color color) {
  radius = std::max(0, std::min(radius, std::min(w, h) / 2));
  if (radius == 0) return fill_rect(x, y, w, h, color);
  SDL_Texture *t = disc(radius);
  if (!t) return fill_rect(x, y, w, h, color);
  SDL_SetTextureColorMod(t, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(t, color.a);
  const int r = radius;
  const SDL_Rect quads[4] = {{0, 0, r, r}, {r, 0, r, r}, {0, r, r, r}, {r, r, r, r}};
  const SDL_Rect corners[4] = {{x, y, r, r}, {x + w - r, y, r, r}, {x, y + h - r, r, r}, {x + w - r, y + h - r, r, r}};
  for (int i = 0; i < 4; i++) SDL_RenderCopy(renderer_, t, &quads[i], &corners[i]);
  fill_rect(x + r, y, w - 2 * r, h, color);
  fill_rect(x, y + r, r, h - 2 * r, color);
  fill_rect(x + w - r, y + r, r, h - 2 * r, color);
}

SDL_Texture *Gfx::shadow_texture(int radius, int blur) {
  auto key = std::make_pair(radius, blur);
  auto it = shadows_.find(key);
  if (it != shadows_.end()) return it->second;
  // A rounded box inset by 2·blur, with an erf falloff of its signed distance:
  // close to a Gaussian-blurred box at a fraction of the cost.
  const int e = radius + 2 * blur;  // corner slice size
  const int size = 2 * e + 2;
  std::vector<float> coverage(static_cast<size_t>(size) * size);
  const float x0 = 2.0f * blur, y0 = 2.0f * blur, x1 = size - 2.0f * blur, y1 = size - 2.0f * blur;
  const float sigma = std::max(1.0f, blur * 0.55f);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      float px = x + 0.5f, py = y + 0.5f;
      float cx = std::max(x0 + radius, std::min(px, x1 - radius));
      float cy = std::max(y0 + radius, std::min(py, y1 - radius));
      float d = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy)) - radius;
      coverage[static_cast<size_t>(y) * size + x] = 0.5f * (1.0f - std::erf(d / (sigma * 1.41421356f)));
    }
  }
  SDL_Texture *t = texture_from_coverage(renderer_, coverage, size);
  shadows_[key] = t;
  return t;
}

void Gfx::shadow(int x, int y, int w, int h, int radius, int blur, int dy, Uint8 alpha) {
  if (alpha == 0 || w <= 0 || h <= 0) return;
  radius = std::max(0, std::min(radius, std::min(w, h) / 2));
  SDL_Texture *t = shadow_texture(radius, blur);
  if (!t) return;
  SDL_SetTextureColorMod(t, 0, 0, 0);
  SDL_SetTextureAlphaMod(t, alpha);
  const int e = radius + 2 * blur, size = 2 * e + 2;
  // Outer rectangle covers the box plus the falloff on every side.
  int ox = x - 2 * blur, oy = y - 2 * blur + dy, ow = w + 4 * blur, oh = h + 4 * blur;
  int mw = ow - 2 * e, mh = oh - 2 * e;  // stretched middle
  const int sx[3] = {0, e, e + 2}, sw[3] = {e, 2, e};
  const int dxs[3] = {ox, ox + e, ox + e + std::max(0, mw)}, dws[3] = {e, std::max(0, mw), e};
  const int dys[3] = {oy, oy + e, oy + e + std::max(0, mh)}, dhs[3] = {e, std::max(0, mh), e};
  for (int r = 0; r < 3; r++)
    for (int c = 0; c < 3; c++) {
      if (dws[c] <= 0 || dhs[r] <= 0) continue;
      SDL_Rect src{sx[c], sx[r], sw[c], sw[r]};
      SDL_Rect dst{dxs[c], dys[r], dws[c], dhs[r]};
      SDL_RenderCopy(renderer_, t, &src, &dst);
    }
  (void)size;
}

SDL_Texture *Gfx::ring(int radius, int width) {
  auto key = std::make_pair(radius, width);
  auto it = rings_.find(key);
  if (it != rings_.end()) return it->second;
  const int size = radius * 2;
  std::vector<float> coverage(static_cast<size_t>(size) * size);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      float dx = x + 0.5f - radius, dy = y + 0.5f - radius;
      float d = std::sqrt(dx * dx + dy * dy);
      float outer = radius - d + 0.5f, inner = d - (radius - width) + 0.5f;
      coverage[static_cast<size_t>(y) * size + x] = std::max(0.0f, std::min(1.0f, std::min(outer, inner)));
    }
  }
  SDL_Texture *t = texture_from_coverage(renderer_, coverage, size);
  rings_[key] = t;
  return t;
}

void Gfx::stroke_round(int x, int y, int w, int h, int radius, int width, Color color) {
  radius = std::max(width, std::min(radius, std::min(w, h) / 2));
  SDL_Texture *t = ring(radius, width);
  if (t) {
    SDL_SetTextureColorMod(t, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(t, color.a);
    const SDL_Rect quads[4] = {{0, 0, radius, radius}, {radius, 0, radius, radius}, {0, radius, radius, radius}, {radius, radius, radius, radius}};
    const SDL_Rect corners[4] = {{x, y, radius, radius}, {x + w - radius, y, radius, radius},
                                 {x, y + h - radius, radius, radius}, {x + w - radius, y + h - radius, radius, radius}};
    for (int i = 0; i < 4; i++) SDL_RenderCopy(renderer_, t, &quads[i], &corners[i]);
  }
  fill_rect(x + radius, y, w - 2 * radius, width, color);
  fill_rect(x + radius, y + h - width, w - 2 * radius, width, color);
  fill_rect(x, y + radius, width, h - 2 * radius, color);
  fill_rect(x + w - width, y + radius, width, h - 2 * radius, color);
}

void Gfx::triangle_right(int x, int y, int size, Color color) {
  SDL_Texture *t = triangle(size);
  if (!t) return;
  SDL_SetTextureColorMod(t, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(t, color.a);
  SDL_Rect dst{x, y, size, size};
  SDL_RenderCopy(renderer_, t, nullptr, &dst);
}

void Gfx::bars(int x, int y, int size, Color color) {
  int bar = size / 3;
  fill_round(x, y, bar, size, 3, color);
  fill_round(x + size - bar, y, bar, size, 3, color);
}

static void ramp_copy(SDL_Renderer *renderer, SDL_Texture *ramp, bool vertical, int x, int y, int w, int h, Color color,
                      Uint8 a0, Uint8 a1) {
  if (!ramp || w <= 0 || h <= 0) return;
  // The ramp runs 0..255; draw the slice between a0 and a1 stretched over the
  // rectangle. Sampling texel centres keeps the ends exact.
  int lo = std::min(a0, a1), hi = std::max(a0, a1);
  if (hi == lo) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, static_cast<Uint8>(lo * color.a / 255));
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer, &r);
    return;
  }
  SDL_SetTextureColorMod(ramp, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(ramp, color.a);
  SDL_Rect src = vertical ? SDL_Rect{0, lo, 1, hi - lo + 1} : SDL_Rect{lo, 0, hi - lo + 1, 1};
  SDL_Rect dst{x, y, w, h};
  bool flip = a0 > a1;
  SDL_RenderCopyEx(renderer, ramp, &src, &dst, 0, nullptr,
                   flip ? (vertical ? SDL_FLIP_VERTICAL : SDL_FLIP_HORIZONTAL) : SDL_FLIP_NONE);
}

void Gfx::gradient_h(int x, int y, int w, int h, Color color, Uint8 a0, Uint8 a1) {
  ramp_copy(renderer_, ramp_h_, false, x, y, w, h, color, a0, a1);
}

void Gfx::gradient_v(int x, int y, int w, int h, Color color, Uint8 a0, Uint8 a1) {
  ramp_copy(renderer_, ramp_v_, true, x, y, w, h, color, a0, a1);
}

void Gfx::texture(SDL_Texture *texture, int x, int y, int w, int h, Uint8 alpha) {
  if (!texture) return;
  SDL_SetTextureColorMod(texture, 255, 255, 255);
  SDL_SetTextureAlphaMod(texture, alpha);
  SDL_Rect dst{x, y, w, h};
  SDL_RenderCopy(renderer_, texture, nullptr, &dst);
}

void Gfx::clip(int x, int y, int w, int h) {
  SDL_Rect r{x, y, w, h};
  SDL_RenderSetClipRect(renderer_, &r);
}

void Gfx::unclip() { SDL_RenderSetClipRect(renderer_, nullptr); }

bool Gfx::begin_layer() {
  if (!canvas_) return false;
  if (!layer_) {
    layer_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w_, h_);
    if (!layer_) return false;
    SDL_SetTextureBlendMode(layer_, SDL_BLENDMODE_BLEND);
  }
  SDL_SetRenderTarget(renderer_, layer_);
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
  SDL_RenderClear(renderer_);
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
  return true;
}

void Gfx::end_layer(float alpha, int dy) {
  SDL_SetRenderTarget(renderer_, canvas_);
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(layer_, static_cast<Uint8>(std::max(0.0f, std::min(1.0f, alpha)) * 255));
  SDL_Rect dst{0, dy, w_, h_};
  SDL_RenderCopy(renderer_, layer_, nullptr, &dst);
}
