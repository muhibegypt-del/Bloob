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
#include "headphones.h"
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
struct GlassSpec;

// Eases toward a target; exponential approach, frame-rate independent.
// An animated value. Two kinds, as Elements of A uses them [EoA M7, M8]:
//  - timed: a fixed duration with an easing curve (linear, ease-in p², ease-out
//    p(2−p), ease-in-out), restarted from the current value when the target moves;
//  - spring: damping / stiffness / mass, for scrolling, which retargets smoothly.
struct Anim {
  float value = 0, target = 0;
  Motion motion;
  Anim() = default;
  explicit Anim(Motion m, float v = 0) : value(v), target(v), motion(m) {}
  void snap(float v) {
    value = target = from_ = v;
    velocity_ = 0;
    t_ms_ = 0;
  }
  void to(float v);
  bool step(double dt);  // true while moving
  bool settled() const { return value == target; }

 private:
  float from_ = 0, velocity_ = 0;
  double t_ms_ = 0;
};

struct Ctx {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  Gfx gfx;
  Fonts fonts;
  Icons icons;
  Images images;
  Tasks api_tasks, image_tasks, fetch_tasks;
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
  int subtitle_size = 0;                        // the TV's subtitle size level, 0 smallest .. 4 (TV default 2)
  bool subtitle_background = false;             // a dark box behind subtitles
  std::string preferred_cut = "Theatrical";     // the edition the versions list opens on (2.8): the last one chosen
  std::string picture = "best";                 // picture preference: "best", "hdr10" or "sdr"
  std::string sources = "all";                  // source indexes: "all" or "torrentio"
  bool autoplay_next = true;                    // Up next starts the next episode by itself
  bool reduce_motion = false;                   // no auto-advancing hero, page movement or focus zoom
  bool reduce_transparency = false;             // Glass panels become solid
  bool high_contrast = false;                   // secondary and tertiary labels brighter
  // Headphones mode (3.2).
  std::string headphones = "auto";              // "auto" (the TV's sound on Bluetooth), "on" or "off"
  std::string headphone_room = "lounge";        // binaural::rooms() id
  int headphone_boost = 9;                      // dB after the render, 0..15
  bool headphone_night = false;
  bool headphone_xm5 = true;                    // the WF-1000XM5 correction
  int headphone_lead_ms = 0;                    // sound ahead of the picture, 0..500
  std::string sound_output;                     // the TV's sound output ("bt_soundbar", "tv_speaker"…), "" until known
  // Measured responses the service found in its data folder (3.2.1): each usable
  // one is a room choice "file:<id>"; the bytes are copied to /tmp when chosen.
  struct HeadphoneResponse {
    std::string id, name, why;
    bool usable = false;
  };
  std::vector<HeadphoneResponse> headphone_responses;
  std::map<std::string, std::string> headphone_cache;  // id -> local copy
  std::set<std::string> headphone_fetching;
  void refresh_headphone_responses();
  // Room choices, built-in rooms first: (id, name).
  std::vector<std::pair<std::string, std::string>> headphone_rooms() const;
  std::string headphone_room_name(const std::string &room) const;
  // The options now; fetches a chosen measured response on first use.
  HeadphoneOptions headphone_options();

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
  // R5a: a wake only means "events to drain". A visible change asks for a frame
  // here (or with animating / redraw_at); nothing else repaints.
  bool dirty = false;
  void invalidate() { dirty = true; }
  // R5b: memory policy (normal, playback, background), set by the shell.
  MemoryPolicy memory_policy = MemoryPolicy::Normal;
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
  std::function<void(const GlassSpec &)> glass;  // a glass panel over the top screen's artwork
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
// The y to pass to text() so the style's capital letters sit centred on `cy`
// (optical centring, rather than centring the whole line box).
int text_y_centered(Text style, int cy);
// White at the role's opacity (Glass); `alpha` fades it further.
Color label(Label role, float alpha = 1.0f);
// Wrapped paragraph; returns the y below it.
int paragraph(Text style, const std::string &s, int x, int y, Color color, int max_w, int max_lines);
std::string ellipsize(Font &font, const std::string &text, int size, int max_w);
std::vector<std::string> wrap(Font &font, const std::string &text, int size, int max_w, int max_lines);
std::string upper(std::string s);

// ---- components

constexpr int kPlayIcon = 1000;   // the solid play triangle (not a line icon)
constexpr int kPauseIcon = 1001;  // two solid bars

// A button in a list a screen builds (drawn as a GlassButton).
struct ButtonSpec {
  std::string label;
  int icon = -1;  // Icon value, kPlayIcon, kPauseIcon, or -1
};

// Artwork card: image (with missing-art fallback), optional title on the art,
// progress, and the focus treatment (lift, shadow, frame).
// A row of cards draws in passes (all shadows, then all bodies, then all text)
// so draws that share a texture are adjacent and SDL can batch them (study G6).
enum class CardPass { All, Shadow, Body, Text };
struct CardSpec {
  std::string art, fallback_art, title;
  int w = theme::card_w, h = theme::card_h, radius = theme::card_radius;
  bool title_on_art = false;
  double progress = -1;    // 0..1 to show a progress bar
  float focus = 0;         // 0..1 focus animation (lift + shadow)
  bool focused = false;    // frame appears at once
  std::string sub;         // Glass, with title_on_art: right-aligned note ("6 min left")
  float alpha = 1.0f;
  CardPass pass = CardPass::All;
};
// Draws centred on the card's resting box (x, y, w, h); returns the drawn box.
SDL_Rect draw_card(int x, int y, const CardSpec &c);
void draw_missing_art(int x, int y, int w, int h, int radius, const std::string &title, float alpha = 1.0f);
void draw_focus_frame(int x, int y, int w, int h, int radius, float alpha = 1.0f);

// Pills: compact controls (player track buttons, season tabs, filters).

// List row (menus, sources): white when focused.

// Panels over blurred art.
void draw_panel(int x, int y, int w, int h, float alpha = 1.0f);

// ---- Glass (2.0)
// Frost: the blurred art under the panel, baked (needs the full-screen art the
// panel floats over). Smoke: over video, which the UI cannot see. Solid: the
// fallback in Lite, with Reduce transparency, and while frost is baking.
enum class GlassKind { Frost, Smoke, Solid };
struct GlassSpec {
  int x = 0, y = 0, w = 0, h = 0;
  int radius = theme::glass_radius;
  GlassKind kind = GlassKind::Frost;
  std::string art;        // Frost: the full-screen artwork under the panel
  float alpha = 1.0f;
  bool shadow = true;
};
void draw_glass(const GlassSpec &g);
// Glass control (button or pill): focused = white with dark label and lift;
// candidate = chosen but not focused (20% white, check); otherwise glass.
// Button variants [EoA B10-B12]: Dark over artwork (hero, title page, Up next),
// Default on panels and sheets, Player on the transport bar.
enum class ButtonStyle { Dark, Default, Player };

struct GlassButton {
  std::string label;
  int icon = -1;          // Icon value, kPlayIcon, or -1 (icon-only when label is empty)
  bool focused = false;
  bool candidate = false;
  bool disabled = false;
  float lift = 1.0f;      // 0..1 focus animation
  float alpha = 1.0f;
  int h = theme::button_h;
  Text style = Text::Button;
  ButtonStyle variant = ButtonStyle::Dark;
  int min_w = 0, max_w = 0;  // labelled buttons only; 0 = none
  // Season-pill treatment [EoA P1-P6]: 66 high, 30 padding, no fill at rest,
  // 10% white as candidate, white when focused, growing 8 px wide.
  bool pill = false;
};
int glass_button_width(const GlassButton &b);
// Spoken position, "3 of 12" (G7).
std::string position(int index, int count);
int draw_glass_button(int x, int y, const GlassButton &b);  // returns width
// Small outlined fact on art ("4K", "Dolby Vision").
int draw_glass_badge(int x, int y, const std::string &label, float alpha = 1.0f);  // returns width

void draw_progress(int x, int y, int w, int h, double fraction, Color fill = rgba(0xffffff),
                   Color track = rgba(0xffffff, 50));
// Spinner that only appears after `since` + spinner delay (quick loads show nothing).
void draw_spinner(int cx, int cy, int size, Color color = rgba(0xffffff, 166), long long since = 0);
// "OK  Play" hint: a key cap and its meaning.
int draw_hint(int x, int y, const std::string &key, const std::string &label);

// ---- formatting

std::string clock_text(long long ms);
std::string minutes_left(long long ms);
std::string join(const std::vector<std::string> &parts, const std::string &sep = " \xC2\xB7 ");
