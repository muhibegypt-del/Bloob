#pragma once
// Bloob's own demuxer for the playback engine (3.0). FFmpeg reads the file
// (over plain HTTP from the service, which proxies AllDebrid) and hands out
// what the TV's decoders take:
//   - video as Annex-B access units (H.264/HEVC converted from MP4/Matroska
//     form). Dolby Vision profile 7 (disc remuxes, two layers) becomes
//     single-layer profile 8.1 (4.0): the enhancement layer is dropped and each
//     RPU rewritten (dovi.h, dovi_tool's library). Without that library, or
//     with the HDR10/SDR picture preference, the RPU goes too and the HDR10
//     base plays, as before;
//   - sound as Dolby: AC-3 and E-AC-3 (with Atmos) pass through untouched;
//     everything else (DTS, TrueHD, AAC, FLAC, PCM, Opus…) is decoded and
//     re-encoded to E-AC-3, 5.1 at most, on this thread;
//   - text subtitles as cues for Bloob to draw;
//   - in Headphones mode (3.2), every track (Dolby included) decoded and
//     rendered for two ears (headphones.h), then sent as E-AC-3 stereo.
// Not thread-safe: one thread (the engine's feeder) calls everything.
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "headphones.h"
#include "media.h"

struct DemuxFacts {
  long long duration_ms = 0;
  std::string container;
  // Video: codec as the TV pipeline names it ("H264", "H265", "AV1", "VP9"), or empty.
  std::string video_codec;
  int width = 0, height = 0;
  int fps_num = 0, fps_den = 0;
  // Colour: FFmpeg's numbers (ITU-T H.273), and the HDR family named for people.
  int transfer = 0, primaries = 0, matrix = 0;
  bool full_range = false;
  std::string hdr;  // "Dolby Vision", "HDR10", "HLG", "SDR"
  int dv_profile = -1, dv_level = 0;
  bool dv_el = false, dv_rpu = false, dv_bl_compatible = false;
  // The profile the TV is told and given: 8 for a converted profile 7, else dv_profile.
  int dv_profile_out = -1;
  bool dv_converted = false;
  // Mastering display (0.00002 units for chromaticity, 0.0001 cd/m2 for luminance) and light level.
  bool has_mastering = false;
  int primaries_xy[3][2] = {{0, 0}, {0, 0}, {0, 0}}, white_xy[2] = {0, 0};
  long long max_luminance = 0, min_luminance = 0;
  bool has_light_level = false;
  int max_cll = 0, max_fall = 0;
  std::vector<MediaTrack> audio;  // index = position in this list
  std::vector<MediaTrack> text;
  struct Chapter {
    long long start_ms = 0, end_ms = 0;
    std::string title;
  };
  std::vector<Chapter> chapters;
};

// What the selected sound becomes on its way to the TV.
struct AudioOut {
  std::string codec;  // "AC3" or "AC3 PLUS" (the pipeline's names)
  int channels = 0;
  int sample_rate = 0;
  bool atmos = false;      // E-AC-3 with Atmos, passed through
  bool converted = false;  // decoded and re-encoded by Bloob
  bool headphones = false; // rendered for headphones (stereo out)
  std::string from;        // what it was, for people: "TrueHD 7.1"
};

// A picture subtitle (PGS, from Blu-ray remuxes): pieces placed on a canvas of
// `canvas_w` x `canvas_h` (the disc's frame, usually 1920x1080). No pieces: clear the screen.
struct SubPicture {
  int canvas_w = 0, canvas_h = 0;
  struct Piece {
    int x = 0, y = 0, w = 0, h = 0;
    std::vector<uint32_t> rgba;  // w*h pixels, R G B A bytes in memory order
  };
  std::vector<Piece> pieces;
};

