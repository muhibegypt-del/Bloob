#pragma once
// Shared UI services and components: the app context every screen uses, text
// in the type scale, eased values, and the visual system's controls (buttons,
// cards, pills, list rows, panels, focus frames).
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
#include "theme.h"

inline Color with_alpha(Color c, int a) { return {c.r, c.g, c.b, static_cast<Uint8>(std::max(0, std::min(255, a)))}; }
inline Color fade(Color c, float alpha) { return with_alpha(c, static_cast<int>(c.a * std::max(0.0f, std::min(1.0f, alpha)))); }
inline Color mix(Color a, Color b, float t) {
  auto m = [t](Uint8 x, Uint8 y) { return static_cast<Uint8>(x + (y - x) * t + 0.5f); };
  return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

struct Fonts {
  Font regular, medium, semibold, bold;
};

class Screen;

// Eases toward a target; exponential approach, frame-rate independent.
struct Anim {
  float value = 0, target = 0;
  float rate = 14.0f;  // higher is snappier (theme:: *_rate)
  Anim() = default;
  explicit Anim(float r, float v = 0) : value(v), target(v), rate(r) {}
  void snap(float v) { value = target = v; }
  void to(float v) { target = v; }
  bool step(double dt);  // true while moving
  bool settled() const { return value == target; }
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

  // Navigation (implemented by the app shell).
  std::function<void(std::unique_ptr<Screen>)> push;
  std::function<void()> pop;
  std::function<void(std::unique_ptr<Screen>)> replace;  // swap the top screen
  std::function<void(const std::string &)> toast;
  std::function<void()> quit;
  std::function<void()> focus_tabs;   // root screens hand focus up to the tab bar
  std::function<void(bool)> show_tabs;  // root screens hide the bar when scrolled
};

Ctx &ctx();

// ---- text in the type scale

Font &font_for(Text style);
int size_for(Text style);
int line_for(Text style);
// Draws text with its top at `y`; `max_w` ellipsizes. Returns the width drawn.
int text(Text style, const std::string &s, int x, int y, Color color, int max_w = 0);
int text_right(Text style, const std::string &s, int right, int y, Color color);
int text_center(Text style, const std::string &s, int cx, int y, Color color, int max_w = 0);
int measure(Text style, const std::string &s);
// Wrapped paragraph; returns the y below it.
int paragraph(Text style, const std::string &s, int x, int y, Color color, int max_w, int max_lines);
std::string ellipsize(Font &font, const std::string &text, int size, int max_w);
std::vector<std::string> wrap(Font &font, const std::string &text, int size, int max_w, int max_lines);
std::string upper(std::string s);

// ---- components

constexpr int kPlayIcon = 1000;  // the solid play triangle (not a line icon)

struct ButtonSpec {
  std::string label;
  int icon = -1;          // Icon value, kPlayIcon, or -1
  bool focused = false;
  float lift = 1.0f;      // 0..1 focus animation progress
  float alpha = 1.0f;
  int h = theme::button_h;
};
int button_width(const ButtonSpec &b);
int draw_button(int x, int y, const ButtonSpec &b);  // returns width

// Artwork card: image (with missing-art fallback), optional title on the art,
// progress, and the focus treatment (lift, shadow, frame).
struct CardSpec {
  std::string art, fallback_art, title;
  int w = theme::card_w, h = theme::card_h, radius = theme::card_radius;
  bool title_on_art = false;
  double progress = -1;    // 0..1 to show a progress bar
  float focus = 0;         // 0..1 focus animation (lift + shadow)
  bool focused = false;    // frame appears at once
  float alpha = 1.0f;
};
// Draws centred on the card's resting box (x, y, w, h); returns the drawn box.
SDL_Rect draw_card(int x, int y, const CardSpec &c);
void draw_missing_art(int x, int y, int w, int h, int radius, const std::string &title, float alpha = 1.0f);
void draw_focus_frame(int x, int y, int w, int h, int radius, float alpha = 1.0f);

// Pills: compact controls (player track buttons, season tabs, filters).
int pill_width(const std::string &label, int icon = -1);
int draw_pill(int x, int y, const std::string &label, bool focused, bool selected = false, int icon = -1, float alpha = 1.0f);

// List row (menus, sources): white when focused.
void draw_row_focus(int x, int y, int w, int h);

// Panels over blurred art.
void draw_panel(int x, int y, int w, int h, float alpha = 1.0f);

void draw_progress(int x, int y, int w, int h, double fraction, Color fill = theme::accent,
                   Color track = with_alpha(theme::focus, 50));
void draw_badge(int x, int y, const std::string &label, Color color = theme::tertiary);  // outlined "4K"
int badge_width(const std::string &label);
// Spinner that only appears after `since` + spinner delay (quick loads show nothing).
void draw_spinner(int cx, int cy, int size, Color color = theme::secondary, long long since = 0);
// "OK  Play" hint: a key cap and its meaning.
int draw_hint(int x, int y, const std::string &key, const std::string &label);

// ---- formatting

std::string clock_text(long long ms);
std::string minutes_left(long long ms);
std::string join(const std::vector<std::string> &parts, const std::string &sep = " \xC2\xB7 ");
