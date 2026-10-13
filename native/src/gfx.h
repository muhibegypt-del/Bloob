#pragma once
// Drawing primitives over SDL_Renderer: anti-aliased rounded rectangles and
// text from TrueType glyph atlases. Uses only SDL APIs present in old SDL2
// releases, because the app links against the TV's own SDL.
#include <SDL.h>

#include <map>
#include <memory>
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
  // Width in pixels of `text` at `size`.
  int measure(const std::string &text, int size);
  // Draws with the top of the line box at `y`. Returns the advance width.
  int draw(const std::string &text, int x, int y, int size, Color color);
  int line_height(int size);

 private:
  struct Glyph {
    SDL_Rect src;
    float xoff, yoff, advance;
  };
  struct Atlas {
    SDL_Texture *texture = nullptr;
    std::map<unsigned, Glyph> glyphs;
    float ascent = 0, line = 0, scale = 0;
  };
  Atlas *atlas(int size);

  SDL_Renderer *renderer_ = nullptr;
  std::vector<unsigned char> data_;
  void *info_ = nullptr;  // stbtt_fontinfo
  std::map<int, Atlas> atlases_;
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

 private:
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *circle_ = nullptr;    // anti-aliased disc used for corners
  SDL_Texture *triangle_ = nullptr;  // anti-aliased play glyph
  SDL_Texture *canvas_ = nullptr;
  int w_ = 1920, h_ = 1080;
  static constexpr int kCircle = 256;
};

// UTF-8 → code points.
std::vector<unsigned> utf8_decode(const std::string &text);
