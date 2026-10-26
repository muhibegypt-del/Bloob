#pragma once
// EsPlayer: Bloob's own playback engine (3.0). Bloob demuxes the file itself
// (demux.h) and feeds the video and Dolby sound to the TV's hardware decoders
// through LG's media pipeline for elementary streams (the route Kodi's webOS
// build takes). Compared with handing LG's player a URL:
//   - every audio track plays: DTS, TrueHD and the rest become E-AC-3;
//   - Bloob knows the file: tracks, chapters, Dolby Vision profile, HDR metadata;
//   - subtitles (text and Blu-ray pictures) are drawn by Bloob;
//   - a seek or an audio change reloads the pipeline at the exact time
//     (pauseAtDecodeTime), so it lands where it was asked.
// Threads: the UI thread calls the MediaBackend methods; a feeder thread owns
// the demuxer and feeds the sink; the sink's events arrive on its own thread
// and are posted to the UI thread.
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "demux.h"
#include "media.h"

// Where elementary streams go. Events may arrive on any thread.
class EsSink {
 public:
  // Pipeline events (StarfishMediaAPIs' PF_EVENT_* numbers).
  enum : int {
    FrameReady = 0x0,
    VideoInfo = 0x4,
    AudioInfo = 0x7,
    IntError = 0x12,
    StrError = 0x13,
    LoadCompleted = 0x16,
    UnloadCompleted = 0x17,
    SeekDone = 0x19,
    Playing = 0x1a,
    Paused = 0x1b,
    EndOfStream = 0x1c,
    BufferLow = 0x2c,
    BufferFull = 0x2d,
    BufferLowStr = 0x2e,
    DroppedFrame = 0x30,
  };
  using Event = std::function<void(int type, long long num, const std::string &str)>;
  virtual ~EsSink() = default;
  virtual bool load(const std::string &payload, Event on_event) = 0;
  virtual void set_hdr(const std::string &payload) {}
  // Returns the pipeline's answer: contains "Ok", "BufferFull", or an error.
  virtual std::string feed(const uint8_t *data, size_t size, long long pts_ns, bool video) = 0;
  virtual bool play() = 0;
  virtual bool pause() = 0;
  virtual bool push_eos() = 0;
  virtual void unload() = 0;
  virtual void notify_foreground() {}
  virtual void notify_background() {}
  virtual const char *name() const = 0;
};

// "3840×2160 · 23.976 fps · HDR10" from a reply naming width/height/frameRate/hdrType.
std::string summarise_video_info(const std::string &json);

// The TV's pipeline (libplayerAPIs, opened at run time), or null where it isn't there.
std::unique_ptr<EsSink> make_starfish_sink();
// A stand-in that "plays" by the wall clock (desktop runs and tests).
std::unique_ptr<EsSink> make_clock_sink();

class EsPlayer : public MediaBackend {
 public:
  // `post` runs a function on the UI thread (Tasks::post); `sink` is where streams go.
  EsPlayer(std::unique_ptr<EsSink> sink, std::function<void(std::function<void()>)> post);
  ~EsPlayer() override;

  bool load(const std::string &uri, const std::string &app_id, const std::string &window_id,
            const std::string &load_type, Listener listener, long long start_ms = 0) override;
  void play() override;
  void pause() override;
  void seek(long long position_ms) override;
  void select_track(const std::string &type, int index) override;
  void set_subtitles(bool enabled) override;
  void set_subtitle_style(const SubtitleStyle &style) override { style_ = style; }
  void set_subtitle_position(int position, std::function<void(bool ok)> done) override {
    if (done) done(true);  // Bloob draws subtitles: the player places them
  }
  void unload() override;
  void notify_foreground() override;
  void notify_background() override;
  const MediaInfo &info() const override { return info_; }
  bool draws_subtitles() const override { return true; }
  std::vector<SubtitleCue> cues_at(long long ms) const override;
  long long next_cue_change(long long ms) const override;
  bool converts_audio() const override { return true; }
  // Reloads the sound at the current time when the options change.
  void set_headphones(const HeadphoneOptions &options) override;
  std::string audio_note() const override { return audio_note_; }
  std::string decoder_note() const override { return decoder_note_; }
  const char *engine_name() const override;
  const std::vector<DemuxFacts::Chapter> &chapters() const { return chapters_; }

  // The Load payload for these facts (exposed for tests).
  static std::string load_payload(const DemuxFacts &f, const AudioOut &a, const std::string &app_id,
                                  const std::string &window_id, long long start_ns);
  static std::string hdr_payload(const DemuxFacts &f);

 private:
  struct Command {
    enum Kind { Seek, Audio, Text, Headphones, Stop } kind;
    long long value = 0;
  };
  void run();
  bool start_pipeline(long long position_ms);
  void stop_pipeline();
  void on_sink(int type, long long num, const std::string &str);
  void emit(MediaEvent event);
  void publish_facts(bool track_change);

  std::unique_ptr<EsSink> sink_;
  std::function<void(std::function<void()>)> post_;
  Listener listener_;
  MediaInfo info_;  // UI thread only
  std::vector<DemuxFacts::Chapter> chapters_;
  std::string audio_note_;
  std::string decoder_note_;  // from the pipeline's video info (UI thread)
  SubtitleStyle style_;
  std::string url_, app_id_, window_id_;
  long long start_ms_ = 0;

  std::thread feeder_;
  std::mutex mutex_;  // commands_, cues_
  std::condition_variable cv_;
  std::deque<Command> commands_;
  HeadphoneOptions headphones_;  // under mutex_
  std::mutex sink_mutex_;  // every sink call
  Demuxer demux_;

  std::atomic<bool> loaded_{false}, want_playing_{true}, playing_{false}, started_{false};
  std::atomic<bool> seeking_{false}, eos_sent_{false}, stopping_{false};
  bool pipeline_open_ = false;  // under sink_mutex_
  std::atomic<long long> clock_ns_{-1};      // last frame shown
  std::atomic<long long> target_ns_{0};      // where the current pipeline starts
  std::atomic<long long> last_time_post_{0};
  std::atomic<int> generation_{0};          // bumps per pipeline, so late events are dropped

  mutable std::mutex cue_mutex_;
  std::deque<SubtitleCue> cues_;
  bool subtitles_on_ = false;
  int text_index_ = -1;
  std::shared_ptr<int> alive_ = std::make_shared<int>(0);
};
