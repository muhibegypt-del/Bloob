#include "ass.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace ass {
namespace {

std::string trim(const std::string &s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
  while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
  return s.substr(a, b - a);
}

std::string lower(std::string s) {
  for (auto &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::vector<std::string> split(const std::string &s, char sep, size_t max_parts = 0) {
  std::vector<std::string> out;
  size_t start = 0;
  while (true) {
    if (max_parts && out.size() + 1 == max_parts) {
      out.push_back(s.substr(start));
      break;
    }
    size_t p = s.find(sep, start);
    if (p == std::string::npos) {
      out.push_back(s.substr(start));
      break;
    }
    out.push_back(s.substr(start, p - start));
    start = p + 1;
  }
  return out;
}

float num(const std::string &s, float fallback = 0) {
  std::string t = trim(s);
  if (t.empty()) return fallback;
  char *end = nullptr;
  float v = std::strtof(t.c_str(), &end);
  return end == t.c_str() ? fallback : v;
}

// SSA v4 alignment (1-3 bottom, +4 top, +8 middle) to numpad.
int legacy_align(int a) {
  int h = a & 3;
  if (h == 0) h = 2;
  if (a & 4) return 6 + h;
  if (a & 8) return 3 + h;
  return h;
}

int clamp_align(int a) { return a >= 1 && a <= 9 ? a : 2; }

}  // namespace

Rgba parse_colour(const std::string &raw) {
  std::string s = trim(raw);
  while (!s.empty() && (s.back() == '&' || s.back() == 'H' || s.back() == 'h')) s.pop_back();
  unsigned long v = 0;
  if (s.size() >= 2 && s[0] == '&' && (s[1] == 'H' || s[1] == 'h')) v = std::strtoul(s.c_str() + 2, nullptr, 16);
  else if (!s.empty() && (s[0] == 'H' || s[0] == 'h')) v = std::strtoul(s.c_str() + 1, nullptr, 16);
  else v = std::strtoul(s.c_str(), nullptr, 10);
  Rgba c;
  c.r = static_cast<uint8_t>(v & 0xff);
  c.g = static_cast<uint8_t>((v >> 8) & 0xff);
  c.b = static_cast<uint8_t>((v >> 16) & 0xff);
  c.a = static_cast<uint8_t>(255 - ((v >> 24) & 0xff));
  return c;
}

const Style &Script::style(const std::string &name) const {
  auto it = styles.find(name);
  if (it == styles.end()) it = styles.find(lower(name) == "default" ? "Default" : name);
  if (it != styles.end()) return it->second;
  static const Style kDefault;
  auto d = styles.find("Default");
  return d != styles.end() ? d->second : kDefault;
}

Script parse_header(const std::string &header) {
  Script sc;
  std::istringstream in(header);
  std::string line, section;
  std::vector<std::string> format;
  int rx = 0, ry = 0;
  bool legacy = false;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::string t = trim(line);
    if (t.empty() || t[0] == ';') continue;
    if (t[0] == '[') {
      section = lower(t);
      if (section == "[script info]" || section.find("styles") != std::string::npos) sc.valid = true;
      legacy = section == "[v4 styles]";
      format.clear();
      continue;
    }
    size_t colon = t.find(':');
    if (colon == std::string::npos) continue;
    std::string key = lower(trim(t.substr(0, colon))), value = trim(t.substr(colon + 1));
    if (section == "[script info]") {
      if (key == "playresx") rx = static_cast<int>(num(value));
      else if (key == "playresy") ry = static_cast<int>(num(value));
      else if (key == "wrapstyle") sc.wrap_style = static_cast<int>(num(value));
    } else if (section.find("styles") != std::string::npos) {
      if (key == "format") {
        format.clear();
        for (auto &f : split(value, ',')) format.push_back(lower(trim(f)));
      } else if (key == "style") {
        if (format.empty())
          format = {"name", "fontname", "fontsize", "primarycolour", "secondarycolour", "outlinecolour", "backcolour",
                    "bold", "italic", "underline", "strikeout", "scalex", "scaley", "spacing", "angle", "borderstyle",
                    "outline", "shadow", "alignment", "marginl", "marginr", "marginv", "encoding"};
        auto v = split(value, ',', format.size());
        Style st;
        for (size_t i = 0; i < v.size() && i < format.size(); i++) {
          const std::string &f = format[i];
          const std::string x = trim(v[i]);
          if (f == "name") st.name = x;
          else if (f == "fontsize") st.size = num(x, 48);
          else if (f == "primarycolour") st.primary = parse_colour(x);
          else if (f == "outlinecolour" || (legacy && f == "tertiarycolour")) st.outline = parse_colour(x);
          else if (f == "backcolour") st.back = parse_colour(x);
          else if (f == "bold") st.bold = num(x) != 0;
          else if (f == "italic") st.italic = num(x) != 0;
          else if (f == "borderstyle") st.border_style = static_cast<int>(num(x, 1));
          else if (f == "outline") st.outline_w = num(x, 2);
          else if (f == "shadow") st.shadow = num(x, 2);
          else if (f == "alignment") st.align = legacy ? legacy_align(static_cast<int>(num(x, 2))) : clamp_align(static_cast<int>(num(x, 2)));
          else if (f == "marginl") st.margin_l = static_cast<int>(num(x));
          else if (f == "marginr") st.margin_r = static_cast<int>(num(x));
          else if (f == "marginv") st.margin_v = static_cast<int>(num(x));
        }
        if (!st.name.empty() && st.name[0] == '*') st.name = st.name.substr(1);
        sc.styles[st.name] = st;
      }
    }
  }
  // libass's rules for a missing play resolution.
  if (rx <= 0 && ry <= 0) rx = 384, ry = 288;
  else if (ry <= 0) ry = rx == 1280 ? 1024 : rx * 3 / 4;
  else if (rx <= 0) rx = ry == 1024 ? 1280 : ry * 4 / 3;
  sc.res_x = rx;
  sc.res_y = ry;
  return sc;
}

bool Event::empty() const {
  for (const auto &l : lines)
    for (const auto &r : l)
      if (r.text.find_first_not_of(' ') != std::string::npos) return false;
  return true;
}

namespace {

// The arguments of "\tag(a,b,c)", or the bare value of "\tag12".
std::vector<std::string> args_of(const std::string &rest) {
  std::string r = trim(rest);
  if (!r.empty() && r[0] == '(') {
    size_t close = r.rfind(')');
    std::string inner = r.substr(1, close == std::string::npos ? std::string::npos : close - 1);
    std::vector<std::string> out;
    for (auto &a : split(inner, ',')) out.push_back(trim(a));
    return out;
  }
  return {r};
}

// Applies one override ("an8", "pos(320,40)", "c&H00FFFF&") to the run look and the event.
void apply_tag(const std::string &tag, Style &look, const Style &base, const Script &sc, Event &e, bool &drawing, bool &placed_align) {
  std::string t = tag;
  auto starts = [&](const char *p) { return t.compare(0, std::char_traits<char>::length(p), p) == 0; };
  auto rest = [&](size_t n) { return t.substr(n); };
  auto first = [&](size_t n) { auto a = args_of(rest(n)); return a.empty() ? std::string() : a[0]; };
  if (t.empty()) return;
  if (std::isdigit(static_cast<unsigned char>(t[0])) && t.size() >= 2 && (t[1] == 'c' || t[1] == 'a')) {
    int which = t[0] - '0';
    std::string v = trim(t.substr(2));
    if (t[1] == 'c') {
      // An empty colour goes back to the style's.
      const Rgba *from = which == 1 ? &base.primary : which == 3 ? &base.outline : which == 4 ? &base.back : nullptr;
      Rgba c = v.empty() && from ? *from : parse_colour(v);
      Rgba *dst = which == 1 ? &look.primary : which == 3 ? &look.outline : which == 4 ? &look.back : nullptr;
      if (dst) dst->r = c.r, dst->g = c.g, dst->b = c.b;
    } else {
      uint8_t a = static_cast<uint8_t>(255 - (std::strtoul(v.c_str() + (v.size() > 1 && v[0] == '&' ? 2 : 0), nullptr, 16) & 0xff));
      Rgba *dst = which == 1 ? &look.primary : which == 3 ? &look.outline : which == 4 ? &look.back : nullptr;
      if (dst) dst->a = a;
    }
    return;
  }
  if (starts("alpha")) {
    std::string v = trim(rest(5));
    uint8_t a = static_cast<uint8_t>(255 - (std::strtoul(v.c_str() + (v.size() > 1 && v[0] == '&' ? 2 : 0), nullptr, 16) & 0xff));
    look.primary.a = look.outline.a = look.back.a = a;
    return;
  }
  if (starts("an")) {
    if (!placed_align) e.align = clamp_align(static_cast<int>(num(rest(2), 2)));
    placed_align = true;
    return;
  }
  if (starts("pos")) {
    auto a = args_of(rest(3));
    if (a.size() >= 2 && !e.has_pos) e.has_pos = true, e.x = num(a[0]), e.y = num(a[1]);
    return;
  }
  if (starts("move")) {
    auto a = args_of(rest(4));
    if (a.size() >= 4 && !e.has_pos) {
      e.has_pos = e.has_move = true;
      e.x = num(a[0]), e.y = num(a[1]), e.x2 = num(a[2]), e.y2 = num(a[3]);
      if (a.size() >= 6) e.move_t1 = static_cast<long long>(num(a[4])), e.move_t2 = static_cast<long long>(num(a[5]));
    }
    return;
  }
  if (starts("fade") || starts("fad")) {
    auto a = args_of(rest(starts("fade") ? 4 : 3));
    if (a.size() == 2) e.fade_in = static_cast<long long>(num(a[0])), e.fade_out = static_cast<long long>(num(a[1]));
    else if (a.size() >= 7) e.fade_in = static_cast<long long>(num(a[4]) - num(a[3])), e.fade_out = static_cast<long long>(num(a[6]) - num(a[5]));
    return;
  }
  if (starts("fscx") || starts("fscy") || starts("fsp") || starts("frx") || starts("fry") || starts("frz") || starts("fr") ||
      starts("fn") || starts("fe") || starts("clip") || starts("iclip") || starts("org") || starts("blur") || starts("be") ||
      starts("kf") || starts("ko") || starts("k") || starts("K") || starts("t(") || starts("q") || starts("u") || (starts("s") && !starts("shad")) ||
      starts("xbord") || starts("ybord") || starts("xshad") || starts("yshad") || starts("pbo"))
    return;  // read, not shown
  if (starts("fs")) {
    std::string v = trim(rest(2));
    if (v.empty()) look.size = base.size;
    else if (v[0] == '+' || v[0] == '-') look.size = std::max(1.0f, look.size * (1 + num(v) / 10));
    else look.size = std::max(1.0f, num(v, base.size));
    return;
  }
  if (starts("bord")) {
    look.outline_w = std::max(0.0f, num(first(4), base.outline_w));
    return;
  }
  if (starts("shad")) {
    look.shadow = std::max(0.0f, num(first(4), base.shadow));
    return;
  }
  if (starts("b")) {
    std::string v = trim(rest(1));
    int w = v.empty() ? -1 : static_cast<int>(num(v));
    look.bold = w < 0 ? base.bold : w == 1 || w >= 600;
    return;
  }
  if (starts("i")) {
    std::string v = trim(rest(1));
    look.italic = v.empty() ? base.italic : num(v) != 0;
    return;
  }
  if (starts("p")) {
    drawing = num(rest(1)) > 0;
    return;
  }
  if (starts("r")) {
    std::string name = trim(rest(1));
    look = name.empty() ? base : sc.style(name);
    return;
  }
  if (starts("c")) {
    Rgba c = trim(rest(1)).empty() ? base.primary : parse_colour(trim(rest(1)));
    look.primary.r = c.r, look.primary.g = c.g, look.primary.b = c.b;
    return;
  }
  if (starts("a")) {
    if (!placed_align) e.align = legacy_align(static_cast<int>(num(rest(1), 2)));
    placed_align = true;
  }
}

}  // namespace

Event parse_event(const Script &sc, const std::string &packet) {
  Event e;
  auto f = split(packet, ',', 9);
  std::string style_name = "Default", text = packet;
  if (f.size() == 9) {
    e.layer = static_cast<int>(num(f[1]));
    style_name = trim(f[2]);
    e.margin_l = static_cast<int>(num(f[4]));
    e.margin_r = static_cast<int>(num(f[5]));
    e.margin_v = static_cast<int>(num(f[6]));
    text = f[8];
  }
  const Style &base = sc.style(style_name);
  e.align = base.align;
  Style look = base;
  bool drawing = false, placed_align = false;
  e.lines.emplace_back();
  std::string cur;
  auto flush = [&]() {
    if (!cur.empty()) e.lines.back().push_back({cur, look});
    cur.clear();
  };
  for (size_t i = 0; i < text.size(); i++) {
    char ch = text[i];
    if (ch == '{') {
      size_t close = text.find('}', i);
      if (close == std::string::npos) break;
      std::string block = text.substr(i + 1, close - i - 1);
      i = close;
      if (block.find('\\') == std::string::npos) continue;  // a comment
      flush();
      // Tags: split at backslashes outside parentheses.
      int depth = 0;
      std::string tag;
      for (size_t k = 0; k <= block.size(); k++) {
        char b = k < block.size() ? block[k] : '\\';
        if (b == '(') depth++;
        if (b == ')') depth = std::max(0, depth - 1);
        if (b == '\\' && depth == 0) {
          apply_tag(trim(tag), look, base, sc, e, drawing, placed_align);
          tag.clear();
        } else {
          tag += b;
        }
      }
      continue;
    }
    if (drawing) continue;
    if (ch == '\\' && i + 1 < text.size()) {
      char n = text[i + 1];
      if (n == 'N' || (n == 'n' && sc.wrap_style == 2)) {
        flush();
        e.lines.emplace_back();
        i++;
        continue;
      }
      if (n == 'n' || n == 'h') {
        cur += ' ';
        i++;
        continue;
      }
    }
    if (ch == '\r' || ch == '\n') continue;
    cur += ch;
  }
  flush();
  return e;
}

Layout layout(const Script &sc, const Event &e, int w, int h, long long ms, long long left_ms, const Measure &measure,
              const LineHeight &line_height) {
  Layout out;
  out.align = e.align;
  out.positioned = e.has_pos;
  const float sx = static_cast<float>(w) / std::max(1, sc.res_x), sy = static_cast<float>(h) / std::max(1, sc.res_y);
  // Fades multiply every colour's alpha.
  float fade = 1;
  if (e.fade_in > 0 && ms < e.fade_in) fade = std::min(fade, static_cast<float>(std::max(0LL, ms)) / e.fade_in);
  if (e.fade_out > 0 && left_ms < e.fade_out) fade = std::min(fade, static_cast<float>(std::max(0LL, left_ms)) / e.fade_out);

  const Style *first = nullptr;
  for (const auto &l : e.lines)
    for (const auto &r : l)
      if (!first) first = &r.look;
  if (!first) return out;
  int ml = static_cast<int>((e.margin_l ? e.margin_l : first->margin_l) * sx);
  int mr = static_cast<int>((e.margin_r ? e.margin_r : first->margin_r) * sx);
  int mv = static_cast<int>((e.margin_v ? e.margin_v : first->margin_v) * sy);
  int max_w = std::max(100, w - ml - mr);

  struct Piece { std::string text; const Style *look; int size; int width; };
  std::vector<std::vector<Piece>> lines;
  for (const auto &l : e.lines) {
    std::vector<Piece> line;
    for (const auto &r : l) {
      int size = std::max(8, static_cast<int>(std::lround(r.look.size * sy / 2.0) * 2));  // even sizes: fewer glyph bakes
      line.push_back({r.text, &r.look, size, measure(r.text, size, r.look.bold)});
    }
    // Wrap a long one-style line at spaces (positioned signs never wrap).
    if (!e.has_pos && sc.wrap_style != 2 && line.size() == 1 && line[0].width > max_w) {
      const Piece p = line[0];
      std::string acc;
      std::vector<std::string> words = split(p.text, ' ');
      for (const auto &word : words) {
        std::string trial = acc.empty() ? word : acc + " " + word;
        if (!acc.empty() && measure(trial, p.size, p.look->bold) > max_w) {
          lines.push_back({{acc, p.look, p.size, measure(acc, p.size, p.look->bold)}});
          acc = word;
        } else {
          acc = trial;
        }
      }
      if (!acc.empty()) lines.push_back({{acc, p.look, p.size, measure(acc, p.size, p.look->bold)}});
      continue;
    }
    lines.push_back(line);
  }
  // Line heights and the block's size.
  std::vector<int> lh(lines.size(), 0), lw(lines.size(), 0);
  int block_h = 0, block_w = 0;
  for (size_t i = 0; i < lines.size(); i++) {
    int tallest = 0;
    for (const auto &p : lines[i]) {
      tallest = std::max(tallest, line_height(p.size));
      lw[i] += p.width;
    }
    if (lines[i].empty()) tallest = line_height(std::max(8, static_cast<int>(first->size * sy)));
    lh[i] = tallest;
    block_h += tallest;
    block_w = std::max(block_w, lw[i]);
  }
  int col = (e.align - 1) % 3;  // 0 left, 1 centre, 2 right
  int row = (e.align - 1) / 3;  // 0 bottom, 1 middle, 2 top
  float ax, ay;  // anchor
  if (e.has_pos) {
    float px = e.x, py = e.y;
    if (e.has_move) {
      long long t1 = e.move_t1, t2 = e.move_t2;
      if (t1 == 0 && t2 == 0) t2 = ms + left_ms;
      float k = t2 > t1 ? static_cast<float>(std::min(std::max(ms, t1), t2) - t1) / static_cast<float>(t2 - t1) : 1;
      px = e.x + (e.x2 - e.x) * k;
      py = e.y + (e.y2 - e.y) * k;
    }
    ax = px * sx;
    ay = py * sy;
  } else {
    ax = col == 0 ? static_cast<float>(ml) : col == 1 ? static_cast<float>(ml + (w - ml - mr) / 2) : static_cast<float>(w - mr);
    ay = row == 0 ? static_cast<float>(h - mv) : row == 1 ? h / 2.0f : static_cast<float>(mv);
  }
  float top = row == 0 ? ay - block_h : row == 1 ? ay - block_h / 2.0f : ay;
  out.top = top;
  out.bottom = top + block_h;
  float y = top;
  for (size_t i = 0; i < lines.size(); i++) {
    float x = col == 0 ? ax : col == 1 ? ax - lw[i] / 2.0f : ax - lw[i];
    for (const auto &p : lines[i]) {
      Placed pl;
      pl.text = p.text;
      pl.x = x;
      pl.y = y + (lh[i] - line_height(p.size));  // runs share a baseline at the line's bottom
      pl.size = p.size;
      pl.bold = p.look->bold;
      pl.italic = p.look->italic;
      auto faded = [&](Rgba c) { c.a = static_cast<uint8_t>(c.a * fade); return c; };
      pl.fill = faded(p.look->primary);
      pl.outline = faded(p.look->outline);
      pl.shadow_colour = faded(p.look->back);
      pl.box = faded(p.look->outline);  // BorderStyle 3 fills the box with the outline colour
      pl.opaque_box = p.look->border_style == 3;
      pl.outline_px = std::min(6.0f, p.look->outline_w * sy);
      pl.shadow_px = std::min(6.0f, p.look->shadow * sy);
      pl.width = p.width;
      pl.line_h = lh[i];
      out.runs.push_back(pl);
      x += p.width;
    }
    y += lh[i];
  }
  return out;
}

void stack(std::vector<Layout> &layouts, int h) {
  // Bottom: later blocks go above earlier ones; top: below.
  float bottom_edge = -1, top_edge = -1;
  for (auto &l : layouts) {
    if (l.positioned || l.runs.empty()) continue;
    int row = (l.align - 1) / 3;
    float shift = 0;
    if (row == 0) {
      if (bottom_edge >= 0 && l.bottom > bottom_edge) shift = bottom_edge - l.bottom;
      bottom_edge = l.top + shift;
    } else if (row == 2) {
      if (top_edge >= 0 && l.top < top_edge) shift = top_edge - l.top;
      top_edge = l.bottom + shift;
    }
    if (shift != 0) {
      for (auto &r : l.runs) r.y += shift;
      l.top += shift;
      l.bottom += shift;
    }
  }
  (void)h;
}

}  // namespace ass
