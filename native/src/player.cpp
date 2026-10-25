// Player: LG's media pipeline in the hardware video plane under a transparent
// window, driven by the native Buna session. Quiet while watching; any key
// reveals the title, an accurate timeline and a short row of controls (play,
// audio, subtitles, start over, next episode). Track lists come from the file
// itself. If the TV takes the pipeline away (system UI, resource conflict),
// playback reloads where it was.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <map>

#include "common.h"
#include "demux.h"
#include "es_player.h"
#include "log.h"
#include "luna.h"
#include "media.h"
#include "play.h"
#include "stats.h"
#include "inspect.h"
#include "screens.h"
#include "session.h"
#include "speech.h"

namespace {

using namespace theme;

constexpr long long kSaveEveryMs = 15000;
constexpr long long kPrepareNextBeforeEndMs = 150000;  // start finding the next episode in the credits
constexpr long long kUpNextMs = 10000;
constexpr long long kBufferingDelayMs = 500;
// The engine has this long to show a picture before LG's player takes over.
constexpr long long kEngineFirstFrameMs = 20000;
constexpr int kMaxReloads = 3;
constexpr long long kReloadWindowMs = 60000;

std::string language_name(const std::string &code) {
  static const std::pair<const char *, const char *> names[] = {
      {"en", "English"}, {"eng", "English"}, {"fr", "French"}, {"fre", "French"}, {"fra", "French"},
      {"de", "German"}, {"ger", "German"}, {"deu", "German"}, {"es", "Spanish"}, {"spa", "Spanish"},
      {"it", "Italian"}, {"ita", "Italian"}, {"pt", "Portuguese"}, {"por", "Portuguese"}, {"ru", "Russian"},
      {"rus", "Russian"}, {"ja", "Japanese"}, {"jpn", "Japanese"}, {"ko", "Korean"}, {"kor", "Korean"},
      {"zh", "Chinese"}, {"chi", "Chinese"}, {"zho", "Chinese"}, {"ar", "Arabic"}, {"ara", "Arabic"},
      {"hi", "Hindi"}, {"hin", "Hindi"}, {"nl", "Dutch"}, {"dut", "Dutch"}, {"nld", "Dutch"}, {"pl", "Polish"},
      {"pol", "Polish"}, {"tr", "Turkish"}, {"tur", "Turkish"}, {"sv", "Swedish"}, {"swe", "Swedish"},
      {"da", "Danish"}, {"dan", "Danish"}, {"no", "Norwegian"}, {"nor", "Norwegian"}, {"fi", "Finnish"},
      {"fin", "Finnish"}, {"el", "Greek"}, {"gre", "Greek"}, {"he", "Hebrew"}, {"heb", "Hebrew"}, {"ur", "Urdu"},
      {"urd", "Urdu"}, {"bn", "Bengali"}, {"ben", "Bengali"}, {"ta", "Tamil"}, {"tam", "Tamil"}, {"te", "Telugu"},
      {"tel", "Telugu"}, {"th", "Thai"}, {"tha", "Thai"}, {"vi", "Vietnamese"}, {"vie", "Vietnamese"},
      {"id", "Indonesian"}, {"ind", "Indonesian"}, {"ms", "Malay"}, {"may", "Malay"}, {"cs", "Czech"},
      {"cze", "Czech"}, {"hu", "Hungarian"}, {"hun", "Hungarian"}, {"ro", "Romanian"}, {"rum", "Romanian"},
      {"uk", "Ukrainian"}, {"ukr", "Ukrainian"}, {"fa", "Persian"}, {"per", "Persian"}, {"fas", "Persian"},
      {"ell", "Greek"}, {"ces", "Czech"}, {"ron", "Romanian"}, {"msa", "Malay"}, {"nb", "Norwegian"},
      {"nob", "Norwegian"}, {"nn", "Norwegian"}, {"nno", "Norwegian"}, {"hr", "Croatian"}, {"hrv", "Croatian"},
      {"sr", "Serbian"}, {"srp", "Serbian"}, {"bg", "Bulgarian"}, {"bul", "Bulgarian"}, {"sk", "Slovak"},
      {"slk", "Slovak"}, {"slo", "Slovak"}, {"et", "Estonian"}, {"est", "Estonian"}, {"lv", "Latvian"},
      {"lav", "Latvian"}, {"lt", "Lithuanian"}, {"lit", "Lithuanian"}, {"sl", "Slovenian"}, {"slv", "Slovenian"},
      {"is", "Icelandic"}, {"isl", "Icelandic"}, {"ice", "Icelandic"}, {"ca", "Catalan"}, {"cat", "Catalan"},
      {"fil", "Filipino"}, {"tl", "Filipino"}, {"tgl", "Filipino"}};
  for (auto &n : names) if (code == n.first) return n.second;
  if (code.empty() || code == "und") return "Unknown language";
  return upper(code);
}

std::string codec_name(std::string codec) {
  codec = upper(codec);
  if (codec == "EAC3" || codec == "E-AC-3") return "Dolby Digital Plus";
  if (codec == "AC3" || codec == "AC-3") return "Dolby Digital";
  if (codec.find("TRUEHD") != std::string::npos) return "Dolby TrueHD";
  if (codec.rfind("DTS", 0) == 0 || codec.rfind("DCA", 0) == 0) return "DTS";
  if (codec.find("AAC") != std::string::npos || codec.rfind("MP4A", 0) == 0) return "AAC";
  if (codec == "FLAC") return "FLAC";
  if (codec == "OPUS") return "Opus";
  if (codec == "MP3" || codec == "MPEG" || codec == "MPGA") return "MP3";
  if (codec.rfind("PCM", 0) == 0 || codec == "LPCM") return "PCM";
  return codec;
}

std::string channels_name(int channels) {
  return channels >= 8 ? "7.1" : channels == 6 ? "5.1" : channels == 2 ? "Stereo" : channels == 1 ? "Mono"
         : channels ? std::to_string(channels) + " ch" : "";
}

// What a track is for, beyond its language: commentary, forced, SDH.
std::string track_role(const MediaTrack &t, bool audio) {
  std::string title = upper(t.title);
  auto has = [&](const char *w) { return title.find(w) != std::string::npos; };
  if (t.commentary || has("COMMENTARY")) return "Commentary";
  if (audio) return (has("DESCRI") || has("AUDIO DESC")) ? "Audio description" : "";
  if (t.forced || has("FORCED")) return "Forced";
  if (t.hearing_impaired || has("SDH") || has("HEARING")) return "SDH";
  return "";
}

// "English · Dolby Digital Plus 5.1", "English · Commentary · AAC Stereo"
std::string audio_label(const MediaTrack &t) {
  return join({language_name(t.language), track_role(t, true), join({codec_name(t.codec), channels_name(t.channels)}, " ")});
}

std::string subtitle_label(const MediaTrack &t) {
  std::string kind = upper(t.codec);
  if (kind == "TEXT" || kind == "SUBTITLE") kind.clear();  // generic: says nothing
  return join({language_name(t.language), track_role(t, false), kind});
}

// Labels for a whole list: identical ones get "#2", "#3" so every row is distinct.
std::vector<std::string> distinct_labels(const std::vector<MediaTrack> &tracks, bool audio) {
  std::vector<std::string> base, out;
  for (auto &t : tracks) base.push_back(audio ? audio_label(t) : subtitle_label(t));
  for (size_t i = 0; i < base.size(); i++) {
    int same = 1;
    for (size_t j = 0; j < i; j++) same += base[j] == base[i];
    out.push_back(same > 1 ? base[i] + " #" + std::to_string(same) : base[i]);
  }
  return out;
}

// Sound this TV's player can't decode, which brings up "audio codec not supported":
// DTS (LG left it out of its 2020-2022 TVs, 2.8.1) and TrueHD (remuxes' usual
// first track; TV apps can't decode or pass it through, 3.0). The player picks
// the Dolby or AAC track beside it; a file with nothing else goes through the bridge.
bool dts(const MediaTrack &t) {
  std::string c = upper(t.codec);
  return c.find("DTS") != std::string::npos || c.find("DCA") != std::string::npos ||
         c.find("TRUEHD") != std::string::npos || c.find("MLP") != std::string::npos;
}

// A preference such as "English" matches a track whose code names that language.
bool speaks(const MediaTrack &t, const std::string &language) {
  return !language.empty() && language != "Off" && language_name(t.language) == language;
}

// "21:43" in the TV's local time.
std::string wall_clock(long long offset_ms = 0) {
  std::time_t now = std::time(nullptr) + static_cast<std::time_t>(offset_ms / 1000);
  std::tm local{};
  localtime_r(&now, &local);
  char buf[8];
  std::snprintf(buf, sizeof buf, "%02d:%02d", local.tm_hour, local.tm_min);
  return buf;
}

class MediaPort : public PlayerPort {
 public:
  explicit MediaPort(std::unique_ptr<MediaBackend> &media) : media_(media) {}
  void play() override { media_->play(); }
  void pause() override { media_->pause(); }
  void seek(long long ms) override { media_->seek(ms); }
  void select_track(const std::string &type, int index) override { media_->select_track(type, index); }
  void set_subtitles(bool enabled) override { media_->set_subtitles(enabled); }

 private:
  std::unique_ptr<MediaBackend> &media_;
};

enum class Ctl { Toggle, Audio, Subtitles, StartOver, Next, Stats };

class PlayerScreen : public Screen {
 public:
  PlayerScreen(const PlayRequest &req, const std::string &url, const PlaySourceInfo &info, const std::string &load_type)
      : req_(req), url_(url), source_(info.headline), load_type_(load_type), dolby_vision_(info.dolby_vision), info_(info) {
    start_at_ = req.resume_ms;
    choose_backend();
    if (!info.bridge_url.empty()) media_->set_bridge({info.bridge_url, info.bridge_duration_ms});
    // One viewing, one session: reports carry it and a rising sequence so the
    // service can drop late ones (UX-PLAN X1). A reopen keeps the session.
    char id[40];
    std::snprintf(id, sizeof id, "%llx-%04x", static_cast<unsigned long long>(ctx().now), static_cast<unsigned>(std::rand() & 0xffff));
    progress_session_ = id;
    today_ = today_iso();
    open();
  }
  ~PlayerScreen() override {
    close();
    clear_sub_textures();
  }

