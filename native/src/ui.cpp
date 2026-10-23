#include "ui.h"

#include "clock.h"

#include <cmath>
#include <cstdio>

#include "log.h"
#include "screens.h"

void Screen::shell_glass(const GlassSpec &g) const {
  GlassSpec solid = g;
  solid.kind = GlassKind::Solid;
  draw_glass(solid);
}

Ctx &ctx() {
  static Ctx instance;
  return instance;
}

void Ctx::save_settings() {
  std::string list;
  for (auto &s : recent_searches) list += (list.empty() ? "" : ",") + Json::quote(s);
  api.post("/api/settings", "{\"language\":" + Json::quote(language) + ",\"subtitleLanguage\":" + Json::quote(subtitle_language) +
                                ",\"subtitleSize\":" + std::to_string(subtitle_size) +
                                ",\"subtitleBackground\":" + (subtitle_background ? "true" : "false") +
                                ",\"picture\":" + Json::quote(picture) + ",\"sources\":" + Json::quote(sources) +
                                ",\"cut\":" + Json::quote(preferred_cut) +
                                ",\"autoplayNext\":" + (autoplay_next ? "true" : "false") +
                                ",\"reduceMotion\":" + (reduce_motion ? "true" : "false") +
                                ",\"reduceTransparency\":" + (reduce_transparency ? "true" : "false") +
                                ",\"highContrast\":" + (high_contrast ? "true" : "false") +
                                ",\"welcomeSeen\":" + (welcome_seen ? "true" : "false") + ",\"recentSearches\":[" + list + "]}");
}

void Ctx::mark(const char *stage) {
  if (!marks.insert(stage).second) return;
  logf("launch", "%s at %lld ms", stage, now_ms());
}

static float ease(Ease e, float p) {
  switch (e) {
    case Ease::Linear: return p;
    case Ease::In: return p * p;
    case Ease::Out: return p * (2 - p);
    case Ease::InOut: return p < 0.5f ? 2 * p * p : 1 - 2 * (1 - p) * (1 - p);
  }
  return p;
}

void Anim::to(float v) {
  if (v == target) return;
  target = v;
  from_ = value;
  t_ms_ = 0;
}

bool Anim::step(double dt) {
  if (value == target && velocity_ == 0) return false;
  if (motion.spring) {
    // Semi-implicit Euler in steps of at most 1/120 s; settles inside 0.2% of the move.
    double left = dt;
    while (left > 0) {
      float h = static_cast<float>(std::min(left, 1.0 / 120));
      float force = -motion.stiffness * (value - target) - motion.damping * velocity_;
      velocity_ += force / motion.mass * h;
      value += velocity_ * h;
      left -= h;
    }
    float scale = std::max(1.0f, std::fabs(target));
    if (std::fabs(target - value) < 0.002f * scale && std::fabs(velocity_) < 0.01f * scale) {
      value = target;
      velocity_ = 0;
      return false;
    }
    return true;
  }
  t_ms_ += dt * 1000.0;
  float p = motion.ms <= 0 ? 1.0f : static_cast<float>(std::min(1.0, t_ms_ / motion.ms));
  value = from_ + (target - from_) * ease(motion.ease, p);
  if (p >= 1.0f) {
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
    // Nothing below 22 px: the smallest size that stays readable at three metres.
    // Headings are the Emphasized roles (Bold); text is Standard (Medium) [EoA T12].
    case Text::Display: return {3, 104, 112};
    case Text::Title1: return {3, 76, 96};
    case Text::Title2: return {3, 57, 66};
    case Text::Title3: return {3, 48, 56};
    case Text::Title3Std: return {1, 48, 56};      // Standard.Title3: keyboard keys [EoA S4]
    case Text::Headline: return {3, 38, 46};
    case Text::Callout: return {1, 31, 38};
    case Text::CalloutBold: return {3, 31, 38};     // Emphasized.Callout: shelf headers, pills [EoA T13]
    case Text::BodyBold: return {3, 29, 36};        // Emphasized.Body: player controls
    case Text::BodySemi: return {2, 29, 36};        // Regular.Body: settings titles, buttons
    case Text::Caption1Semi: return {2, 25, 32};    // Regular.Caption1: settings subtitles
    case Text::Caption2Bold: return {3, 23, 30};    // Emphasized.Caption2: player times
    case Text::Body: return {1, 29, 36};
    case Text::Caption1: return {1, 25, 32};
    case Text::Caption2: return {1, 23, 30};
    case Text::Caption3: return {1, 21, 28};  // [EoA T9]; only where text is fitted, never body copy
    case Text::Button: return {2, 29, 36};
    case Text::Pill: return {3, 31, 38};      // Emphasized.Callout, Bold [EoA P2]
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

int text_y_centered(Text style, int cy) {
  Font &f = font_for(style);
  int size = size_for(style);
  return static_cast<int>(std::lround(cy + f.cap_height(size) / 2 - f.ascent(size)));
}

Color label(Label role, float alpha) {
  // Dark theme: 100 / 50 / 30 / 20%; Increase contrast 100 / 85 / 75 / 55% [EoA C1, C2].
  static const int normal[] = {255, 128, 77, 51}, high[] = {255, 217, 191, 140};
  int a = (ctx().high_contrast ? high : normal)[static_cast<int>(role)];
  return with_alpha(rgba(0xffffff), static_cast<int>(a * std::max(0.0f, std::min(1.0f, alpha))));
}

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
      c.fonts.semibold.draw(l, x + 24, ty, 26, label(Label::Secondary, alpha));
      ty += 32;
    }
  }
}

