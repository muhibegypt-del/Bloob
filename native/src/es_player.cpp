// EsPlayer: Bloob's own playback engine (3.0). See es_player.h.
#include "es_player.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include <sstream>

#include "log.h"

namespace {

using Clock = std::chrono::steady_clock;
constexpr long long kFeedAheadNs = 1600LL * 1000 * 1000;  // stay this far ahead of the picture
constexpr long long kTimeEveryNs = 250LL * 1000 * 1000;   // clock reports to the UI

long long steady_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}

std::string quoted(const std::string &s) {
  std::string out = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') out += '\\';
    if (static_cast<unsigned char>(c) >= 0x20) out += c;
  }
  return out + "\"";
}

}  // namespace

// "3840×2160 · 23.976 fps · HDR10" from a JSON reply that names width, height,
// frame rate and HDR type somewhere (the pipeline's video info, videooutput's status).
std::string summarise_video_info(const std::string &json) {
  auto number = [&](const char *key) -> double {
    std::string k = std::string("\"") + key + "\"";
    size_t at = json.find(k);
    if (at == std::string::npos) return 0;
    at = json.find(':', at + k.size());
    return at == std::string::npos ? 0 : std::atof(json.c_str() + at + 1);
  };
  auto text = [&](const char *key) -> std::string {
    std::string k = std::string("\"") + key + "\"";
    size_t at = json.find(k);
    if (at == std::string::npos) return "";
    size_t q1 = json.find('"', json.find(':', at + k.size()) + 1);
    size_t q2 = q1 == std::string::npos ? q1 : json.find('"', q1 + 1);
    return q2 == std::string::npos ? "" : json.substr(q1 + 1, q2 - q1 - 1);
  };
  int w = static_cast<int>(number("width")), h = static_cast<int>(number("height"));
  if (w <= 0 || h <= 0) return "";
  char buf[96];
  std::snprintf(buf, sizeof buf, "%d\xC3\x97%d", w, h);
  std::string out = buf;
  double fps = number("frameRate");
  if (fps > 0) {
    std::snprintf(buf, sizeof buf, " \xC2\xB7 %.3f fps", fps);
    out += buf;
  }
  std::string hdr = text("hdrType");
  if (!hdr.empty() && hdr != "none" && hdr != "NONE") out += " \xC2\xB7 " + hdr;
  return out;
}

// ---------------------------------------------------------------- payloads

std::string EsPlayer::load_payload(const DemuxFacts &f, const AudioOut &a, const std::string &app_id,
                                   const std::string &window_id, long long start_ns) {
  std::ostringstream c;  // externalStreamingInfo.contents
  c << "{\"codec\":{\"video\":" << quoted(f.video_codec);
  if (!a.codec.empty()) c << ",\"audio\":" << quoted(a.codec);
  c << "},\"esInfo\":{\"pauseAtDecodeTime\":true,\"seperatedPTS\":true,\"ptsToDecode\":" << start_ns
    << ",\"videoWidth\":" << f.width << ",\"videoHeight\":" << f.height;
  if (f.fps_num > 0 && f.fps_den > 0) c << ",\"videoFpsValue\":" << f.fps_num << ",\"videoFpsScale\":" << f.fps_den;
  c << "},\"format\":\"RAW\",\"provider\":" << quoted(app_id);
  if (a.codec == "AC3 PLUS") {
    char freq[16];
    std::snprintf(freq, sizeof freq, "%.1f", a.sample_rate / 1000.0);
    c << ",\"ac3PlusInfo\":{\"channels\":" << (a.atmos ? a.channels + 2 : a.channels) << ",\"frequency\":" << freq << "}";
  }
  if (a.atmos) c << ",\"immersive\":\"ATMOS\"";
  if (f.hdr == "Dolby Vision")
    c << ",\"DolbyHdrInfo\":{\"encryptionType\":\"clear\",\"profileId\":" << f.dv_profile << ",\"trackType\":\"single\"}";
  c << "}";

  std::ostringstream p;
  p << "{\"args\":[{\"mediaTransportType\":\"BUFFERSTREAM\",\"option\":{\"appId\":" << quoted(app_id);
  if (!window_id.empty()) p << ",\"windowId\":" << quoted(window_id);
  if (a.codec.empty()) p << ",\"needAudio\":false";
  p << ",\"transmission\":{\"contentsType\":\"LIVE\",\"trickType\":\"client-side\"},\"seekMode\":\"keep-rate\""
       ",\"useDroppedFrameEvent\":true"
       ",\"externalStreamingInfo\":{\"contents\":"
    << c.str()
    << ",\"streamQualityInfo\":true,\"streamQualityInfoNonFlushable\":true,\"streamQualityInfoCorruptedFrame\":true"
       ",\"bufferingCtrInfo\":{\"preBufferByte\":0,\"bufferMinLevel\":0,\"bufferMaxLevel\":0,\"qBufferLevelVideo\":0,"
       "\"srcBufferLevelVideo\":{\"minimum\":0,\"maximum\":8388608},\"qBufferLevelAudio\":0,"
       "\"srcBufferLevelAudio\":{\"minimum\":0,\"maximum\":2097152}}}}}]}";
  return p.str();
}

