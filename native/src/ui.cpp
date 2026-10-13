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

// ---------------------------------------------------------------- type scale

namespace {
struct Spec {
  int weight;  // 0 regular, 1 medium, 2 semibold, 3 bold
  int size, line;
};
Spec spec(Text style) {
  switch (style) {
    case Text::Display: return {3, 76, 84};
    case Text::Title1: return {3, 48, 56};
    case Text::Title2: return {2, 34, 42};
    case Text::Headline: return {2, 28, 36};
    case Text::Body: return {0, 26, 38};
    case Text::Callout: return {1, 24, 32};
    case Text::Caption: return {1, 21, 28};
    case Text::Footnote: return {1, 19, 26};
    case Text::Eyebrow: return {2, 19, 24};
    case Text::Button: return {2, 26, 32};
  }
  return {0, 24, 32};
}
}  // namespace

Font &font_for(Text style) {
  Fonts &f = ctx().fonts;
  switch (spec(style).weight) {
    case 3: return f.bold;
    case 2: return f.semibold;
    case 1: return f.medium;
    default: return f.regular;
  }
}
int size_for(Text style) { return spec(style).size; }
int line_for(Text style) { return spec(style).line; }

int text(Text style, const std::string &s, int x, int y, Color color, int max_w) {
  Font &f = font_for(style);
  int size = size_for(style);
  return f.draw(max_w > 0 ? ellipsize(f, s, size, max_w) : s, x, y, size, color);
}

int text_right(Text style, const std::string &s, int right, int y, Color color) {
  int w = measure(style, s);
  font_for(style).draw(s, right - w, y, size_for(style), color);
  return w;
}

int text_center(Text style, const std::string &s, int cx, int y, Color color, int max_w) {
  Font &f = font_for(style);
  std::string shown = max_w > 0 ? ellipsize(f, s, size_for(style), max_w) : s;
  int w = f.measure(shown, size_for(style));
  f.draw(shown, cx - w / 2, y, size_for(style), color);
  return w;
}

int measure(Text style, const std::string &s) { return font_for(style).measure(s, size_for(style)); }

int paragraph(Text style, const std::string &s, int x, int y, Color color, int max_w, int max_lines) {
  for (auto &line : wrap(font_for(style), s, size_for(style), max_w, max_lines)) {
    font_for(style).draw(line, x, y, size_for(style), color);
    y += line_for(style);
  }
  return y;
}

