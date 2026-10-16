#pragma once
// UmsPlayer: LG's uMediaServer (luna://com.webos.media) driven over the bus.
// It plays a URI through the TV's own pipeline, the same hardware path the
// web engine uses, and reports source/track/time events.
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

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

class UmsPlayer {
 public:
  using Listener = std::function<void(const MediaEvent &)>;

  // `load_type` is uMediaServer's pipeline type ("media" by default).
  // `start_ms` asks the pipeline to begin there (LG's documented
  // transmission.playTime.start option; unverified on this TV). If the TV
  // refuses a load carrying it, the load is retried once without it.
  bool load(const std::string &uri, const std::string &app_id, const std::string &window_id,
            const std::string &load_type, Listener listener, long long start_ms = 0);
  void play();
  void pause();
  void seek(long long position_ms);
  void select_track(const std::string &type, int index);
  void set_subtitles(bool enabled);
  // Subtitle look, re-sent after every enable: TVs drop it when subtitles are
  // switched on again. Community-documented uMediaServer calls (also used by
  // Stremio's webOS player): setSubtitleFontSize 0 (smallest) .. 4, the TV's
  // default being 2; character and background colours by name, opacity 0..255.
  struct SubtitleStyle {
    int size = 0;
    bool background = false;
  };
  void set_subtitle_style(const SubtitleStyle &style) { style_ = style; }
  // Undocumented uMediaServer call (community-documented): -3 lowest .. 4 highest, 0 default.
  // `done(false)` when the TV refuses it.
  void set_subtitle_position(int position, std::function<void(bool ok)> done);
  void unload();
  // uMediaServer's resource policy follows the app's foreground state.
  void notify_foreground();
  void notify_background();

  const std::string &media_id() const { return media_id_; }
  const MediaInfo &info() const { return info_; }

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
  std::set<std::string> seen_keys_;
  bool lost_ = false;
  SubtitleStyle style_;
  void apply_subtitle_style();
  std::shared_ptr<int> alive_ = std::make_shared<int>(0);  // guards late luna replies
};
