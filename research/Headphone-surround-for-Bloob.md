# Headphone surround for Bloob (research, 26 October 2026)

The owner wants films in Bloob to sound "breathtaking" in Sony WF-1000XM5 earbuds over Bluetooth: full surround (Atmos if possible) and loud. The evidence below is labelled **read** (pages and code read), **design proposal** or **unknown**.

## Constraints (read)

- **Bluetooth carries two channels.** Surround in headphones is binaural rendering: each speaker channel convolved with the head-related impulse response (HRIR) for its direction. With a room included, a binaural room impulse response (BRIR) externalises much better than an anechoic HRIR.
- **WF-1000XM5 codecs (Sony help guide):**
  - Classic Audio (A2DP): SBC, AAC, LDAC;
  - LE Audio: LC3, the low-latency one.
- **What the LG G2 sends:** owners report SBC, perhaps AAC, but no LDAC (unknown; Reddit threads could not be opened). Replacing the TV's Bluetooth stack is out of scope.
- **Bloob already owns film audio (3.0 engine):**
  - it decodes and re-encodes everything except AC-3/E-AC-3, which pass through;
  - Headphones mode means decoding those too and sending a binaural stereo render (E-AC-3/AC-3 stereo, or PCM if the pipeline takes it; unknown).

## Atmos height and objects (read)

- **Cavern** (github.com/VoidXH/Cavern):
  - an object-based audio engine that decodes **E-AC-3 + JOC (Dolby Digital Plus Atmos)** objects, and **TrueHD Atmos** via truehdd (in its Cavernize tool);
  - has headphone "direction and distance virtualisation" and claims ultra-low latency;
  - library: .NET Standard, no dependencies; CavernAmp is plain C++17;
  - licence: its own. Free to use and modify; modifications released free with a link to the repository; no selling, no adverts; credit with a link to the creator for public or commercial use. Fine for Bloob as a free personal app; keep it out of anything sold.
  - its HRTF measurements are in github.com/VoidXH/HRTF (licence unknown; the README was not readable).
- **truehdd** (github.com/truehdd/truehdd):
  - Rust, Apache-2.0, decodes TrueHD to PCM or a Dolby Atmos master set (bed plus objects with 3D positions); extracts objects but doesn't render them;
  - its README: "research and development, not … consumer playback systems".
- **FFmpeg** decodes only the channel bed (5.1/7.1) of Atmos streams, not objects or heights.
- **Dolby's own** Atmos headphone renderer is proprietary.

## Binaural renderers and data (read)

- **In Bloob's own FFmpeg:**
  - `headphone`: multichannel HRIR as input; HeSuVi's 14-channel WAV layout is supported;
  - `sofalizer`: SOFA files; needs libmysofa;
  - `crossfeed`, `bs2b`.
  - Bloob's builds have audio filters disabled today; enabling them is a rebuild.
- **Steam Audio** (Valve, open source): HRTF binaural and Ambisonics, accepts SOFA. A heavier dependency; ARM Linux build unknown.
- **Bring your own HRIR:**
  - HeSuVi users load 14-channel HRIR/BRIR WAVs;
  - Impulcifer measures a personal BRIR with in-ear microphones, the best possible externalisation.
  - Bloob can read a WAV the owner puts in its data folder.
  - HeSuVi's bundled presets are mostly captured from commercial virtualisers. Bloob doesn't ship those; the owner may use them personally.
- **AutoEQ** (github.com/jaakkopasanen/AutoEq, MIT): has a correction for the **Sony WF-1000XM5** (oratory1990 measurement), a parametric EQ to a neutral target. A binaural render assumes neutral headphones, so this EQ comes first.

## Loudness (read and design proposal)

- E-AC-3/AC-3 streams carry dialnorm, which makes decoders attenuate (often 4–7 dB). Kodi's webOS build offers "bypass dialog normalization".
- Bloob decoding itself can skip dialnorm, then use a true-peak limiter: loud, never clipped.
- An optional night mode compresses dynamics.

## Lip-sync (design proposal)

- Bluetooth adds roughly 100–250 ms.
- Bloob reads audio ahead of the picture, so it can lead the audio by the measured latency: perfect sync with no video delay.
- A slider in the player, remembered per headphone.

## Phone route (design proposal, later)

- webos-audiostream-remote (github.com/nickvsnetworking/webos-audiostream-remote) proves a rooted LG can stream PCM to a phone browser (it taps ALSA `hw:0,13`).
- Bloob could stream its own binaural render to the owner's phone over Wi-Fi, and the phone sends **LDAC** to the XM5: better quality than the TV's SBC.
- Sync by timestamps, with Bloob leading the audio as above.

## Plan

- **Tier 1: built in 3.2.0 (26 October 2026, not yet on the TV).** See `CLAUDE.md` and `native/TV-TEST.md` §17. The rooms are Bloob's own model (Brown and Duda), not bundled third-party data, and Bluetooth is detected with `soundOutput` (`bt_soundbar`).
- **Tier 1, as planned:** Headphones mode in Bloob's engine. Decode every track, then:
  1. dialnorm bypass;
  2. 7.1/5.1 bed through a BRIR (`headphone` filter);
  3. centre (dialogue) lift;
  4. AutoEQ XM5 correction;
  5. true-peak limiter;
  6. stereo out;
  7. lip-sync lead.
- **Tier 1 controls:**
  - Settings: Surround style (bundled open BRIR sets, plus the owner's own WAV);
  - Loudness; Dialogue; Night;
  - automatic when the TV's sound output is Bluetooth (how to read that is still to be checked on the TV).
- **Tier 2:** Atmos heights for E-AC-3 JOC, from Cavern. Either a C++ port of its JOC decoder and renderer, or its .NET library hosted on the TV; CPU to be measured.
- **Tier 3:** TrueHD Atmos objects via truehdd, rendered by the Tier 2 renderer.
- **Tier 4:** the phone/LDAC route.
