#include "session.h"

#include <algorithm>
#include <cstdlib>

const char *phase_name(Phase phase) {
  switch (phase) {
    case Phase::Idle: return "idle";
    case Phase::Loading: return "loading";
    case Phase::Playing: return "playing";
    case Phase::Paused: return "paused";
    case Phase::Buffering: return "buffering";
    case Phase::Seeking: return "seeking";
    case Phase::Ended: return "ended";
    case Phase::Error: return "error";
  }
  return "?";
}

static long long nudge_step_ms(int burst) {
  return burst >= 16 ? 120000 : burst >= 8 ? 60000 : burst >= 4 ? 30000 : 10000;
}

void Session::open(PlayerPort *port, long long now) {
  *this = Session{};
  port_ = port;
  open_ = true;
  opened_at_ = now;
  last_advance_at_ = now;
}

void Session::close() {
  open_ = false;
  port_ = nullptr;
}

Phase Session::phase() const {
  if (!open_) return Phase::Idle;
  if (!error_.empty()) return Phase::Error;
  if (ended_) return Phase::Ended;
  if (pending_) return Phase::Seeking;
  if (!started_) return (!intent_play_ && loaded_) ? Phase::Paused : Phase::Loading;
  if (!intent_play_) return Phase::Paused;
  if (buffering_ || stalled_) return Phase::Buffering;
  return Phase::Playing;
}

long long Session::position_ms(long long now) const {
  // Interpolate between media-service time reports so the clock moves smoothly,
  // but never run more than 2 s ahead of the last report.
  if (phase() == Phase::Playing) return media_ms_ + std::min(now - media_at_, 2000LL);
  return media_ms_;
}

long long Session::display_ms(long long now) const {
  if (preview_) return *preview_;
  if (pending_) return *pending_;
  long long pos = position_ms(now);
  return duration_ms_ > 0 ? std::min(pos, duration_ms_) : pos;
}

void Session::on_event(const MediaEvent &event, const MediaInfo &info, long long now) {
  if (!open_) return;
  switch (event.kind) {
    case MediaEvent::Loaded:
      if (!loaded_) {
        loaded_ = true;
        if (intent_play_ && port_) port_->play();
      }
      break;
    case MediaEvent::SourceInfo:
      duration_ms_ = info.duration_ms;
      seekable_ = info.seekable;
      break;
    case MediaEvent::Playing:
      started_ = true;
      paused_ = false;
      intent_play_ = true;
      buffering_ = false;
      last_advance_at_ = now;
      if (first_frame_ms_ < 0) first_frame_ms_ = now - opened_at_;
      break;
    case MediaEvent::Paused:
      paused_ = true;
      intent_play_ = false;
      break;
    case MediaEvent::BufferingStart:
      if (!buffering_) buffering_since_ = now;
      buffering_ = true;
      break;
    case MediaEvent::BufferingEnd: buffering_ = false; break;
    case MediaEvent::Time: {
      if (event.time_ms > media_ms_ + 20 || event.time_ms < media_ms_) last_advance_at_ = now;
      if (pending_) {
        // A fresh report near the target, after the seek settled, confirms it.
        if (now - seek_started_ > 300 && std::llabs(event.time_ms - *pending_) < 3000) pending_.reset();
        else break;
      }
      media_ms_ = event.time_ms;
      media_at_ = now;
      if (stalled_ && now - last_advance_at_ < kStallMs) stalled_ = false;
      if (!started_ && event.time_ms > 0) {
        started_ = true;
        if (first_frame_ms_ < 0) first_frame_ms_ = now - opened_at_;
      }
      break;
    }
    case MediaEvent::SeekDone:
      if (pending_) {
        media_ms_ = *pending_;
        media_at_ = now;
        pending_.reset();
        last_advance_at_ = now;
      }
      break;
    case MediaEvent::EndOfStream:
      ended_ = true;
      intent_play_ = false;
      break;
    case MediaEvent::Error:
      error_ = event.text.empty() ? "Playback failed." : event.text;
      break;
    case MediaEvent::VideoInfo:
    case MediaEvent::TrackSelected:
    case MediaEvent::Lost:  // the player reloads; the session is reopened
      break;
  }
}

bool Session::tick(long long now) {
  if (!open_) return false;
  bool changed = false;
  if (preview_ && now >= preview_deadline_) {
    commit(*preview_, now);
    changed = true;
  }
  if (pending_ && now - seek_started_ > kSeekTimeoutMs) {
    pending_.reset();
    changed = true;
  }
  if (!started_ && error_.empty() && now - opened_at_ > kStartupTimeoutMs) {
    error_ = "The video did not start. The link may have expired or the server is slow.";
    changed = true;
  }
  Phase p = phase();
  if ((p == Phase::Playing) && !stalled_ && now - last_advance_at_ > kStallMs) {
    stalled_ = true;
    buffering_since_ = now;
    changed = true;
  }
  return changed;
}

void Session::play() {
  if (!open_ || !error_.empty()) return;
  intent_play_ = true;
  ended_ = false;
  if (port_) port_->play();
}

void Session::pause() {
  if (!open_ || !error_.empty()) return;
  intent_play_ = false;
  if (port_) port_->pause();
}

void Session::toggle() {
  if (intent_play_ && !ended_) pause();
  else play();
}

long long Session::clamp_seek(long long ms) const {
  long long upper = duration_ms_ > 1000 ? duration_ms_ - 1000 : ms;
  return std::max(0LL, std::min(ms, upper));
}

void Session::nudge(int direction, long long now) {
  if (!open_ || !error_.empty() || !seekable_ || !started_) return;
  burst_ = (direction == burst_direction_ && preview_) ? burst_ + 1 : 0;
  burst_direction_ = direction;
  long long base = preview_ ? *preview_ : pending_ ? *pending_ : position_ms(now);
  if (!preview_ && !pending_) seek_origin_ = base;
  preview_ = clamp_seek(base + direction * nudge_step_ms(burst_));
  preview_deadline_ = now + kSeekDebounceMs;
}

bool Session::commit_preview(long long now) {
  if (!preview_) return false;
  commit(*preview_, now);
  return true;
}

bool Session::cancel_preview() {
  if (!preview_) return false;
  preview_.reset();
  burst_ = 0;
  return true;
}

void Session::seek_to(long long target_ms, long long now) {
  if (!open_ || !error_.empty() || !seekable_) return;
  if (!preview_ && !pending_) seek_origin_ = position_ms(now);
  commit(clamp_seek(target_ms), now);
}

void Session::commit(long long target, long long now) {
  preview_.reset();
  burst_ = 0;
  ended_ = false;
  pending_ = target;
  seek_started_ = now;
  if (port_) port_->seek(target);
}

void Session::select_audio(int index) {
  if (port_) port_->select_track("audio", index);
}

void Session::select_text(int index) {
  subtitle_index_ = index;
  if (!port_) return;
  if (index < 0) {
    port_->set_subtitles(false);
  } else {
    port_->select_track("text", index);
    port_->set_subtitles(true);
  }
}
