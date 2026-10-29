#include "model.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace cal {

void Monotone::fit(std::vector<double> x, std::vector<double> y) {
  // Sort, merge equal x, and force y non-decreasing (light never falls as drive rises).
  std::vector<size_t> idx(x.size());
  std::iota(idx.begin(), idx.end(), 0);
  std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return x[a] < x[b]; });
  x_.clear();
  y_.clear();
  for (size_t i : idx) {
    if (!x_.empty() && std::fabs(x[i] - x_.back()) < 1e-9) {
      y_.back() = 0.5 * (y_.back() + y[i]);
      continue;
    }
    x_.push_back(x[i]);
    y_.push_back(y[i]);
  }
  for (size_t i = 1; i < y_.size(); i++) y_[i] = std::max(y_[i], y_[i - 1]);
  size_t n = x_.size();
  m_.assign(n, 0);
  if (n < 2) return;
  std::vector<double> d(n - 1);
  for (size_t i = 0; i + 1 < n; i++) d[i] = (y_[i + 1] - y_[i]) / (x_[i + 1] - x_[i]);
  m_[0] = d[0];
  m_[n - 1] = d[n - 2];
  for (size_t i = 1; i + 1 < n; i++) m_[i] = (d[i - 1] * d[i] <= 0) ? 0 : 0.5 * (d[i - 1] + d[i]);
  for (size_t i = 0; i + 1 < n; i++) {
    if (d[i] == 0) {
      m_[i] = m_[i + 1] = 0;
      continue;
    }
    double a = m_[i] / d[i], b = m_[i + 1] / d[i], h = a * a + b * b;
    if (h > 9) {
      double t = 3 / std::sqrt(h);
      m_[i] = t * a * d[i];
      m_[i + 1] = t * b * d[i];
    }
  }
}

double Monotone::operator()(double x) const {
  size_t n = x_.size();
  if (n == 0) return x;
  if (n == 1) return y_[0];
  if (x <= x_[0]) return y_[0] + m_[0] * (x - x_[0]);
  if (x >= x_[n - 1]) return y_[n - 1] + m_[n - 1] * (x - x_[n - 1]);
  size_t i = std::upper_bound(x_.begin(), x_.end(), x) - x_.begin() - 1;
  double h = x_[i + 1] - x_[i], t = (x - x_[i]) / h, t2 = t * t, t3 = t2 * t;
  return (2 * t3 - 3 * t2 + 1) * y_[i] + (t3 - 2 * t2 + t) * h * m_[i] + (-2 * t3 + 3 * t2) * y_[i + 1] + (t3 - t2) * h * m_[i + 1];
}

double Monotone::slope(double x) const {
  const double e = 1e-4;
  return ((*this)(x + e) - (*this)(x - e)) / (2 * e);
}

Vec3 PanelModel::baseline(Vec3 rgb) const {
  Vec3 t{trc_[0](rgb.x), trc_[1](rgb.y), trc_[2](rgb.z)};
  return black_ + M_ * t;
}

// Trilinear interpolation in the residual lattice.
static void corners(Vec3 p, int n, int idx[8], double w[8]) {
  double s = n - 1;
  double x = std::clamp(p.x, 0.0, 1.0) * s, y = std::clamp(p.y, 0.0, 1.0) * s, z = std::clamp(p.z, 0.0, 1.0) * s;
  int i = std::min(static_cast<int>(x), n - 2), j = std::min(static_cast<int>(y), n - 2), k = std::min(static_cast<int>(z), n - 2);
  double fx = x - i, fy = y - j, fz = z - k;
  int q = 0;
  for (int dz = 0; dz < 2; dz++)
    for (int dy = 0; dy < 2; dy++)
      for (int dx = 0; dx < 2; dx++, q++) {
        idx[q] = ((k + dz) * n + (j + dy)) * n + (i + dx);
        w[q] = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy) * (dz ? fz : 1 - fz);
      }
}

Vec3 PanelModel::residual(Vec3 rgb) const {
  if (lat_.empty()) return {};
  int idx[8];
  double w[8];
  corners(rgb, n_, idx, w);
  Vec3 r;
  for (int q = 0; q < 8; q++) r = r + lat_[idx[q]] * w[q];
  return r * scale_;
}

