#include "greyscale.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cal {

double cct_mccamy(double x, double y) {
  double d = 0.1858 - y;
  if (std::fabs(d) < 1e-9) return 0;
  double n = (x - 0.3320) / d;
  return 449.0 * n * n * n + 3525.0 * n * n + 6823.3 * n + 5520.33;
}

GreySummary2 greyscale_report(std::vector<GreyRow> &rows, double gamma) {
  GreySummary2 s;
  std::sort(rows.begin(), rows.end(), [](const GreyRow &a, const GreyRow &b) { return a.pct < b.pct; });
  for (auto &r : rows) {
    if (r.pct >= 99.999) s.white_y = r.xyz.y;
    if (r.pct <= 0.001) s.black_y = r.xyz.y;
  }
  if (s.white_y <= 0) return s;
  s.contrast = s.black_y > 0 ? s.white_y / s.black_y : 0;
  const Mat3 to_rgb = rgb_to_xyz(kRec709).inverse();
  const Vec3 white = xyY(kD65x, kD65y, s.white_y);
  double gsum = 0, desum = 0, dcsum = 0;
  int gn = 0, den = 0;
  for (auto &r : rows) {
    double v = r.pct / 100.0;
    r.target_y = power_eotf(v, gamma, s.white_y, s.black_y);
    if (r.xyz.y > 0) chromaticity(r.xyz, &r.x, &r.y);
    r.cct = r.xyz.y > 0.05 ? cct_mccamy(r.x, r.y) : 0;
    if (v > 0.001 && v < 0.999 && r.xyz.y > 0 && s.white_y > 0) {
      r.gamma = std::log(r.xyz.y / s.white_y) / std::log(v);
      if (v >= 0.1 && v <= 0.9) {
        gsum += r.gamma;
        gn++;
      }
    }
    // RGB balance: each channel against the luminance, so D65 reads 100/100/100.
    if (r.xyz.y > 0) {
      Vec3 rgb = to_rgb * (r.xyz / r.xyz.y);
      r.r = rgb.x * 100;
      r.g = rgb.y * 100;
      r.b = rgb.z * 100;
    }
    // ΔE2000 against D65 at the target luminance, and at the measured luminance (colour only).
    if (v > 0.001) {
      r.de = de2000_xyz(r.xyz, xyY(kD65x, kD65y, r.target_y), white);
      r.de_colour = r.xyz.y > 0 ? de2000_xyz(r.xyz, xyY(kD65x, kD65y, r.xyz.y), white) : 0;
      desum += r.de;
      dcsum += r.de_colour;
      den++;
      if (r.de > s.max_de) {
        s.max_de = r.de;
        s.max_de_pct = r.pct;
      }
    }
    if (r.pct >= 99.999) s.white_cct = r.cct;
  }
  s.avg_gamma = gn ? gsum / gn : 0;
  s.avg_de = den ? desum / den : 0;
  s.avg_de_colour = den ? dcsum / den : 0;
  return s;
}

namespace {
std::string f(double v, int d) {
  char b[64];
  std::snprintf(b, sizeof b, "%.*f", d, v);
  return b;
}
}  // namespace

std::string greyscale_csv(const std::vector<GreyRow> &rows) {
  std::string o = "pct,Y,target_Y,x,y,gamma,CCT,R,G,B,dE2000,dE2000_colour,X,Z\n";
  for (const auto &r : rows)
    o += f(r.pct, 1) + "," + f(r.xyz.y, 4) + "," + f(r.target_y, 4) + "," + f(r.x, 4) + "," + f(r.y, 4) + "," + f(r.gamma, 3) + "," +
         f(r.cct, 0) + "," + f(r.r, 1) + "," + f(r.g, 1) + "," + f(r.b, 1) + "," + f(r.de, 2) + "," + f(r.de_colour, 2) + "," +
         f(r.xyz.x, 4) + "," + f(r.xyz.z, 4) + "\n";
  return o;
}

