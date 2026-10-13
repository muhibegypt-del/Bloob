#pragma once
// UmsPlayer: LG's uMediaServer (luna://com.webos.media) driven over the bus.
// It plays a URI through the TV's own pipeline, the same hardware path the
// web engine uses, and reports source/track/time events.
#include <functional>
#include <string>
#include <vector>

struct MediaTrack {
  int index = 0;
  std::string language;
  std::string codec;
  int channels = 0;
  bool selected = false;
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
              TrackSelected, EndOfStream, Error } kind;
  long long time_ms = 0;
  std::string text;  // error text or track type
  int code = 0;      // error code or track index
};

class UmsPlayer {
 public:
  using Listener = std::function<void(const MediaEvent &)>;

  // `load_type` is uMediaServer's pipeline type ("media" by default).
  bool load(const std::string &uri, const std::string &app_id, const std::string &window_id,
            const std::string &load_type, Listener listener);
  void play();
  void pause();
  void seek(long long position_ms);
  void select_track(const std::string &type, int index);
  void set_subtitles(bool enabled);
  void unload();

  const std::string &media_id() const { return media_id_; }
  const MediaInfo &info() const { return info_; }

 private:
  void command(const char *method, const std::string &extra = "");
  void on_event(const std::string &payload);

  std::string media_id_;
  Listener listener_;
  MediaInfo info_;
  int subscription_ = 0;
  int generation_ = 0;  // bumps on unload so late load replies are discarded
  bool logged_keys_ = false;
};