Vec3 PanelModel::predict(Vec3 rgb) const { return baseline(rgb) + residual(rgb); }

// Lattice regression: minimise Σ w_i |interp(x_i) − r_i|² + λ Σ (second differences)²,
// by conjugate gradients on the normal equations, one component at a time.
bool PanelModel::fit_lattice(const std::vector<Sample> &s, const std::vector<int> &use, double lambda, std::vector<Vec3> *out) const {
  int n = n_, N = n * n * n;
  std::vector<std::array<int, 8>> I(use.size());
  std::vector<std::array<double, 8>> W(use.size());
  std::vector<Vec3> r(use.size());
  for (size_t u = 0; u < use.size(); u++) {
    const Sample &x = s[use[u]];
    corners(x.rgb, n, I[u].data(), W[u].data());
    r[u] = (x.XYZ - baseline(x.rgb)) / scale_;
  }
  // The data weight is normalised so λ means the same for any number of samples.
  double total_w = 0;
  for (int u : use) total_w += s[u].weight;
  double norm = static_cast<double>(N) / std::max(total_w, 1e-9);
  auto at = [n](int i, int j, int k) { return (k * n + j) * n + i; };
  auto apply = [&](const std::vector<double> &v, std::vector<double> &y) {
    std::fill(y.begin(), y.end(), 0.0);
    for (size_t u = 0; u < use.size(); u++) {
      double a = 0;
      for (int q = 0; q < 8; q++) a += W[u][q] * v[I[u][q]];
      a *= s[use[u]].weight * norm;
      for (int q = 0; q < 8; q++) y[I[u][q]] += W[u][q] * a;
    }
    for (int axis = 0; axis < 3; axis++)
      for (int k = 0; k < n; k++)
        for (int j = 0; j < n; j++)
          for (int i = 1; i + 1 < n; i++) {
            int a, b, c;
            if (axis == 0) a = at(i - 1, j, k), b = at(i, j, k), c = at(i + 1, j, k);
            else if (axis == 1) a = at(j, i - 1, k), b = at(j, i, k), c = at(j, i + 1, k);
            else a = at(j, k, i - 1), b = at(j, k, i), c = at(j, k, i + 1);
            double d = (v[a] - 2 * v[b] + v[c]) * lambda;
            y[a] += d;
            y[b] -= 2 * d;
            y[c] += d;
          }
    for (int q = 0; q < N; q++) y[q] += 1e-9 * v[q];
  };
  out->assign(N, Vec3{});
  for (int comp = 0; comp < 3; comp++) {
    std::vector<double> b(N, 0.0), x(N, 0.0), rr(N), p(N), Ap(N);
    for (size_t u = 0; u < use.size(); u++)
      for (int q = 0; q < 8; q++) b[I[u][q]] += W[u][q] * r[u][comp] * s[use[u]].weight * norm;
    rr = b;
    p = rr;
    double rs = std::inner_product(rr.begin(), rr.end(), rr.begin(), 0.0), b2 = std::max(rs, 1e-30);
    for (int it = 0; it < 800 && rs > 1e-18 * b2; it++) {
      apply(p, Ap);
      double pAp = std::inner_product(p.begin(), p.end(), Ap.begin(), 0.0);
      if (pAp <= 0) break;
      double a = rs / pAp;
      for (int q = 0; q < N; q++) {
        x[q] += a * p[q];
        rr[q] -= a * Ap[q];
      }
      double rs2 = std::inner_product(rr.begin(), rr.end(), rr.begin(), 0.0);
      for (int q = 0; q < N; q++) p[q] = rr[q] + rs2 / rs * p[q];
      rs = rs2;
    }
    for (int q = 0; q < N; q++) (*out)[q][comp] = x[q];
  }
  return true;
}

