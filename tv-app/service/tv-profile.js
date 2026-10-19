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
    // Containers this TV shows Dolby Vision from. The owner reports Dolby Vision
    // in MKV works on this TV (16 October 2026), which overrides the public
    // sources that said webOS 24 needs MP4 or MPEG-TS. Another TV's profile can
    // leave MKV out; the ranker then labels such versions as the HDR10 they play as.
    dolbyVisionContainers: ['MKV', 'MP4', 'M4V', 'TS'],
    hdr10plus: false,
    threeD: false,
    // The owner's TV is on 5 GHz Wi-Fi with a 2 Gbps line (19 October 2026):
    // no bitrate limit. A profile for a TV on its 100 Mbit Ethernet port could set
    // comfortableMbps/warnMbps; null means bitrate never costs a version anything.
    connection: 'wifi',
    comfortableMbps: null,
    warnMbps: null,
  },
};

const DEFAULT_PROFILE = 'lg-2022-oled';

/** A profile by ID, or the default. Accepts a profile object as is. */
function profileFor(value) {
  if (value && typeof value === 'object') return value;
  return PROFILES[value] || PROFILES[DEFAULT_PROFILE];
}

module.exports = { PROFILES, DEFAULT_PROFILE, profileFor };