std::string EsPlayer::hdr_payload(const DemuxFacts &f) {
  const char *type = f.transfer == 16 ? "HDR10" : f.transfer == 18 ? "HLG" : nullptr;  // SMPTE 2084, ARIB B67
  if (!type || (!f.has_mastering && !f.has_light_level)) return "";  // let the TV read the stream itself
  std::ostringstream s;
  bool first = true;
  auto field = [&](const char *name, long long v) {
    s << (first ? "" : ",") << "\"" << name << "\":" << v;
    first = false;
  };
  if (f.has_mastering) {
    // Green, blue, red: the order the pipeline expects (CTA-861.3).
    field("displayPrimariesX0", f.primaries_xy[1][0]);
    field("displayPrimariesY0", f.primaries_xy[1][1]);
    field("displayPrimariesX1", f.primaries_xy[2][0]);
    field("displayPrimariesY1", f.primaries_xy[2][1]);
    field("displayPrimariesX2", f.primaries_xy[0][0]);
    field("displayPrimariesY2", f.primaries_xy[0][1]);
    field("whitePointX", f.white_xy[0]);
    field("whitePointY", f.white_xy[1]);
    field("minDisplayMasteringLuminance", f.min_luminance);
    field("maxDisplayMasteringLuminance", f.max_luminance);
  }
  if (f.has_light_level) {
    field("maxContentLightLevel", f.max_cll);
    field("maxPicAverageLightLevel", f.max_fall);
  }
  std::ostringstream p;
  p << "{\"hdrType\":\"" << type << "\",\"sei\":{" << s.str() << "},\"vui\":{\"transferCharacteristics\":" << f.transfer
    << ",\"colorPrimaries\":" << f.primaries << ",\"matrixCoeffs\":" << f.matrix
    << ",\"videoFullRangeFlag\":" << (f.full_range ? "true" : "false") << "}}";
  return p.str();
}

// ---------------------------------------------------------------- lifecycle

EsPlayer::EsPlayer(std::unique_ptr<EsSink> sink, std::function<void(std::function<void()>)> post)
    : sink_(std::move(sink)), post_(std::move(post)) {}

EsPlayer::~EsPlayer() { unload(); }

const char *EsPlayer::engine_name() const { return sink_ && std::string(sink_->name()) == "clock" ? "Bloob engine (desktop)" : "Bloob engine"; }

bool EsPlayer::load(const std::string &uri, const std::string &app_id, const std::string &window_id,
                    const std::string &load_type, Listener listener, long long start_ms) {
  unload();
  if (!sink_) return false;
  url_ = uri;
  app_id_ = app_id;
  window_id_ = window_id;
  start_ms_ = start_ms;
  listener_ = std::move(listener);
  info_ = MediaInfo();
  stopping_ = false;
  want_playing_ = true;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    commands_.clear();
  }
  demux_.resume();
  feeder_ = std::thread([this] { run(); });
  return true;
}