SDL_Rect draw_card(int x, int y, const CardSpec &cs) {
  Ctx &c = ctx();
  // Focus grows the card 50 wider, height in proportion, about its centre [EoA K2].
  float s = c.reduce_motion ? 1.0f : 1.0f + static_cast<float>(theme::card_grow) / cs.w * cs.focus;
  int w = static_cast<int>(std::lround(cs.w * s)), h = static_cast<int>(std::lround(cs.h * s));
  int cx = x - (w - cs.w) / 2, cy = y - (h - cs.h) / 2;
  // Two shadows, blended by focus: a light one at rest, a deep one when focused [EoA K4].
  float f = std::max(0.0f, std::min(1.0f, cs.focus));
  int blur = static_cast<int>(theme::card_shadow_blur + (theme::card_focus_shadow_blur - theme::card_shadow_blur) * f);
  int dy = static_cast<int>(theme::card_shadow_dy + (theme::card_focus_shadow_dy - theme::card_shadow_dy) * f);
  float sa = theme::card_shadow_alpha + (theme::card_focus_shadow_alpha - theme::card_shadow_alpha) * f;
  bool all = cs.pass == CardPass::All;
  if ((all || cs.pass == CardPass::Shadow) && !c.lite()) c.gfx.shadow(cx, cy, w, h, cs.radius, blur, dy, static_cast<Uint8>(sa * cs.alpha));
  if (cs.pass == CardPass::Shadow) return SDL_Rect{cx, cy, w, h};
  ImageRef img = cs.art.empty() ? ImageRef{} : c.images.get(cs.art, cs.w, cs.h, cs.radius, Fit::Cover, cs.title_on_art);
  if ((img.failed || cs.art.empty()) && !cs.fallback_art.empty())
    img = c.images.get(cs.fallback_art, cs.w, cs.h, cs.radius, Fit::Cover, cs.title_on_art);
  bool missing = img.failed || (cs.art.empty() && cs.fallback_art.empty());
  bool body = all || cs.pass == CardPass::Body;
  bool words = all || cs.pass == CardPass::Text;
  if (missing) {
    if (body) draw_missing_art(cx, cy, w, h, cs.radius, cs.title, cs.alpha);
  } else {
    if (body && img.alpha < 255) c.gfx.fill_round(cx, cy, w, h, cs.radius, fade(rgba(0xffffff, 18), cs.alpha));
    if (body && img.texture) c.gfx.texture(img.texture, cx, cy, w, h, static_cast<Uint8>(img.alpha * cs.alpha));
    if (words && cs.title_on_art && !cs.title.empty()) {
      const int pad = theme::card_text_pad;
      int ty = cy + h - (cs.progress >= 0 ? theme::progress_pad_y + 5 + 10 : pad) - line_for(Text::Caption2);
      int sw = cs.sub.empty() ? 0 : measure(Text::Caption2, cs.sub);
      if (sw) text(Text::Caption2, cs.sub, cx + w - pad - sw, ty, label(Label::Secondary, cs.alpha * 1.2f));
      text(Text::Caption2, cs.title, cx + pad, ty, label(Label::Primary, cs.alpha), w - 2 * pad - 12 - sw);
    }
  }
  if (!body) return SDL_Rect{cx, cy, w, h};
  if (cs.progress >= 0) {
    // Progress inset 18 / 12 inside the card [EoA K6]; tertiary under primary [EoA K7].
    int pw = w - 2 * theme::progress_pad_x, py = cy + h - theme::progress_pad_y - 5;
    c.gfx.fill_round(cx + theme::progress_pad_x, py, pw, 5, 2, fade(theme::progress_back, cs.alpha));
    int fw = static_cast<int>(pw * std::max(0.0, std::min(1.0, cs.progress)));
    if (fw > 0) c.gfx.fill_round(cx + theme::progress_pad_x, py, std::max(5, fw), 5, 2, fade(rgba(0xffffff), cs.alpha));
  }
  // Focus: the lift and shadow above, plus a diagonal sheen across the art.
  if (cs.focus > 0.01f && !missing) c.gfx.sheen(cx, cy, w, h, cs.radius, cs.w, cs.h, static_cast<Uint8>(56 * cs.focus * cs.alpha));
  return SDL_Rect{cx, cy, w, h};
}

