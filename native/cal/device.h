#pragma once
// What the calibration procedure needs from a TV and a meter. On the TV this is
// a bridge to the service (cal-tv.js and cal-meter.js); in tests it is the
// virtual G2 (sim.h). The procedure never knows which one it is driving.
#include <string>

#include "colour.h"
#include "lut.h"

namespace cal {

enum class Slot { Bt709, Bt2020 };

// A pattern as 10-bit codes, in the range the current mode expects.
struct Patch {
  int r = 0, g = 0, b = 0;
};

class Device {
 public:
  virtual ~Device() = default;
  // Shows the patch in the measuring window (with the grey between patches) and
  // returns one meter reading, absolute XYZ in cd/m². False if the meter failed.
  virtual bool measure(const Patch &p, Vec3 *XYZ) = 0;
  virtual bool upload_1d(const Lut1D &lut) = 0;
  virtual bool upload_3d(Slot slot, const Lut3D &lut) = 0;
  // Back to LG's factory data for this mode (reset_factory_data_*: dataOpt 2). Unlike an
  // upload, a reset leaves Dolby Vision's configuration pop-up available.
  virtual bool reset_factory() = 0;
  // OLED light, 0–100 (BACKLIGHT_UI_DATA).
  virtual bool set_light(int value) = 0;
  virtual int light() const = 0;
  // HDR10 tone mapping (1D_TONEMAP_PARAM): peak, then three mastering peaks with roll-offs.
  virtual bool set_tonemap(const int params[7]) = 0;
  // Calibration mode (CAL_START / CAL_END). Outside it the TV tone-maps HDR10 again;
  // final verification happens outside it, in the viewing state.
  virtual bool set_calibration_mode(bool on) = 0;
  // Dolby Vision: writes the user display configuration to the USB stick and has
  // the owner load it from the TV's pop-up. False without a stick.
  virtual bool load_dolby_config(const std::string &text) = 0;
};

}  // namespace cal
