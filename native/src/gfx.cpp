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
  // Strict decoder: a bad lead byte, a missing or wrong continuation byte, an
  // overlong form, a surrogate or a value above U+10FFFF skips one byte and
  // resynchronises on the next, so malformed catalogue text never stops the
  // rest of a string from drawing.
  std::vector<unsigned> out;
  out.reserve(text.size());
  const size_t n = text.size();
  for (size_t i = 0; i < n;) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    if (c < 0x80) {
      out.push_back(c);
      i++;
      continue;
    }
    unsigned cp, min;
    size_t extra;
    if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; min = 0x80; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; min = 0x800; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; min = 0x10000; }
    else { i++; continue; }  // stray continuation byte or invalid lead
    bool ok = i + extra < n;
    for (size_t k = 1; ok && k <= extra; k++) {
      unsigned char t = static_cast<unsigned char>(text[i + k]);
      if ((t & 0xC0) != 0x80) ok = false;
      else cp = (cp << 6) | (t & 0x3F);
    }
    if (!ok || cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
      i++;
      continue;
    }
    out.push_back(cp);
    i += extra + 1;
  }
  return out;
}

// Code points baked into every atlas up front: Latin-1 plus the punctuation
// and arrows the UI uses, so common text never triggers a re-bake.
static std::vector<int> base_codepoints() {
  std::vector<int> cps;
  for (int c = 32; c < 127; c++) cps.push_back(c);
  for (int c = 160; c < 256; c++) cps.push_back(c);
  const int extra[] = {0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x2212,
                       0x2190, 0x2191, 0x2192, 0x2193, 0x2715, 0x2713};
  cps.insert(cps.end(), std::begin(extra), std::end(extra));
  return cps;
}

static size_t base_count() {
  static const size_t count = base_codepoints().size();
  return count;
}

namespace {
// On-demand glyphs per atlas (per pixel size) beyond the base set.
constexpr int kMaxExtras = 2000;
// Text sizes also load the rest of a missing glyph's 128-code-point block
// (Latin Extended-A, Greek, Cyrillic…), so a page of Polish or Russian titles
// costs one re-bake per block rather than one per new letter. Neighbours may
// use at most this share of the budget; glyphs actually asked for get the rest.
constexpr int kMaxPrefetchSize = 48;
constexpr int kPrefetchBudget = kMaxExtras / 2;

// Control and format characters that should take no space when the font has
// no glyph for them (zero-width spaces, joiners, direction marks, BOM,
// variation selectors, tags).
bool invisible(unsigned cp) {
  return cp < 0x20 || (cp >= 0x7F && cp < 0xA0) || (cp >= 0x200B && cp <= 0x200F) ||
         (cp >= 0x2028 && cp <= 0x202E) || (cp >= 0x2060 && cp <= 0x206F) || (cp >= 0xFE00 && cp <= 0xFE0F) ||
         cp == 0xFEFF || (cp >= 0xE0000 && cp <= 0xE0FFF);
}
}  // namespace

