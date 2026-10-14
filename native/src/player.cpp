// Player: LG's media pipeline in the hardware video plane under a transparent
// window, driven by the native Buna session. Quiet while watching; any key
// reveals the title, an accurate timeline and a short row of controls (play,
// audio, subtitles, start over, next episode). Track lists come from the file
// itself. If the TV takes the pipeline away (system UI, resource conflict),
// playback reloads where it was.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

#include "common.h"
#include "log.h"
#include "luna.h"
#include "media.h"
#include "play.h"
#include "screens.h"
#include "session.h"

namespace {

using namespace theme;

constexpr long long kSaveEveryMs = 15000;
constexpr long long kPrepareNextBeforeEndMs = 150000;  // start finding the next episode in the credits
constexpr long long kUpNextMs = 10000;
constexpr long long kBufferingDelayMs = 500;
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

class UmsPort : public PlayerPort {
 public:
  explicit UmsPort(UmsPlayer &player) : player_(player) {}
  void play() override { player_.play(); }
  void pause() override { player_.pause(); }
  void seek(long long ms) override { player_.seek(ms); }
  void select_track(const std::string &type, int index) override { player_.select_track(type, index); }
  void set_subtitles(bool enabled) override { player_.set_subtitles(enabled); }

 private:
  UmsPlayer &player_;
};

enum class Ctl { Toggle, Audio, Subtitles, StartOver, Next };

class PlayerScreen : public Screen {
 public:
  PlayerScreen(const PlayRequest &req, const std::string &url, const std::string &source, const std::string &load_type)
      : req_(req), url_(url), source_(source), load_type_(load_type) {
    start_at_ = req.resume_ms;
    open();
  }
  ~PlayerScreen() override { close(); }

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
    ums_.load(url_, c.app_id, window_id_, load_type_, [this](const MediaEvent &event) { on_media(event); }, load_start_);
  }

  // Release the pipeline. `keep_window` when reopening straight away.
  void close(bool keep_window = false) {
    if (!opened_) return;
    save_progress();
    Ctx &c = ctx();
    session_.account(c.now);
    total_ += session_.stats();
    ums_.unload();
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
    save_progress();
    ums_.notify_background();
  }

  void foreground() override {
    if (!in_background_) return;
    in_background_ = false;
    ums_.notify_foreground();
    if (lost_) reload("pipeline released while away");
  }