  // Bloob's engine where the TV has its pipeline (the desktop plays by the
  // clock instead), unless it already failed for this film; else LG's player.
  void choose_backend() {
    std::unique_ptr<EsSink> sink;
    if (!engine_failed_) {
      sink = make_starfish_sink();
      const char *choice = std::getenv("BLOOB_ENGINE");
      if (!sink && !(choice && std::string(choice) == "lg")) sink = make_clock_sink();
    }
    if (sink) {
      media_.reset(new EsPlayer(std::move(sink), [](std::function<void()> f) { ctx().api_tasks.post(std::move(f)); }));
    } else {
      media_.reset(new UmsPlayer());
    }
    logf("player", "engine: %s", media_->engine_name());
  }

  // The engine couldn't play this file: LG's player takes over at the same place.
  // A DTS- or TrueHD-only version goes through the audio bridge there.
  void fall_back(const std::string &why) {
    if (engine_failed_ || !media_->converts_audio()) return;
    logf("player", "engine gave up (%s); LG's player takes over", why.c_str());
    long long at = session_.first_frame_ms() >= 0 ? session_.position_ms(ctx().now) : start_at_;
    close(true);
    engine_failed_ = true;
    start_at_ = at;
    choose_backend();
    std::string claims = info_.headline + " " + info_.release;
    bool needs_bridge = claims.find("DTS") != std::string::npos || claims.find("TrueHD") != std::string::npos;
    if (!needs_bridge || url_.compare(0, 4, "http") != 0) {
      open();
      return;
    }
    std::weak_ptr<int> alive = alive_;
    ctx().api.post("/api/bridge/open", "{\"url\":" + Json::quote(url_) + ",\"language\":" + Json::quote(ctx().language) + "}",
                   [this, alive](const ApiResult &b) {
                     if (!alive.lock()) return;
                     if (b.ok() && b.json["bridge"].as_bool()) {
                       info_.bridge_note = b.json["note"].as_string();
                       media_->set_bridge({b.json["play"].as_string(), b.json["durationMs"].as_int(0)});
                     }
                     open();
                   }, alive_);
  }

  const char *name() const override { return "player"; }
  bool over_video() const override { return true; }

  // ---------------------------------------------------------------- lifecycle

  void open() {
    Ctx &c = ctx();
    panel_ = Panel::None;
    zone_ = Zone::Timeline;
    error_focus_ = 0;
    resume_done_ = false;
    resume_check_ = false;
    tracks_applied_ = false;
    sub_pos_sent_ = sub_pos_want_ = 0;
    up_next_ = false;
    opened_at_ = c.now;
    show_hud();
    session_.open(&port_, c.now);
    if (window_id_.empty() && c.webos.create_exported_window) {
      const char *created = c.webos.create_exported_window(0);
      window_id_ = created ? created : "";
      logf("player", "exported video window: %s", window_id_.empty() ? "(none)" : window_id_.c_str());
    }
    place_video(0, 0);
    opened_ = true;
    // A reopen continues exactly where it was; a first start only past the resume threshold.
    load_start_ = start_at_ >= (reconnecting_ ? 1000 : play::kResumeMinMs) ? start_at_ : 0;
    media_->set_subtitle_style({c.subtitle_size, c.subtitle_background});
    // The engine reads AllDebrid's HTTPS link through the service, which keeps it in memory.
    if (media_->converts_audio() && url_.compare(0, 8, "https://") == 0 && engine_url_.empty()) {
      if (engine_url_asked_) {
        fall_back("no local address for the file");
        return;
      }
      engine_url_asked_ = true;
      std::weak_ptr<int> alive = alive_;
      c.api.post("/api/bridge/source", "{\"url\":" + Json::quote(url_) + "}", [this, alive](const ApiResult &r) {
        if (!alive.lock() || !opened_) return;
        engine_url_ = r.ok() ? r.json["src"].as_string() : "";
        Ctx &c2 = ctx();
        if (engine_url_.empty()) return fall_back("the service gave no local address");
        media_->load(engine_url_, c2.app_id, window_id_, load_type_, [this](const MediaEvent &event) { on_media(event); }, load_start_);
      }, alive_);
      return;
    }
    const std::string &uri = media_->converts_audio() && !engine_url_.empty() ? engine_url_ : url_;
    media_->load(uri, c.app_id, window_id_, load_type_, [this](const MediaEvent &event) { on_media(event); }, load_start_);
  }

  // Release the pipeline. `keep_window` when reopening straight away.
  void close(bool keep_window = false) {
    if (!opened_) return;
    save_progress(keep_window ? nullptr : "stopped");
    Ctx &c = ctx();
    session_.account(c.now);
    total_ += session_.stats();
    media_->unload();
    session_.close();
    opened_ = false;
    if (!keep_window) log_quality();
    if (!keep_window && !window_id_.empty() && c.webos.destroy_exported_window) {
      c.webos.destroy_exported_window(window_id_.c_str());
      window_id_.clear();
    }
  }

  // Where the viewer is, for saving and reopening. Until a reopened pipeline is
  // playing again, or while a seek is unconfirmed, the target is the truth;
  // the media clock still reads zero or the old position.
  long long viewer_position() const {
    if (!session_.started() || !resume_done_ || resume_check_) return start_at_;
    if (auto pending = session_.pending_ms()) return *pending;
    return session_.position_ms(ctx().now);
  }

  // Reopen at the last known position: after the TV dropped the pipeline, or Try again.
  void reload(const char *why) {
    Ctx &c = ctx();
    if (c.now - reload_window_start_ > kReloadWindowMs) {
      reload_window_start_ = c.now;
      reloads_ = 0;
    }
    if (++reloads_ > kMaxReloads) {
      logf("player", "giving up after %d reloads", reloads_ - 1);
      failure_ = "Playback keeps stopping. The link may have expired; try choosing the source again.";
      return;
    }
    long long at = viewer_position();
    logf("player", "reloading at %lld ms (%s)", at, why);
    total_reloads_++;
    start_at_ = at;
    reconnecting_ = true;
    lost_ = false;
    close(true);
    open();
  }

  void exit(bool another = false) {
    player_wants_another_source() = another;
    close();
    ctx().pop();
  }

  void background() override {
    in_background_ = true;
    if (!session_.started()) return;
    if (session_.intent_play()) session_.pause();
    save_progress("paused");
    media_->notify_background();
  }

  void foreground() override {
    if (!in_background_) return;
    in_background_ = false;
    media_->notify_foreground();
    if (lost_) reload("pipeline released while away");
  }

  void on_media(const MediaEvent &event) {
    Ctx &c = ctx();
    long long t = c.now;
    // The media clock (every 200 ms in the reference pipeline) changes nothing
    // on screen by itself: the HUD schedules its own redraws while it shows a
    // time. Every other event can change what the player shows (R5a, F10).
    if (event.kind != MediaEvent::Time) c.invalidate();
    if (event.kind == MediaEvent::Lost) {
      logf("player", "pipeline lost: %s", event.text.c_str());
      lost_ = true;
      if (!in_background_) reload(event.text.c_str());
      return;
    }
    bool had_frame = session_.first_frame_ms() >= 0;
    if (event.kind == MediaEvent::Error && event.code <= -100 && !had_frame && media_->converts_audio()) {
      logf("player", "engine: %s", event.text.c_str());
      fall_back(event.text);
      return;
    }
    auto pending = session_.pending_ms();
    long long seek_started = session_.seek_started_ms();
    session_.on_event(event, media_->info(), t);
    if (!had_frame && session_.first_frame_ms() >= 0) logf("player", "first frame after %lld ms", session_.first_frame_ms());
    if (pending && !session_.pending_ms())
      logf("player", "seek to %lld ms landed at %lld ms after %lld ms", *pending, session_.position_ms(t), t - seek_started);
    // The version promised Dolby Vision: say in the log whether the TV agrees, so
    // a TV run shows if Dolby Vision is lost on the way (TV, 2.2.0: "HDR every
    // time the video is DV").
    if (event.kind == MediaEvent::VideoInfo && !media_->info().hdr.empty() && (dolby_vision_ || source_.find("Dolby Vision") != std::string::npos)) {
      std::string got = upper(media_->info().hdr);
      bool dv = got.find("DOLBY") != std::string::npos || got == "DV";
      logf("qoe", "picture: version claims Dolby Vision (p%d), TV reports %s%s", dolby_vision_, media_->info().hdr.c_str(),
           dv ? "" : " (the TV did not report Dolby Vision)");
    }
    if (event.kind == MediaEvent::VideoInfo || event.kind == MediaEvent::SourceInfo || event.kind == MediaEvent::Loaded)
      place_video(media_->info().width, media_->info().height);
    if ((event.kind == MediaEvent::SourceInfo || event.kind == MediaEvent::TrackSelected) && !tracks_applied_ &&
        (!media_->info().audio.empty() || !media_->info().text.empty()))
      apply_tracks();
    // Continue where the viewer left off. The load asked the pipeline to start
    // there; the first clock report shows whether it did, else seek.
    if (event.kind == MediaEvent::Playing && !resume_done_) {
      resume_done_ = true;
      playing_at_ = t;
      long long d = session_.duration_ms();
      bool wanted = load_start_ > 0 && (reconnecting_ || d <= 0 || load_start_ < d * play::kResumeMaxPercent / 100);
      if (wanted) {
        resume_check_ = true;
        resumed_ = req_.resume_ms > 0;
      }
      reconnecting_ = false;
    }
    if (resume_check_ && event.kind == MediaEvent::Time) check_start(event.time_ms);
    if (event.kind == MediaEvent::Paused) save_progress("paused");
    if (event.kind == MediaEvent::EndOfStream) {
      save_progress("ended");
      if (next_ready()) {
        up_next_ = true;
        up_next_at_ = t;
      }
      show_hud();
    }
  }

