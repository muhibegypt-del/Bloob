#pragma once
// Shared UI services and building blocks: the app context every screen uses,
// the theme, text layout (ellipsis, wrapping), eased animation values and the
// common controls (buttons, chips, focus rings, progress bars).
#include <SDL.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "api.h"
#include "gfx.h"
#include "icons.h"
#include "images.h"
#include "keys.h"
#include "platform.h"
#include "tasks.h"

constexpr int W = 1920;
constexpr int H = 1080;

namespace theme {
const Color bg = rgba(0x07080a);
const Color text = rgba(0xf2f1ee);
const Color text2 = rgba(0xc7cbd1);
const Color text3 = rgba(0x8d929b);
const Color text4 = rgba(0x6d737c);
const Color panel = rgba(0x121418);
const Color panel2 = rgba(0x1b1e23);
const Color line = rgba(0x1b1d22);
const Color accent = rgba(0xf2b84b);
const Color ok = rgba(0x5fd38d);
const Color warn = rgba(0xf2b84b);
const Color warn_ink = rgba(0x8a5a00);
const Color bad = rgba(0xff7a6b);
const Color white = rgba(0xffffff);
const Color ink = rgba(0x07080a);
const Color ink2 = rgba(0x3a3f47);
}  // namespace theme

inline Color with_alpha(Color c, int a) { return {c.r, c.g, c.b, static_cast<Uint8>(std::max(0, std::min(255, a)))}; }
inline Color mix(Color a, Color b, float t) {
  auto m = [t](Uint8 x, Uint8 y) { return static_cast<Uint8>(x + (y - x) * t + 0.5f); };
  return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

struct Fonts {
  Font regular, semibold, bold, heavy;
};

class Screen;

// Eases toward a target; exponential approach, frame-rate independent.
struct Anim {
  float value = 0, target = 0;
  float rate = 14.0f;  // higher is snappier
  void snap(float v) { value = target = v; }
  void to(float v) { target = v; }
  bool step(double dt);  // returns true while moving
};

struct Ctx {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  Gfx gfx;
  Fonts fonts;
  Icons icons;
  Images images;
  Tasks api_tasks, image_tasks;
  Api api;
  WebOS webos;
  std::string dir, app_id;
  std::string tv_model = "…";
  std::string last_key = "Press any remote key";
  std::string service_error;   // set when the background service cannot start
  bool paired = false;          // AllDebrid paired (refreshed on demand)
  std::string language = "English";            // preferred audio language for ranking
  std::vector<std::string> recent_searches;     // newest first
  void save_settings();
  long long now = 0;
  double dt = 0;
  bool animating = false;       // a screen needs another frame soon
  long long next_redraw = 0;    // a screen wants a frame at this time (0 = none)
  void redraw_at(long long t) {
    if (next_redraw == 0 || t < next_redraw) next_redraw = t;
  }

  // Navigation (implemented by the app).
  std::function<void(std::unique_ptr<Screen>)> push;
  std::function<void()> pop;
  std::function<void(std::unique_ptr<Screen>)> replace;  // swap the top screen
  std::function<void(const std::string &)> toast;
  std::function<void()> quit;
  std::function<void()> refresh_home;  // continue-watching changed
};

Ctx &ctx();

// ---- text

// Fits `text` into `max_w`, ending with "…" when cut.
std::string ellipsize(Font &font, const std::string &text, int size, int max_w);
// Wraps into at most `max_lines` lines no wider than `max_w`; the last is ellipsized.
std::vector<std::string> wrap(Font &font, const std::string &text, int size, int max_w, int max_lines);
int draw_text(Font &font, const std::string &text, int x, int y, int size, Color color, int max_w = 0);
int draw_text_right(Font &font, const std::string &text, int right, int y, int size, Color color);
int draw_text_center(Font &font, const std::string &text, int cx, int y, int size, Color color);
// Draws a paragraph; returns the y below it.
int draw_paragraph(Font &font, const std::string &text, int x, int y, int size, int line_h, Color color, int max_w, int max_lines);
std::string upper(std::string s);

// ---- controls

// A pill button. Returns its width. `icon`: an Icon value, kPlayIcon, or -1.
constexpr int kPlayIcon = 1000;
int button_width(const std::string &label, bool has_icon, int h = 76);
int draw_button(int x, int y, const std::string &label, bool focused, bool primary = false, int icon = -1, int h = 76, float lift = 0,
                float alpha = 1.0f);
void draw_chip(int x, int y, const std::string &label, bool focused, Color bg = theme::panel, Color fg = theme::text2);
int chip_width(const std::string &label);
void draw_focus_ring(int x, int y, int w, int h, int radius, float strength = 1.0f);
void draw_progress(int x, int y, int w, int h, double fraction, Color fill = theme::accent, Color track = with_alpha(theme::white, 50));
void draw_badge(int x, int y, const std::string &label, Color color = theme::text4);  // outlined "4K"
int badge_width(const std::string &label);
void draw_spinner(int cx, int cy, int size, Color color = theme::text2);

// ---- formatting

std::string clock_text(long long ms);
std::string minutes_left(long long ms);
std::string join(const std::vector<std::string> &parts, const std::string &sep = " \xC2\xB7 ");
