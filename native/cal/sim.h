#pragma once
// The virtual G2 (CALIBRATION-PLAN.md §10): a WOLED panel behind LG's
// calibration pipeline, with a noisy meter in front of it. The procedure sees
// it only through Device, exactly as it sees the real TV.
//
// The panel is deliberately not what the solver assumes:
// - a white sub-pixel takes over part of the colour shared by all three
//   channels, so colours do not add up (the main reason WOLED needs a measured model);
// - each channel has its own gamma and ripple, and the native white is off D65;
// - tables are quantised exactly as the TV stores them (encode, then decode);
// - optional faults: the 1D LUT before the 3D LUT, slow dimming (ASBL-like),
//   a firmware that accepts a 3D LUT and ignores it, crushed blacks.
#include <random>
#include <string>

#include "device.h"

namespace cal {

enum class SimMode { Sdr, Hdr10, DolbyVision };

struct PanelSpec {
  std::string name = "G2";
  double red[2] = {0.675, 0.318}, green[2] = {0.270, 0.680}, blue[2] = {0.147, 0.055};
  double rgb_white[2] = {0.300, 0.318};  // R+G+B at full drive (cooler than D65)
  double white_sub[2] = {0.306, 0.330};  // the white sub-pixel
  double w_share = 0.6;                  // share of the common colour the panel gives to white
  double gamma[3] = {2.28, 2.34, 2.40};
  double ripple[3] = {0.03, -0.02, 0.025};
  double crush = 0.0;      // drive below which a channel stays dark
  double peak = 420;       // SDR full white at light 100, cd/m² (10% window)
  double hdr_peak = 780;   // HDR and Dolby Vision peak, cd/m²
  // Meter noise, as a tristimulus colorimeter has it: mostly shared by X, Y and Z
  // (luminance), a little per channel (colour), and a floor near black.
  double noise = 0.002;         // relative, shared
  double chroma_noise = 0.0007; // relative, per channel
  double floor = 0.0008;        // absolute, cd/m²
  double drift = 0.0;      // luminance lost per patch shown (ASBL-like), relative
  bool lut1d_first = false;  // pipeline order: false means 3D LUT, then 1D LUT
  bool ignores_3d = false;   // firmware fault: 3D LUT accepted, never applied
  bool usb_stick = true;     // a stick for the Dolby Vision configuration
  bool dolby_mapped = false; // Dolby Vision patches go through Dolby's mapping (no pass-through)
};

class VirtualTv : public Device {
 public:
  VirtualTv(const PanelSpec &spec, SimMode mode, unsigned seed = 1);

  bool measure(const Patch &p, Vec3 *XYZ) override;
  bool upload_1d(const Lut1D &lut) override;
  bool upload_3d(Slot slot, const Lut3D &lut) override;
  bool set_light(int value) override;
  int light() const override { return light_; }
  bool set_tonemap(const int params[7]) override;
  bool set_calibration_mode(bool on) override {
    cal_mode_ = on;
    return true;
  }
  bool reset_factory() override;
  // The TV offers the pop-up only with a stick in and no 1D LUT uploaded since the last reset [bsc].
  bool load_dolby_config(const std::string &text) override {
    if (!spec_.usb_stick || uploaded_1d_) return false;
    dolby_config_ = text;
    return true;
  }
  const std::string &dolby_config() const { return dolby_config_; }
  // Uploads with a NaN, a value outside [0, 1] or a falling 1D curve (the procedure must never send one).
  int bad_uploads() const { return bad_uploads_; }
  const int *tonemap() const { return tonemap_; }

  // Without meter noise or drift, for tests.
  Vec3 render(const Patch &p) const;
  long patches() const { return shown_; }
  const PanelSpec &spec() const { return spec_; }

 private:
  Vec3 emit(Vec3 drive, double peak) const;
  double trc(int c, double d) const;

  PanelSpec spec_;
  SimMode mode_;
  std::mt19937 rng_;
  Lut1D lut1d_ = Lut1D::unity();
  Lut3D lut709_ = Lut3D::unity(), lut2020_ = Lut3D::unity();
  int light_ = 33;
  long shown_ = 0;
  bool cal_mode_ = true;
  int tonemap_[7] = {0, 1000, 70, 4000, 60, 10000, 50};
  std::string dolby_config_;
  bool uploaded_1d_ = false;
  int bad_uploads_ = 0;
  Lut1D factory1d_ = Lut1D::unity();  // Dolby Vision's factory table balances white roughly
  Vec3 primary_[3], white_;
  Mat3 factory_hdr_;  // BT.2020 to the panel's own primaries (HDR pipeline)
};

// A few panels to prove the procedure on: the expected G2, and harder ones.
PanelSpec panel_g2();
PanelSpec panel_hard();     // strongly uneven gammas, green-heavy white, crushed blacks
PanelSpec panel_drifting(); // the G2 with dimming while measuring

}  // namespace cal