void EsPlayer::unload() {
  if (!feeder_.joinable()) return;
  stopping_ = true;
  demux_.interrupt();
  cv_.notify_all();
  feeder_.join();
  {
    std::lock_guard<std::mutex> lock(cue_mutex_);
    cues_.clear();
  }
}

void EsPlayer::emit(MediaEvent event) {
  std::weak_ptr<int> alive = alive_;
  post_([this, alive, event] {
    if (!alive.lock() || !listener_) return;
    // A copy: the listener may replace this player (the fallback), destroying listener_.
    Listener listener = listener_;
    listener(event);
  });
}

// ---------------------------------------------------------------- UI-thread commands

void EsPlayer::play() {
  want_playing_ = true;
  bool ok = false;
  {
    std::lock_guard<std::mutex> lock(sink_mutex_);
    if (loaded_) ok = sink_->play();
  }
  if (ok && listener_) listener_({MediaEvent::Playing});
}

void EsPlayer::pause() {
  want_playing_ = false;
  bool ok = false;
  {
    std::lock_guard<std::mutex> lock(sink_mutex_);
    if (loaded_) ok = sink_->pause();
  }
  if (ok && listener_) listener_({MediaEvent::Paused});
}

void EsPlayer::seek(long long position_ms) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    // A newer seek replaces one not yet started.
    commands_.erase(std::remove_if(commands_.begin(), commands_.end(), [](const Command &c) { return c.kind == Command::Seek; }),
                    commands_.end());
    commands_.push_back({Command::Seek, std::max(0LL, position_ms)});
  }
  demux_.interrupt();
  cv_.notify_all();
}

void EsPlayer::select_track(const std::string &type, int index) {
  bool audio = type == "audio";
  {
    std::lock_guard<std::mutex> lock(mutex_);
    commands_.push_back({audio ? Command::Audio : Command::Text, index});
  }
  if (audio) demux_.interrupt();  // the pipeline reloads anyway
  else text_index_ = index;
  cv_.notify_all();
}

void EsPlayer::set_subtitles(bool enabled) {
  subtitles_on_ = enabled;
  if (!enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    commands_.push_back({Command::Text, -1});
    cv_.notify_all();
  } else if (text_index_ >= 0) {
    select_track("text", text_index_);
  }
}

void EsPlayer::notify_foreground() {
  std::lock_guard<std::mutex> lock(sink_mutex_);
  sink_->notify_foreground();
}

void EsPlayer::notify_background() {
  std::lock_guard<std::mutex> lock(sink_mutex_);
  sink_->notify_background();
}

std::vector<SubtitleCue> EsPlayer::cues_at(long long ms) const {
  std::vector<SubtitleCue> out;
  std::lock_guard<std::mutex> lock(cue_mutex_);
  for (const auto &c : cues_)
    if (c.start_ms <= ms && ms < c.end_ms) out.push_back(c);
  return out;
}

long long EsPlayer::next_cue_change(long long ms) const {
  long long next = -1;
  std::lock_guard<std::mutex> lock(cue_mutex_);
  for (const auto &c : cues_) {
    if (c.start_ms > ms && (next < 0 || c.start_ms < next)) next = c.start_ms;
    if (c.end_ms > ms && c.end_ms != LLONG_MAX && (next < 0 || c.end_ms < next)) next = c.end_ms;
  }
  return next;
}

// ---------------------------------------------------------------- feeder thread

