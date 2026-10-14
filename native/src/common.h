#pragma once
// Pieces several screens share: the full-bleed backdrop (crossfades only once
// the next picture is decoded, can soften into blur), its scrims, the title
// block (logo art or set type) and the meta line.
#include <string>

#include "model.h"
#include "ui.h"

class Backdrop {
 public:
  // Change the picture; it fades in over the old one once decoded.
  void set(const std::string &url, bool immediate = false);
  const std::string &url() const { return current_; }
  void update(double dt);
  // `dim` 0..1 darkens; `blur` 0..1 crossfades to a soft version (sheets, scrolled pages).
  void draw(float dim = 0, float blur = 0);

 private:
  std::string current_, previous_;
  Anim fade_{theme::backdrop_rate, 1};
};

// Readable gradients over art: from the left (where text sits) and from the bottom.
void draw_scrims(float left = 1.0f, int bottom_from = 520, float bottom = 1.0f);

// Logo art when the title has one (bottom-aligned in the box), else the name in
// display type. Returns the y just below the box.
int draw_title(const Item &item, int x, int top, int box_w, int box_h, float alpha = 1.0f, bool centred = false);

// Essential context only: "2024 · Action, Thriller · 2h 26m". Ratings and
// technical facts are elsewhere, on request.
std::string meta_line(const Item &item);
std::string runtime_text(const std::string &runtime);
int runtime_minutes(const std::string &runtime);  // 0 when unknown

// Title details ahead of need: Home asks once focus rests on a card, Details reuses it.
std::string meta_key(const Item &item);
void prefetch_meta(const Item &item);
