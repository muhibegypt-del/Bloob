// Host tests for the engine's demuxer (3.0): a generated Matroska file with
// HEVC, TrueHD, DTS, AC-3, SubRip and chapters goes in; Annex-B video, Dolby
// sound (converted where the TV can't decode it) and plain cues come out.
// Needs the desktop ffmpeg command to make the file; skipped without it.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "demux.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void test_cue_text() {
  CHECK(demux::cue_text("SubRip", "<i>Hello</i>\r\nthere ") == "Hello\nthere");
  CHECK(demux::cue_text("ASS", "0,0,Default,,0,0,0,,{\\an8}Up\\Nhere") == "Up\nhere");
  CHECK(demux::cue_text("mov_text", std::string("\0\x03" "Hi!", 5)) == "Hi!");
}

static void test_strip_nals() {
  // Three HEVC units: VPS (32), an RPU (62) with a four-byte start code, a slice (1).
  std::vector<uint8_t> b = {0, 0, 1, 32 << 1, 1, 0xAA, 0, 0, 0, 1, 62 << 1, 1, 0xBB, 0, 0, 1, 1 << 1, 1, 0xCC};
  demux::strip_hevc_nals(b, 62, 63);
  std::vector<uint8_t> want = {0, 0, 1, 32 << 1, 1, 0xAA, 0, 0, 1, 1 << 1, 1, 0xCC};
  CHECK(b == want);
  std::vector<uint8_t> keep = want;
  demux::strip_hevc_nals(keep, 62, 63);
  CHECK(keep == want);
}

static bool make_file(const std::string &path) {
  if (std::system("ffmpeg -version >/dev/null 2>&1") != 0) return false;
  std::ofstream("/tmp/bloob-demux-test.srt") << "1\n00:00:01,000 --> 00:00:02,500\n<i>First line</i>\n\n2\n00:00:05,000 --> 00:00:06,000\nSecond\n";
  std::ofstream("/tmp/bloob-demux-test.meta") << ";FFMETADATA1\n[CHAPTER]\nTIMEBASE=1/1000\nSTART=0\nEND=3000\ntitle=Intro\n"
                                                 "[CHAPTER]\nTIMEBASE=1/1000\nSTART=3000\nEND=8000\ntitle=Film\n";
  std::string cmd =
      "ffmpeg -v error -y -f lavfi -i testsrc2=s=320x180:r=24:d=8 -f lavfi -i sine=f=440:d=8:r=48000 "
      "-i /tmp/bloob-demux-test.srt -i /tmp/bloob-demux-test.meta "
      "-filter_complex '[1:a]pan=5.1|FL=c0|FR=c0|FC=c0|LFE=c0|BL=c0|BR=c0,asplit=2[a][b];[1:a]asplit=1[c]' "
      "-map 0:v -map '[a]' -map '[b]' -map '[c]' -map 2:s -map_metadata 3 -map_chapters 3 "
      "-c:v libx265 -x265-params log-level=none -preset ultrafast "
      "-c:a:0 truehd -c:a:1 dca -c:a:2 ac3 -strict -2 -c:s srt "
      "-metadata:s:a:0 language=eng -metadata:s:a:1 language=fre -metadata:s:a:2 title=Commentary "
      "-metadata:s:s:0 language=eng " + path + " 2>/dev/null";
  return std::system(cmd.c_str()) == 0;
}

static void test_file() {
  const std::string path = "/tmp/bloob-demux-test.mkv";
  if (!make_file(path)) {
    std::printf("demux file test skipped (no ffmpeg with libx265/truehd/dca)\n");
    return;
  }
  Demuxer d;
  std::string error;
  CHECK(d.open("file:" + path, &error));
  if (!error.empty()) std::printf("  open error: %s\n", error.c_str());
  const DemuxFacts &f = d.facts();
  CHECK(f.container == "MKV");
  CHECK(f.video_codec == "H265");
  CHECK(f.width == 320 && f.height == 180);
  CHECK(f.duration_ms > 7000 && f.duration_ms < 9000);
  CHECK(f.audio.size() == 3);
  if (f.audio.size() == 3) {
    CHECK(f.audio[0].codec == "TrueHD" && f.audio[0].language == "eng" && f.audio[0].channels == 6);
    CHECK(f.audio[1].codec.find("DTS") == 0);
    CHECK(f.audio[2].codec == "AC-3" && f.audio[2].commentary);
  }
  CHECK(f.text.size() == 1 && f.text[0].codec == "SubRip");
  CHECK(f.chapters.size() == 2 && f.chapters[0].title == "Intro" && f.chapters[1].start_ms == 3000);

  // TrueHD 5.1 becomes E-AC-3 5.1; subtitles on.
  CHECK(d.select_audio(0));
  CHECK(d.audio_out().converted && d.audio_out().codec == "AC3 PLUS" && d.audio_out().channels == 6);
  d.select_text(0);
  int video = 0, audio = 0, key = 0;
  long long last_audio = -1, first_audio = -1;
  bool annexb = true, sync = true;
  std::string cues;
  for (int i = 0; i < 100000; i++) {  // TrueHD comes in 1/1200 s packets
    DemuxPacket p = d.read();
    if (p.kind == DemuxPacket::End || p.kind == DemuxPacket::Error) break;
    if (p.kind == DemuxPacket::Video) {
      video++;
      key += p.key;
      annexb = annexb && p.data.size() > 4 && p.data[0] == 0 && p.data[1] == 0 && (p.data[2] == 1 || (p.data[2] == 0 && p.data[3] == 1));
    } else if (p.kind == DemuxPacket::Audio) {
      audio++;
      sync = sync && p.data.size() > 2 && p.data[0] == 0x0B && p.data[1] == 0x77;  // AC-3 family sync word
      if (first_audio < 0) first_audio = p.pts_us;
      CHECK(p.pts_us > last_audio);
      last_audio = p.pts_us;
    } else if (p.kind == DemuxPacket::Text) {
      cues += p.text + "|";
    }
  }
  CHECK(video >= 190 && key >= 1 && annexb);
  CHECK(audio > 200 && sync);  // 8 s at 32 ms per frame
  CHECK(first_audio >= 0 && first_audio < 100000);
  CHECK(cues == "First line|Second|");

  // DTS converts too; AC-3 passes through.
  CHECK(d.select_audio(1) && d.audio_out().converted);
  CHECK(d.select_audio(2) && !d.audio_out().converted && d.audio_out().codec == "AC3");

  // Seek: the next video is a key frame at or before 5 s, and audio follows near it.
  CHECK(d.seek(5000));
  bool first_video = true;
  long long seek_audio = -1;
  for (int i = 0; i < 20000 && (first_video || seek_audio < 0); i++) {
    DemuxPacket p = d.read();
    if (p.kind == DemuxPacket::Video && first_video) {
      first_video = false;
      CHECK(p.key);
      CHECK(p.pts_us <= 5000000 + 50000);
    }
    if (p.kind == DemuxPacket::Audio && seek_audio < 0) seek_audio = p.pts_us;
  }
  CHECK(!first_video);
  CHECK(seek_audio >= 0 && seek_audio < 5500000);
}

int run_demux_tests() {
  failures = 0;
  test_cue_text();
  test_strip_nals();
  test_file();
  if (!failures) std::printf("demux tests passed\n");
  return failures;
}
