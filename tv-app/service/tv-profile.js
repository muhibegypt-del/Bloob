// What a TV can play, kept as data so the ranker reads a table instead of
// hard-coded conditions. Add a profile per TV model; the ranker never names one.
// Values are what we know or have been told, not what was measured, unless the
// field says 'yes' after a TV test.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).

const PROFILES = {
  // LG OLED55G26LA (2022 G2), webOS 9.2.2: the owner's TV.
  'lg-2022-oled': {
    id: 'lg-2022-oled',
    name: 'LG OLED 2022 (G2)',
    // 'none': no decoder and no passthrough; 'passthrough' or 'decode' otherwise.
    dts: 'none',
    // TrueHD (and TrueHD Atmos) to a receiver over eARC: 'yes', 'no' or 'unverified'.
    truehdPassthrough: 'unverified',
    // Disc Dolby Vision (profile 7, dual layer): 'yes' or 'no'. 'no' plays the HDR10 base layer.
    dolbyVisionProfile7: 'no',
    hdr10plus: false,
    threeD: false,
    // Wired connection speed; the 2022 OLEDs have a 100 Mbit port.
    ethernetMbps: 95,
    // Estimated video bitrate where buffering becomes a risk, and where to warn.
    comfortableMbps: 40,
    warnMbps: 60,
  },
};

const DEFAULT_PROFILE = 'lg-2022-oled';

/** A profile by ID, or the default. Accepts a profile object as is. */
function profileFor(value) {
  if (value && typeof value === 'object') return value;
  return PROFILES[value] || PROFILES[DEFAULT_PROFILE];
}

module.exports = { PROFILES, DEFAULT_PROFILE, profileFor };