void EsPlayer::publish_facts(bool track_change) {
  DemuxFacts f = demux_.facts();
  AudioOut a = demux_.audio_out();
  std::weak_ptr<int> alive = alive_;
  post_([this, alive, f, a, track_change] {
    if (!alive.lock()) return;
    MediaInfo &m = info_;
    m.container = f.container;
    m.duration_ms = f.duration_ms;
    m.seekable = true;
    m.video_codec = f.video_codec;
    m.width = f.width;
    m.height = f.height;
    m.frame_rate = f.fps_den > 0 ? static_cast<double>(f.fps_num) / f.fps_den : 0;
    m.hdr = f.hdr == "SDR" ? "" : f.hdr;
    m.audio = f.audio;
    m.text = f.text;
    chapters_ = f.chapters;
    audio_note_ = a.converted ? a.from + " \xE2\x86\x92 Dolby Digital Plus " + (a.channels >= 6 ? "5.1" : a.channels == 1 ? "Mono" : "Stereo") +
                                    " (converted by Bloob)"
                              : (a.atmos ? a.from + " (Atmos, passed to the TV)" : "");
    if (!listener_) return;
    Listener listener = listener_;  // see emit()
    if (track_change) {
      int selected = -1;
      for (size_t i = 0; i < f.audio.size(); i++)
        if (f.audio[i].selected) selected = static_cast<int>(i);
      listener({MediaEvent::TrackSelected, 0, "audio", selected});
    } else {
      listener({MediaEvent::SourceInfo});
      if (!alive.lock()) return;
      listener({MediaEvent::VideoInfo});
      if (!alive.lock()) return;
      listener({MediaEvent::Loaded});
    }
  });
}

bool EsPlayer::start_pipeline(long long position_ms) {
  int gen = ++generation_;
  loaded_ = false;
  started_ = false;
  eos_sent_ = false;
  clock_ns_ = -1;
  long long start_ns = position_ms * 1000000LL;
  target_ns_ = start_ns;
  std::string payload = load_payload(demux_.facts(), demux_.audio_out(), app_id_, window_id_, start_ns);
  std::string hdr = hdr_payload(demux_.facts());
  bool ok;
  {
    std::lock_guard<std::mutex> lock(sink_mutex_);
    sink_->notify_foreground();
    ok = sink_->load(payload, [this, gen](int type, long long num, const std::string &str) {
      if (gen == generation_.load()) on_sink(type, num, str);
    });
    if (ok && !hdr.empty()) sink_->set_hdr(hdr);
    pipeline_open_ = ok;
  }
  logf("engine", "pipeline %s at %lld ms (%s, %s%s)", ok ? "loading" : "refused the load", position_ms,
       demux_.facts().video_codec.c_str(), demux_.audio_out().codec.c_str(), demux_.audio_out().converted ? ", converted" : "");
  return ok;
}

void EsPlayer::stop_pipeline() {
  generation_++;
  std::lock_guard<std::mutex> lock(sink_mutex_);
  if (pipeline_open_) sink_->unload();
  pipeline_open_ = false;
  loaded_ = false;
  started_ = false;
}

void EsPlayer::on_sink(int type, long long num, const std::string &str) {
  switch (type) {
    case EsSink::FrameReady: {
      clock_ns_ = num;
      if (!started_.exchange(true)) {
        if (seeking_.exchange(false)) emit({MediaEvent::SeekDone});
        if (want_playing_) emit({MediaEvent::Playing});
        logf("engine", "first frame at %lld ms", num / 1000000);
      }
      long long now = steady_ns();
      if (now - last_time_post_.load() >= kTimeEveryNs) {
        last_time_post_ = now;
        emit({MediaEvent::Time, num / 1000000});
      }
      break;
    }
    case EsSink::LoadCompleted:
      loaded_ = true;
      cv_.notify_all();
      break;
    case EsSink::EndOfStream:
      emit({MediaEvent::EndOfStream});
      break;
    case EsSink::IntError:
    case EsSink::StrError:
      logf("engine", "pipeline error %lld %s", num, str.c_str());
      break;
    case EsSink::VideoInfo: {
      logf("engine", "pipeline video info: %.400s", str.c_str());
      // "width":3840,"height":2160,"frameRate":23.976,"hdrType":"…" (field names as the pipeline sends them).
      std::string note = summarise_video_info(str);
      if (!note.empty()) {
        std::weak_ptr<int> alive = alive_;
        post_([this, alive, note] {
          if (alive.lock()) decoder_note_ = note;
        });
      }
      break;
    }
    case EsSink::AudioInfo:
      logf("engine", "pipeline audio info: %.300s", str.c_str());
      break;
    default:
      break;
  }
}