std::string greyscale_json(const std::vector<GreyRow> &rows, const GreySummary2 &s, double gamma, const std::string &when) {
  std::string o = "{\"ok\":true,\"kind\":\"greyscale\",\"when\":\"" + when + "\",\"target\":\"BT.709, power " + f(gamma, 2) +
                  ", D65\",\"summary\":{\"white\":" + f(s.white_y, 3) + ",\"black\":" + f(s.black_y, 4) + ",\"contrast\":" + f(s.contrast, 0) +
                  ",\"avg_gamma\":" + f(s.avg_gamma, 3) + ",\"avg_de\":" + f(s.avg_de, 2) + ",\"max_de\":" + f(s.max_de, 2) +
                  ",\"max_de_at\":" + f(s.max_de_pct, 1) + ",\"avg_de_colour\":" + f(s.avg_de_colour, 2) + ",\"white_cct\":" + f(s.white_cct, 0) +
                  "},\"levels\":[";
  for (size_t i = 0; i < rows.size(); i++) {
    const auto &r = rows[i];
    o += std::string(i ? "," : "") + "{\"pct\":" + f(r.pct, 1) + ",\"Y\":" + f(r.xyz.y, 4) + ",\"target\":" + f(r.target_y, 4) + ",\"x\":" + f(r.x, 4) +
         ",\"y\":" + f(r.y, 4) + ",\"gamma\":" + f(r.gamma, 3) + ",\"cct\":" + f(r.cct, 0) + ",\"r\":" + f(r.r, 1) + ",\"g\":" + f(r.g, 1) +
         ",\"b\":" + f(r.b, 1) + ",\"de\":" + f(r.de, 2) + ",\"de_colour\":" + f(r.de_colour, 2) + ",\"X\":" + f(r.xyz.x, 4) + ",\"Z\":" + f(r.xyz.z, 4) + "}";
  }
  return o + "]}";
}

namespace {
// One chart: a polyline per series over pct 0..100. `lines` are y values at reference levels.
std::string chart(const std::string &title, const std::vector<GreyRow> &rows, const std::vector<std::pair<std::string, std::vector<double>>> &series,
                  const std::vector<std::string> &colours, double lo, double hi, const std::vector<double> &guides) {
  const int W = 640, H = 260, L = 48, R = 12, T = 28, B = 30;
  auto px = [&](double p) { return L + (W - L - R) * p / 100.0; };
  auto py = [&](double v) { return T + (H - T - B) * (1 - (std::min(hi, std::max(lo, v)) - lo) / (hi - lo)); };
  std::string o = "<svg viewBox=\"0 0 " + std::to_string(W) + " " + std::to_string(H) + "\" role=\"img\" aria-label=\"" + title + "\">";
  o += "<text x=\"" + std::to_string(L) + "\" y=\"18\" class=\"t\">" + title + "</text>";
  for (int i = 0; i <= 4; i++) {
    double v = lo + (hi - lo) * i / 4;
    o += "<line x1=\"" + f(L, 0) + "\" x2=\"" + f(W - R, 0) + "\" y1=\"" + f(py(v), 1) + "\" y2=\"" + f(py(v), 1) + "\" class=\"g\"/>";
    o += "<text x=\"" + f(L - 6, 0) + "\" y=\"" + f(py(v) + 4, 1) + "\" class=\"a\" text-anchor=\"end\">" + f(v, hi - lo < 5 ? 2 : 0) + "</text>";
  }
  for (int p = 0; p <= 100; p += 20) o += "<text x=\"" + f(px(p), 1) + "\" y=\"" + f(H - 10, 0) + "\" class=\"a\" text-anchor=\"middle\">" + std::to_string(p) + "</text>";
  for (double gv : guides) o += "<line x1=\"" + f(L, 0) + "\" x2=\"" + f(W - R, 0) + "\" y1=\"" + f(py(gv), 1) + "\" y2=\"" + f(py(gv), 1) + "\" class=\"ref\"/>";
  for (size_t si = 0; si < series.size(); si++) {
    std::string pts;
    for (size_t i = 0; i < rows.size() && i < series[si].second.size(); i++) {
      double v = series[si].second[i];
      if (std::isnan(v)) continue;
      pts += f(px(rows[i].pct), 1) + "," + f(py(v), 1) + " ";
    }
    o += "<polyline fill=\"none\" stroke=\"" + colours[si] + "\" stroke-width=\"2\" points=\"" + pts + "\"/>";
  }
  o += "</svg>";
  return o;
}
}  // namespace

