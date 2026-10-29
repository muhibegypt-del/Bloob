#include "colour.h"

#include <algorithm>
#include <cmath>

namespace cal {

const Primaries kRec709{0.640, 0.330, 0.300, 0.600, 0.150, 0.060, kD65x, kD65y};
const Primaries kBt2020{0.708, 0.292, 0.170, 0.797, 0.131, 0.046, kD65x, kD65y};
const Primaries kP3D65{0.680, 0.320, 0.265, 0.690, 0.150, 0.060, kD65x, kD65y};

Mat3 Mat3::columns(Vec3 a, Vec3 b, Vec3 c) {
  Mat3 r;
  for (int i = 0; i < 3; i++) {
    r.m[i][0] = a[i];
    r.m[i][1] = b[i];
    r.m[i][2] = c[i];
  }
  return r;
}

Vec3 Mat3::operator*(const Vec3 &v) const {
  return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z, m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
          m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
}

Mat3 Mat3::operator*(const Mat3 &o) const {
  Mat3 r;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j];
  return r;
}

double Mat3::det() const {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

Mat3 Mat3::inverse() const {
  double d = det();
  Mat3 r;
  if (std::fabs(d) < 1e-300) return r;
  r.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) / d;
  r.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) / d;
  r.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) / d;
  r.m[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) / d;
  r.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) / d;
  r.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) / d;
  r.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) / d;
  r.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) / d;
  r.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) / d;
  return r;
}

Vec3 xyY(double x, double y, double Y) {
  if (y <= 0) return {0, 0, 0};
  return {x * Y / y, Y, (1 - x - y) * Y / y};
}

void chromaticity(const Vec3 &c, double *x, double *y) {
  double s = c.x + c.y + c.z;
  *x = s > 0 ? c.x / s : kD65x;
  *y = s > 0 ? c.y / s : kD65y;
}

double duv(const Vec3 &a, const Vec3 &b) {
  auto uv = [](const Vec3 &c, double *u, double *v) {
    double d = c.x + 15 * c.y + 3 * c.z;
    *u = d > 0 ? 4 * c.x / d : 0.1978;
    *v = d > 0 ? 9 * c.y / d : 0.4683;
  };
  double ua, va, ub, vb;
  uv(a, &ua, &va);
  uv(b, &ub, &vb);
  return std::hypot(ua - ub, va - vb);
}

Mat3 rgb_to_xyz(const Primaries &p) {
  Vec3 r = xyY(p.rx, p.ry, 1), g = xyY(p.gx, p.gy, 1), b = xyY(p.bx, p.by, 1);
  Mat3 m = Mat3::columns(r, g, b);
  Vec3 s = m.inverse() * xyY(p.wx, p.wy, 1);
  return Mat3::columns(r * s.x, g * s.y, b * s.z);
}

static double lab_f(double t) {
  const double e = 216.0 / 24389.0, k = 24389.0 / 27.0;
  return t > e ? std::cbrt(t) : (k * t + 16) / 116;
}

Vec3 lab(const Vec3 &c, const Vec3 &w) {
  double fx = lab_f(c.x / w.x), fy = lab_f(c.y / w.y), fz = lab_f(c.z / w.z);
  return {116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)};
}