// A sheet on glass over the top screen's artwork (solid where it has none).
void draw_panel(int x, int y, int w, int h, float alpha) {
  GlassSpec g;
  g.x = x;
  g.y = y;
  g.w = w;
  g.h = h;
  g.alpha = alpha;
  if (ctx().glass) ctx().glass(g);
  else draw_glass(g);
}

void draw_glass(const GlassSpec &g) {
  Ctx &c = ctx();
  if (g.w <= 0 || g.h <= 0 || g.alpha <= 0.01f) return;
  if (g.shadow) c.gfx.shadow(g.x, g.y, g.w, g.h, g.radius, 40, 18, static_cast<Uint8>(90 * g.alpha));
  if (g.kind == GlassKind::Smoke && !c.reduce_transparency) {
    c.gfx.fill_round(g.x, g.y, g.w, g.h, g.radius, fade(theme::smoke, g.alpha));
    c.gfx.stroke_round(g.x, g.y, g.w, g.h, g.radius, 1, fade(theme::smoke_hairline, g.alpha));
    return;
  }
  bool frost = g.kind == GlassKind::Frost && !g.art.empty() && !c.lite() && !c.reduce_transparency;
  ImageRef f = frost ? c.images.frost(g.art, GlassRect{g.x, g.y, g.w, g.h}, g.radius) : ImageRef{};
  // Solid underneath until the frost has faded in.
  if (!f.texture || f.alpha < 255) c.gfx.fill_round(g.x, g.y, g.w, g.h, g.radius, fade(theme::solid, g.alpha));
  if (f.texture) c.gfx.texture(f.texture, g.x, g.y, g.w, g.h, static_cast<Uint8>(f.alpha * g.alpha));
  c.gfx.stroke_round(g.x, g.y, g.w, g.h, g.radius, 1, fade(theme::frost_hairline, g.alpha));
}

std::string position(int index, int count) {
  return count > 1 ? std::to_string(index + 1) + " of " + std::to_string(count) : "";
}

namespace {
// A button's geometry, worked out once: the width and the drawing both come from
// it, so the margins either side of the content are always equal.
struct ButtonLayout {
  int w, h, pad, icon, gap, label_w, check_w, content;
  Text style;
};

ButtonLayout layout_of(const GlassButton &b) {
  ButtonLayout l{};
  l.style = b.pill ? Text::Pill : b.style;
  l.h = b.pill ? theme::pill_h : b.h;
  l.pad = b.pill ? theme::pill_pad : theme::button_pad;          // 30 at every size [EoA B2]
  l.icon = l.h >= theme::button_h ? 26 : 22;                       // hero play icon 26 [EoA B13]
  l.gap = l.h <= theme::button_small_h ? 8 : theme::button_icon_gap;
  l.label_w = b.label.empty() ? 0 : measure(l.style, b.label);
  l.check_w = b.candidate && !b.pill ? 28 + l.gap : 0;
  if (b.label.empty()) {
    l.content = l.icon;
    l.w = l.h;  // icon-only buttons are round
    return l;
  }
  l.content = (b.icon >= 0 ? l.icon + l.gap : 0) + l.label_w + l.check_w;
  l.w = l.content + 2 * l.pad;
  if (b.min_w > 0) l.w = std::max(l.w, b.min_w);
  if (b.max_w > 0 && l.w > b.max_w) {
    l.label_w = std::max(0, l.label_w - (l.w - b.max_w));  // the label is truncated to fit
    l.content = (b.icon >= 0 ? l.icon + l.gap : 0) + l.label_w + l.check_w;
    l.w = b.max_w;
  }
  return l;
}
}  // namespace

int glass_button_width(const GlassButton &b) { return layout_of(b).w; }

