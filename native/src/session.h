#pragma once
// Native Buna session: one playback, owning the clock, seek preview/commit,
// buffering/stall detection and teardown. Platform-free so it is unit-tested
// on the build machine; the TV media service sits behind PlayerPort.
#include <optional>
#include <string>

#include "media.h"

struct PlayerPort {
  virtual ~PlayerPort() = default;
  virtual void play() = 0;
  virtual void pause() = 0;
  virtual void seek(long long ms) = 0;
  virtual void select_track(const std::string &type, int index) = 0;
  virtual void set_subtitles(bool enabled) = 0;
};

enum class Phase { Idle, Loading, Playing, Paused, Buffering, Seeking, Ended, Error };

// What the viewer experienced, for the end-of-session log line. Stall time counts
// only waiting while playback was wanted (not seeks, pauses or the start-up).
struct SessionStats {
  long long first_frame_ms = -1;
  int stalls = 0;
  long long stall_ms = 0;
  long long play_wall_ms = 0;      // wall-clock time spent wanting to play, after the start
  long long watched_media_ms = 0;  // media time that actually advanced
  int seeks = 0, seeks_landed = 0;
  long long seek_error_ms = 0;     // sum of |landing - target|
  bool start_failed = false;
  SessionStats &operator+=(const SessionStats &o) {
    if (first_frame_ms < 0) first_frame_ms = o.first_frame_ms;
    stalls += o.stalls;
    stall_ms += o.stall_ms;
    play_wall_ms += o.play_wall_ms;
    watched_media_ms += o.watched_media_ms;
    seeks += o.seeks;
    seeks_landed += o.seeks_landed;
    seek_error_ms += o.seek_error_ms;
    start_failed = start_failed || o.start_failed;
    return *this;
  }
};
const char *phase_name(Phase phase);

class Session {
 public:
  static constexpr long long kSeekDebounceMs = 400;  // theme::seek_commit_ms (G5)
  static constexpr long long kSeekTimeoutMs = 15000;
  static constexpr long long kStallMs = 3000;
  static constexpr long long kStartupTimeoutMs = 45000;

  void open(PlayerPort *port, long long now);
  void close();
  void on_event(const MediaEvent &event, const MediaInfo &info, long long now);
  // Drive debounce, timeouts and the stall watchdog. Returns true if state changed.
  bool tick(long long now);
  // Bring the counters up to `now` (also done by every event and tick); call before stats().
  void account(long long now);
  const SessionStats &stats() const { return stats_; }

  void toggle();
  void play();
  void pause();
  void nudge(int direction, long long now);
  bool commit_preview(long long now);
  bool cancel_preview();
  void seek_to(long long target_ms, long long now);
  void select_audio(int index);
  void select_text(int index);  // -1 = off

  Phase phase() const;
  bool intent_play() const { return intent_play_; }
  // Clock for display: preview, else pending seek, else interpolated media time.
  long long display_ms(long long now) const;
  long long position_ms(long long now) const;
  long long duration_ms() const { return duration_ms_; }
  std::optional<long long> preview_ms() const { return preview_; }
  std::optional<long long> pending_ms() const { return pending_; }
  bool started() const { return started_; }
  // Position the current seek gesture started from (for the "+30s" label).
  long long seek_origin_ms(long long fallback) const { return (preview_ || pending_) ? seek_origin_ : fallback; }
  long long first_frame_ms() const { return first_frame_ms_; }
  long long seek_started_ms() const { return seek_started_; }
  // When the current buffering or stall began (for a delayed spinner).
  long long stalled_since() const { return buffering_since_; }
  const std::string &error() const { return error_; }
  int subtitle_index() const { return subtitle_index_; }

 private:
  long long clamp_seek(long long ms) const;
  void commit(long long target, long long now);

  PlayerPort *port_ = nullptr;
  long long opened_at_ = 0;
  bool open_ = false;
  bool loaded_ = false;
  bool started_ = false;
  bool intent_play_ = true;
  bool paused_ = false;
  bool buffering_ = false;
  bool stalled_ = false;
  long long buffering_since_ = 0;
  bool ended_ = false;
  std::string error_;

  long long media_ms_ = 0;       // last time reported by the media service
  long long media_at_ = 0;       // when it was reported
  long long last_advance_at_ = 0;
  long long duration_ms_ = 0;
  bool seekable_ = true;
  long long first_frame_ms_ = -1;

  std::optional<long long> preview_;
  std::optional<long long> pending_;
  long long preview_deadline_ = 0;
  long long seek_origin_ = 0;
  long long seek_started_ = 0;
  int burst_ = 0;
  int burst_direction_ = 0;
  int subtitle_index_ = -1;
  SessionStats stats_;
  long long accounted_at_ = 0;
  bool was_stalling_ = false;
};
