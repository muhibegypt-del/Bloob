#pragma once
// Pieces several screens share: the full-bleed backdrop with cross-fades and
// scrims, the title block (logo art or set type), and the meta line.
#include <string>

#include "model.h"
#include "ui.h"

class Backdrop {
 public:
  // Change the image; the new one fades in over the old once it has loaded.
  void set(const std::string &url);
  const std::string &url() const { return current_; }
  void update(double dt);
  // `dim` 0..1 darkens the picture (for focus moving into rows).
  void draw(float dim = 0);

 private:
  std::string current_, previous_;
  Anim fade_;
};

// Left-to-right and bottom scrims so text over a backdrop stays readable.
void draw_scrims(float left = 1.0f, int bottom_from = 520, float bottom = 1.0f);

// Logo art when the title has one (bottom-aligned in the box), else the name in
// large type. Returns the y just below what was drawn.
int draw_title(const Item &item, int x, int top, int box_w, int box_h, int text_size, float alpha = 1.0f);

// "2024 · 2h 25m · Action, Adventure · IMDb 8.0"
std::string meta_line(const Item &item);
std::string runtime_text(const std::string &runtime);
