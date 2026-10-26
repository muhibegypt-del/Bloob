#pragma once
// UmsPlayer: LG's uMediaServer (luna://com.webos.media) driven over the bus.
// It plays a URI through the TV's own pipeline, the same hardware path the
// web engine uses, and reports source/track/time events.
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "headphones.h"

struct MediaTrack {
  int index = 0;
  std::string language;
  std::string codec;
  int channels = 0;
  bool selected = false;
  // Only when the TV reports them (field names are unverified on this TV;
  // the first track's raw keys are logged so a run shows what exists).
  std::string title;
  bool forced = false;
  bool hearing_impaired = false;
  bool commentary = false;
  bool is_default = false;
};

struct MediaInfo {
  std::string container;
  long long duration_ms = 0;
  bool seekable = false;
  std::string video_codec;
  int width = 0;
  int height = 0;
  double frame_rate = 0;
  std::string hdr;
  std::vector<MediaTrack> audio;
  std::vector<MediaTrack> text;
  // uMediaServer's bufferRange report (milliseconds; -1 when not reported).
  long long buffer_begin_ms = -1, buffer_end_ms = -1, buffer_remaining_ms = -1;
  int buffer_percent = -1;
};

struct MediaEvent {
  enum Kind { Loaded, Playing, Paused, BufferingStart, BufferingEnd, SeekDone, Time, SourceInfo, VideoInfo,
              TrackSelected, EndOfStream, Error,
              // The pipeline is gone (the TV released it, or a command was refused
              // because the media ID no longer exists). The player reloads.
              Lost } kind;
  long long time_ms = 0;
  std::string text;  // error text or track type
  int code = 0;      // error code or track index
};

// A subtitle Bloob draws itself (the engine's subtitles; uMediaServer draws its own).
struct SubPicture;
struct SubtitleCue {
  long long start_ms = 0, end_ms = 0;
  std::string text;
  std::shared_ptr<SubPicture> picture;  // a PGS picture instead of text
};

// What the player screen drives. Two backends: UmsPlayer (LG's player, given
// the URL) and EsPlayer (Bloob's engine: its own demuxer feeding the TV's decoders).
class MediaBackend {
 public:
  using Listener = std::function<void(const MediaEvent &)>;
  struct SubtitleStyle {
    int size = 0;
    bool background = false;
  };
  struct Bridge {
    std::string url;
    long long duration_ms = 0;
  };
  virtual ~MediaBackend() = default;
  virtual bool load(const std::string &uri, const std::string &app_id, const std::string &window_id,
                    const std::string &load_type, Listener listener, long long start_ms = 0) = 0;
  virtual void play() = 0;
  virtual void pause() = 0;
  virtual void seek(long long position_ms) = 0;
  virtual void select_track(const std::string &type, int index) = 0;
  virtual void set_subtitles(bool enabled) = 0;
  virtual void set_subtitle_style(const SubtitleStyle &style) = 0;
  virtual void set_subtitle_position(int position, std::function<void(bool ok)> done) = 0;
  virtual void unload() = 0;
  virtual void notify_foreground() = 0;
  virtual void notify_background() = 0;
  virtual const MediaInfo &info() const = 0;
  virtual void set_bridge(const Bridge &) {}
  // The engine draws subtitles in Bloob's UI: the cues showing at `ms`.
  virtual bool draws_subtitles() const { return false; }
  virtual std::vector<SubtitleCue> cues_at(long long ms) const { return {}; }
  // When what cues_at shows next changes after `ms` (-1: nothing known yet).
  virtual long long next_cue_change(long long ms) const { return -1; }
  // Every audio track plays (the engine converts what the TV can't decode).
  virtual bool converts_audio() const { return false; }
  // For the info panel: what the sound becomes, when converted ("TrueHD 7.1 -> Dolby Digital Plus 5.1").
  virtual std::string audio_note() const { return ""; }
  // What the TV's decoder reports it is decoding ("3840×2160 · 23.976 fps · DolbyVision"), when known.
  virtual std::string decoder_note() const { return ""; }
  // Headphones mode (3.2): the engine renders the sound for two ears; others ignore it.
  virtual void set_headphones(const HeadphoneOptions &) {}
  virtual const char *engine_name() const = 0;
};

class UmsPlayer : public MediaBackend {
 public:

  // `load_type` is uMediaServer's pipeline type ("media" by default).
  // `start_ms` asks the pipeline to begin there (LG's documented
  // transmission.playTime.start option; unverified on this TV). If the TV
  // refuses a load carrying it, the load is retried once without it.
  bool load(const std::string &uri, const std::string &app_id, const std::string &window_id,
            const std::string &load_type, Listener listener, long long start_ms = 0) override;
  void play() override;
  void pause() override;
  void seek(long long position_ms) override;
  void select_track(const std::string &type, int index) override;
  void set_subtitles(bool enabled) override;
  // Subtitle look, re-sent after every enable: TVs drop it when subtitles are
  // switched on again. Community-documented uMediaServer calls (also used by
  // Stremio's webOS player): setSubtitleFontSize 0 (smallest) .. 4, the TV's
  // default being 2; character and background colours by name, opacity 0..255.
  void set_subtitle_style(const SubtitleStyle &style) override { style_ = style; }
  // Undocumented uMediaServer call (community-documented): -3 lowest .. 4 highest, 0 default.
  // `done(false)` when the TV refuses it.
  void set_subtitle_position(int position, std::function<void(bool ok)> done) override;
  void unload() override;
  // uMediaServer's resource policy follows the app's foreground state.
  void notify_foreground() override;
  void notify_background() override;

  const std::string &media_id() const { return media_id_; }
  const MediaInfo &info() const override { return info_; }
  const char *engine_name() const override { return bridged() ? "LG player (audio bridge)" : "LG player"; }

  // The audio bridge (3.0): for a DTS-only file, play the service's converted
  // stream instead. `url` takes &t=<seconds>; loads start there and clock
  // reports are offset by it, the duration is the file's, and a seek reloads
  // the stream at the new time without the rest of the player noticing.
  void set_bridge(const Bridge &bridge) override { bridge_ = bridge; }
  bool bridged() const { return !bridge_.url.empty(); }

 private:
  void command(const char *method, const std::string &extra = "", std::function<void(bool ok)> done = nullptr);
  void on_event(const std::string &payload);

  std::string media_id_;
  Listener listener_;
  MediaInfo info_;
  int subscription_ = 0;
  int generation_ = 0;  // bumps on unload so late load replies are discarded
  bool logged_keys_ = false;
  bool logged_track_keys_ = false;
  bool logged_buffer_ = false;
  std::set<std::string> seen_keys_;
  bool lost_ = false;
  SubtitleStyle style_;
  Bridge bridge_;
  long long offset_ = 0;  // bridge: where the current stream starts in the film
  bool reseeking_ = false, resume_play_ = false, playing_ = false, subs_on_ = false;
  int text_index_ = -1;
  std::string app_id_, window_id_, load_type_;
  void reseek(long long position_ms);
  void apply_subtitle_style();
  std::shared_ptr<int> alive_ = std::make_shared<int>(0);  // guards late luna replies
};