bool PanelModel::fit(const std::vector<Sample> &s, int lattice, std::string *error) {
  n_ = std::max(lattice, 3);
  lat_.clear();
  // Black, white and the channel ramps.
  black_ = {};
  double white_y = 0;
  int blacks = 0;
  std::array<std::vector<double>, 3> rx, ry;
  Vec3 full[3];
  double full_v[3] = {0, 0, 0};
  for (const Sample &x : s) {
    double m = std::max({x.rgb.x, x.rgb.y, x.rgb.z});
    if (m < 1e-6) {
      black_ = black_ + x.XYZ;
      blacks++;
    }
    white_y = std::max(white_y, x.XYZ.y);
  }
  if (blacks) black_ = black_ / blacks;
  for (int c = 0; c < 3; c++) {
    for (const Sample &x : s) {
      double v = x.rgb[c], o1 = x.rgb[(c + 1) % 3], o2 = x.rgb[(c + 2) % 3];
      if (o1 > 1e-6 || o2 > 1e-6 || v < 1e-6) continue;
      if (v > full_v[c]) {
        full_v[c] = v;
        full[c] = x.XYZ - black_;
      }
    }
    if (full_v[c] < 0.999) {
      if (error) *error = "the model needs each channel's ramp up to full drive";
      return false;
    }
  }
  M_ = Mat3::columns(full[0], full[1], full[2]);
  Mat3 inv = M_.inverse();
  for (int c = 0; c < 3; c++) {
    std::vector<double> xs{0}, ys{0};
    for (const Sample &x : s) {
      double v = x.rgb[c], o1 = x.rgb[(c + 1) % 3], o2 = x.rgb[(c + 2) % 3];
      if (o1 > 1e-6 || o2 > 1e-6 || v < 1e-6) continue;
      xs.push_back(v);
      ys.push_back(std::max(0.0, (inv * (x.XYZ - black_))[c]));
    }
    trc_[c].fit(xs, ys);
  }
  scale_ = std::max(white_y, 1e-6);
  // Choose λ by 4-fold cross-validation on ΔE2000 relative to the brightest sample.
  Vec3 white_ref = predict({1, 1, 1});
  std::vector<int> all(s.size());
  std::iota(all.begin(), all.end(), 0);
  const double lambdas[] = {1e-4, 1e-3, 1e-2, 1e-1};
  double best = 1e300;
  for (double lam : lambdas) {
    double err = 0;
    int cnt = 0;
    for (int fold = 0; fold < 4; fold++) {
      std::vector<int> train, test;
      for (int i : all) (i % 4 == fold ? test : train).push_back(i);
      std::vector<Vec3> lat;
      fit_lattice(s, train, lam, &lat);
      std::swap(lat_, lat);
      for (int i : test) {
        err += de2000_xyz(predict(s[i].rgb), s[i].XYZ, white_ref);
        cnt++;
      }
      std::swap(lat_, lat);
      lat_.clear();
    }
    err /= std::max(cnt, 1);
    if (err < best) {
      best = err;
      lambda_ = lam;
    }
  }
  holdout_de_ = best;
  fit_lattice(s, all, lambda_, &lat_);
  return true;
}

double PanelModel::invert(const Vec3 &target, const Vec3 &white, Vec3 *rgb) const {
  Vec3 x = *rgb;
  Vec3 tl = lab(target, white);
  auto err = [&](Vec3 p) { return de2000(lab(predict(p), white), tl); };
  double e = err(x);
  for (int it = 0; it < 30 && e > 0.01; it++) {
    // Newton step on XYZ relative to white, with a numeric Jacobian.
    Vec3 f = (predict(x) - target) / white.y;
    Mat3 J;
    const double h = 1e-4;
    for (int c = 0; c < 3; c++) {
      Vec3 xp = x, xm = x;
      xp[c] = std::min(1.0, x[c] + h);
      xm[c] = std::max(0.0, x[c] - h);
      Vec3 d = (predict(xp) - predict(xm)) / (white.y * (xp[c] - xm[c]));
      for (int k = 0; k < 3; k++) J.m[k][c] = d[k];
    }
    Vec3 step = J.inverse() * f;
    double t = 1;
    bool moved = false;
    for (int ls = 0; ls < 12; ls++, t *= 0.5) {
      Vec3 y{std::clamp(x.x - t * step.x, 0.0, 1.0), std::clamp(x.y - t * step.y, 0.0, 1.0), std::clamp(x.z - t * step.z, 0.0, 1.0)};
      double ey = err(y);
      if (ey < e) {
        x = y;
        e = ey;
        moved = true;
        break;
      }
    }
    if (!moved) break;
  }
  *rgb = x;
  return e;
}

}  // namespace cal
