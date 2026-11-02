#pragma once
// Styled subtitles (4.0): Bloob's own reader for ASS/SSA, the format anime
// fansubs and many Blu-ray rips use. It honours what most scripts rely on:
// the script's styles (font size, bold, italic, colours, outline, shadow,
// alignment, margins), the play resolution, and the common override tags:
// \an \a \pos \move \fad \fade(2 values) \c \1c \3c \4c \alpha \1a \3a \4a
// \bord \shad \b \i \fs \r \p (drawings skipped) \N \n \h. Karaoke, \t
// transforms, clips, rotation and blur are read and ignored (their text still
// shows). Fonts are Bloob's own Inter weights; a script's own fonts are not
// loaded. Pure: layout() works with any measuring function, so it is tested
// on the desktop without a renderer.
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ass {

struct Rgba {
  uint8_t r = 255, g = 255, b = 255, a = 255;
  bool operator==(const Rgba &o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
};

struct Style {
  std::string name = "Default";
  float size = 48;  // script pixels: the cell height, as VSFilter reads it
  Rgba primary{255, 255, 255, 255}, outline{0, 0, 0, 255}, back{0, 0, 0, 128};
  bool bold = false, italic = false;
  float outline_w = 2, shadow = 2;
  int border_style = 1;  // 1 outline and shadow, 3 opaque box
  int align = 2;         // numpad: 1-3 bottom, 4-6 middle, 7-9 top
  int margin_l = 10, margin_r = 10, margin_v = 10;
};

struct Script {
  int res_x = 384, res_y = 288;
  int wrap_style = 0;
  std::map<std::string, Style> styles;
  bool valid = false;  // a [Script Info] or styles section was found
  const Style &style(const std::string &name) const;
};

// "[Script Info]", "[V4+ Styles]" (or SSA's "[V4 Styles]") from the track's header.
Script parse_header(const std::string &header);
// "&H00FFFFFF", "&HFFFFFF&", decimal: ASS colours are AABBGGRR, alpha 0 opaque.
Rgba parse_colour(const std::string &s);

struct Run {
  std::string text;
  Style look;  // the style after this run's overrides
};

struct Event {
  int layer = 0;
  int align = 2;
  int margin_l = 0, margin_r = 0, margin_v = 0;  // 0: the style's
  bool has_pos = false, has_move = false;
  float x = 0, y = 0, x2 = 0, y2 = 0;
  long long move_t1 = 0, move_t2 = 0;  // ms from the event's start; both 0: its whole length
  long long fade_in = 0, fade_out = 0;
  std::vector<std::vector<Run>> lines;  // split at \N
  bool empty() const;
};

// One Matroska ASS packet: "ReadOrder,Layer,Style,Name,MarginL,MarginR,MarginV,Effect,Text".
Event parse_event(const Script &script, const std::string &packet);

// Where each run goes on a `w` x `h` screen.
struct Placed {
  std::string text;
  float x = 0, y = 0;  // top-left of the run's line box
  int size = 0;        // Font size (cell height) in screen pixels
  bool bold = false, italic = false;
  Rgba fill, outline, shadow_colour, box;
  float outline_px = 0, shadow_px = 0;
  bool opaque_box = false;
  int width = 0, line_h = 0;
};

struct Layout {
  std::vector<Placed> runs;
  float top = 0, bottom = 0;  // the block's extent, for stacking
  int align = 2;
  bool positioned = false;
};

using Measure = std::function<int(const std::string &text, int size, bool bold)>;
using LineHeight = std::function<int(int size)>;

// Lays out an event shown `ms` after its start (moves and fades) and `left_ms`
// before its end. Long unpositioned lines wrap at spaces within the margins.
Layout layout(const Script &script, const Event &event, int w, int h, long long ms, long long left_ms,
              const Measure &measure, const LineHeight &line_height);

// Moves unpositioned blocks of the same alignment apart, as players do when
// two speakers talk at once: bottom ones upwards, top ones downwards.
void stack(std::vector<Layout> &layouts, int h);

}  // namespace ass