std::string greyscale_html(const std::vector<GreyRow> &rows, const GreySummary2 &s, double gamma, const std::string &when) {
  std::vector<double> y, t, gm, r, g, b, de;
  for (const auto &row : rows) {
    y.push_back(row.xyz.y);
    t.push_back(row.target_y);
    gm.push_back(row.gamma > 0 ? row.gamma : std::nan(""));
    r.push_back(row.r);
    g.push_back(row.g);
    b.push_back(row.b);
    de.push_back(row.pct > 0.001 ? row.de : std::nan(""));
  }
  double rgb_lo = 80, rgb_hi = 120;
  for (size_t i = 0; i < rows.size(); i++)
    if (rows[i].pct >= 5) rgb_lo = std::min(rgb_lo, std::min(r[i], std::min(g[i], b[i]))), rgb_hi = std::max(rgb_hi, std::max(r[i], std::max(g[i], b[i])));
  std::string o =
      "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<title>Greyscale report</title><style>"
      ":root{--bg:#fff;--fg:#111;--mut:#666;--grid:#e3e3e3;--ref:#999}"
      "@media (prefers-color-scheme:dark){:root{--bg:#121212;--fg:#eee;--mut:#999;--grid:#2c2c2c;--ref:#777}}"
      "body{background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,sans-serif;margin:0 auto;max-width:980px;padding:24px 16px}"
      "h1{font-size:22px;margin:0 0 4px}.m{color:var(--mut)}.k{display:flex;flex-wrap:wrap;gap:18px;margin:16px 0}"
      ".k div{min-width:120px}.k b{display:block;font-size:22px}svg{width:100%;height:auto;margin:10px 0}"
      ".t{fill:var(--fg);font-size:13px;font-weight:600}.a{fill:var(--mut);font-size:11px}.g{stroke:var(--grid)}"
      ".ref{stroke:var(--ref);stroke-dasharray:4 4}table{border-collapse:collapse;width:100%;font-variant-numeric:tabular-nums;font-size:13px}"
      "th,td{padding:4px 6px;text-align:right;border-bottom:1px solid var(--grid)}th{color:var(--mut);font-weight:600}"
      ".bad{color:#d33}.ok{color:#2a8}.wrap{overflow-x:auto}</style></head><body>";
  o += "<h1>Greyscale report</h1><div class=\"m\">" + when + " &middot; target BT.709, power gamma " + f(gamma, 2) + ", D65 &middot; Bloob</div>";
  o += "<div class=\"k\"><div>Average &Delta;E2000<b>" + f(s.avg_de, 2) + "</b></div><div>Worst &Delta;E2000<b>" + f(s.max_de, 2) + "</b><span class=\"m\">at " +
       f(s.max_de_pct, 1) + "%</span></div><div>Average gamma<b>" + f(s.avg_gamma, 2) + "</b><span class=\"m\">10&ndash;90%</span></div><div>White<b>" +
       f(s.white_y, 1) + "</b><span class=\"m\">cd/m&sup2;</span></div><div>Black<b>" + f(s.black_y, 4) + "</b><span class=\"m\">cd/m&sup2;</span></div><div>White CCT<b>" +
       f(s.white_cct, 0) + " K</b></div></div>";
  o += chart("Luminance (cd/m\xC2\xB2): measured and target", rows, {{"Y", y}, {"target", t}}, {"#2f7de1", "#999"}, 0, std::max(s.white_y, 1.0) * 1.05, {});
  o += chart("Gamma at each level (target dashed)", rows, {{"gamma", gm}}, {"#2f7de1"}, gamma - 0.4, gamma + 0.4, {gamma});
  o += chart("RGB balance, % (100 = D65)", rows, {{"R", r}, {"G", g}, {"B", b}}, {"#e0453a", "#2ea44f", "#2f7de1"}, rgb_lo, rgb_hi, {100});
  o += chart("\xCE\x94" "E2000 (1 and 3 dashed)", rows, {{"dE", de}}, {"#e0453a"}, 0, std::max(4.0, s.max_de * 1.1), {1, 3});
  o += "<div class=\"wrap\"><table><tr><th>%</th><th>Y</th><th>Target</th><th>x</th><th>y</th><th>Gamma</th><th>CCT</th><th>R</th><th>G</th><th>B</th>"
       "<th>&Delta;E</th><th>&Delta;E colour</th></tr>";
  for (const auto &row : rows) {
    std::string cls = row.de >= 3 ? "bad" : row.de < 1 ? "ok" : "";
    o += "<tr><td>" + f(row.pct, 1) + "</td><td>" + f(row.xyz.y, 3) + "</td><td>" + f(row.target_y, 3) + "</td><td>" + f(row.x, 4) + "</td><td>" + f(row.y, 4) +
         "</td><td>" + (row.gamma > 0 ? f(row.gamma, 2) : "") + "</td><td>" + (row.cct > 0 ? f(row.cct, 0) : "") + "</td><td>" + f(row.r, 1) + "</td><td>" +
         f(row.g, 1) + "</td><td>" + f(row.b, 1) + "</td><td class=\"" + cls + "\">" + (row.pct > 0.001 ? f(row.de, 2) : "") + "</td><td>" +
         (row.pct > 0.001 ? f(row.de_colour, 2) : "") + "</td></tr>";
  }
  o += "</table></div></body></html>";
  return o;
}

}  // namespace cal