void EsPlayer::run() {
  std::string error;
  for (int tries = 0; !demux_.open(url_, &error); tries++) {
    if (stopping_) return;
    // A seek pressed while the file was opening interrupts the read: open again.
    bool command = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      command = !commands_.empty();
    }
    if (!command || tries >= 2) {
      logf("engine", "could not open the file: %s", error.c_str());
      emit({MediaEvent::Error, 0, "Bloob's engine could not read this file (" + error + ").", -100});
      return;
    }
    demux_.resume();
  }
  // First sound: the file's default main track, else the first main track. The
  // player then applies the viewer's language (select_track).
  const auto &audio = demux_.facts().audio;
  int choice = -1;
  for (size_t i = 0; i < audio.size() && choice < 0; i++)
    if (audio[i].is_default && !audio[i].commentary) choice = static_cast<int>(i);
  for (size_t i = 0; i < audio.size() && choice < 0; i++)
    if (!audio[i].commentary) choice = static_cast<int>(i);
  if (choice < 0 && !audio.empty()) choice = 0;
  if (choice >= 0 && !demux_.select_audio(choice))
    for (size_t i = 0; i < audio.size(); i++)
      if (static_cast<int>(i) != choice && demux_.select_audio(static_cast<int>(i))) break;
  publish_facts(false);
  if (start_ms_ > 0) demux_.seek(start_ms_);
  if (!start_pipeline(start_ms_)) {
    emit({MediaEvent::Error, 0, "The TV refused Bloob's engine.", -101});
    return;
  }

  DemuxPacket pending;
  bool have = false, played = false, failed = false;
  int played_gen = -1;
  std::string last_feed_error;
  auto wait = [&](int ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(ms), [&] { return stopping_.load() || !commands_.empty(); });
  };
  while (!stopping_) {
    Command cmd{Command::Stop};
    bool got = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!commands_.empty()) {
        cmd = commands_.front();
        commands_.pop_front();
        got = true;
      }
    }
    if (got) {
      demux_.resume();
      have = false;
      if (cmd.kind == Command::Seek || cmd.kind == Command::Audio) {
        long long at = cmd.kind == Command::Seek ? cmd.value
                       : (clock_ns_ >= 0 ? clock_ns_.load() : target_ns_.load()) / 1000000;
        stop_pipeline();
        if (cmd.kind == Command::Audio) {
          demux_.select_audio(static_cast<int>(cmd.value));
          publish_facts(true);
        } else {
          seeking_ = true;
        }
        {
          std::lock_guard<std::mutex> lock(cue_mutex_);
          cues_.clear();
        }
        demux_.seek(at);
        failed = false;
        if (!start_pipeline(at)) {
          emit({MediaEvent::Error, 0, "The TV refused Bloob's engine.", -101});
          failed = true;
        }
      } else if (cmd.kind == Command::Text) {
        demux_.select_text(static_cast<int>(cmd.value));
        std::lock_guard<std::mutex> lock(cue_mutex_);
        cues_.clear();
      }
      continue;
    }
    if (failed || !loaded_) {
      wait(10);
      continue;
    }
    if (played_gen != generation_.load()) {
      played_gen = generation_.load();
      played = false;
    }
    if (!played) {
      played = true;
      std::lock_guard<std::mutex> lock(sink_mutex_);
      if (want_playing_) sink_->play();
      else sink_->pause();
    }
    if (!have) {
      pending = demux_.read();
      have = true;
    }
    switch (pending.kind) {
      case DemuxPacket::Again:
        have = false;
        break;
      case DemuxPacket::End:
        have = false;
        if (!eos_sent_.exchange(true)) {
          std::lock_guard<std::mutex> lock(sink_mutex_);
          sink_->push_eos();
          logf("engine", "end of file");
        }
        wait(50);
        break;
      case DemuxPacket::Error:
        have = false;
        logf("engine", "read failed: %s", pending.text.c_str());
        emit({MediaEvent::Error, 0, "The film stopped arriving (" + pending.text + ").", -102});
        failed = true;
        break;
      case DemuxPacket::Text:
      case DemuxPacket::Picture: {
        have = false;
        SubtitleCue cue;
        cue.start_ms = pending.pts_us / 1000;
        cue.end_ms = pending.duration_us > 0 ? cue.start_ms + pending.duration_us / 1000 : LLONG_MAX;
        cue.text = pending.text;
        cue.picture = pending.picture;
        std::lock_guard<std::mutex> lock(cue_mutex_);
        // A Blu-ray picture lasts until the next one; an empty one only clears.
        if (cue.picture)
          for (auto &c : cues_)
            if (c.picture && c.end_ms > cue.start_ms) c.end_ms = cue.start_ms;
        if (pending.kind == DemuxPacket::Text && cue.end_ms == LLONG_MAX) cue.end_ms = cue.start_ms + 4000;
        if (!cue.picture || !cue.picture->pieces.empty()) cues_.push_back(cue);
        long long now_ms = clock_ns_ / 1000000;
        while (!cues_.empty() && (cues_.size() > 200 || cues_.front().end_ms < now_ms - 10000)) cues_.pop_front();
        break;
      }
      case DemuxPacket::Video:
      case DemuxPacket::Audio: {
        long long pts_ns = pending.pts_us * 1000;
        long long clock = clock_ns_.load();
        if (started_ && clock >= 0 && pts_ns - clock > kFeedAheadNs) {
          wait(10);
          break;
        }
        std::string answer;
        {
          std::lock_guard<std::mutex> lock(sink_mutex_);
          answer = sink_->feed(pending.data.data(), pending.data.size(), pts_ns, pending.kind == DemuxPacket::Video);
        }
        if (answer.find("BufferFull") != std::string::npos) {
          wait(10);
          break;
        }
        if (answer.find("Ok") == std::string::npos && answer != last_feed_error) {
          last_feed_error = answer;
          logf("engine", "feed: %.200s", answer.c_str());
        }
        have = false;
        break;
      }
    }
  }
  stop_pipeline();
  demux_.close();
}

