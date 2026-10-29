#include "sim.h"

#include <algorithm>
#include <cmath>

namespace cal {

VirtualTv::VirtualTv(const PanelSpec &spec, SimMode mode, unsigned seed) : spec_(spec), mode_(mode), rng_(seed) {
  // Scale the three primaries so that R+G+B at full drive lands on rgb_white with Y = 1.
  Vec3 r = xyY(spec.red[0], spec.red[1], 1), g = xyY(spec.green[0], spec.green[1], 1), b = xyY(spec.blue[0], spec.blue[1], 1);
  Vec3 s = Mat3::columns(r, g, b).inverse() * xyY(spec.rgb_white[0], spec.rgb_white[1], 1);
  primary_[0] = r * s.x;
  primary_[1] = g * s.y;
  primary_[2] = b * s.z;
  white_ = xyY(spec.white_sub[0], spec.white_sub[1], 1);
  // The factory's HDR gamut conversion, BT.2020 into the panel's primaries, made for a
  // nominal P3 panel rather than this one, as a production line would.
  factory_hdr_ = rgb_to_xyz(kP3D65).inverse() * rgb_to_xyz(kBt2020);
  if (mode == SimMode::DolbyVision) {
    // LG's factory data: per-channel gains that bring full white near D65.
    Vec3 g = Mat3::columns(primary_[0], primary_[1], primary_[2]).inverse() * xyY(kD65x, kD65y, 1);
    double top = std::max({g.x, g.y, g.z});
    for (int c = 0; c < 3; c++)
      for (int i = 0; i < Lut1D::kSize; i++) factory1d_.ch[c][i] = i / 1023.0 * std::pow(g[c] / top, 1 / 2.3);
    lut1d_ = factory1d_;
  }
}

bool VirtualTv::reset_factory() {
  lut1d_ = factory1d_;
  lut709_ = lut2020_ = Lut3D::unity();
  uploaded_1d_ = false;
  return true;
}

double VirtualTv::trc(int c, double d) const {
  d = std::clamp(d, 0.0, 1.0);
  if (spec_.crush > 0) d = std::max(d - spec_.crush, 0.0) / (1 - spec_.crush);
  double t = std::pow(d, spec_.gamma[c]);
  return t * (1 + spec_.ripple[c] * std::sin(2 * 3.14159265358979 * d) * (1 - d));
}

// The panel: linear channel light, part of the shared colour moved to the white sub-pixel.
Vec3 VirtualTv::emit(Vec3 drive, double peak) const {
  double lin[3] = {trc(0, drive.x), trc(1, drive.y), trc(2, drive.z)};
  double w = spec_.w_share * std::min({lin[0], lin[1], lin[2]});
  Vec3 out = white_ * w;
  for (int c = 0; c < 3; c++) out = out + primary_[c] * (lin[c] - w);
  return out * peak;
}

Vec3 VirtualTv::render(const Patch &p) const {
  auto apply = [&](Vec3 v, const Lut3D &lut3) {
    if (spec_.lut1d_first) {
      v = {lut1d_.eval(0, v.x), lut1d_.eval(1, v.y), lut1d_.eval(2, v.z)};
      return spec_.ignores_3d ? v : lut3.eval(v);
    }
    if (!spec_.ignores_3d) v = lut3.eval(v);
    return Vec3{lut1d_.eval(0, v.x), lut1d_.eval(1, v.y), lut1d_.eval(2, v.z)};
  };
  if (mode_ == SimMode::Sdr) {
    Vec3 v{std::clamp((p.r - 64) / 876.0, 0.0, 1.0), std::clamp((p.g - 64) / 876.0, 0.0, 1.0), std::clamp((p.b - 64) / 876.0, 0.0, 1.0)};
    return emit(apply(v, lut709_), spec_.peak * light_ / 100.0);
  }
  if (mode_ == SimMode::Hdr10) {
    // Calibration mode: no tone mapping. PQ to light, BT.2020 to the panel's primaries, clipped at its peak.
    Vec3 v{std::clamp((p.r - 64) / 876.0, 0.0, 1.0), std::clamp((p.g - 64) / 876.0, 0.0, 1.0), std::clamp((p.b - 64) / 876.0, 0.0, 1.0)};
    Vec3 d = apply(v, lut2020_);
    Vec3 nits{pq_eotf(d.x), pq_eotf(d.y), pq_eotf(d.z)};
    if (!cal_mode_) {
      // Outside calibration mode the TV tone-maps: unchanged up to the knee, then a
      // smooth roll-off that approaches its peak (a stand-in for LG's curve).
      double P = tonemap_[0] > 0 ? tonemap_[0] : spec_.hdr_peak, K = P * tonemap_[2] / 100.0;
      for (int c = 0; c < 3; c++)
        if (nits[c] > K) nits[c] = K + (P - K) * (1 - std::exp(-(nits[c] - K) / (P - K)));
    }
    Vec3 lin = nits / spec_.hdr_peak;
    Vec3 nat = factory_hdr_ * lin;
    Vec3 drive;
    for (int c = 0; c < 3; c++) drive[c] = std::pow(std::clamp(nat[c], 0.0, 1.0), 1 / 2.3);  // the panel's gammas then add their error
    return emit(drive, spec_.hdr_peak);
  }
  // Dolby Vision calibration patches: full range, passed through as gamma 2.2 to the peak (Calman's target).
  Vec3 v{std::clamp(p.r / 1023.0, 0.0, 1.0), std::clamp(p.g / 1023.0, 0.0, 1.0), std::clamp(p.b / 1023.0, 0.0, 1.0)};
  Vec3 d{lut1d_.eval(0, v.x), lut1d_.eval(1, v.y), lut1d_.eval(2, v.z)};
  // Without pass-through, Dolby's mapping bends the ramp (here: a strong S-curve).
  double g = spec_.dolby_mapped ? 1.3 : 2.2;
  for (int c = 0; c < 3; c++) {
    double t = std::pow(d[c], g);
    if (spec_.dolby_mapped) t = t * t * (3 - 2 * t);
    d[c] = std::pow(t, 1 / 2.3);
  }
  return emit(d, spec_.hdr_peak);
}

bool VirtualTv::measure(const Patch &p, Vec3 *XYZ) {
  shown_++;
  Vec3 x = render(p) * std::max(0.0, 1 - spec_.drift * shown_);
  std::normal_distribution<double> shared(0, spec_.noise), own(0, spec_.chroma_noise), abs(0, spec_.floor);
  double common = 1 + shared(rng_);
  for (int c = 0; c < 3; c++) x[c] = x[c] * common * (1 + own(rng_)) + abs(rng_);
  *XYZ = x;
  return true;
}

// Uploads are quantised exactly as the TV stores them.
bool VirtualTv::upload_1d(const Lut1D &lut) {
  for (const auto &c : lut.ch) {
    bool bad = c.size() != Lut1D::kSize;
    for (size_t i = 0; i < c.size() && !bad; i++)
      bad = !std::isfinite(c[i]) || c[i] < 0 || c[i] > 1 || (i && c[i] < c[i - 1] - 1e-9);
    if (bad) bad_uploads_++;
  }
  lut1d_ = Lut1D::decode(lut.encode());
  uploaded_1d_ = true;
  return true;
}

bool VirtualTv::upload_3d(Slot slot, const Lut3D &lut) {
  for (const Vec3 &v : lut.node)
    for (int c = 0; c < 3; c++)
      if (!std::isfinite(v[c]) || v[c] < 0 || v[c] > 1) {
        bad_uploads_++;
        goto checked;
      }
checked:
  (slot == Slot::Bt709 ? lut709_ : lut2020_) = Lut3D::decode(lut.encode(), lut.n);
  return true;
}

bool VirtualTv::set_tonemap(const int params[7]) {
  if (params[0] < 100 || params[0] > 4000) return false;
  for (int i = 0; i < 7; i++) tonemap_[i] = params[i];
  return true;
}

bool VirtualTv::set_light(int value) {
  if (value < 0 || value > 100) return false;
  light_ = value;
  return true;
}

PanelSpec panel_g2() { return PanelSpec{}; }

PanelSpec panel_hard() {
  PanelSpec s;
  s.name = "hard";
  s.rgb_white[0] = 0.290;
  s.rgb_white[1] = 0.340;
  s.gamma[0] = 2.05;
  s.gamma[1] = 2.55;
  s.gamma[2] = 2.25;
  s.ripple[0] = 0.06;
  s.ripple[1] = -0.05;
  s.ripple[2] = 0.05;
  s.w_share = 0.75;
  s.crush = 0.008;
  s.noise = 0.004;
  s.chroma_noise = 0.0012;
  return s;
}

PanelSpec panel_drifting() {
  PanelSpec s;
  s.name = "drifting";
  s.drift = 0.00002;  // about 2% lost over a thousand patches
  return s;
}

}  // namespace cal