  // Did the pipeline honour the start position in the load request? If not, seek there.
  void check_start(long long at) {
    resume_check_ = false;
    if (std::llabs(at - load_start_) <= 10000) {
      logf("player", "started at %lld ms from the load request", at);
      return;
    }
    logf("player", "the load start was not honoured (at %lld ms); seeking to %lld ms", at, load_start_);
    session_.seek_to(load_start_, ctx().now);
  }

  // Tracks for this pipeline: what the viewer chose before a reopen, else, on the
  // first start, the preferred audio and subtitle languages from Settings.
  void apply_tracks() {
    tracks_applied_ = true;
    const MediaInfo &info = media_->info();
    Ctx &c = ctx();
    if (!preferences_applied_) {
      preferences_applied_ = true;
      bool current_fits = false, current_dts = false;
      // Bloob's engine plays every track (it converts what the TV can't decode).
      auto unplayable = [this](const MediaTrack &a) { return !media_->converts_audio() && dts(a); };
      for (auto &a : info.audio) {
        current_fits = current_fits || (a.selected && speaks(a, c.language) && !unplayable(a));
        current_dts = current_dts || (a.selected && unplayable(a));
      }
      // Remuxes often start on a DTS track with a Dolby or AAC one beside it:
      // the viewer's language first, then any main track this TV can play.
      if (chosen_audio_ < 0 && !current_fits)
        for (int pass = 0; pass < 2 && chosen_audio_ < 0; pass++)
          for (size_t i = 0; i < info.audio.size(); i++) {
            const MediaTrack &a = info.audio[i];
            if (unplayable(a) || !track_role(a, true).empty() || (pass == 0 && !speaks(a, c.language))) continue;
            if (pass == 1 && !current_dts) break;  // a playable track is on: only DTS makes a stranger worth it
            chosen_audio_ = static_cast<int>(i);
            break;
          }
      bool any_playable = false;
      for (auto &a : info.audio) any_playable = any_playable || !unplayable(a);
      if (!info.audio.empty() && !any_playable) {
        logf("player", "every audio track is DTS or TrueHD (%zu tracks)", info.audio.size());
        failure_ = "This version\xE2\x80\x99s sound is DTS or TrueHD only, which this TV can\xE2\x80\x99t play. Choose another version: "
                   "Bloob shows the ones with Dolby or AAC sound first.";
        error_focus_ = 1;  // Choose another version
        session_.pause();
      } else if (chosen_audio_ >= 0 && current_dts) {
        logf("player", "skipping a DTS/TrueHD track for %s", audio_label(info.audio[static_cast<size_t>(chosen_audio_)]).c_str());
      }
      if (!chosen_text_ && c.subtitle_language != "Off") {
        int any = -1;
        for (size_t i = 0; i < info.text.size(); i++) {
          if (!speaks(info.text[i], c.subtitle_language)) continue;
          if (any < 0) any = static_cast<int>(i);
          if (track_role(info.text[i], false) != "Forced") {
            any = static_cast<int>(i);
            break;
          }
        }
        if (any >= 0) chosen_text_ = any;
      }
    }
    if (chosen_audio_ >= 0 && chosen_audio_ < static_cast<int>(info.audio.size()) &&
        !info.audio[static_cast<size_t>(chosen_audio_)].selected) {
      logf("player", "audio track %d (%s)", chosen_audio_, audio_label(info.audio[static_cast<size_t>(chosen_audio_)]).c_str());
      session_.select_audio(chosen_audio_);
    }
    if (chosen_text_ && *chosen_text_ < static_cast<int>(info.text.size()) && *chosen_text_ != session_.subtitle_index()) {
      logf("player", "subtitles %d", *chosen_text_);
      session_.select_text(*chosen_text_);
    }
  }

  // One line per viewing, across reopens: what the viewer actually experienced.
  void log_quality() {
    const SessionStats &q = total_;
    if (q.play_wall_ms > 0 || q.first_frame_ms >= 0 || q.start_failed) {
      double ratio = q.play_wall_ms > 0 ? 100.0 * static_cast<double>(q.stall_ms) / static_cast<double>(q.play_wall_ms) : 0;
      logf("qoe", "first frame %lld ms; stalls %d, %lld ms (%.2f%% of %lld s wanting to play); media watched %lld s; "
           "seeks %d, %d landed, mean error %lld ms; reloads %d; start failed %s; dropped frames unknown",
           q.first_frame_ms, q.stalls, q.stall_ms, ratio, q.play_wall_ms / 1000, q.watched_media_ms / 1000, q.seeks,
           q.seeks_landed, q.seek_error_ms / std::max(1, q.seeks_landed), total_reloads_, q.start_failed ? "yes" : "no");
    }
    total_ = SessionStats{};
    total_reloads_ = 0;
  }

  void place_video(int video_w, int video_h) {
    Ctx &c = ctx();
    if (window_id_.empty() || !c.webos.set_exported_window) return;
    int dw = W, dh = H;
    SDL_GetWindowSize(c.window, &dw, &dh);
    SDL_Rect src{0, 0, video_w > 0 ? video_w : dw, video_h > 0 ? video_h : dh};
    SDL_Rect dst{0, 0, dw, dh};
    c.webos.set_exported_window(window_id_.c_str(), &src, &dst);
  }

  // The next episode exists and has aired (an unaired one gets no Up next).
  bool next_ready() const { return play::after(req_, today_).has_value(); }

  // Report where the viewer is. `state`: "paused", "stopped" (leaving), "ended";
  // otherwise from the session. The service decides whether that is finished.
  void save_progress(const char *state = nullptr) {
    if (!session_.started() || req_.video_id == "launch") return;
    bool ended = state && std::string(state) == "ended";
    long long duration = session_.duration_ms();
    long long position = ended ? duration : viewer_position();
    if (duration <= 0 || position <= 0) return;
    std::string now_state = state ? state : session_.phase() == Phase::Paused ? "paused" : "playing";
    std::string body = "{\"videoId\":" + Json::quote(req_.video_id) + ",\"id\":" + Json::quote(req_.item.id) +
                       ",\"type\":" + Json::quote(req_.item.type) + ",\"name\":" + Json::quote(req_.item.name) +
                       ",\"season\":" + std::to_string(req_.season) + ",\"episode\":" + std::to_string(req_.episode) +
                       ",\"episodeTitle\":" + Json::quote(req_.episode_title) + ",\"position\":" + std::to_string(position) +
                       ",\"duration\":" + std::to_string(duration) + ",\"state\":" + Json::quote(now_state) +
                       ",\"session\":" + Json::quote(progress_session_) + ",\"seq\":" + std::to_string(++progress_seq_);
    // The version played, so the show's next episode keeps to it (the service
    // turns the ID into quality words and a torrent hash, never a URL).
    if (!req_.candidate.empty()) body += ",\"edition\":" + Json::quote(req_.candidate);
    if (req_.next) {
      body += ",\"next\":{\"videoId\":" + Json::quote(req_.next->video_id) + ",\"season\":" + std::to_string(req_.next->season) +
              ",\"episode\":" + std::to_string(req_.next->episode) + ",\"title\":" + Json::quote(req_.next->title) +
              ",\"released\":" + Json::quote(req_.next->released) + "}";
    }
    body += "}";
    send_progress(body, 0);
    last_save_ = ctx().now;
  }

  // A report that didn't reach the service is sent again a few times while the
  // player is open; the sequence number keeps a late copy from winning.
  void send_progress(const std::string &body, int attempt) {
    ctx().api.post("/api/progress", body, [this, body, attempt](const ApiResult &r) {
      if (r.ok() || (r.status >= 400 && r.status < 500) || attempt >= 2) return;
      retry_body_ = body;
      retry_attempt_ = attempt + 1;
      retry_at_ = ctx().now + 4000 * (attempt + 1);
    }, alive_);
  }

  void play_next() {
    auto next = play::after(req_, today_);
    if (!next) return;
    // The episode after that, from the title details fetched during the credits.
    if (const Json *meta = ctx().meta_cache.get(meta_key(req_.item), ctx().now))
      next->next = next_episode(parse_meta(*meta), next->video_id);
    close();
    start_playback(*next, PlayOrigin::UpNext, true, true);  // replaces this screen
  }

  // In the credits: rank the next episode's sources and start resolving the
  // recommended one, so Up next starts in seconds. The source screen picks the
  // job up if it recommends the same version.
  void prepare_next() {
    prepared_ = true;
    if (!req_.next || !ctx().paired) return;
    prefetch_meta(req_.item);
    NextEpisode n = *req_.next;
    std::string body = "{\"type\":" + Json::quote(req_.item.type) + ",\"videoId\":" + Json::quote(n.video_id) +
                       ",\"language\":" + Json::quote(ctx().language);
    if (int minutes = runtime_minutes(req_.item.runtime)) body += ",\"runtimeMinutes\":" + std::to_string(minutes);
    body += "}";
    logf("player", "preparing %s during the credits", episode_code(n.season, n.episode).c_str());
    ctx().api.post("/api/sources/ranked", body, [this, n](const ApiResult &r) {
      if (!r.ok()) return;
      Ranking ranking = parse_ranking(r.json);
      if (!ranking.recommended) return;
      std::string headline = ranking.recommended->headline;
      std::string start = "{\"candidate\":" + Json::quote(ranking.recommended->id) + ",\"season\":" + std::to_string(n.season) +
                          ",\"episode\":" + std::to_string(n.episode) + "}";
      ctx().api.post("/api/resolve/start", start, [n, headline](const ApiResult &j) {
        std::string id = j.ok() ? j.json["id"].as_string() : "";
        if (id.empty()) return;
        ctx().prepared_next = Ctx::Prepared{n.video_id, id, headline, ctx().now};
        logf("player", "next episode is being prepared");
      }, alive_);
    }, alive_);
  }

