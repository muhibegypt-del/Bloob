#pragma once
// Shared UI services and components: the app context every screen uses, text
// in the type scale, eased values, and the visual system's controls (buttons,
// cards, pills, list rows, panels, focus frames).
#include <SDL.h>

#include <functional>
#include <map>
#include <set>
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
  bool account_known = false;   // the service has answered about the pairing at least once
  bool settings_loaded = false; // saved settings have arrived from the service
  bool welcome_seen = false;    // the one-time AllDebrid note was shown
  std::string language = "English";            // preferred audio language: ranking and the starting track
  std::string subtitle_language = "Off";        // subtitles to switch on at start ("Off" = none)
  bool autoplay_next = true;                    // Up next starts the next episode by itself
  bool reduce_motion = false;                   // no auto-advancing hero, page movement or focus zoom

  // Effects tier: Full until rendering is measured to be slow, then Lite for the session
  // (no soft shadows, plain dimmed backdrops, a once-a-second player clock).
  enum class Effects { Full, Lite };
  Effects effects = Effects::Full;
  int slow_windows = 0;
  std::string perf_line;                        // last interaction-timing window, for Diagnostics
  bool lite() const { return effects == Effects::Lite; }

  // Network state from the TV's connection manager (unknown until it answers).
  bool online = true;
  std::string network = "";                     // "wired", "wifi" or ""

  // Launch stages, logged once each with the time since start.
  std::set<std::string> marks;
  void mark(const char *stage);

  // Title details fetched ahead of need (Home dwell), reused by Details.
  struct MetaCache {
    struct Entry {
      Json json;
      long long at = 0;
    };
    std::map<std::string, Entry> entries;
    std::set<std::string> in_flight;
    const Json *get(const std::string &key, long long now) const {
      auto it = entries.find(key);
      return it != entries.end() && now - it->second.at < 10 * 60 * 1000 ? &it->second.json : nullptr;
    }
    void put(const std::string &key, Json json, long long now) {
      if (entries.size() >= 24 && !entries.count(key)) {
        auto oldest = entries.begin();
        for (auto it = entries.begin(); it != entries.end(); ++it)
          if (it->second.at < oldest->second.at) oldest = it;
        entries.erase(oldest);
      }
      entries[key] = {std::move(json), now};
    }
  } meta_cache;

  // An episode resolved ahead of time during the credits (Up next), for Sources to pick up.
  struct Prepared {
    std::string video_id, job_id, headline;
    long long at = 0;
  } prepared_next;
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