std::string ellipsize(Font &font, const std::string &text, int size, int max_w) {
  if (max_w <= 0 || font.measure(text, size) <= max_w) return text;
  const std::string dots = "\xE2\x80\xA6";
  std::vector<unsigned> cps = utf8_decode(text);
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
  size_t lo = 0, hi = cps.size();
  while (lo < hi) {
    size_t mid = (lo + hi + 1) / 2;
    if (font.measure(encode(mid) + dots, size) <= max_w) lo = mid;
    else hi = mid - 1;
  }
  std::string cut = encode(lo);
  while (!cut.empty() && (cut.back() == ' ' || cut.back() == ',')) cut.pop_back();
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

std::string upper(std::string s) {
  for (auto &c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
  return s;
}

// ---------------------------------------------------------------- components

int button_width(const ButtonSpec &b) {
  int w = measure(Text::Button, b.label) + 72;
  if (b.icon >= 0) w += 38;
  return w;
}

int draw_button(int x, int y, const ButtonSpec &b) {
  Ctx &c = ctx();
  int w = button_width(b), h = b.h;
  float s = b.focused ? 1.0f + 0.05f * b.lift : 1.0f;
  int gw = static_cast<int>(w * s), gh = static_cast<int>(h * s);
  int gx = x - (gw - w) / 2, gy = y - (gh - h) / 2;
  if (b.focused) {
    c.gfx.shadow(gx, gy, gw, gh, gh / 2, 22, 10, static_cast<Uint8>(150 * b.lift * b.alpha));
    c.gfx.fill_round(gx, gy, gw, gh, gh / 2, fade(theme::focus, b.alpha));
  } else {
    c.gfx.fill_round(gx, gy, gw, gh, gh / 2, fade(theme::glass_light, b.alpha));
  }
  Color fg = fade(b.focused ? theme::ink : theme::primary, b.alpha);
  int tx = gx + (gw - (w - 72)) / 2;
  if (b.icon >= 0) {
    int is = 28;
    int iy = gy + (gh - is) / 2;
    if (b.icon == kPlayIcon) c.gfx.triangle_right(tx - 2, iy, is, fg);
    else c.icons.draw(static_cast<Icon>(b.icon), tx - 2, iy, is, fg);
    tx += 38;
  }
  text(Text::Button, b.label, tx, gy + (gh - line_for(Text::Button)) / 2, fg);
  return w;
}

void draw_missing_art(int x, int y, int w, int h, int radius, const std::string &title, float alpha) {
  Ctx &c = ctx();
  // A quiet, title-specific tone instead of an empty box.
  unsigned hash = 2166136261u;
  for (char ch : title) hash = (hash ^ static_cast<unsigned char>(ch)) * 16777619u;
  static const unsigned tones[] = {0x1f2a33, 0x2a2333, 0x33281f, 0x1f3329, 0x2b2b30, 0x33201f, 0x1f2633};
  Color base = rgba(tones[hash % (sizeof tones / sizeof tones[0])]);
  c.gfx.fill_round(x, y, w, h, radius, fade(base, alpha));
  if (w >= 200 && !title.empty()) {
    auto lines = wrap(c.fonts.semibold, title, 26, w - 48, 2);
    int ty = y + h - 24 - static_cast<int>(lines.size()) * 32;
    for (auto &l : lines) {
      c.fonts.semibold.draw(l, x + 24, ty, 26, fade(theme::secondary, alpha));
      ty += 32;
    }
  }
}

void draw_focus_frame(int x, int y, int w, int h, int radius, float alpha) {
  ctx().gfx.stroke_round(x - 1, y - 1, w + 2, h + 2, radius + 1, 4, fade(theme::focus, alpha));
}

SDL_Rect draw_card(int x, int y, const CardSpec &cs) {
  Ctx &c = ctx();
  float s = 1.0f + (theme::card_lift - 1.0f) * cs.focus;
  int w = static_cast<int>(cs.w * s), h = static_cast<int>(cs.h * s);
  int cx = x - (w - cs.w) / 2, cy = y - (h - cs.h) / 2;
  if (cs.focus > 0.01f) c.gfx.shadow(cx, cy, w, h, cs.radius, 28, 16, static_cast<Uint8>(170 * cs.focus * cs.alpha));
  ImageRef img = cs.art.empty() ? ImageRef{} : c.images.get(cs.art, cs.w, cs.h, cs.radius, Fit::Cover, cs.title_on_art);
  if ((img.failed || cs.art.empty()) && !cs.fallback_art.empty())
    img = c.images.get(cs.fallback_art, cs.w, cs.h, cs.radius, Fit::Cover, cs.title_on_art);
  bool missing = img.failed || (cs.art.empty() && cs.fallback_art.empty());
  if (missing) {
    draw_missing_art(cx, cy, w, h, cs.radius, cs.title, cs.alpha);
  } else {
    if (img.alpha < 255) c.gfx.fill_round(cx, cy, w, h, cs.radius, fade(theme::raised, cs.alpha));
    if (img.texture) c.gfx.texture(img.texture, cx, cy, w, h, static_cast<Uint8>(img.alpha * cs.alpha));
    if (cs.title_on_art && !cs.title.empty())
      text(Text::Headline, cs.title, cx + 22, cy + h - 22 - line_for(Text::Headline), fade(theme::primary, cs.alpha), w - 44);
  }
  if (cs.progress >= 0) {
    int pw = w - 36;
    c.gfx.fill_round(cx + 18, cy + h - 20, pw, 6, 3, fade(with_alpha(theme::focus, 70), cs.alpha));
    int fw = static_cast<int>(pw * std::max(0.0, std::min(1.0, cs.progress)));
    if (fw > 0) c.gfx.fill_round(cx + 18, cy + h - 20, std::max(6, fw), 6, 3, fade(theme::accent, cs.alpha));
  }
  if (cs.focused) draw_focus_frame(cx, cy, w, h, cs.radius, cs.alpha);
  return SDL_Rect{cx, cy, w, h};
}

int pill_width(const std::string &label, int icon) { return measure(Text::Callout, label) + 52 + (icon >= 0 ? 36 : 0); }

int draw_pill(int x, int y, const std::string &label, bool focused, bool selected, int icon, float alpha) {
  Ctx &c = ctx();
  int w = pill_width(label, icon), h = 56;
  if (focused) c.gfx.shadow(x, y, w, h, h / 2, 16, 8, static_cast<Uint8>(130 * alpha));
  Color bg = focused ? theme::focus : selected ? with_alpha(theme::focus, 58) : theme::glass_light;
  c.gfx.fill_round(x, y, w, h, h / 2, fade(bg, alpha));
  Color fg = fade(focused ? theme::ink : theme::primary, alpha);
  int tx = x + 26;
  if (icon >= 0) {
    c.icons.draw(static_cast<Icon>(icon), tx - 4, y + 13, 30, fg);
    tx += 36;
  }
  text(Text::Callout, label, tx, y + (h - line_for(Text::Callout)) / 2, fg);
  return w;
}

void draw_row_focus(int x, int y, int w, int h) {
  Ctx &c = ctx();
  c.gfx.shadow(x, y, w, h, 16, 18, 8, 120);
  c.gfx.fill_round(x, y, w, h, 16, theme::focus);
}

void draw_panel(int x, int y, int w, int h, float alpha) {
  ctx().gfx.fill_round(x, y, w, h, theme::panel_radius, fade(theme::glass, alpha));
}

void draw_progress(int x, int y, int w, int h, double fraction, Color fill, Color track) {
  Ctx &c = ctx();
  c.gfx.fill_round(x, y, w, h, h / 2, track);
  int fw = static_cast<int>(w * std::max(0.0, std::min(1.0, fraction)));
  if (fw > 0) c.gfx.fill_round(x, y, std::max(fw, h), h, h / 2, fill);
}

int badge_width(const std::string &label) { return measure(Text::Footnote, label) + 22; }

void draw_badge(int x, int y, const std::string &label, Color color) {
  int w = badge_width(label);
  ctx().gfx.stroke_round(x, y, w, 32, 7, 2, color);
  text(Text::Footnote, label, x + 11, y + 3, with_alpha(theme::primary, color.a));
}

void draw_spinner(int cx, int cy, int size, Color color, long long since) {
  Ctx &c = ctx();
  if (since > 0 && c.now - since < theme::spinner_delay_ms) {
    c.redraw_at(since + theme::spinner_delay_ms);
    return;
  }
  double angle = std::fmod(c.now * 0.36, 360.0);
  c.icons.spinner(cx, cy, size, angle, color);
  c.animating = true;
}

int draw_hint(int x, int y, const std::string &key, const std::string &label) {
  Ctx &c = ctx();
  int kw = std::max(40, measure(Text::Footnote, key) + 20);
  c.gfx.stroke_round(x, y, kw, 34, 8, 2, theme::tertiary);
  text_center(Text::Footnote, key, x + kw / 2, y + 4, theme::secondary);
  int lw = text(Text::Caption, label, x + kw + 12, y + 3, theme::tertiary);
  return kw + 12 + lw;
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