  // ---------------------------------------------------------------- controls

  enum class Zone { Timeline, Controls };
  enum class Panel { None, Audio, Subtitles };
  // Full: title, timeline and controls. Reduced: the timeline alone, for a seek
  // started from a clear picture.
  enum class HudMode { Full, Reduced };

  long long hud_timeout() const {
    return hud_timeout_for();
  }

  std::string spoken() const override {
    Phase phase = session_.phase();
    if (phase == Phase::Error || !failure_.empty()) {
      const char *names[3] = {"Try again", "Choose another version", "Exit"};
      return "Playback stopped. " + std::string(names[error_focus_]) + ", button";
    }
    if (up_next_ && req_.next) return "Up next, " + episode_code(req_.next->season, req_.next->episode) + ", " + req_.next->title + ". Play now, button";
    if (!session_.started()) return reconnecting_ ? "Reconnecting" : "Starting " + req_.item.name;
    if (panel_ != Panel::None) {
      const MediaInfo &info = media_->info();
      auto labels = distinct_labels(panel_ == Panel::Audio ? info.audio : info.text, panel_ == Panel::Audio);
      int rows = panel_rows();
      std::string name = panel_ == Panel::Subtitles && panel_index_ == 0 ? "Off"
                         : panel_ == Panel::Audio ? (panel_index_ < static_cast<int>(labels.size()) ? labels[static_cast<size_t>(panel_index_)] : "")
                         : (panel_index_ - 1 < static_cast<int>(labels.size()) ? labels[static_cast<size_t>(panel_index_ - 1)] : "");
      return std::string(panel_ == Panel::Audio ? "Audio, " : "Subtitles, ") + name + ", " + position(panel_index_, rows);
    }
    if (!hud_) return "";
    if (zone_ == Zone::Controls) {
      auto list = controls();
      int i = std::min(control_, static_cast<int>(list.size()) - 1);
      Ctl ctl = list[static_cast<size_t>(i)];
      std::string name = ctl == Ctl::Toggle ? (session_.intent_play() ? "Pause" : "Play")
                         : ctl == Ctl::Audio ? "Audio, " + control_label(ctl)
                         : ctl == Ctl::Subtitles ? "Subtitles, " + control_label(ctl) : control_label(ctl);
      return name + ", button, " + position(i, static_cast<int>(list.size()));
    }
    // The timeline: a seek target, or the state (not the running clock, which
    // would be read out every second).
    long long t = ctx().now;
    if (session_.preview_ms()) return "Seek to " + clock_text(session_.display_ms(t));
    if (phase == Phase::Paused) return "Paused at " + clock_text(session_.display_ms(t)) + " of " + clock_text(session_.duration_ms());
    return "Playing " + req_.item.name;
  }

  long long hud_timeout_for() const {
    long long base = hud_mode_ == HudMode::Reduced ? hud_reduced_timeout_ms
                     : zone_ == Zone::Controls || session_.phase() == Phase::Paused ? hud_paused_timeout_ms
                                                                                    : hud_timeout_ms;
    // With the screen reader on, every delay doubles [EoA V4].
    return speech().on() ? base * hud_reader_factor : base;
  }

  void show_hud(HudMode mode = HudMode::Full) {
    // A reduced request never downgrades controls already shown in full.
    if (!hud_ || mode == HudMode::Full) hud_mode_ = mode;
    hud_ = true;
    hud_shown_at_ = ctx().now;
  }

  bool hud_pinned() const {
    Phase p = session_.phase();
    // Paused is not pinned: the bar fades after 8 s so a paused frame can be looked at.
    return panel_ != Panel::None || p == Phase::Ended || p == Phase::Loading || p == Phase::Error ||
           session_.preview_ms().has_value() || session_.pending_ms().has_value() || !failure_.empty();
  }

  std::vector<Ctl> controls() const {
    std::vector<Ctl> list = {Ctl::Toggle, Ctl::Audio, Ctl::Subtitles};
    if (resumed_) list.push_back(Ctl::StartOver);
    if (next_ready()) list.push_back(Ctl::Next);
    list.push_back(Ctl::Stats);
    return list;
  }

  std::string control_label(Ctl ctl) const {
    const MediaInfo &info = media_->info();
    switch (ctl) {
      case Ctl::Audio: {
        for (auto &a : info.audio)
          if (a.selected) return join({language_name(a.language), channels_name(a.channels)}, " ");
        return "Audio";
      }
      case Ctl::Subtitles: {
        int i = session_.subtitle_index();
        if (i >= 0 && i < static_cast<int>(info.text.size())) return language_name(info.text[static_cast<size_t>(i)].language);
        return "Subtitles off";
      }
      case Ctl::StartOver: return "Start over";
      case Ctl::Next: return "Next episode";
      case Ctl::Stats: return stats_ ? "Hide info" : "Info";
      case Ctl::Toggle: return "";
    }
    return "";
  }

  void choose_track(int row) {
    if (panel_ == Panel::Audio) {
      chosen_audio_ = row;
      session_.select_audio(row);
    } else {
      chosen_text_ = row - 1;
      session_.select_text(row - 1);
    }
  }

  void run_control(Ctl ctl) {
    long long t = ctx().now;
    switch (ctl) {
      case Ctl::Toggle: session_.toggle(); break;
      case Ctl::Audio: open_panel(Panel::Audio); break;
      case Ctl::Subtitles: open_panel(Panel::Subtitles); break;
      case Ctl::StartOver:
        session_.seek_to(0, t);
        resumed_ = false;
        control_ = 0;
        break;
      case Ctl::Next: play_next(); return;
      case Ctl::Stats: toggle_stats(); break;
    }
    show_hud();
  }

  void open_panel(Panel p) {
    panel_ = p;
    const MediaInfo &info = media_->info();
    if (p == Panel::Audio) {
      panel_index_ = 0;
      for (size_t i = 0; i < info.audio.size(); i++) if (info.audio[i].selected) panel_index_ = static_cast<int>(i);
    } else {
      panel_index_ = session_.subtitle_index() + 1;  // row 0 = Off
    }
  }

  int panel_rows() const {
    const MediaInfo &info = media_->info();
    return panel_ == Panel::Audio ? static_cast<int>(info.audio.size()) : static_cast<int>(info.text.size()) + 1;
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    long long t = ctx().now;
    if (session_.phase() == Phase::Error || !failure_.empty()) {
      if (key == Key::Left) error_focus_ = std::max(0, error_focus_ - 1);
      if (key == Key::Right) error_focus_ = std::min(2, error_focus_ + 1);
      if (key == Key::Ok && !repeat) {
        if (error_focus_ == 0) {
          failure_.clear();
          reloads_ = 0;
          reload("try again");
        } else {
          exit(error_focus_ == 1);
        }
      }
      if (key == Key::Back || key == Key::Stop) exit();
      return true;
    }
    if (up_next_) {
      if (key == Key::Ok && !repeat) play_next();
      else if (key == Key::Back) up_next_ = false;
      return true;
    }
    if (panel_ != Panel::None) {
      int rows = panel_rows();
      if (key == Key::Up) panel_index_ = std::max(0, panel_index_ - 1);
      if (key == Key::Down) panel_index_ = std::min(std::max(0, rows - 1), panel_index_ + 1);
      if (key == Key::Ok && rows > 0 && !repeat) {
        choose_track(panel_index_);
        panel_ = Panel::None;
      }
      if (key == Key::Back || key == Key::Left) panel_ = Panel::None;
      show_hud();
      return true;
    }
    if (key == Key::Info || key == Key::Blue) {
      toggle_stats();
      return true;
    }
    switch (key) {
      case Key::Play: session_.play(); show_hud(); return true;
      case Key::Pause: session_.pause(); show_hud(); return true;
      case Key::PlayPause: session_.toggle(); show_hud(); return true;
      case Key::Forward: session_.nudge(1, t); show_hud(); return true;
      case Key::Rewind: session_.nudge(-1, t); show_hud(); return true;
      case Key::Stop: exit(); return true;
      default: break;
    }
    if (key == Key::Back) {
      if (stats_ && !hud_) { toggle_stats(); return true; }
      if (hud_ && hud_mode_ == HudMode::Reduced && !session_.preview_ms()) { hud_ = false; return true; }
      if (session_.cancel_preview()) { show_hud(hud_mode_); return true; }
      if (hud_ && session_.phase() == Phase::Playing && zone_ == Zone::Timeline) { hud_ = false; return true; }
      if (hud_ && zone_ == Zone::Controls) { zone_ = Zone::Timeline; show_hud(); return true; }
      exit();
      return true;
    }
    auto list = controls();
    int count = static_cast<int>(list.size());
    if (!hud_) {
      // First press only reveals the controls, except the obvious ones.
      if (key == Key::Left || key == Key::Right) {
        session_.nudge(key == Key::Right ? 1 : -1, t);
        zone_ = Zone::Timeline;
        show_hud(HudMode::Reduced);  // just the timeline while seeking from nothing
        return true;
      }
      if (key == Key::Ok && !repeat) session_.toggle();
      zone_ = key == Key::Down || key == Key::Up ? Zone::Controls : Zone::Timeline;
      control_ = 0;
      show_hud();
      return true;
    }
    if (hud_mode_ == HudMode::Reduced) {
      if (key == Key::Left || key == Key::Right) {
        session_.nudge(key == Key::Right ? 1 : -1, t);
        show_hud(HudMode::Reduced);
        return true;
      }
      show_hud(HudMode::Full);  // the viewer wants more than the timeline
      if (key != Key::Down) return true;
    }
    show_hud();
    if (zone_ == Zone::Timeline) {
      if (key == Key::Left || key == Key::Right) session_.nudge(key == Key::Right ? 1 : -1, t);
      if (key == Key::Ok && !repeat) { if (!session_.commit_preview(t)) session_.toggle(); }
      if (key == Key::Down || key == Key::Up) { zone_ = Zone::Controls; control_ = std::min(control_, count - 1); }
      return true;
    }
    if (key == Key::Down) zone_ = Zone::Timeline;  // the controls sit above the timeline in the bar
    if (key == Key::Left) control_ = std::max(0, control_ - 1);
    if (key == Key::Right) control_ = std::min(count - 1, control_ + 1);
    if (key == Key::Ok && !repeat) run_control(list[static_cast<size_t>(std::min(control_, count - 1))]);
    return true;
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    long long t = c.now;
    if (session_.tick(t)) c.animating = true;
    // A reduced timeline hides a moment after the seek lands, not after the last key.
    if (hud_ && hud_mode_ == HudMode::Reduced && (session_.preview_ms() || session_.pending_ms())) hud_shown_at_ = t;
    long long hud_until = hud_shown_at_ + hud_timeout();
    if (hud_ && t > hud_until && !hud_pinned()) hud_ = false;
    if (session_.phase() == Phase::Playing && t - last_save_ > kSaveEveryMs) save_progress();
    if (!retry_body_.empty() && t >= retry_at_) {
      std::string body;
      body.swap(retry_body_);
      send_progress(body, retry_attempt_);
    }
    // No clock report after the start: the load option may not apply here.
    if (resume_check_ && t - playing_at_ > 2000) check_start(session_.position_ms(t));
    // The engine loaded but showed nothing: LG's player takes over.
    if (opened_ && media_->converts_audio() && session_.first_frame_ms() < 0 && t - opened_at_ > kEngineFirstFrameMs)
      fall_back("no picture after " + std::to_string(kEngineFirstFrameMs / 1000) + " s");
    long long duration = session_.duration_ms();
    if (!prepared_ && next_ready() && c.autoplay_next && session_.phase() == Phase::Playing && duration > 0 &&
        duration - session_.position_ms(t) <= kPrepareNextBeforeEndMs)
      prepare_next();
    if (up_next_ && c.autoplay_next && t - up_next_at_ >= kUpNextMs) {
      play_next();
      return;
    }
    if (stats_ && t >= stats_next_) sample_stats(t);
    if (stats_) c.redraw_at(stats_next_);
    bool show = hud_ || hud_pinned();
    hud_anim_.to(show ? 1.0f : 0.0f);
    panel_anim_.to(panel_ != Panel::None ? 1.0f : 0.0f);
    if (hud_anim_.step(dt) | panel_anim_.step(dt)) c.animating = true;
    // Redraw for a visible moving clock (4×/s), the seek preview and countdowns.
    if (session_.preview_ms() || (up_next_ && c.autoplay_next) || session_.phase() == Phase::Buffering || !session_.started())
      c.animating = true;
    else if (show) c.redraw_at((t / 250 + 1) * 250);
    if (hud_ && !hud_pinned()) c.redraw_at(hud_until + 1);
    update_subtitle_position(t, show);
  }