Font::~Font() {
  for (auto &entry : atlases_) {
    if (!entry.second.texture) continue;
    SDL_DestroyTexture(entry.second.texture);
    gpu_budget().remove(GpuClass::Glyphs, gpu_bytes(entry.second.tex_w, entry.second.tex_h));
  }
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

// Packs `cps` into the smallest texture that holds them and, on success,
// replaces the atlas texture and glyph table. On failure the atlas is left as
// it was and false is returned.
bool Font::bake(Atlas &a, const std::vector<int> &cps) {
  auto *info = static_cast<stbtt_fontinfo *>(info_);
  // 2× horizontal oversampling keeps stems even at text sizes; large display
  // type does not need it.
  const int ox = a.size <= 48 ? 2 : 1;
  const int padding = 1;

  // Estimate the packed area from glyph boxes (cheap: no rasterising) so the
  // packer starts near the right texture size instead of failing upward.
  double area = 0;
  for (int cp : cps) {
    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(info, cp, a.scale * ox, a.scale, &x0, &y0, &x1, &y1);
    area += static_cast<double>(x1 - x0 + padding + ox - 1) * (y1 - y0 + padding);
  }

  int max_w = 4096, max_h = 4096;
  SDL_RendererInfo rinfo;
  if (renderer_ && SDL_GetRendererInfo(renderer_, &rinfo) == 0) {
    if (rinfo.max_texture_width > 0) max_w = std::min(max_w, rinfo.max_texture_width);
    if (rinfo.max_texture_height > 0) max_h = std::min(max_h, rinfo.max_texture_height);
  }
  // Candidate sizes in order of area; the texture is sized to the glyph set.
  static const int dims[][2] = {{128, 128},   {256, 128},   {256, 256},   {512, 256},
                                {512, 512},   {1024, 512},  {1024, 1024}, {2048, 1024},
                                {2048, 2048}, {4096, 2048}, {4096, 4096}};
  const int count = static_cast<int>(sizeof dims / sizeof dims[0]);
  int last = -1;
  for (int i = 0; i < count; i++)
    if (dims[i][0] <= max_w && dims[i][1] <= max_h) last = i;

  std::vector<stbtt_packedchar> packed(cps.size());
  std::vector<unsigned char> pixels;
  int w = 0, h = 0;
  bool ok = false;
  for (int i = 0; i <= last && !ok; i++) {
    // Skip sizes the estimate says cannot hold the set (skyline packing wastes ~15%).
    if (i < last && area > 0.85 * dims[i][0] * dims[i][1]) continue;
    w = dims[i][0];
    h = dims[i][1];
    pixels.assign(static_cast<size_t>(w) * h, 0);
    std::fill(packed.begin(), packed.end(), stbtt_packedchar{});
    stbtt_pack_context pack{};
    if (!stbtt_PackBegin(&pack, pixels.data(), w, h, 0, padding, nullptr)) return false;
    stbtt_PackSetOversampling(&pack, static_cast<unsigned>(ox), 1);
    stbtt_pack_range range{};
    range.font_size = static_cast<float>(a.size);
    range.array_of_unicode_codepoints = const_cast<int *>(cps.data());
    range.num_chars = static_cast<int>(cps.size());
    range.chardata_for_range = packed.data();
    ok = stbtt_PackFontRanges(&pack, data_.data(), 0, &range, 1) != 0;
    stbtt_PackEnd(&pack);
  }
  // The very first bake keeps whatever fitted (as before); later bakes must fit
  // everything or leave the working atlas alone.
  if (!ok && (a.texture || w == 0)) return false;

  // White glyphs with coverage in alpha; colour comes from texture colour mod.
  // Coverage is lifted slightly (gamma) because blending is done in linear
  // space: light text on dark grounds otherwise looks thin and stepped.
  static Uint8 lift[256];
  static bool lift_ready = false;
  if (!lift_ready) {
    for (int i = 0; i < 256; i++) lift[i] = static_cast<Uint8>(std::lround(255.0 * std::pow(i / 255.0, 1.0 / 1.35)));
    lift_ready = true;
  }
  std::vector<Uint32> rgba_pixels(pixels.size());
  for (size_t i = 0; i < pixels.size(); i++) rgba_pixels[i] = (static_cast<Uint32>(lift[pixels[i]]) << 24) | 0x00FFFFFF;
  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(rgba_pixels.data(), w, h, 32, w * 4, 0x00FF0000, 0x0000FF00,
                                                  0x000000FF, 0xFF000000);
  SDL_Texture *texture = surface ? SDL_CreateTextureFromSurface(renderer_, surface) : nullptr;
  if (surface) SDL_FreeSurface(surface);
  if (!texture && a.texture) {
    logf("gfx", "atlas %dpx: texture %dx%d failed (%s)", a.size, w, h, SDL_GetError());
    return false;
  }
  if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  if (a.texture) {
    SDL_DestroyTexture(a.texture);
    gpu_budget().remove(GpuClass::Glyphs, gpu_bytes(a.tex_w, a.tex_h));
  }
  if (texture) gpu_budget().add(GpuClass::Glyphs, gpu_bytes(w, h));
  a.texture = texture;
  a.tex_w = w;
  a.tex_h = h;
  a.codepoints = cps;
  a.glyphs.clear();
  for (size_t i = 0; i < cps.size(); i++) {
    const stbtt_packedchar &p = packed[i];
    a.glyphs[static_cast<unsigned>(cps[i])] = {{p.x0, p.y0, p.x1 - p.x0, p.y1 - p.y0}, p.xoff, p.yoff, p.xoff2, p.yoff2, p.xadvance};
  }
  return true;
}

Font::Atlas *Font::atlas(int size) {
  auto found = atlases_.find(size);
  if (found != atlases_.end()) return &found->second;
  if (!info_) return nullptr;
  auto *info = static_cast<stbtt_fontinfo *>(info_);

  Atlas &a = atlases_[size];
  a.size = size;
  a.scale = stbtt_ScaleForPixelHeight(info, static_cast<float>(size));
  int ascent, descent, gap;
  stbtt_GetFontVMetrics(info, &ascent, &descent, &gap);
  a.ascent = ascent * a.scale;
  a.line = (ascent - descent + gap) * a.scale;
  if (!bake(a, base_codepoints())) logf("gfx", "atlas %dpx: base set could not be baked", size);
  return &a;
}

void Font::collect(Atlas *a, const std::vector<unsigned> &cps) {
  auto *info = static_cast<stbtt_fontinfo *>(info_);
  for (unsigned cp : cps) {
    if (a->glyphs.count(cp) || a->pending.count(cp) || missing_.count(cp) || a->refused.count(cp)) continue;
    int glyph = stbtt_FindGlyphIndex(info, static_cast<int>(cp));
    if (!glyph) {
      missing_.insert(cp);  // remembered for every size: never looked up again
      continue;
    }
    if (a->extras + static_cast<int>(a->pending.size()) >= kMaxExtras) {
      a->refused.insert(cp);
      if (!a->cap_logged) {
        logf("gfx", "atlas %dpx: %d on-demand glyphs reached; further code points draw as '?'", a->size, kMaxExtras);
        a->cap_logged = true;
      }
      continue;
    }
    // Same arithmetic stbtt_PackFontRanges uses for xadvance, so a measured
    // width matches the drawn one exactly once the glyph is baked.
    int advance, lsb;
    stbtt_GetGlyphHMetrics(info, glyph, &advance, &lsb);
    a->pending[cp] = a->scale * advance;
  }
}

void Font::flush(Atlas *a) {
  if (a->pending.empty()) return;
  auto *info = static_cast<stbtt_fontinfo *>(info_);
  std::vector<int> wanted;
  for (auto &p : a->pending) wanted.push_back(static_cast<int>(p.first));
  a->pending.clear();

  std::vector<int> neighbours;
  if (a->size <= kMaxPrefetchSize) {
    std::set<unsigned> queued(wanted.begin(), wanted.end());
    std::set<unsigned> blocks;
    for (int cp : wanted) blocks.insert(static_cast<unsigned>(cp) & ~0x7Fu);
    for (unsigned b : blocks)
      for (unsigned cp = b; cp < b + 0x80; cp++) {
        if (a->extras + static_cast<int>(wanted.size() + neighbours.size()) >= kPrefetchBudget) break;
        if (invisible(cp) || a->glyphs.count(cp) || queued.count(cp) || missing_.count(cp) || a->refused.count(cp)) continue;
        if (!stbtt_FindGlyphIndex(info, static_cast<int>(cp))) {
          missing_.insert(cp);
          continue;
        }
        neighbours.push_back(static_cast<int>(cp));
        queued.insert(cp);
      }
  }

  const size_t before = a->codepoints.size();
  std::vector<int> next = a->codepoints;
  next.insert(next.end(), wanted.begin(), wanted.end());
  const size_t without_neighbours = next.size();
  next.insert(next.end(), neighbours.begin(), neighbours.end());
  Uint32 t0 = SDL_GetTicks();
  bool ok = bake(*a, next);
  if (!ok && !neighbours.empty()) {
    next.resize(without_neighbours);
    ok = bake(*a, next);
  }
  if (!ok) {
    // Too large for the biggest texture at this size: draw these as '?' and
    // never try them again.
    for (int cp : wanted) a->refused.insert(static_cast<unsigned>(cp));
    logf("gfx", "atlas %dpx: %zu glyphs did not fit; drawn as '?'", a->size, wanted.size());
    return;
  }
  a->extras = static_cast<int>(a->codepoints.size() - base_count());
  logf("gfx", "atlas %dpx: +%zu glyphs (%d on demand), %dx%d, %u ms", a->size, a->codepoints.size() - before, a->extras,
       a->tex_w, a->tex_h, static_cast<unsigned>(SDL_GetTicks() - t0));
}

bool Font::advance_for(Atlas *a, unsigned cp, unsigned &key, float &advance) {
  auto it = a->glyphs.find(cp);
  if (it != a->glyphs.end()) {
    key = cp;
    advance = it->second.advance;
    return true;
  }
  auto queued = a->pending.find(cp);
  if (queued != a->pending.end()) {
    key = cp;
    advance = queued->second;
    return true;
  }
  if (invisible(cp)) return false;
  it = a->glyphs.find('?');
  if (it == a->glyphs.end()) return false;
  key = '?';
  advance = it->second.advance;
  return true;
}

float Font::ascent(int size) {
  Atlas *a = atlas(size);
  return a ? a->ascent : size * 0.8f;
}

float Font::cap_height(int size) {
  Atlas *a = atlas(size);
  if (!a || !info_) return size * 0.7f;
  int x0, y0, x1, y1;
  if (!stbtt_GetCodepointBox(static_cast<stbtt_fontinfo *>(info_), 'H', &x0, &y0, &x1, &y1)) return size * 0.7f;
  return y1 * a->scale;
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

// Measuring and drawing share collect() and advance_for(), so the same
// glyphs, fallbacks, advances and kerning pairs are used by both. Measuring
// never re-bakes: wrap() and ellipsize() measure many prefixes of one string,
// and the next draw bakes everything they queued in one go.
int Font::measure(const std::string &text, int size) {
  Atlas *a = atlas(size);
  if (!a) return 0;
  std::vector<unsigned> cps = utf8_decode(text);
  collect(a, cps);
  float x = 0;
  unsigned prev = 0;
  for (unsigned cp : cps) {
    unsigned key = 0;
    float advance = 0;
    if (!advance_for(a, cp, key, advance)) continue;
    x += kern(a, prev, key) + advance;
    prev = key;
  }
  return static_cast<int>(std::ceil(x));
}

int Font::draw(const std::string &text, int x, int y, int size, Color color) {
  Atlas *a = atlas(size);
  if (!a) return 0;
  std::vector<unsigned> cps = utf8_decode(text);
  collect(a, cps);
  flush(a);  // may replace the texture: fetch it afterwards
  if (!a->texture) return 0;
  SDL_SetTextureColorMod(a->texture, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(a->texture, color.a);
  float pen = static_cast<float>(x);
  float baseline = static_cast<float>(y) + a->ascent;
  unsigned prev = 0;
  for (unsigned cp : cps) {
    unsigned key = 0;
    float advance = 0;
    if (!advance_for(a, cp, key, advance)) continue;
    pen += kern(a, prev, key);
    prev = key;
    const Glyph &g = a->glyphs.find(key)->second;  // baked by flush() above
    if (g.src.w > 0 && g.src.h > 0) {
      int x0 = static_cast<int>(std::lround(pen + g.xoff)), y0 = static_cast<int>(std::lround(baseline + g.yoff));
      SDL_Rect dst{x0, y0, static_cast<int>(std::lround(pen + g.xoff2)) - x0, static_cast<int>(std::lround(baseline + g.yoff2)) - y0};
      SDL_RenderCopy(renderer_, a->texture, &g.src, &dst);
    }
    pen += advance;
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
    gpu_budget().add(GpuClass::Surfaces, gpu_bytes(w_, h_));
    SDL_SetTextureBlendMode(canvas_, SDL_BLENDMODE_NONE);
  } else {
    logf("gfx", "no render-target support (%s); using logical scaling", SDL_GetError());
    SDL_RenderSetLogicalSize(renderer_, w_, h_);
  }
  return true;
}

void Gfx::set_color(Color c) {
  if (color_known_ && c.r == color_.r && c.g == color_.g && c.b == color_.b && c.a == color_.a) {
    skipped_++;
    return;
  }
  SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
  color_ = c;
  color_known_ = true;
}

void Gfx::set_blend(SDL_BlendMode mode) {
  if (blend_known_ && mode == blend_) {
    skipped_++;
    return;
  }
  SDL_SetRenderDrawBlendMode(renderer_, mode);
  blend_ = mode;
  blend_known_ = true;
}

void Gfx::begin_frame() {
  forget_state();
  if (canvas_) SDL_SetRenderTarget(renderer_, canvas_);
  // end_frame() leaves blending off for the final copy; drawing needs it on.
  set_blend(SDL_BLENDMODE_BLEND);
}

void Gfx::end_frame() {
  if (!canvas_) return;
  SDL_SetRenderTarget(renderer_, nullptr);
  // Copy without blending so transparent pixels stay transparent and the
  // hardware video plane beneath the window remains visible.
  set_blend(SDL_BLENDMODE_NONE);
  set_color({0, 0, 0, 0});
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, canvas_, nullptr, nullptr);
  forget_state();
}

void Gfx::shutdown() {
  release_caches();
  for (SDL_Texture *t : pool_) {
    SDL_DestroyTexture(t);
    gpu_budget().remove(GpuClass::Layers, gpu_bytes(w_, h_));
  }
  pool_.clear();
  if (canvas_) {
    SDL_DestroyTexture(canvas_);
    gpu_budget().remove(GpuClass::Surfaces, gpu_bytes(w_, h_));
  }
  if (ramp_h_) SDL_DestroyTexture(ramp_h_);
  if (ramp_v_) SDL_DestroyTexture(ramp_v_);
  canvas_ = ramp_h_ = ramp_v_ = nullptr;
}

void Gfx::housekeeping() {
  // Every couple of seconds: shapes unused for ~10 s go, beyond a working set.
  if (current_frame() % 120 != 0) return;
  // Full-size targets cost 8 MB each: once transitions and page fades have
  // been idle for ~5 s, the spare and the fade layer go too (recreated on use).
  if (current_frame() - targets_used_ > 300) {
    if (layer_) give_target(layer_);
    layer_ = nullptr;
    for (SDL_Texture *t : pool_) {
      SDL_DestroyTexture(t);
      gpu_budget().remove(GpuClass::Layers, gpu_bytes(w_, h_));
    }
    pool_.clear();
  }
  discs_.prune(600, 48);
  rings_.prune(600, 24);
  shadows_.prune(600, 24);
  sheens_.prune(600, 8);
  triangles_.prune(600, 8);
}

void Gfx::release_caches() {
  discs_.clear();
  triangles_.clear();
  shadows_.clear();
  rings_.clear();
  sheens_.clear();
  for (auto &kv : cached_) give_target(kv.second.texture);
  cached_.clear();
  if (layer_) give_target(layer_);
  layer_ = nullptr;
  for (SDL_Texture *t : pool_) {
    SDL_DestroyTexture(t);
    gpu_budget().remove(GpuClass::Layers, gpu_bytes(w_, h_));
  }
  pool_.clear();
}

Gfx::Stats Gfx::stats() const {
  Stats s;
  s.discs = discs_.size();
  s.rings = rings_.size();
  s.shadows = shadows_.size();
  s.sheens = sheens_.size();
  s.triangles = triangles_.size();
  s.layers = cached_.size() + (layer_ ? 1 : 0);
  s.pool = pool_.size();
  s.state_calls_skipped = skipped_;
  return s;
}

SDL_Texture *Gfx::take_target() {
  if (!pool_.empty()) {
    SDL_Texture *t = pool_.back();
    pool_.pop_back();
    return t;
  }
  SDL_Texture *t = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w_, h_);
  if (t) {
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    gpu_budget().add(GpuClass::Layers, gpu_bytes(w_, h_));
  }
  return t;
}

void Gfx::give_target(SDL_Texture *t) {
  if (!t) return;
  // Keep one spare; more would only hold memory.
  if (pool_.empty()) {
    pool_.push_back(t);
  } else {
    SDL_DestroyTexture(t);
    gpu_budget().remove(GpuClass::Layers, gpu_bytes(w_, h_));
  }
}

bool Gfx::cached_valid(const std::string &name, const std::string &key) const {
  auto it = cached_.find(name);
  return it != cached_.end() && it->second.texture && it->second.key == key;
}

bool Gfx::begin_cached(const std::string &name) {
  if (!canvas_) return false;
  targets_used_ = current_frame();
  Cached &c = cached_[name];
  if (!c.texture) c.texture = take_target();
  if (!c.texture) {
    cached_.erase(name);
    return false;
  }
  c.key.clear();
  SDL_SetRenderTarget(renderer_, c.texture);
  forget_state();
  set_blend(SDL_BLENDMODE_NONE);
  set_color({0, 0, 0, 0});
  SDL_RenderClear(renderer_);
  set_blend(SDL_BLENDMODE_BLEND);
  return true;
}

void Gfx::end_cached(const std::string &name, const std::string &key) {
  auto it = cached_.find(name);
  if (it != cached_.end()) it->second.key = key;
  SDL_SetRenderTarget(renderer_, canvas_);
  forget_state();
  set_blend(SDL_BLENDMODE_BLEND);
}

void Gfx::composite_cached(const std::string &name, float alpha, int dy) {
  targets_used_ = current_frame();
  auto it = cached_.find(name);
  if (it == cached_.end() || !it->second.texture) return;
  SDL_Texture *t = it->second.texture;
  SDL_SetTextureAlphaMod(t, static_cast<Uint8>(std::max(0.0f, std::min(1.0f, alpha)) * 255));
  SDL_Rect dst{0, dy, w_, h_};
  SDL_RenderCopy(renderer_, t, nullptr, &dst);
}

void Gfx::drop_cached(const std::string &name) {
  auto it = cached_.find(name);
  if (it == cached_.end()) return;
  give_target(it->second.texture);
  cached_.erase(it);
}

void Gfx::clear(Color color) {
  // No blending: alpha 0 must reach the window so the video plane shows through.
  set_blend(SDL_BLENDMODE_NONE);
  set_color(color);
  SDL_RenderClear(renderer_);
  set_blend(SDL_BLENDMODE_BLEND);
}

void Gfx::fill_rect(int x, int y, int w, int h, Color color) {
  if (w <= 0 || h <= 0) return;
  set_color(color);
  SDL_Rect rect{x, y, w, h};
  SDL_RenderFillRect(renderer_, &rect);
}

SDL_Texture *Gfx::disc(int radius) {
  radius = std::max(1, radius);
  if (SDL_Texture *found = discs_.find(radius)) return found;
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
  return discs_.put(radius, texture_from_coverage(renderer_, coverage, size), size, size);
}

void Gfx::sheen(int x, int y, int w, int h, int radius, int base_w, int base_h, Uint8 alpha) {
  if (!shadows || alpha == 0 || base_w <= 0 || base_h <= 0) return;
  std::string key = std::to_string(base_w) + "," + std::to_string(base_h) + "," + std::to_string(radius);
  SDL_Texture *t = sheens_.find(key);
  if (!t) {
    std::vector<Uint32> px(static_cast<size_t>(base_w) * base_h);
    float r = static_cast<float>(std::min(radius, std::min(base_w, base_h) / 2));
    for (int py = 0; py < base_h; py++)
      for (int qx = 0; qx < base_w; qx++) {
        float d = (static_cast<float>(qx) / base_w * 0.6f + static_cast<float>(py) / base_h * 0.4f);
        float light = d < 0.4f ? 1.0f - d / 0.4f : 0.0f;
        float fx = qx + 0.5f, fy = py + 0.5f;
        float cx = fx < r ? r : (fx > base_w - r ? base_w - r : fx);
        float cy = fy < r ? r : (fy > base_h - r ? base_h - r : fy);
        float dx = fx - cx, dy = fy - cy;
        float coverage = std::max(0.0f, std::min(1.0f, r - std::sqrt(dx * dx + dy * dy) + 0.5f));
        Uint32 a = static_cast<Uint32>(light * coverage * 255.0f + 0.5f);
        px[static_cast<size_t>(py) * base_w + qx] = (a << 24) | 0x00FFFFFF;
      }
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(px.data(), base_w, base_h, 32, base_w * 4, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    t = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
    if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    sheens_.put(key, t, base_w, base_h);
  }
  if (t) texture(t, x, y, w, h, alpha);
}

SDL_Texture *Gfx::triangle(int size) {
  size = std::max(4, size);
  if (SDL_Texture *found = triangles_.find(size)) return found;
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
  return triangles_.put(size, texture_from_coverage(renderer_, tri, size), size, size);
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
  if (SDL_Texture *found = shadows_.find(key)) return found;
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
  return shadows_.put(key, texture_from_coverage(renderer_, coverage, size), size, size);
}

void Gfx::shadow(int x, int y, int w, int h, int radius, int blur, int dy, Uint8 alpha) {
  if (!shadows || alpha == 0 || w <= 0 || h <= 0) return;
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
  if (SDL_Texture *found = rings_.find(key)) return found;
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
  return rings_.put(key, texture_from_coverage(renderer_, coverage, size), size, size);
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
  color_known_ = false;  // ramp_copy may set the draw colour directly
}

void Gfx::gradient_v(int x, int y, int w, int h, Color color, Uint8 a0, Uint8 a1) {
  ramp_copy(renderer_, ramp_v_, true, x, y, w, h, color, a0, a1);
  color_known_ = false;
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
  targets_used_ = current_frame();
  if (!layer_) layer_ = take_target();
  if (!layer_) return false;
  SDL_SetRenderTarget(renderer_, layer_);
  forget_state();
  set_blend(SDL_BLENDMODE_NONE);
  set_color({0, 0, 0, 0});
  SDL_RenderClear(renderer_);
  set_blend(SDL_BLENDMODE_BLEND);
  return true;
}

void Gfx::end_layer(float alpha, int dy) {
  SDL_SetRenderTarget(renderer_, canvas_);
  forget_state();
  set_blend(SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(layer_, static_cast<Uint8>(std::max(0.0f, std::min(1.0f, alpha)) * 255));
  SDL_Rect dst{0, dy, w_, h_};
  SDL_RenderCopy(renderer_, layer_, nullptr, &dst);
}