struct DemuxPacket {
  enum Kind { Video, Audio, Text, Picture, End, Error, Again } kind = Again;
  std::vector<uint8_t> data;
  long long pts_us = 0;
  long long duration_us = 0;
  bool key = false;
  std::string text;  // Text: the cue; Error: what failed
  std::string ass;   // Text from an ASS/SSA track: the raw event (4.0, ass.h)
  std::shared_ptr<SubPicture> picture;  // Picture
};

class Demuxer {
 public:
  Demuxer();
  ~Demuxer();
  Demuxer(const Demuxer &) = delete;
  Demuxer &operator=(const Demuxer &) = delete;

  // Blocks while FFmpeg reads the header. `error` says why on failure.
  bool open(const std::string &url, std::string *error);
  void close();
  const DemuxFacts &facts() const { return facts_; }

  // Dolby Vision profile 7 → 8.1 (default on). Set before open().
  void set_dolby_vision(bool convert_p7) { dv_convert_ = convert_p7; }
  // RPUs rewritten and refused since open (a refused one is dropped from its frame).
  long long dv_rpus_ok() const { return dv_ok_; }
  long long dv_rpus_bad() const { return dv_bad_; }

  // Headphones mode, applied at the next select_audio().
  void set_headphones(const HeadphoneOptions &options) { headphones_ = options; }
  const HeadphoneOptions &headphones() const { return headphones_; }

  // Track choice (-1: no sound / no subtitles). Returns false for a track it can't play.
  bool select_audio(int index);
  void select_text(int index);
  int audio_index() const { return audio_choice_; }
  int text_index() const { return text_choice_; }
  const AudioOut &audio_out() const { return audio_out_; }
  // The selected text track's ASS/SSA header (styles, play resolution), empty for other formats.
  const std::string &text_header() const { return text_header_; }

  // Moves to the key frame at or before `ms`; the next packets start there.
  bool seek(long long ms);
  // The next packet. Again: nothing to hand out this time (a packet of an
  // unselected stream, or the audio encoder still filling a frame).
  DemuxPacket read();

  // From any thread: makes a blocking read give up (closing, seeking).
  void interrupt() { interrupted_.store(true); }
  void resume() { interrupted_.store(false); }

  struct Impl;  // FFmpeg state (demux.cpp)

 private:
  std::unique_ptr<Impl> impl_;
  DemuxFacts facts_;
  AudioOut audio_out_;
  HeadphoneOptions headphones_;
  int audio_choice_ = -1, text_choice_ = -1;
  bool dv_convert_ = true;
  std::string text_header_;
  long long dv_ok_ = 0, dv_bad_ = 0;
  std::atomic<bool> interrupted_{false};
  friend int demux_interrupt(void *);
};

// Pure helpers, exposed for tests.
namespace demux {
// Removes HEVC NAL units of the given types (Annex-B input), e.g. 62/63 for
// Dolby Vision's RPU and enhancement layer.
void strip_hevc_nals(std::vector<uint8_t> &annexb, int type_a, int type_b);
// Dolby Vision profile 7 → 8.1 on one access unit (Annex-B): enhancement-layer
// units (type 63) removed, each RPU (type 62) rewritten by `convert` (the NAL
// unit without start code in, the new one out; false: refused, the RPU is
// dropped). Adds to `ok` / `bad`. dovi_available() says whether Bloob was built
// with the converter; dovi_convert() is it.
using RpuConvert = bool (*)(const uint8_t *nal, size_t len, std::vector<uint8_t> &out);
void dv_p7_to_81(std::vector<uint8_t> &annexb, RpuConvert convert, long long &ok, long long &bad);
bool dovi_available();
bool dovi_convert(const uint8_t *nal, size_t len, std::vector<uint8_t> &out);
// The profile an RPU unit (no start code) carries, -1 unreadable or no converter.
int dovi_profile(const uint8_t *nal, size_t len);
// Plain text from a subtitle packet: SubRip/WebVTT tags and ASS overrides removed, \N as new lines.
std::string cue_text(const std::string &codec, const std::string &payload);
}  // namespace demux