int draw_glass_button(int x, int y, const GlassButton &b) {
  Ctx &c = ctx();
  ButtonLayout l = layout_of(b);
  int w = l.w, h = l.h;
  // Focus grows the button sideways by a fixed amount; there is no scale [EoA B4, P4].
  int grow_by = b.pill ? theme::pill_focus_grow : theme::button_grow;
  int grow = b.focused && !c.reduce_motion ? static_cast<int>(std::lround(grow_by * b.lift)) : 0;
  int gx = x - grow / 2, gy = y, gw = w + grow, gh = h;
  int radius = gh / 2;
  Color rest = b.variant == ButtonStyle::Player ? theme::button_player
               : b.variant == ButtonStyle::Default ? theme::button_default : theme::button_dark;
  Color cand = b.pill ? theme::candidate
               : b.variant == ButtonStyle::Player ? theme::button_player_candidate
               : b.variant == ButtonStyle::Default ? theme::button_default_candidate : theme::button_dark_candidate;
  // The shadow shows when focused or candidate [EoA B14]; pills only when focused.
  if (b.focused || (b.candidate && !b.pill))
    c.gfx.shadow(gx, gy, gw, gh, radius, theme::shadow_blur, theme::shadow_dy, static_cast<Uint8>(theme::shadow_alpha * b.alpha * (b.focused ? 1.0f : 0.5f)));
  if (b.focused) {
    c.gfx.fill_round(gx, gy, gw, gh, radius, fade(b.pill ? rgba(0xffffff) : theme::button_focus, b.alpha));
  } else if (b.pill) {
    if (b.candidate) c.gfx.fill_round(gx, gy, gw, gh, radius, fade(cand, b.alpha));
  } else {
    c.gfx.fill_round(gx, gy, gw, gh, radius, fade(b.candidate ? cand : rest, b.alpha));
    if (b.variant != ButtonStyle::Default && !b.candidate)
      c.gfx.stroke_round(gx, gy, gw, gh, radius, 1, fade(c.high_contrast ? theme::hairline_high : theme::frost_hairline, b.alpha));
  }
  // Pills: secondary at rest, primary as candidate [EoA P5].
  Label rest_label = b.disabled ? Label::Disabled : b.pill && !b.candidate ? Label::Secondary : Label::Primary;
  Color fg = b.focused ? fade(rgba(0x000000), b.alpha) : label(rest_label, b.alpha);
  int cy = gy + gh / 2, isz = l.icon;
  if (b.label.empty()) {
    if (b.icon == kPlayIcon) c.gfx.triangle_right(gx + gw / 2 - isz / 3, cy - isz / 2, isz, fg);
    else if (b.icon == kPauseIcon) c.gfx.bars(gx + (gw - isz * 4 / 5) / 2, cy - isz * 2 / 5, isz * 4 / 5, fg);
    else if (b.icon >= 0) c.icons.draw(static_cast<Icon>(b.icon), gx + (gw - isz) / 2, cy - isz / 2, isz, fg);
    return w;
  }
  // Content centred in the (possibly grown) shape: equal space either side.
  int tx = gx + (gw - l.content) / 2;
  if (b.icon == kPlayIcon) {
    c.gfx.triangle_right(tx, cy - isz / 2, isz, fg);
    tx += isz + l.gap;
  } else if (b.icon >= 0) {
    c.icons.draw(static_cast<Icon>(b.icon), tx, cy - isz / 2, isz, fg);
    tx += isz + l.gap;
  }
  text(l.style, b.label, tx, text_y_centered(l.style, cy), fg, l.label_w + 1);
  if (l.check_w) c.icons.draw(Icon::Check, tx + l.label_w + l.gap, cy - 14, 28, fg);
  return w;
}

int draw_glass_badge(int x, int y, const std::string &s, float alpha) {
  Ctx &c = ctx();
  int w = measure(Text::Caption2, s) + 24, h = 36;
  c.gfx.fill_round(x, y, w, h, theme::badge_radius, fade(with_alpha(rgba(0xffffff), 40), alpha));
  c.gfx.stroke_round(x, y, w, h, theme::badge_radius, 1, fade(with_alpha(rgba(0xffffff), 70), alpha));
  text(Text::Caption2, s, x + 12, y + (h - line_for(Text::Caption2)) / 2, label(Label::Primary, alpha));
  return w;
}

void draw_progress(int x, int y, int w, int h, double fraction, Color fill, Color track) {
  Ctx &c = ctx();
  c.gfx.fill_round(x, y, w, h, h / 2, track);
  int fw = static_cast<int>(w * std::max(0.0, std::min(1.0, fraction)));
  if (fw > 0) c.gfx.fill_round(x, y, std::max(fw, h), h, h / 2, fill);
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

int draw_hint(int x, int y, const std::string &key, const std::string &name) {
  Ctx &c = ctx();
  int kw = std::max(42, measure(Text::Caption2, key) + 20);
  c.gfx.stroke_round(x, y, kw, 36, 8, 2, label(Label::Tertiary));
  text_center(Text::Caption2, key, x + kw / 2, y + (36 - line_for(Text::Caption2)) / 2, label(Label::Secondary));
  int lw = text(Text::Caption1, name, x + kw + 12, y + (36 - line_for(Text::Caption1)) / 2, label(Label::Tertiary));
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