// CIEDE2000, as in Sharma, Wu and Dalal (2005), including their notes on hue.
double de2000(const Vec3 &l1, const Vec3 &l2) {
  const double pi = 3.14159265358979323846, deg = pi / 180;
  double C1 = std::hypot(l1.y, l1.z), C2 = std::hypot(l2.y, l2.z), Cb = (C1 + C2) / 2;
  double Cb7 = std::pow(Cb, 7), G = 0.5 * (1 - std::sqrt(Cb7 / (Cb7 + std::pow(25.0, 7))));
  double a1 = (1 + G) * l1.y, a2 = (1 + G) * l2.y;
  double c1 = std::hypot(a1, l1.z), c2 = std::hypot(a2, l2.z);
  auto hue = [&](double b, double a) {
    if (a == 0 && b == 0) return 0.0;
    double h = std::atan2(b, a) / deg;
    return h < 0 ? h + 360 : h;
  };
  double h1 = hue(l1.z, a1), h2 = hue(l2.z, a2);
  double dL = l2.x - l1.x, dC = c2 - c1, dh = 0;
  if (c1 * c2 != 0) {
    dh = h2 - h1;
    if (dh > 180) dh -= 360;
    else if (dh < -180) dh += 360;
  }
  double dH = 2 * std::sqrt(c1 * c2) * std::sin(dh * deg / 2);
  double Lb = (l1.x + l2.x) / 2, cb = (c1 + c2) / 2, hb = h1 + h2;
  if (c1 * c2 != 0) {
    if (std::fabs(h1 - h2) <= 180) hb = (h1 + h2) / 2;
    else hb = (h1 + h2 < 360) ? (h1 + h2 + 360) / 2 : (h1 + h2 - 360) / 2;
  }
  double T = 1 - 0.17 * std::cos((hb - 30) * deg) + 0.24 * std::cos(2 * hb * deg) +
             0.32 * std::cos((3 * hb + 6) * deg) - 0.20 * std::cos((4 * hb - 63) * deg);
  double dtheta = 30 * std::exp(-std::pow((hb - 275) / 25, 2));
  double cb7 = std::pow(cb, 7), Rc = 2 * std::sqrt(cb7 / (cb7 + std::pow(25.0, 7)));
  double Sl = 1 + 0.015 * (Lb - 50) * (Lb - 50) / std::sqrt(20 + (Lb - 50) * (Lb - 50));
  double Sc = 1 + 0.045 * cb, Sh = 1 + 0.015 * cb * T;
  double Rt = -std::sin(2 * dtheta * deg) * Rc;
  double tl = dL / Sl, tc = dC / Sc, th = dH / Sh;
  return std::sqrt(tl * tl + tc * tc + th * th + Rt * tc * th);
}

double de2000_xyz(const Vec3 &a, const Vec3 &b, const Vec3 &white) { return de2000(lab(a, white), lab(b, white)); }

double bt1886(double v, double Lw, double Lb) {
  double rw = std::pow(Lw, 1 / 2.4), rb = std::pow(std::max(Lb, 0.0), 1 / 2.4);
  double a = std::pow(rw - rb, 2.4), b = rb / (rw - rb);
  return a * std::pow(std::max(v + b, 0.0), 2.4);
}

double power_eotf(double v, double gamma, double Lw, double Lb) {
  return Lb + (Lw - Lb) * std::pow(std::max(v, 0.0), gamma);
}

static const double m1 = 2610.0 / 16384, m2 = 2523.0 / 4096 * 128, c1 = 3424.0 / 4096, c2 = 2413.0 / 4096 * 32,
                    c3 = 2392.0 / 4096 * 32;

double pq_eotf(double e) {
  e = std::clamp(e, 0.0, 1.0);
  double p = std::pow(e, 1 / m2);
  return 10000 * std::pow(std::max(p - c1, 0.0) / (c2 - c3 * p), 1 / m1);
}

double pq_inverse(double L) {
  double y = std::pow(std::clamp(L / 10000, 0.0, 1.0), m1);
  return std::pow((c1 + c2 * y) / (1 + c3 * y), m2);
}

Vec3 ictcp(const Vec3 &XYZ) {
  static const Mat3 to2020 = rgb_to_xyz(kBt2020).inverse();
  Vec3 rgb = to2020 * XYZ;
  double l = (1688 * rgb.x + 2146 * rgb.y + 262 * rgb.z) / 4096;
  double m = (683 * rgb.x + 2951 * rgb.y + 462 * rgb.z) / 4096;
  double s = (99 * rgb.x + 309 * rgb.y + 3688 * rgb.z) / 4096;
  double L = pq_inverse(std::max(l, 0.0)), M = pq_inverse(std::max(m, 0.0)), S = pq_inverse(std::max(s, 0.0));
  return {0.5 * L + 0.5 * M, (6610 * L - 13613 * M + 7003 * S) / 4096, (17933 * L - 17390 * M - 543 * S) / 4096};
}

double de_itp(const Vec3 &a, const Vec3 &b) {
  Vec3 p = ictcp(a), q = ictcp(b);
  double di = p.x - q.x, dt = 0.5 * (p.y - q.y), dp = p.z - q.z;
  return 720 * std::sqrt(di * di + dt * dt + dp * dp);
}

}  // namespace cal