// ---------------------------------------------------------------- desktop stand-in

namespace {

// Plays by the wall clock: loads after 200 ms, shows a "frame" every 40 ms,
// accepts streams up to 3 s ahead, and ends when it passes the last one fed.
class ClockSink : public EsSink {
 public:
  ~ClockSink() override { unload(); }
  bool load(const std::string &payload, Event on_event) override {
    unload();
    on_event_ = std::move(on_event);
    size_t at = payload.find("\"ptsToDecode\":");
    start_ns_ = at == std::string::npos ? 0 : std::atoll(payload.c_str() + at + 14);
    position_ns_ = start_ns_.load();
    last_fed_ns_ = start_ns_.load();
    playing_ = false;
    eos_ = false;
    stop_ = false;
    thread_ = std::thread([this] {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      if (stop_) return;
      on_event_(LoadCompleted, 0, "");
      long long last = steady_ns();
      while (!stop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        long long now = steady_ns();
        if (playing_ && fed_any_) {
          position_ns_ = std::min(position_ns_.load() + (now - last), last_fed_ns_.load());
          on_event_(FrameReady, position_ns_.load(), "");
          if (eos_ && position_ns_ >= last_fed_ns_) {
            on_event_(EndOfStream, 0, "");
            eos_ = false;
          }
        }
        last = now;
      }
    });
    return true;
  }
  std::string feed(const uint8_t *, size_t, long long pts_ns, bool video) override {
    if (pts_ns - position_ns_ > 3000000000LL) return "BufferFull";
    fed_any_ = true;
    if (video && pts_ns > last_fed_ns_) last_fed_ns_ = pts_ns;
    return "Ok";
  }
  bool play() override { playing_ = true; return true; }
  bool pause() override { playing_ = false; return true; }
  bool push_eos() override { eos_ = true; return true; }
  void unload() override {
    stop_ = true;
    if (thread_.joinable()) thread_.join();
    fed_any_ = false;
  }
  const char *name() const override { return "clock"; }

 private:
  Event on_event_;
  std::thread thread_;
  std::atomic<bool> stop_{false}, playing_{false}, eos_{false}, fed_any_{false};
  std::atomic<long long> start_ns_{0}, position_ns_{0}, last_fed_ns_{0};
};

}  // namespace

std::unique_ptr<EsSink> make_clock_sink() { return std::unique_ptr<EsSink>(new ClockSink); }