  // Embedded subtitles are drawn by the TV under this window; lift them clear of
  // the controls while those show. Undocumented call: one refusal turns it off.
  void update_subtitle_position(long long t, bool shown) {
    int want = !shown ? 0 : hud_mode_ == HudMode::Reduced ? 2 : 4;
    if (want != sub_pos_want_) {
      sub_pos_want_ = want;
      sub_pos_since_ = t;
    }
    if (!sub_pos_supported_ || session_.subtitle_index() < 0 || sub_pos_want_ == sub_pos_sent_) return;
    if (t - sub_pos_since_ < 300) {
      ctx().redraw_at(sub_pos_since_ + 300);
      return;
    }
    sub_pos_sent_ = sub_pos_want_;
    std::weak_ptr<int> alive = alive_;
    media_->set_subtitle_position(sub_pos_sent_, [this, alive](bool ok) {
      if (alive.expired() || ok || !sub_pos_supported_) return;
      sub_pos_supported_ = false;
      logf("player", "subtitle position not supported here; leaving subtitles where the TV puts them");
    });
  }

  // ---------------------------------------------------------------- subtitles (engine)

  void clear_sub_textures(const std::vector<SubtitleCue> *keep = nullptr) {
    for (auto it = sub_textures_.begin(); it != sub_textures_.end();) {
      bool kept = false;
      if (keep)
        for (const auto &c : *keep) kept = kept || c.picture.get() == it->first;
      if (kept) {
        ++it;
        continue;
      }
      for (SDL_Texture *t : it->second) SDL_DestroyTexture(t);
      it = sub_textures_.erase(it);
    }
  }

  // The engine's subtitles, drawn by Bloob over the picture: text in the size and
  // background from Settings, Blu-ray pictures as the disc placed them. They rise
  // above the bar while the controls show. Redraws only when a cue starts or ends.
  void draw_subtitles() {
    if (!media_ || !media_->draws_subtitles() || session_.subtitle_index() < 0) return;
    Ctx &c = ctx();
    long long pos = session_.position_ms(c.now);
    std::vector<SubtitleCue> cues = media_->cues_at(pos);
    long long next = media_->next_cue_change(pos);
    long long wait = next > pos ? std::min(next - pos, 500LL) : 500;
    if (session_.phase() == Phase::Playing) c.redraw_at(c.now + wait);
    clear_sub_textures(&cues);
    float lift = 150.0f * hud_anim_.value;
    for (const auto &cue : cues) {
      if (!cue.picture) continue;
      const SubPicture &pic = *cue.picture;
      auto &textures = sub_textures_[cue.picture.get()];
      if (textures.empty())
        for (const auto &piece : pic.pieces) {
          SDL_Texture *t = SDL_CreateTexture(c.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, piece.w, piece.h);
          if (t) {
            SDL_UpdateTexture(t, nullptr, piece.rgba.data(), piece.w * 4);
            SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
          }
          textures.push_back(t);
        }
      float sx = static_cast<float>(W) / std::max(1, pic.canvas_w), sy = static_cast<float>(H) / std::max(1, pic.canvas_h);
      for (size_t i = 0; i < pic.pieces.size() && i < textures.size(); i++) {
        if (!textures[i]) continue;
        const auto &piece = pic.pieces[i];
        int y = static_cast<int>(piece.y * sy);
        if (y + piece.h * sy > H * 0.6f) y -= static_cast<int>(lift);  // bottom pictures move up with the bar
        c.gfx.texture(textures[i], static_cast<int>(piece.x * sx), y, static_cast<int>(piece.w * sx), static_cast<int>(piece.h * sy));
      }
    }
    // Text: wrapped to 80% of the width, bottom-centred.
    static const int kSizes[] = {40, 46, 52, 60, 68};
    int size = kSizes[std::max(0, std::min(4, c.subtitle_size))];
    Font &font = c.fonts.medium;
    std::vector<std::string> lines;
    for (const auto &cue : cues) {
      if (cue.picture || cue.text.empty()) continue;
      size_t start = 0;
      while (start <= cue.text.size()) {
        size_t nl = cue.text.find('\n', start);
        std::string para = cue.text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        std::string line;
        size_t w = 0;
        while (w <= para.size()) {
          size_t sp = para.find(' ', w);
          std::string word = para.substr(w, sp == std::string::npos ? std::string::npos : sp - w);
          std::string trial = line.empty() ? word : line + " " + word;
          if (!line.empty() && font.measure(trial, size) > W * 4 / 5) {
            lines.push_back(line);
            line = word;
          } else {
            line = trial;
          }
          if (sp == std::string::npos) break;
          w = sp + 1;
        }
        if (!line.empty()) lines.push_back(line);
        if (nl == std::string::npos) break;
        start = nl + 1;
      }
    }
    if (lines.empty()) return;
    int lh = font.line_height(size);
    int y = H - 90 - static_cast<int>(lift) - lh * static_cast<int>(lines.size());
    for (const auto &line : lines) {
      int w = font.measure(line, size);
      int x = (W - w) / 2;
      if (c.subtitle_background) c.gfx.fill_round(x - 18, y - 4, w + 36, lh + 8, 10, {0, 0, 0, 170});
      else {
        font.draw(line, x + 2, y + 3, size, {0, 0, 0, 200});  // a soft shadow keeps white text readable on bright scenes
      }
      font.draw(line, x, y, size, {255, 255, 255, 255});
      y += lh;
    }
  }

  void draw() override {
    Ctx &c = ctx();
    Phase phase = session_.phase();
    if ((!session_.started() || reconnecting_) && phase != Phase::Error && failure_.empty()) {
      draw_startup();
      return;
    }
    // Transparent: the hardware video plane under the window shows through.
    c.gfx.clear({0, 0, 0, 0});
    if (phase == Phase::Error || !failure_.empty()) return draw_error();
    if (up_next_) return draw_up_next();
    draw_subtitles();

    float a = hud_anim_.value;
    if (phase == Phase::Buffering && session_.started()) {
      long long since = session_.stalled_since();
      if (c.now - since > kBufferingDelayMs) {
        c.gfx.fill_circle(W / 2, H / 2, 58, smoke);
        draw_spinner(W / 2, H / 2, 52, label(Label::Primary));
      } else {
        c.redraw_at(since + kBufferingDelayMs);
      }
    }
    if (stats_) draw_stats();
    if (a <= 0.01f) return;
    draw_hud(a, phase);
    if (panel_anim_.value > 0.01f) draw_panel_tracks(panel_anim_.value);
  }

  // ---------------------------------------------------------------- info panel

  void toggle_stats() {
    stats_ = !stats_;
    if (stats_) {
      net_.reset();
      cpu_total_ = cpu_idle_ = self_ticks_ = -1;
      tv_cpu_ = app_cpu_ = -1;
      sample_stats(ctx().now);
    }
    logf("player", "info panel %s", stats_ ? "shown" : "hidden");
    ctx().invalidate();
  }

