#pragma once
// Line icons drawn from a few strokes on a 24-unit grid, rasterised with
// anti-aliasing at the exact pixel size they are shown at, and cached.
#include <SDL.h>

#include <map>
#include <utility>

#include "gfx.h"

enum class Icon { Home, Search, Film, Tv, Settings, Check, ChevronRight, ChevronLeft, ChevronDown, Back10, Forward10,
                  Audio, Subtitles, Spinner, Restart, Next, Link, Warning, Info, Close, Bolt };

class Icons {
 public:
  void init(SDL_Renderer *renderer) { renderer_ = renderer; }
  void shutdown();
  void draw(Icon icon, int x, int y, int size, Color color);
  // A rotating spinner arc centred on (cx, cy).
  void spinner(int cx, int cy, int size, double angle_degrees, Color color);

 private:
  SDL_Texture *texture(Icon icon, int size);
  SDL_Renderer *renderer_ = nullptr;
  std::map<std::pair<int, int>, SDL_Texture *> cache_;
};
