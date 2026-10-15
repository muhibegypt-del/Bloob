#pragma once
// Drawing primitives over SDL_Renderer: anti-aliased rounded rectangles and
// text from TrueType glyph atlases. Uses only SDL APIs present in old SDL2
// releases, because the app links against the TV's own SDL.
#include <SDL.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct Color {
  Uint8 r, g, b, a;
};

inline Color rgba(unsigned hex, Uint8 alpha = 255) {
  return {static_cast<Uint8>(hex >> 16), static_cast<Uint8>(hex >> 8), static_cast<Uint8>(hex), alpha};
}

class Font {
 public:
  ~Font();
  bool load(SDL_Renderer *renderer, const std::string &path);
  // Width in pixels of `text` at `size`. Measuring and drawing load the same
  // glyphs and lay them out identically, so widths always match what is drawn.
  int measure(const std::string &text, int size);
  // Draws with the top of the line box at `y`. Returns the advance width.
  int draw(const std::string &text, int x, int y, int size, Color color);
  int line_height(int size);
  // Distance from the top of the line box to the baseline, and the height of a
  // capital letter above it: what optical centring needs.
  float ascent(int size);
  float cap_height(int size);

 private:
  struct Glyph {
    SDL_Rect src;                   // in the (horizontally oversampled) atlas
    float xoff, yoff, xoff2, yoff2;  // destination box relative to the pen, in pixels
    float advance;
  };
  // One atlas per pixel size. It starts with the Latin-1 base set; any other
  // code point the font has is queued on first use (measuring lays it out from
  // the font's own metrics) and the next draw re-bakes the atlas once with
  // every queued code point.
  struct Atlas {
    int size = 0;
    SDL_Texture *texture = nullptr;
    int tex_w = 0, tex_h = 0;
    std::map<unsigned, Glyph> glyphs;
    std::map<unsigned long long, float> kerning;  // pair -> pixels, filled on first use
    std::vector<int> codepoints;                  // baked set: base first, then extras
    std::map<unsigned, float> pending;            // queued for the next bake -> advance
    std::set<unsigned> refused;                   // over the cap or did not fit: never retried
    int extras = 0;
    bool cap_logged = false;
    float ascent = 0, line = 0, scale = 0;
  };
  Atlas *atlas(int size);
  bool bake(Atlas &atlas, const std::vector<int> &codepoints);
  // Queues every code point of `cps` the font has but the atlas lacks.
  void collect(Atlas *atlas, const std::vector<unsigned> &cps);
  // Re-bakes the atlas once with everything queued.
  void flush(Atlas *atlas);
  // Advance for `cp` and the code point used for it (itself, or the '?'
  // fallback); false for an invisible control or format character.
  bool advance_for(Atlas *atlas, unsigned cp, unsigned &key, float &advance);
  float kern(Atlas *atlas, unsigned prev, unsigned cp);

  SDL_Renderer *renderer_ = nullptr;
  std::vector<unsigned char> data_;
  void *info_ = nullptr;  // stbtt_fontinfo
  std::map<int, Atlas> atlases_;
  std::set<unsigned> missing_;  // code points this font has no glyph for (any size)
};

class Gfx {
 public:
  // Draws every frame at exactly `w`×`h` into an offscreen canvas, then scales
  // the finished frame to the window once, so shapes never seam when the TV
  // gives us a non-1080p surface. Falls back to logical scaling if the GPU
  // cannot render to a texture.
  bool init(SDL_Renderer *renderer, int w, int h);
  void shutdown();
  SDL_Renderer *renderer() const { return renderer_; }
  void begin_frame();
  void end_frame();
  SDL_Texture *canvas() const { return canvas_; }

  void clear(Color color);
  void fill_rect(int x, int y, int w, int h, Color color);
  void fill_round(int x, int y, int w, int h, int radius, Color color);
  void stroke_round(int x, int y, int w, int h, int radius, int width, Color color);
  void fill_circle(int cx, int cy, int radius, Color color);
  void triangle_right(int x, int y, int size, Color color);
  void bars(int x, int y, int size, Color color);
  // Soft drop shadow for a rounded box: `blur` px of falloff, offset by (dx, dy).
  void shadow(int x, int y, int w, int h, int radius, int blur, int dy, Uint8 alpha);
  // Off in the Lite effects tier: depth comes from fills and focus colour alone.
  bool shadows = true;
  // Glass card focus: white light across the top-left of a rounded box, fading out
  // by 40% of the diagonal. Baked once per resting size and stretched while the
  // card lifts. Off with shadows.
  void sheen(int x, int y, int w, int h, int radius, int base_w, int base_h, Uint8 alpha);
  // Colour with alpha running from a0 to a1, left→right or top→bottom.
  void gradient_h(int x, int y, int w, int h, Color color, Uint8 a0, Uint8 a1);
  void gradient_v(int x, int y, int w, int h, Color color, Uint8 a0, Uint8 a1);
  // Draw a texture at (x, y), scaled to w×h when given.
  void texture(SDL_Texture *texture, int x, int y, int w, int h, Uint8 alpha = 255);
  void clip(int x, int y, int w, int h);
  void unclip();
  // Draw into an offscreen layer, then composite it with alpha and a vertical
  // offset (screen transitions). Returns false if layers are unavailable.
  bool begin_layer();
  void end_layer(float alpha, int dy);

 private:
  SDL_Renderer *renderer_ = nullptr;
  // Shapes are rasterised at the exact size they are drawn: scaling one large
  // texture down without mipmaps aliases (jagged corners on the TV).
  std::map<int, SDL_Texture *> discs_;      // radius -> disc
  std::map<int, SDL_Texture *> triangles_;  // size -> play glyph
  std::map<std::pair<int, int>, SDL_Texture *> shadows_;  // (radius, blur) -> 9-slice source
  SDL_Texture *disc(int radius);
  SDL_Texture *triangle(int size);
  SDL_Texture *shadow_texture(int radius, int blur);
  SDL_Texture *ramp_h_ = nullptr;    // 256×1 alpha ramp
  SDL_Texture *ramp_v_ = nullptr;    // 1×256 alpha ramp
  SDL_Texture *canvas_ = nullptr;
  SDL_Texture *layer_ = nullptr;
  std::map<std::pair<int, int>, SDL_Texture *> rings_;  // (radius, width) -> anti-aliased annulus
  std::map<std::string, SDL_Texture *> sheens_;         // "w,h,r" -> sheen
  SDL_Texture *ring(int radius, int width);
  int w_ = 1920, h_ = 1080;
};

// UTF-8 → code points.
std::vector<unsigned> utf8_decode(const std::string &text);