  void sample_stats(long long t) {
    stats_next_ = t + 500;
    net_.add(t, stats::read_net_rx(&net_iface_));
    long long total = 0, idle = 0, self = stats::read_self_ticks();
    if (stats::read_cpu_total(&total, &idle)) {
      if (cpu_total_ >= 0 && total > cpu_total_) {
        double dt = static_cast<double>(total - cpu_total_);
        tv_cpu_ = 100.0 * (1.0 - static_cast<double>(idle - cpu_idle_) / dt);
        if (self >= 0 && self_ticks_ >= 0) app_cpu_ = 100.0 * static_cast<double>(self - self_ticks_) / dt;
      }
      cpu_total_ = total;
      cpu_idle_ = idle;
    }
    self_ticks_ = self;
    ctx().invalidate();
  }

  // Everything the TV and Bloob know about this playback, like a desktop
  // player's statistics: the stream, the file's bitrate, the network, the
  // buffer, how playback has gone, and the device.
  void draw_stats() {
    Ctx &c = ctx();
    const MediaInfo &m = media_->info();
    long long t = c.now;
    std::vector<std::pair<std::string, std::string>> rows;
    char buf[160];

    std::string video = m.video_codec.empty() ? "\xE2\x80\x94" : m.video_codec;
    if (m.width > 0) {
      std::snprintf(buf, sizeof buf, " \xC2\xB7 %d\xC3\x97%d", m.width, m.height);
      video += buf;
    }
    if (m.frame_rate > 0) {
      std::snprintf(buf, sizeof buf, " \xC2\xB7 %.3f fps", m.frame_rate);
      video += buf;
    }
    video += " \xC2\xB7 picture: " + (m.hdr.empty() ? std::string("not reported") : m.hdr);
    rows.push_back({"Video", video});
    std::string audio = "\xE2\x80\x94";
    for (size_t i = 0; i < m.audio.size(); i++)
      if (m.audio[i].selected) {
        audio = audio_label(m.audio[i]) + "  (" + std::to_string(i + 1) + " of " + std::to_string(m.audio.size()) + ")";
        if (!m.audio[i].codec.empty()) audio += " \xC2\xB7 " + m.audio[i].codec;
      }
    if (!media_->audio_note().empty()) audio = media_->audio_note();
    else if (!info_.bridge_note.empty()) audio = info_.bridge_note;
    rows.push_back({"Audio", audio});
    rows.push_back({"Engine", media_->engine_name()});
    int sub = session_.subtitle_index();
    rows.push_back({"Subtitles", sub >= 0 && sub < static_cast<int>(m.text.size())
                                     ? subtitle_label(m.text[static_cast<size_t>(sub)]) + "  (" + std::to_string(sub + 1) + " of " +
                                           std::to_string(m.text.size()) + ")"
                                     : "Off (" + std::to_string(m.text.size()) + " available)"});
    long long duration = session_.duration_ms();
    rows.push_back({"Container", join({m.container.empty() ? std::string("\xE2\x80\x94") : m.container,
                                       duration > 0 ? clock_text(duration) : std::string()})});

    // The file's average bitrate: its size over its running time.
    std::string file;
    if (info_.file_bytes > 0) {
      file = stats::bytes_text(info_.file_bytes);
      if (duration > 0)
        file += " \xC2\xB7 average " +
                stats::mbps_text(static_cast<double>(info_.file_bytes) * 8.0 / (static_cast<double>(duration) / 1000.0) / 1e6);
    } else {
      file = "size not known";
    }
    rows.push_back({"File bitrate", file});

    // The TV's own network counters: during playback almost all of it is the stream.
    std::string net = "now " + stats::mbps_text(net_.mbps()) + " \xC2\xB7 average " + stats::mbps_text(net_.average_mbps()) +
                      " \xC2\xB7 peak " + stats::mbps_text(net_.peak_mbps()) + " \xC2\xB7 " + stats::bytes_text(net_.bytes()) +
                      " since opened";
    rows.push_back({"Network", net + (net_iface_.empty() ? "" : "  (" + net_iface_ + ", whole TV)")});

    long long pos = session_.position_ms(t);
    std::string buffer;
    if (m.buffer_remaining_ms >= 0) buffer = std::to_string(m.buffer_remaining_ms / 1000) + " s ahead";
    else if (m.buffer_end_ms > pos) buffer = std::to_string((m.buffer_end_ms - pos) / 1000) + " s ahead";
    if (m.buffer_percent >= 0) buffer = join({buffer, std::to_string(m.buffer_percent) + "%"});
    rows.push_back({"Buffer", buffer.empty() ? "not reported by the TV" : buffer});

    SessionStats q = total_;
    q += session_.stats();
    const char *state = session_.phase() == Phase::Playing ? "Playing" : session_.phase() == Phase::Paused ? "Paused"
                        : session_.phase() == Phase::Buffering ? "Buffering" : session_.phase() == Phase::Seeking ? "Seeking" : "Starting";
    std::snprintf(buf, sizeof buf, "%s \xC2\xB7 stalls %d (%.1f s) \xC2\xB7 first frame %.1f s \xC2\xB7 seeks %d, %d landed \xC2\xB7 reloads %d",
                  state, q.stalls, static_cast<double>(q.stall_ms) / 1000.0,
                  q.first_frame_ms >= 0 ? static_cast<double>(q.first_frame_ms) / 1000.0 : 0.0, q.seeks, q.seeks_landed, total_reloads_);
    rows.push_back({"Playback", clock_text(pos) + " / " + (duration > 0 ? clock_text(duration) : std::string("\xE2\x80\x94")) + " \xC2\xB7 " + buf});

    std::string version = join({info_.headline, info_.source == "REMUX" ? std::string("Remux") : info_.source, info_.group});
    if (!info_.found_by.empty()) version += " \xC2\xB7 found by " + join(info_.found_by, ", ");
    rows.push_back({"Version", version.empty() ? "\xE2\x80\x94" : version});
    if (!info_.release.empty()) rows.push_back({"Release", info_.release});
    if (!info_.file_name.empty() && info_.file_name != info_.release) rows.push_back({"File", info_.file_name});

    ProcMemory mem = read_proc_memory();
    std::string device = "Bloob CPU " + (app_cpu_ >= 0 ? std::to_string(static_cast<int>(app_cpu_ + 0.5)) + "%" : std::string("\xE2\x80\x94")) +
                         " \xC2\xB7 TV CPU " + (tv_cpu_ >= 0 ? std::to_string(static_cast<int>(tv_cpu_ + 0.5)) + "%" : std::string("\xE2\x80\x94"));
    if (mem.rss_kb > 0) device += " \xC2\xB7 memory " + std::to_string(mem.rss_kb / 1024) + " MB";
    rows.push_back({"Device", device});
    if (!c.perf_line.empty()) rows.push_back({"Rendering", c.perf_line});

    int x = theme::safe_x, y = 48, w = 1180, lh = 40, pad = 32;
    int h = pad * 2 + 44 + static_cast<int>(rows.size()) * lh;
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.kind = GlassKind::Smoke;
    g.shadow = false;
    draw_glass(g);
    text(Text::Caption1Semi, "Playback info", x + pad, y + pad - 4, label(Label::Primary));
    text_right(Text::Caption2, "Info or Back to close", x + w - pad, y + pad, label(Label::Tertiary));
    int ry = y + pad + 44;
    for (auto &r : rows) {
      text(Text::Caption2, r.first, x + pad, ry, label(Label::Secondary));
      text(Text::Caption2, r.second, x + pad + 190, ry, label(Label::Primary), w - pad * 2 - 190);
      ry += lh;
    }
  }

  std::vector<std::string> badges() const {
    const MediaInfo &info = media_->info();
    std::vector<std::string> out;
    if (info.height >= 2000) out.push_back("4K");
    else if (info.height >= 1000) out.push_back("HD");
    std::string hdr = upper(info.hdr);
    // uMediaServer may name only a Dolby Vision file's HDR10 base layer while the
    // TV shows Dolby Vision (the owner reports Dolby Vision in MKV works on this
    // TV). For a Dolby Vision version the badge follows the version; the log
    // line "[qoe] picture" keeps what the TV reported.
    bool base_layer = hdr.empty() || hdr.find("HDR10") != std::string::npos;
    if (hdr.find("DOLBY") != std::string::npos || hdr == "DV" || (dolby_vision_ && base_layer)) out.push_back("Dolby Vision");
    // Name the format the TV reports, not a generic "HDR".
    else if (hdr.find("HLG") != std::string::npos) out.push_back("HLG");
    else if (hdr.find("HDR10PLUS") != std::string::npos || hdr.find("HDR10+") != std::string::npos) out.push_back("HDR10+");
    else if (hdr.find("HDR10") != std::string::npos) out.push_back("HDR10");
    else if (!hdr.empty() && hdr != "NONE" && hdr != "SDR") out.push_back("HDR (" + info.hdr + ")");
    for (auto &t : info.audio)
      if (t.selected) {
        std::string codec = upper(t.codec);
        if (codec.find("TRUEHD") != std::string::npos || codec.find("ATMOS") != std::string::npos) out.push_back("Dolby TrueHD");
        else if (codec.find("EAC3") != std::string::npos) out.push_back("Dolby Digital+");
        if (t.channels >= 6) out.push_back(channels_name(t.channels));
      }
    return out;
  }

  // ---------------------------------------------------------------- HUD
  // Elements of A's transport [EoA V14-V22]: no panel, a black gradient behind
  // the controls, a 14 px bar 110 above the bottom edge with time labels below
  // it, the controls row 22 above the bar, and the title above the controls on
  // the left. Video plays in the TV's plane under this window.

  static constexpr int kBarX = safe_x, kBarRight = W - 100, kBarW = kBarRight - safe_x;  // right padding 100
  static constexpr int kTrackH = 14, kTrackBottom = 110;         // bar 14 high, radius 7
  static constexpr int kTimeBaseline = 32;                       // bar to time-label baseline
  static constexpr int kControlsGap = 22;                        // bar to controls
  static constexpr int kControlGap = 25;                         // between controls
  static constexpr int kHeaderGap = 12;                          // header to subheader
  static constexpr int kGradientH = 384;                         // protection gradient [EoA V15]
  const Color kGray92 = rgba(0xebebeb);                          // header colour [EoA V16]