  void on_media(const MediaEvent &event) {
    Ctx &c = ctx();
    long long t = c.now;
    if (event.kind == MediaEvent::Lost) {
      logf("player", "pipeline lost: %s", event.text.c_str());
      lost_ = true;
      if (!in_background_) reload(event.text.c_str());
      return;
    }
    bool had_frame = session_.first_frame_ms() >= 0;
    auto pending = session_.pending_ms();
    long long seek_started = session_.seek_started_ms();
    session_.on_event(event, ums_.info(), t);
    if (!had_frame && session_.first_frame_ms() >= 0) logf("player", "first frame after %lld ms", session_.first_frame_ms());
    if (pending && !session_.pending_ms())
      logf("player", "seek to %lld ms landed at %lld ms after %lld ms", *pending, session_.position_ms(t), t - seek_started);
    if (event.kind == MediaEvent::VideoInfo || event.kind == MediaEvent::SourceInfo || event.kind == MediaEvent::Loaded)
      place_video(ums_.info().width, ums_.info().height);
    if ((event.kind == MediaEvent::SourceInfo || event.kind == MediaEvent::TrackSelected) && !tracks_applied_ &&
        (!ums_.info().audio.empty() || !ums_.info().text.empty()))
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
    if (event.kind == MediaEvent::Paused) save_progress();
    if (event.kind == MediaEvent::EndOfStream) {
      save_progress(true);
      if (req_.next) {
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
    const MediaInfo &info = ums_.info();
    Ctx &c = ctx();
    if (!preferences_applied_) {
      preferences_applied_ = true;
      bool current_fits = false;
      for (auto &a : info.audio) current_fits = current_fits || (a.selected && speaks(a, c.language));
      if (chosen_audio_ < 0 && !current_fits)
        for (size_t i = 0; i < info.audio.size(); i++)
          if (speaks(info.audio[i], c.language) && track_role(info.audio[i], true).empty()) {
            chosen_audio_ = static_cast<int>(i);
            break;
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

  void save_progress(bool finished = false) {
    if (!session_.started() || req_.video_id == "launch") return;
    long long duration = session_.duration_ms();
    long long position = finished ? duration : viewer_position();
    if (duration <= 0 || position <= 0) return;
    std::string body = "{\"videoId\":" + Json::quote(req_.video_id) + ",\"id\":" + Json::quote(req_.item.id) +
                       ",\"type\":" + Json::quote(req_.item.type) + ",\"name\":" + Json::quote(req_.item.name) +
                       ",\"season\":" + std::to_string(req_.season) + ",\"episode\":" + std::to_string(req_.episode) +
                       ",\"episodeTitle\":" + Json::quote(req_.episode_title) + ",\"position\":" + std::to_string(position) +
                       ",\"duration\":" + std::to_string(duration);
    if (req_.next) {
      body += ",\"next\":{\"videoId\":" + Json::quote(req_.next->video_id) + ",\"season\":" + std::to_string(req_.next->season) +
              ",\"episode\":" + std::to_string(req_.next->episode) + ",\"title\":" + Json::quote(req_.next->title) + "}";
    }
    body += "}";
    ctx().api.post("/api/progress", body);
    last_save_ = ctx().now;
  }

  void play_next() {
    auto next = play::after(req_);
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
    if (hud_mode_ == HudMode::Reduced) return hud_reduced_timeout_ms;
    if (zone_ == Zone::Controls) return hud_controls_timeout_ms;
    return hud_timeout_ms;
  }

  void show_hud(HudMode mode = HudMode::Full) {
    // A reduced request never downgrades controls already shown in full.
    if (!hud_ || mode == HudMode::Full) hud_mode_ = mode;
    hud_ = true;
    hud_shown_at_ = ctx().now;
  }

  bool hud_pinned() const {
    Phase p = session_.phase();
    return panel_ != Panel::None || p == Phase::Paused || p == Phase::Ended || p == Phase::Loading || p == Phase::Error ||
           session_.preview_ms().has_value() || session_.pending_ms().has_value() || !failure_.empty();
  }

  std::vector<Ctl> controls() const {
    std::vector<Ctl> list = {Ctl::Toggle, Ctl::Audio, Ctl::Subtitles};
    if (resumed_) list.push_back(Ctl::StartOver);
    if (req_.next) list.push_back(Ctl::Next);
    return list;
  }

  std::string control_label(Ctl ctl) const {
    const MediaInfo &info = ums_.info();
    switch (ctl) {
      case Ctl::Audio: {
        for (auto &a : info.audio)
          if (a.selected) return "Audio \xC2\xB7 " + join({language_name(a.language), channels_name(a.channels)}, " ");
        return "Audio";
      }
      case Ctl::Subtitles: {
        int i = session_.subtitle_index();
        if (i >= 0 && i < static_cast<int>(info.text.size())) return "Subtitles \xC2\xB7 " + language_name(info.text[static_cast<size_t>(i)].language);
        return "Subtitles \xC2\xB7 Off";
      }
      case Ctl::StartOver: return "Start over";
      case Ctl::Next: return "Next episode";
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
    }
    show_hud();
  }

  void open_panel(Panel p) {
    panel_ = p;
    const MediaInfo &info = ums_.info();
    if (p == Panel::Audio) {
      panel_index_ = 0;
      for (size_t i = 0; i < info.audio.size(); i++) if (info.audio[i].selected) panel_index_ = static_cast<int>(i);
    } else {
      panel_index_ = session_.subtitle_index() + 1;  // row 0 = Off
    }
  }

  int panel_rows() const {
    const MediaInfo &info = ums_.info();
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
      zone_ = key == Key::Down ? Zone::Controls : Zone::Timeline;
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
      if (key == Key::Down) { zone_ = Zone::Controls; control_ = std::min(control_, count - 1); }
      return true;
    }
    if (key == Key::Up) zone_ = Zone::Timeline;
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
    // No clock report after the start: the load option may not apply here.
    if (resume_check_ && t - playing_at_ > 2000) check_start(session_.position_ms(t));
    long long duration = session_.duration_ms();
    if (!prepared_ && req_.next && c.autoplay_next && session_.phase() == Phase::Playing && duration > 0 &&
        duration - session_.position_ms(t) <= kPrepareNextBeforeEndMs)
      prepare_next();
    if (up_next_ && c.autoplay_next && t - up_next_at_ >= kUpNextMs) {
      play_next();
      return;
    }
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
    ums_.set_subtitle_position(sub_pos_sent_, [this, alive](bool ok) {
      if (alive.expired() || ok || !sub_pos_supported_) return;
      sub_pos_supported_ = false;
      logf("player", "subtitle position not supported here; leaving subtitles where the TV puts them");
    });
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

    float a = hud_anim_.value;
    if (phase == Phase::Buffering && session_.started()) {
      long long since = session_.stalled_since();
      if (c.now - since > kBufferingDelayMs) {
        c.gfx.fill_circle(W / 2, H / 2, 54, with_alpha(ground, 150));
        draw_spinner(W / 2, H / 2, 52, primary);
      } else {
        c.redraw_at(since + kBufferingDelayMs);
      }
    }
    if (a <= 0.01f) return;
    draw_hud(a, phase);
    if (panel_anim_.value > 0.01f) draw_panel_tracks(panel_anim_.value);
  }

  std::vector<std::string> badges() const {
    const MediaInfo &info = ums_.info();
    std::vector<std::string> out;
    if (info.height >= 2000) out.push_back("4K");
    else if (info.height >= 1000) out.push_back("HD");
    std::string hdr = upper(info.hdr);
    if (hdr.find("DOLBY") != std::string::npos || hdr == "DV") out.push_back("Dolby Vision");
    else if (!hdr.empty() && hdr != "NONE" && hdr != "SDR") out.push_back(hdr.find("HLG") != std::string::npos ? "HLG" : "HDR");
    for (auto &t : info.audio)
      if (t.selected) {
        std::string codec = upper(t.codec);
        if (codec.find("TRUEHD") != std::string::npos || codec.find("ATMOS") != std::string::npos) out.push_back("Dolby TrueHD");
        else if (codec.find("EAC3") != std::string::npos) out.push_back("Dolby Digital+");
        if (t.channels >= 6) out.push_back(channels_name(t.channels));
      }
    return out;
  }

  void draw_hud(float a, Phase phase) {
    Ctx &c = ctx();
    bool reduced = hud_mode_ == HudMode::Reduced;
    if (reduced) {
      c.gfx.gradient_v(0, H - 300, W, 300, rgba(0x000000), 0, static_cast<Uint8>(170 * a));
    } else {
      // Bright scenes must not wash out the title: solid enough behind the text band.
      c.gfx.gradient_v(0, H - 600, W, 240, rgba(0x000000), 0, static_cast<Uint8>(190 * a));
      c.gfx.fill_rect(0, H - 360, W, 360, rgba(0x000000, static_cast<Uint8>(190 * a)));
      c.gfx.gradient_v(0, H - 360, W, 360, rgba(0x000000), 0, static_cast<Uint8>(60 * a));
      c.gfx.gradient_v(0, 0, W, 180, rgba(0x000000), static_cast<Uint8>(140 * a), 0);
      text_right(Text::Callout, wall_clock(), W - margin, 52, fade(secondary, a));
    }
    if (!reduced) draw_hud_title(a);
    draw_timeline(a, phase);
    if (reduced) return;
    draw_controls(a, phase);
  }

  void draw_hud_title(float a) {
    // Title and context, bottom-left; quiet stream badges on the right.
    std::string title = req_.item.series() ? req_.episode_title.empty() ? req_.item.name : req_.episode_title : req_.item.name;
    std::string sub = req_.item.series() ? join({req_.item.name, episode_code(req_.season, req_.episode)}) : join({req_.item.year, req_.item.genres.empty() ? "" : req_.item.genres[0]});
    text(Text::Title2, title, margin, H - 346, fade(primary, a), 1100);
    text(Text::Callout, sub, margin, H - 296, fade(primary, a * 0.8f), 1100);
    int bx = W - margin;
    auto list = badges();
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
      bx -= badge_width(*it);
      draw_badge(bx, H - 296, *it, fade(secondary, a));
      bx -= 10;
    }
  }

  void draw_timeline(float a, Phase phase) {
    Ctx &c = ctx();
    long long t = c.now;
    long long display = session_.display_ms(t), duration = session_.duration_ms(), position = session_.position_ms(t);
    int tx = margin, tw = W - 2 * margin, ty = H - 214;
    bool timeline = zone_ == Zone::Timeline && panel_ == Panel::None;
    int th = timeline ? 8 : 6;
    c.gfx.fill_round(tx, ty - th / 2, tw, th, th / 2, fade(with_alpha(focus, 70), a));
    double frac = duration > 0 ? std::min(1.0, std::max(0.0, static_cast<double>(display) / duration)) : 0;
    c.gfx.fill_round(tx, ty - th / 2, std::max(th, static_cast<int>(tw * frac)), th, th / 2, fade(focus, a));
    int kx = tx + static_cast<int>(tw * frac);
    if (timeline || session_.preview_ms() || session_.pending_ms()) c.gfx.fill_circle(kx, ty, timeline ? 13 : 10, fade(focus, a));
    if (session_.preview_ms() || session_.pending_ms()) {
      long long delta = display - session_.seek_origin_ms(position);
      long long s = (delta < 0 ? -delta : delta) / 1000;
      std::string d = std::string(delta < 0 ? "\xE2\x88\x92" : "+") + (s < 60 ? std::to_string(s) + " s" : clock_text(s * 1000));
      std::string label = clock_text(display);
      int lw = std::max(measure(Text::Headline, label), measure(Text::Caption, d)) + 44;
      int lx = std::max(tx, std::min(tx + tw - lw, kx - lw / 2));
      c.gfx.shadow(lx, ty - 112, lw, 84, 16, 18, 8, static_cast<Uint8>(140 * a));
      c.gfx.fill_round(lx, ty - 112, lw, 84, 16, fade(focus, a));
      text_center(Text::Headline, label, lx + lw / 2, ty - 104, fade(ink, a));
      text_center(Text::Caption, d, lx + lw / 2, ty - 68, fade(ink2, a));
    }
    text(Text::Callout, clock_text(display), tx, ty + 20, fade(secondary, a));
    text_right(Text::Callout, duration > 0 ? "\xE2\x88\x92" + clock_text(std::max(0LL, duration - display)) : "--:--", tx + tw, ty + 20,
               fade(secondary, a));
    if (phase == Phase::Seeking) text_center(Text::Callout, "Seeking\xE2\x80\xA6", W / 2, ty + 20, fade(secondary, a));
    else if (phase == Phase::Paused) text_center(Text::Callout, "Paused", W / 2, ty + 20, fade(secondary, a));
    else if (phase == Phase::Ended) text_center(Text::Callout, "Finished", W / 2, ty + 20, fade(secondary, a));
  }

  void draw_controls(float a, Phase phase) {
    Ctx &c = ctx();
    // When it ends, in wall-clock time, on the right of the controls row.
    long long duration = session_.duration_ms(), left = duration - session_.display_ms(c.now);
    if (duration > 0 && left > 0 && phase != Phase::Ended)
      text_right(Text::Callout, "Ends at " + wall_clock(left), W - margin, H - 128 + 14, fade(tertiary, a));

    // Controls row: play/pause, then the track and episode pills.
    auto ctl = controls();
    int x = margin, y = H - 128;
    for (size_t i = 0; i < ctl.size(); i++) {
      bool f = zone_ == Zone::Controls && static_cast<int>(i) == control_ && panel_ == Panel::None;
      if (ctl[i] == Ctl::Toggle) {
        int r = 30;
        if (f) c.gfx.shadow(x, y - 2, 2 * r, 2 * r, r, 16, 8, static_cast<Uint8>(130 * a));
        c.gfx.fill_circle(x + r, y + 28, r, fade(f ? focus : glass_light, a));
        Color fg = fade(f ? ink : primary, a);
        if (session_.intent_play() && phase != Phase::Ended) c.gfx.bars(x + r - 11, y + 17, 22, fg);
        else c.gfx.triangle_right(x + r - 9, y + 14, 28, fg);
        x += 2 * r + 16;
        continue;
      }
      int icon = ctl[i] == Ctl::Audio ? static_cast<int>(Icon::Audio) : ctl[i] == Ctl::Subtitles ? static_cast<int>(Icon::Subtitles)
                 : ctl[i] == Ctl::StartOver ? static_cast<int>(Icon::Restart) : static_cast<int>(Icon::Next);
      x += draw_pill(x, y, control_label(ctl[i]), f, false, icon, a) + 14;
    }
  }

  void draw_panel_tracks(float a) {
    Ctx &c = ctx();
    const MediaInfo &info = ums_.info();
    int rows = panel_rows();
    int w = 620, x = W - margin - w + static_cast<int>((1.0f - a) * 50), y = 96;
    int h = std::min(H - 192 - 200, 150 + std::max(1, rows) * 76 + 40);
    c.gfx.shadow(x, y, w, h, panel_radius, 36, 16, static_cast<Uint8>(170 * a));
    c.gfx.fill_round(x, y, w, h, panel_radius, fade(rgba(0x1c1e23, 245), a));
    text(Text::Title2, panel_ == Panel::Audio ? "Audio" : "Subtitles", x + 40, y + 40, fade(primary, a));
    int ry = y + 112;
    if (panel_ == Panel::Audio && rows == 0) paragraph(Text::Callout, "This video reports no audio tracks.", x + 40, ry, fade(secondary, a), w - 80, 2);
    auto labels = distinct_labels(panel_ == Panel::Audio ? info.audio : info.text, panel_ == Panel::Audio);
    int visible = (h - 150) / 76;
    int first = std::max(0, std::min(panel_index_ - visible / 2, rows - visible));
    for (int i = first; i < std::min(rows, first + visible); i++) {
      bool f = i == panel_index_;
      std::string label;
      bool selected = false;
      if (panel_ == Panel::Audio) {
        label = labels[static_cast<size_t>(i)];
        selected = info.audio[static_cast<size_t>(i)].selected;
      } else if (i == 0) {
        label = "Off";
        selected = session_.subtitle_index() < 0;
      } else {
        label = labels[static_cast<size_t>(i - 1)];
        selected = session_.subtitle_index() == i - 1;
      }
      if (f) draw_row_focus(x + 20, ry, w - 40, 64);
      text(Text::Callout, label, x + 44, ry + 16, fade(f ? ink : primary, a), w - 150);
      if (selected) c.icons.draw(Icon::Check, x + w - 84, ry + 16, 32, fade(f ? ink : good, a));
      ry += 76;
    }
    if (panel_ == Panel::Subtitles && info.text.empty())
      paragraph(Text::Caption, "This video has no built-in subtitles.", x + 40, ry + 8, fade(tertiary, a), w - 80, 2);
  }

  void draw_startup() {
    Ctx &c = ctx();
    c.gfx.clear(ground);
    ImageRef soft = c.images.get(req_.item.backdrop, W, H, 0, Fit::Cover, false, 48);
    if (soft.texture) {
      c.gfx.texture(soft.texture, 0, 0, W, H, soft.alpha);
      c.gfx.fill_rect(0, 0, W, H, with_alpha(ground, 150));
    }
    int y = draw_title(req_.item, W / 2 - 380, 300, 760, 200, 1.0f, true) + 28;
    if (req_.item.series()) {
      text_center(Text::Title2, episode_code(req_.season, req_.episode), W / 2, y, primary);
      y += line_for(Text::Title2);
      if (!req_.episode_title.empty()) text_center(Text::Callout, req_.episode_title, W / 2, y + 4, secondary, 900);
      y += line_for(Text::Callout) + 8;
    }
    draw_spinner(W / 2, y + 70, 48, primary, opened_at_);
    std::string what = reconnecting_ ? "Reconnecting\xE2\x80\xA6" : join({"Starting", source_});
    if (c.now - opened_at_ > spinner_delay_ms) text_center(Text::Callout, what, W / 2, y + 120, tertiary);
  }

  void draw_error() {
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(ground, 235));
    int w = 1000, h = 400, x = (W - w) / 2, y = (H - h) / 2;
    draw_panel(x, y, w, h);
    c.icons.draw(Icon::Warning, x + 56, y + 58, 40, caution);
    text(Text::Title2, "Playback stopped", x + 112, y + 56, primary);
    paragraph(Text::Body, failure_.empty() ? session_.error() : failure_, x + 56, y + 130, secondary, w - 112, 3);
    const char *labels[3] = {"Try again", "Choose another version", "Exit"};
    int bx = x + 56;
    for (int i = 0; i < 3; i++) {
      ButtonSpec b{labels[i], -1, error_focus_ == i};
      bx += draw_button(bx, y + h - 112, b) + 16;
    }
  }

  void draw_up_next() {
    Ctx &c = ctx();
    const NextEpisode &n = *req_.next;
    // Bright end credits must not wash out the text: a corner scrim, not a dimmed frame.
    c.gfx.gradient_v(0, H - 680, W, 260, rgba(0x000000), 0, 150);
    c.gfx.fill_rect(0, H - 420, W, 420, rgba(0x000000, 150));
    c.gfx.gradient_v(0, H - 420, W, 420, rgba(0x000000), 0, 90);
    long long elapsed = c.now - up_next_at_;
    long long left = std::max(0LL, (kUpNextMs - elapsed + 999) / 1000);
    int x = margin, y = H - 380;
    text(Text::Eyebrow, upper("Up next"), x, y, accent);
    text(Text::Title1, episode_code(n.season, n.episode), x, y + 34, primary);
    text(Text::Title2, n.title, x, y + 98, secondary, 1200);
    ButtonSpec play{"Play now", kPlayIcon, true};
    int bw = draw_button(x, y + 170, play);
    if (!c.autoplay_next) {
      text(Text::Callout, "Back to stay", x + bw + 28, y + 190, secondary);
      return;
    }
    text(Text::Callout, "Starts in " + std::to_string(left) + " s  \xC2\xB7  Back to stay", x + bw + 28, y + 190, secondary);
    draw_progress(x, y + 270, 520, 6, static_cast<double>(elapsed) / kUpNextMs, focus);
  }

 private:
  PlayRequest req_;
  std::string url_, source_, load_type_;
  UmsPlayer ums_;
  UmsPort port_{ums_};
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
  Anim hud_anim_{fade_rate, 1}, panel_anim_{fade_rate};
};

}  // namespace

std::unique_ptr<Screen> make_player(const PlayRequest &request, const std::string &url, const std::string &source_label,
                                    const std::string &load_type) {
  return std::make_unique<PlayerScreen>(request, url, source_label, load_type);
}
