#include "ui.h"

#include <cmath>
#include <cstdio>

Ctx &ctx() {
  static Ctx instance;
  return instance;
}

void Ctx::save_settings() {
  std::string list;
  for (auto &s : recent_searches) list += (list.empty() ? "" : ",") + Json::quote(s);
  api.post("/api/settings", "{\"language\":" + Json::quote(language) + ",\"recentSearches\":[" + list + "]}");
}

bool Anim::step(double dt) {
  if (value == target) return false;
  float k = 1.0f - static_cast<float>(std::exp(-rate * dt));
  value += (target - value) * k;
  if (std::fabs(target - value) < 0.002f * std::max(1.0f, std::fabs(target))) {
    value = target;
    return false;
  }
  return true;
}

// ---------------------------------------------------------------- text

std::string ellipsize(Font &font, const std::string &text, int size, int max_w) {
  if (max_w <= 0 || font.measure(text, size) <= max_w) return text;
  const std::string dots = "\xE2\x80\xA6";
  std::vector<unsigned> cps = utf8_decode(text);
  // Binary search the longest prefix that fits with the ellipsis.
  size_t lo = 0, hi = cps.size();
  auto encode = [&](size_t n) {
    std::string out;
    for (size_t i = 0; i < n; i++) {
      unsigned cp = cps[i];
      if (cp < 0x80) out += static_cast<char>(cp);
      else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
      else if (cp < 0x10000) { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
      else { out += static_cast<char>(0xF0 | (cp >> 18)); out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
    }
    return out;
  };
  while (lo < hi) {
    size_t mid = (lo + hi + 1) / 2;
    if (font.measure(encode(mid) + dots, size) <= max_w) lo = mid;
    else hi = mid - 1;
  }
  std::string cut = encode(lo);
  while (!cut.empty() && cut.back() == ' ') cut.pop_back();
  return cut + dots;
}

std::vector<std::string> wrap(Font &font, const std::string &text, int size, int max_w, int max_lines) {
  std::vector<std::string> words, lines;
  std::string word;
  for (char c : text) {
    if (c == ' ' || c == '\n' || c == '\t') {
      if (!word.empty()) words.push_back(word);
      word.clear();
    } else {
      word += c;
    }
  }
  if (!word.empty()) words.push_back(word);
  size_t i = 0;
  while (i < words.size() && static_cast<int>(lines.size()) < max_lines) {
    std::string line = words[i++];
    while (i < words.size() && font.measure(line + " " + words[i], size) <= max_w) line += " " + words[i++];
    lines.push_back(line);
  }
  if (!lines.empty()) {
    std::string &last = lines.back();
    if (i < words.size()) last = ellipsize(font, last + " " + words[i] + " \xE2\x80\xA6", size, max_w);
    else if (font.measure(last, size) > max_w) last = ellipsize(font, last, size, max_w);
  }
  return lines;
}

int draw_text(Font &font, const std::string &text, int x, int y, int size, Color color, int max_w) {
  return font.draw(max_w > 0 ? ellipsize(font, text, size, max_w) : text, x, y, size, color);
}

int draw_text_right(Font &font, const std::string &text, int right, int y, int size, Color color) {
  int w = font.measure(text, size);
  font.draw(text, right - w, y, size, color);
  return w;
}

int draw_text_center(Font &font, const std::string &text, int cx, int y, int size, Color color) {
  int w = font.measure(text, size);
  font.draw(text, cx - w / 2, y, size, color);
  return w;
}

int draw_paragraph(Font &font, const std::string &text, int x, int y, int size, int line_h, Color color, int max_w, int max_lines) {
  for (auto &line : wrap(font, text, size, max_w, max_lines)) {
    font.draw(line, x, y, size, color);
    y += line_h;
  }
  return y;
}

std::string upper(std::string s) {
  for (auto &c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
  return s;
}

// ---------------------------------------------------------------- controls

int button_width(const std::string &label, bool has_icon, int h) {
  int size = h >= 70 ? 26 : 22;
  int w = ctx().fonts.heavy.measure(label, size) + (h >= 70 ? 72 : 52);
  if (has_icon) w += size + 12;
  return w;
}

int draw_button(int x, int y, const std::string &label, bool focused, bool primary, int icon, int h, float lift, float alpha) {
  Ctx &c = ctx();
  int size = h >= 70 ? 26 : 22;
  int w = button_width(label, icon >= 0, h);
  auto fade = [alpha](Color col) { return with_alpha(col, static_cast<int>(col.a * alpha)); };
  Color bg = fade(focused ? theme::white : (primary ? with_alpha(theme::white, 40) : with_alpha(theme::white, 28)));
  Color fg = fade(focused ? theme::ink : theme::text);
  if (focused) {
    // Lift: grow slightly and glow.
    int grow = static_cast<int>(std::lround(4 * lift));
    c.gfx.fill_round(x - 6 - grow, y - 6 - grow, w + 12 + 2 * grow, h + 12 + 2 * grow, (h + 12) / 2 + grow, fade(with_alpha(theme::white, 50)));
    c.gfx.fill_round(x - grow, y - grow, w + 2 * grow, h + 2 * grow, h / 2 + grow, bg);
  } else {
    c.gfx.fill_round(x, y, w, h, h / 2, bg);
  }
  int tx = x + (h >= 70 ? 36 : 26);
  if (icon >= 0) {
    int is = size + 4;
    if (icon == kPlayIcon) c.gfx.triangle_right(tx - 2, y + (h - is) / 2, is, fg);
    else c.icons.draw(static_cast<Icon>(icon), tx - 2, y + (h - is) / 2, is, fg);
    tx += size + 12;
  }
  c.fonts.heavy.draw(label, tx, y + (h - c.fonts.heavy.line_height(size)) / 2, size, fg);
  return w;
}

int chip_width(const std::string &label) { return ctx().fonts.semibold.measure(label, 20) + 48; }

void draw_chip(int x, int y, const std::string &label, bool focused, Color bg, Color fg) {
  Ctx &c = ctx();
  int w = chip_width(label);
  c.gfx.fill_round(x, y, w, 52, 26, focused ? theme::white : bg);
  c.fonts.semibold.draw(label, x + 24, y + 13, 20, focused ? theme::ink : fg);
}

void draw_focus_ring(int x, int y, int w, int h, int radius, float strength) {
  Ctx &c = ctx();
  int a = static_cast<int>(255 * strength);
  if (a <= 0) return;
  // Ring outside the box: white 4 px band drawn as a filled round rect behind would hide art,
  // so draw four rounded strips.
  int t = 4, gap = 4;
  c.gfx.stroke_round(x - gap - t, y - gap - t, w + 2 * (gap + t), h + 2 * (gap + t), radius + gap + t, t, with_alpha(theme::white, a));
}

void draw_progress(int x, int y, int w, int h, double fraction, Color fill, Color track) {
  Ctx &c = ctx();
  c.gfx.fill_round(x, y, w, h, h / 2, track);
  int fw = static_cast<int>(w * std::max(0.0, std::min(1.0, fraction)));
  if (fw > 0) c.gfx.fill_round(x, y, std::max(fw, h), h, h / 2, fill);
}

int badge_width(const std::string &label) { return ctx().fonts.bold.measure(label, 17) + 22; }

void draw_badge(int x, int y, const std::string &label, Color color) {
  Ctx &c = ctx();
  int w = badge_width(label);
  c.gfx.stroke_round(x, y, w, 32, 6, 2, color);
  c.fonts.bold.draw(label, x + 11, y + 6, 17, theme::text2);
}

void draw_spinner(int cx, int cy, int size, Color color) {
  Ctx &c = ctx();
  double angle = std::fmod(c.now * 0.36, 360.0);
  c.icons.spinner(cx, cy, size, angle, color);
  c.animating = true;
}

// ---------------------------------------------------------------- formatting

std::string clock_text(long long ms) {
  if (ms < 0) return "--:--";
  long long s = ms / 1000;
  char buf[32];
  if (s >= 3600) std::snprintf(buf, sizeof buf, "%lld:%02lld:%02lld", s / 3600, (s / 60) % 60, s % 60);
  else std::snprintf(buf, sizeof buf, "%lld:%02lld", s / 60, s % 60);
  return buf;
}

std::string minutes_left(long long ms) {
  long long m = (ms + 59999) / 60000;
  if (m >= 60) return std::to_string(m / 60) + "h " + std::to_string(m % 60) + "m left";
  return std::to_string(std::max(1LL, m)) + " min left";
}

std::string join(const std::vector<std::string> &parts, const std::string &sep) {
  std::string out;
  for (auto &p : parts) {
    if (p.empty()) continue;
    if (!out.empty()) out += sep;
    out += p;
  }
  return out;
}