  int track_y() const { return H - kTrackBottom - kTrackH; }
  int controls_y() const { return track_y() - kControlsGap - theme::button_h; }
  // Top of everything the HUD draws, for the track menu and subtitles.
  int bar_y(bool reduced) const { return reduced ? track_y() - 24 : controls_y(); }

  void draw_hud(float a, Phase phase) {
    bool reduced = hud_mode_ == HudMode::Reduced;
    // Black 0 → 60% behind the transport [EoA V15].
    ctx().gfx.gradient_v(0, H - kGradientH, W, kGradientH, rgba(0x000000), 0, static_cast<Uint8>(153 * a));
    if (!reduced) {
      draw_clock_pill(a, phase);
      draw_controls(a, phase, controls_y());
      draw_hud_title(a, phase, controls_y() + theme::button_h);
    }
    draw_timeline(a, phase, track_y());
  }

  // Time now and when this ends, in a small smoked pill top right.
  void draw_clock_pill(float a, Phase phase) {
    long long duration = session_.duration_ms(), left = duration - session_.display_ms(ctx().now);
    std::string now = wall_clock();
    std::string ends = duration > 0 && left > 0 && phase != Phase::Ended ? "Ends " + wall_clock(left) : "";
    int w = measure(Text::Caption1, now) + 52 + (ends.empty() ? 0 : measure(Text::Caption1, ends) + 18);
    int x = W - safe_x - w, y = 48, h = 60;
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.radius = h / 2;
    g.kind = GlassKind::Smoke;
    g.alpha = a;
    g.shadow = false;
    draw_glass(g);
    int ty = y + (h - line_for(Text::Caption1)) / 2;
    int tx = x + 26 + text(Text::Caption1, now, x + 26, ty, label(Label::Primary, a)) + 18;
    if (!ends.empty()) text(Text::Caption1, ends, tx, ty, label(Label::Secondary, a));
  }

  void draw_hud_title(float a, Phase phase, int bottom) {
    std::string title = req_.item.series() ? req_.episode_title.empty() ? req_.item.name : req_.episode_title : req_.item.name;
    std::vector<std::string> meta;
    if (phase == Phase::Paused) meta.push_back("Paused");
    else if (phase == Phase::Seeking) meta.push_back("Seeking\xE2\x80\xA6");
    else if (phase == Phase::Ended) meta.push_back("Finished");
    if (req_.item.series()) {
      meta.push_back(req_.item.name);
      meta.push_back(episode_code(req_.season, req_.episode));
    } else {
      meta.push_back(req_.item.year);
      if (!req_.item.genres.empty()) meta.push_back(req_.item.genres[0]);
    }
    for (auto &b : badges()) meta.push_back(b);
    // Header Emphasized.Title2 and subheader Standard.Caption1, both Gray92 [EoA V16].
    int x = kBarX, w = right_controls_x(0) - x - 40;
    int sub_y = bottom - line_for(Text::Caption1), head_y = sub_y - kHeaderGap - line_for(Text::Title2);
    text(Text::Title2, title, x, head_y, fade(kGray92, a), w);
    text(Text::Caption1, join(meta), x, sub_y, fade(kGray92, a), w);
  }

  GlassButton control_button(Ctl ctl, bool focused, Phase phase, float a) const {
    // Transport controls: DarkPlayer buttons, 66 high, Emphasized.Body labels [EoA V13].
    GlassButton b;
    b.variant = ButtonStyle::Player;
    b.style = Text::BodyBold;
    b.focused = focused;
    b.lift = 1.0f;
    b.alpha = a;
    switch (ctl) {
      case Ctl::Toggle: b.icon = session_.intent_play() && phase != Phase::Ended ? kPauseIcon : kPlayIcon; break;
      case Ctl::Audio: b.icon = static_cast<int>(Icon::Audio); b.label = control_label(ctl); break;
      case Ctl::Subtitles: b.icon = static_cast<int>(Icon::Subtitles); b.label = control_label(ctl); break;
      case Ctl::StartOver: b.icon = static_cast<int>(Icon::Restart); b.label = control_label(ctl); break;
      case Ctl::Next: b.icon = static_cast<int>(Icon::Next); b.label = control_label(ctl); break;
      case Ctl::Stats: b.icon = static_cast<int>(Icon::Info); break;
    }
    return b;
  }

  // Where the controls group starts (right-aligned in the bar's top row).
  int right_controls_x(int) const {
    Phase phase = session_.phase();
    int total = 0;
    auto ctl = controls();
    for (size_t i = 0; i < ctl.size(); i++) total += glass_button_width(control_button(ctl[i], false, phase, 1.0f)) + (i ? kControlGap : 0);
    return kBarRight - total;
  }

  void draw_controls(float a, Phase phase, int y) {
    auto ctl = controls();
    int x = right_controls_x(y);
    for (size_t i = 0; i < ctl.size(); i++) {
      bool f = zone_ == Zone::Controls && static_cast<int>(i) == control_ && panel_ == Panel::None;
      x += draw_glass_button(x, y, control_button(ctl[i], f, phase, a)) + kControlGap;
    }
  }

  void draw_timeline(float a, Phase phase, int bar_top) {
    (void)phase;
    Ctx &c = ctx();
    long long t = c.now;
    long long display = session_.display_ms(t), duration = session_.duration_ms(), position = session_.position_ms(t);
    std::string now_text = clock_text(display);
    std::string left_text = duration > 0 ? "\xE2\x88\x92" + clock_text(std::max(0LL, duration - display)) : "--:--";
    bool timeline = zone_ == Zone::Timeline && panel_ == Panel::None;
    // Time labels: Emphasized.Caption2, white when the bar has focus, else 50% [EoA V18].
    Color time_fg = fade(rgba(0xffffff, timeline ? 255 : 128), a);
    int baseline = bar_top + kTrackH + kTimeBaseline;
    int label_y = baseline - static_cast<int>(font_for(Text::Caption2Bold).ascent(size_for(Text::Caption2Bold)));
    text(Text::Caption2Bold, now_text, kBarX, label_y, time_fg);
    text_right(Text::Caption2Bold, left_text, kBarRight, label_y, time_fg);
    // Track 14 high, radius 7. Colours A / C from the reference; which one is
    // played and which remaining is not recorded, so the brighter one is played.
    int tx = kBarX, tw = kBarW, th = kTrackH, ty = bar_top + th / 2;
    c.gfx.fill_round(tx, bar_top, tw, th, th / 2, fade(rgba(0xececec, 66), a));
    double frac = duration > 0 ? std::min(1.0, std::max(0.0, static_cast<double>(display) / duration)) : 0;
    c.gfx.fill_round(tx, bar_top, std::max(th, static_cast<int>(tw * frac)), th, th / 2, fade(rgba(0xf4f4f4, 143), a));
    int kx = tx + static_cast<int>(tw * frac);
    bool seeking = session_.preview_ms() || session_.pending_ms();
    if (timeline || seeking) {
      int r = timeline ? 13 : 10;  // playhead 26 large, 20 [EoA V17]
      c.gfx.shadow(kx - r, ty - r, 2 * r, 2 * r, r, 10, 2, static_cast<Uint8>(90 * a));
      c.gfx.fill_circle(kx, ty, r, fade(rgba(0xffffff), a));
    }
    if (seeking) {
      // The target, in a white bubble above the knob.
      long long delta = display - session_.seek_origin_ms(position);
      long long s = (delta < 0 ? -delta : delta) / 1000;
      std::string d = std::string(delta < 0 ? "\xE2\x88\x92" : "+") + (s < 60 ? std::to_string(s) + " s" : clock_text(s * 1000));
      std::string target = clock_text(display);
      int lw = std::max(measure(Text::Headline, target), measure(Text::Caption2, d)) + 48;
      int lh = 88;
      int lx = std::max(kBarX, std::min(kBarRight - lw, kx - lw / 2));
      int ly = ty - 30 - lh;
      c.gfx.shadow(lx, ly, lw, lh, 22, 22, 10, static_cast<Uint8>(150 * a));
      c.gfx.fill_round(lx, ly, lw, lh, 22, fade(rgba(0xffffff), a));
      text_center(Text::Headline, target, lx + lw / 2, ly + 6, fade(rgba(0x000000), a));
      text_center(Text::Caption2, d, lx + lw / 2, ly + 52, fade(rgba(0x000000, 166), a));
    }
  }

  // Audio and subtitle choices: a smoked panel standing on the bar, right side.
  // Audio and subtitle choices: the shared floating menu [EoA V19-V22]. 454 wide,
  // radius 54, RGB(30,30,30) at 90%, rows 66 high 4 apart, Gray92 when focused,
  // 20 above the controls.
  static constexpr int kMenuW = 454, kMenuRadius = 54, kMenuPad = 21, kMenuHeader = 59, kRowH = 66, kRowGap = 4, kMenuMaxH = 600;

  void draw_panel_tracks(float a) {
    Ctx &c = ctx();
    const MediaInfo &info = media_->info();
    int rows = panel_rows();
    bool empty = (panel_ == Panel::Audio && rows == 0) || (panel_ == Panel::Subtitles && info.text.empty());
    int list_h = std::max(1, rows) * (kRowH + kRowGap) - kRowGap + (empty ? 70 : 0);
    int h = std::min(kMenuMaxH, kMenuPad + kMenuHeader + list_h + kMenuPad);
    int w = kMenuW, x = kBarRight - w;
    int bottom = controls_y() - 20 + static_cast<int>((1.0f - a) * 30);
    int y = bottom - h;
    if (!c.lite()) c.gfx.shadow(x, y, w, h, kMenuRadius, 30, 10, static_cast<Uint8>(77 * a));
    Color bg = c.reduce_transparency ? rgba(0x262626) : rgba(0x1e1e1e, 230);
    c.gfx.fill_round(x, y, w, h, kMenuRadius, fade(bg, a));
    c.gfx.stroke_round(x, y, w, h, kMenuRadius, 1, fade(c.high_contrast ? theme::hairline_high : theme::hairline, a));
    // Header: Standard.Caption2 at 50% [EoA V20].
    text(Text::Caption2, panel_ == Panel::Audio ? "Audio" : "Subtitles", x + kMenuPad + 25, y + kMenuPad + 22, fade(rgba(0xffffff, 128), a));
    int ry = y + kMenuPad + kMenuHeader;
    if (panel_ == Panel::Audio && rows == 0) paragraph(Text::Caption1, "This video reports no audio tracks.", x + kMenuPad + 25, ry, label(Label::Secondary, a), w - 2 * kMenuPad - 50, 2);
    auto labels = distinct_labels(panel_ == Panel::Audio ? info.audio : info.text, panel_ == Panel::Audio);
    int visible = std::max(1, (h - kMenuPad * 2 - kMenuHeader + kRowGap) / (kRowH + kRowGap));
    int first = std::max(0, std::min(panel_index_ - visible / 2, rows - visible));
    int rx = x + kMenuPad, rw = w - 2 * kMenuPad;
    for (int i = first; i < std::min(rows, first + visible); i++) {
      bool f = i == panel_index_;
      std::string name;
      bool selected = false;
      if (panel_ == Panel::Audio) {
        name = labels[static_cast<size_t>(i)];
        selected = info.audio[static_cast<size_t>(i)].selected;
      } else if (i == 0) {
        name = "Off";
        selected = session_.subtitle_index() < 0;
      } else {
        name = labels[static_cast<size_t>(i - 1)];
        selected = session_.subtitle_index() == i - 1;
      }
      if (f) {
        if (!c.lite()) c.gfx.shadow(rx, ry, rw, kRowH, kRowH / 2, theme::shadow_blur, theme::shadow_dy, static_cast<Uint8>(theme::shadow_alpha * a));
        c.gfx.fill_round(rx, ry, rw, kRowH, kRowH / 2, fade(kGray92, a));
      }
      Color fg = f ? fade(rgba(0x000000), a) : label(Label::Primary, a);
      // Leading box 66 holds the check (20 high); title Standard.Caption1 [EoA V21].
      if (selected) c.icons.draw(Icon::Check, rx + (66 - 20) / 2, ry + (kRowH - 20) / 2, 20, fg);
      text(Text::Caption1, name, rx + 66, text_y_centered(Text::Caption1, ry + kRowH / 2), fg, rw - 66 - 20);
      ry += kRowH + kRowGap;
    }
    if (panel_ == Panel::Subtitles && info.text.empty())
      paragraph(Text::Caption1, "This video has no built-in subtitles.", x + kMenuPad + 25, ry + 8, label(Label::Tertiary, a), w - 2 * kMenuPad - 50, 2);
  }

  void draw_startup() {
    Ctx &c = ctx();
    c.gfx.clear(black);
    ImageRef soft = c.images.get(req_.item.backdrop, W, H, 0, Fit::Cover, false, 48);
    if (soft.texture) {
      c.gfx.texture(soft.texture, 0, 0, W, H, soft.alpha);
      c.gfx.fill_rect(0, 0, W, H, with_alpha(black, 150));
    }
    int y = draw_title(req_.item, W / 2 - 380, 300, 760, 200, 1.0f, true) + 28;
    if (req_.item.series()) {
      text_center(Text::Title3, episode_code(req_.season, req_.episode), W / 2, y, label(Label::Primary));
      y += line_for(Text::Title3);
      if (!req_.episode_title.empty()) text_center(Text::Callout, req_.episode_title, W / 2, y + 4, label(Label::Secondary), 900);
      y += line_for(Text::Callout) + 8;
    }
    draw_spinner(W / 2, y + 70, 48, label(Label::Primary), opened_at_);
    std::string what = reconnecting_ ? "Reconnecting\xE2\x80\xA6" : join({"Starting", source_});
    if (c.now - opened_at_ > spinner_delay_ms) text_center(Text::Callout, what, W / 2, y + 120, label(Label::Tertiary));
  }

  void draw_error() {
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(black, 200));
    int w = 1000, h = 360, x = (W - w) / 2, y = (H - h) / 2;
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.kind = GlassKind::Smoke;
    draw_glass(g);
    c.icons.draw(Icon::Warning, x + 56, y + 58, 40, caution);
    text(Text::Title3, "Playback stopped", x + 112, y + 52, label(Label::Primary));
    paragraph(Text::Body, failure_.empty() ? session_.error() : failure_, x + 56, y + 130, label(Label::Secondary), w - 112, 3);
    const char *names[3] = {"Try again", "Choose another version", "Exit"};
    int bx = x + 56;
    for (int i = 0; i < 3; i++) {
      GlassButton b;
      b.label = names[i];
      b.focused = error_focus_ == i;
      b.variant = ButtonStyle::Player;
      bx += draw_glass_button(bx, y + h - 120, b) + theme::title_button_gap;
    }
  }

  // Up next: the post-play lockup [EoA V23], 454 × 460, bottom right over the
  // credits. Title Emphasized.Title2, body Standard.Body, buttons 66.
  void draw_up_next() {
    Ctx &c = ctx();
    const NextEpisode &n = *req_.next;
    c.gfx.gradient_v(0, H - 520, W, 520, rgba(0x000000), 0, 128);  // autoplay background Black50 [EoA V24]
    long long elapsed = c.now - up_next_at_;
    long long left = std::max(0LL, (kUpNextMs - elapsed + 999) / 1000);
    int w = 454, h = 460, x = kBarRight - w, y = H - kTrackBottom - h;
    if (!c.lite()) c.gfx.shadow(x, y, w, h, glass_radius, 30, 10, 77);
    c.gfx.fill_round(x, y, w, h, glass_radius, c.reduce_transparency ? rgba(0x262626) : rgba(0x1e1e1e, 230));
    int px = x + 40, pw = w - 80, py = y + 40;
    text(Text::Caption1, "Up next \xC2\xB7 " + episode_code(n.season, n.episode), px, py, label(Label::Secondary));
    py += line_for(Text::Caption1) + 18;
    py = paragraph(Text::Title2, n.title.empty() ? episode_code(n.season, n.episode) : n.title, px, py, label(Label::Primary), pw, 3);
    GlassButton play;
    play.label = "Play now";
    play.icon = kPlayIcon;
    play.focused = true;
    int by = y + h - 40 - theme::button_h - (c.autoplay_next ? 44 : 0);
    draw_glass_button(px, by, play);
    std::string note = c.autoplay_next ? "Starts in " + std::to_string(left) + " s \xC2\xB7 Back to stay" : "Back to stay";
    text(Text::Caption1, note, px, by + theme::button_h + 14, label(Label::Secondary), pw);
    if (c.autoplay_next) draw_progress(x + 40, y + h - 24, w - 80, 4, static_cast<double>(elapsed) / kUpNextMs, rgba(0xffffff), rgba(0xffffff, 50));
  }

 private:
  PlayRequest req_;
  std::string url_, source_, load_type_;
  int dolby_vision_ = 0;  // the version's guessed Dolby Vision profile (5 or 8), 0 for none
  PlaySourceInfo info_;
  // The info panel (Info, I or the blue key): sampled twice a second while open.
  bool stats_ = false;
  long long stats_next_ = 0;
  stats::RateMeter net_;
  std::string net_iface_;
  long long cpu_total_ = -1, cpu_idle_ = -1, self_ticks_ = -1;
  double tv_cpu_ = -1, app_cpu_ = -1;
  std::unique_ptr<MediaBackend> media_;
  MediaPort port_{media_};
  // 3.0: Bloob's engine plays first; LG's player takes over if it can't.
  bool engine_failed_ = false;
  std::string engine_url_;  // the file through the service (FFmpeg reads plain HTTP)
  bool engine_url_asked_ = false;
  // Picture subtitles uploaded as textures, by the picture they came from.
  std::map<const SubPicture *, std::vector<SDL_Texture *>> sub_textures_;
  Session session_;
  std::string window_id_;
  bool opened_ = false;
  long long opened_at_ = 0;
  long long start_at_ = 0;
  bool reconnecting_ = false;
  bool lost_ = false;
  bool in_background_ = false;
  int reloads_ = 0;
  long long reload_window_start_ = 0;
  std::string failure_;
  bool hud_ = true;
  HudMode hud_mode_ = HudMode::Full;
  long long hud_shown_at_ = 0;
  Zone zone_ = Zone::Timeline;
  int control_ = 0;
  Panel panel_ = Panel::None;
  int panel_index_ = 0;
  int error_focus_ = 0;
  long long last_save_ = 0;
  std::string progress_session_, today_;
  long long progress_seq_ = 0;
  std::string retry_body_;
  int retry_attempt_ = 0;
  long long retry_at_ = 0;
  bool resume_done_ = false, resumed_ = false;
  long long load_start_ = 0, playing_at_ = 0;
  bool resume_check_ = false;  // waiting for the first clock report after a start position
  // Tracks the viewer chose, kept across reopens; preferences apply once.
  int chosen_audio_ = -1;
  std::optional<int> chosen_text_;  // -1 = off
  bool tracks_applied_ = false, preferences_applied_ = false;
  int sub_pos_sent_ = 0, sub_pos_want_ = 0;
  long long sub_pos_since_ = 0;
  bool sub_pos_supported_ = true;
  bool prepared_ = false;
  SessionStats total_;
  int total_reloads_ = 0;
  bool up_next_ = false;
  long long up_next_at_ = 0;
  Anim hud_anim_{overlay_rate, 1}, panel_anim_{overlay_rate};
};

}  // namespace

std::unique_ptr<Screen> make_player(const PlayRequest &request, const std::string &url, const PlaySourceInfo &source,
                                    const std::string &load_type) {
  return std::make_unique<PlayerScreen>(request, url, source, load_type);
}
