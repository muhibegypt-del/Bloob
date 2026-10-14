#include "media.h"

#include <cctype>

#include "json.h"
#include "log.h"
#include "luna.h"

static const char *kService = "luna://com.webos.media/";

bool UmsPlayer::load(const std::string &uri, const std::string &app_id, const std::string &window_id,
                     const std::string &load_type, Listener listener, long long start_ms) {
  listener_ = std::move(listener);
  info_ = MediaInfo{};
  std::string option = "{\"appId\":" + Json::quote(app_id);
  if (!window_id.empty()) option += ",\"windowId\":" + Json::quote(window_id);
  if (start_ms > 0) option += ",\"transmission\":{\"playTime\":{\"start\":" + std::to_string(start_ms) + "}}";
  option += "}";
  std::string payload = "{\"uri\":" + Json::quote(uri) + ",\"type\":" + Json::quote(load_type) +
                        ",\"payload\":{\"option\":" + option + "}}";
  logf("media", "load %s type=%s window=%s start=%lld", redact_uri(uri).c_str(), load_type.c_str(), window_id.c_str(), start_ms);

  int generation = ++generation_;
  std::weak_ptr<int> alive = alive_;
  int id = Luna::instance().call(std::string(kService) + "load", payload, [this, alive, generation, uri, app_id, window_id, load_type,
                                                                        start_ms](const std::string &reply) {
    bool ok = false;
    Json json = Json::parse(reply, &ok);
    if (alive.expired() || generation != generation_) {
      // The viewer left before the pipeline answered: release it immediately.
      std::string stale = json["mediaId"].as_string();
      if (!stale.empty()) {
        logf("media", "discarding late load %s", stale.c_str());
        Luna::instance().call(std::string(kService) + "unload", "{\"mediaId\":" + Json::quote(stale) + "}", nullptr);
      }
      return;
    }
    if (!ok || !json["returnValue"].as_bool()) {
      if (start_ms > 0) {
        logf("media", "load with a start position refused (%lld %s); loading from the beginning",
             json["errorCode"].as_int(-1), json["errorText"].as_string().c_str());
        load(uri, app_id, window_id, load_type, listener_, 0);
        return;
      }
      logf("media", "load rejected: errorCode=%lld %s", json["errorCode"].as_int(-1), json["errorText"].as_string().c_str());
      if (listener_) listener_({MediaEvent::Error, 0, json["errorText"].as_string("The TV media service refused the video."),
                                static_cast<int>(json["errorCode"].as_int(-1))});
      return;
    }
    media_id_ = json["mediaId"].as_string();
    lost_ = false;
    logf("media", "loaded mediaId=%s", media_id_.c_str());
    notify_foreground();
    subscription_ = Luna::instance().call(std::string(kService) + "subscribe",
                                          "{\"mediaId\":" + Json::quote(media_id_) + "}",
                                          [this](const std::string &event) { on_event(event); }, true);
    if (listener_) listener_({MediaEvent::Loaded});
  });
  if (!id && listener_) listener_({MediaEvent::Error, 0, "Could not reach the TV media service.", -2});
  return id != 0;
}

void UmsPlayer::command(const char *method, const std::string &extra, std::function<void(bool)> done) {
  if (media_id_.empty()) {
    if (done) done(false);
    return;
  }
  std::string payload = "{\"mediaId\":" + Json::quote(media_id_) + extra + "}";
  std::string name = method;
  logf("media", "-> %s%s", method, extra.c_str());
  int generation = generation_;
  std::weak_ptr<int> alive = alive_;
  Luna::instance().call(std::string(kService) + method, payload, [this, alive, name, generation, done](const std::string &reply) {
    Json json = Json::parse(reply);
    bool ok = json["returnValue"].as_bool(true);
    if (done && !alive.expired()) done(ok);
    if (ok) return;
    // Codes and text only: replies can carry media IDs and other detail.
    logf("media", "%s failed: %lld %s", name.c_str(), json["errorCode"].as_int(), json["errorText"].as_string().c_str());
    if (alive.expired()) return;  // the player has gone
    // A refused play/pause/seek on a pipeline we still think is live means the
    // TV released it (seen after system overlays). Report it once.
    bool transport = name == "play" || name == "pause" || name == "seek";
    if (transport && generation == generation_ && listener_ && !lost_) {
      lost_ = true;
      listener_({MediaEvent::Lost, 0, name + " refused"});
    }
  });
}

void UmsPlayer::set_subtitle_position(int position, std::function<void(bool)> done) {
  command("setSubtitlePosition", ",\"position\":" + std::to_string(position), std::move(done));
}

void UmsPlayer::notify_foreground() { command("notifyForeground"); }
void UmsPlayer::notify_background() { command("notifyBackground"); }

void UmsPlayer::play() { command("play"); }
void UmsPlayer::pause() { command("pause"); }
void UmsPlayer::seek(long long position_ms) { command("seek", ",\"position\":" + std::to_string(position_ms)); }

void UmsPlayer::select_track(const std::string &type, int index) {
  command("selectTrack", ",\"type\":" + Json::quote(type) + ",\"index\":" + std::to_string(index));
}

void UmsPlayer::set_subtitles(bool enabled) {
  command("setSubtitleEnable", std::string(",\"enable\":") + (enabled ? "true" : "false"));
}

void UmsPlayer::unload() {
  generation_++;
  // Unload before dropping the subscription: uMediaServer releases a pipeline
  // whose subscriber disconnects, and a later unload then fails (seen on the TV).
  if (!media_id_.empty()) {
    logf("media", "unload mediaId=%s", media_id_.c_str());
    command("unload");
  }
  if (subscription_) Luna::instance().cancel(subscription_);
  subscription_ = 0;
  media_id_.clear();
  listener_ = nullptr;
}

static bool flag(const Json &t, const char *a, const char *b) {
  return t[a].as_bool(false) || t[b].as_bool(false);
}

static std::vector<MediaTrack> parse_tracks(const Json &list, bool audio) {
  std::vector<MediaTrack> tracks;
  for (size_t i = 0; i < list.size(); i++) {
    const Json &t = list[i];
    MediaTrack track;
    track.index = static_cast<int>(i);
    track.language = t["language"].as_string();
    track.codec = audio ? t["codec"].as_string() : t["type"].as_string();
    track.channels = audio ? static_cast<int>(t["channels"].as_int()) : 0;
    track.title = t["title"].as_string(t["name"].as_string());
    track.forced = flag(t, "forced", "isForced");
    track.hearing_impaired = flag(t, "hearingImpaired", "sdh");
    track.commentary = flag(t, "commentary", "isCommentary");
    track.is_default = flag(t, "default", "isDefault");
    // Titles often carry what the flags do not ("Commentary", "SDH", "Forced").
    std::string lower;
    for (char ch : track.title) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (lower.find("commentary") != std::string::npos) track.commentary = true;
    if (lower.find("sdh") != std::string::npos) track.hearing_impaired = true;
    if (lower.find("forced") != std::string::npos) track.forced = true;
    tracks.push_back(track);
  }
  return tracks;
}

static std::string keys_of(const Json &j) {
  std::string keys;
  for (auto &item : j.items()) keys += item.first + " ";
  return keys;
}

void UmsPlayer::on_event(const std::string &payload) {
  bool ok = false;
  Json json = Json::parse(payload, &ok);
  if (!ok || !json.is_object() || !listener_) return;
  if (!logged_keys_) {
    std::string keys;
    for (auto &item : json.items()) keys += item.first + " ";
    logf("media", "first event keys: %s", keys.c_str());
    logged_keys_ = true;
  }
  // State events are rare; log them so a TV run shows play/pause/seek/track
  // acknowledgements with timestamps. Clock and buffer reports are skipped.
  {
    std::string keys;
    for (auto &item : json.items()) {
      const std::string &k = item.first;
      if (k == "currentTime" || k == "bufferRange" || k == "sourceInfo" || k == "videoInfo" || k == "returnValue" ||
          k == "subscription" || k == "mediaId")
        continue;
      keys += k + " ";
    }
    if (!keys.empty()) logf("media", "<- %s", keys.c_str());
  }
  // Anything Bloob does not handle yet, once per name, with its value: this is how
  // a TV run tells us which events and fields this firmware really sends.
  static const std::set<std::string> known = {
      "currentTime", "bufferRange", "sourceInfo", "videoInfo", "returnValue", "subscription", "mediaId",
      "trackSelected", "loadCompleted", "playing", "paused", "bufferingStart", "bufferingEnd", "seekDone",
      "endOfStream", "unloadCompleted", "released", "suspended", "policyAction", "error"};
  for (auto &item : json.items()) {
    if (known.count(item.first) || seen_keys_.count(item.first)) continue;
    seen_keys_.insert(item.first);
    logf("media", "unhandled event %s: %s", item.first.c_str(), Json::stringify(item.second).substr(0, 300).c_str());
  }

  if (json.has("currentTime")) {
    const Json &value = json["currentTime"];
    long long t = value.is_object() ? value["currentTime"].as_int() : value.as_int();
    listener_({MediaEvent::Time, t});
  }
  if (json.has("sourceInfo")) {
    const Json &s = json["sourceInfo"];
    const Json &program = s["programInfo"][0];
    info_.container = s["container"].as_string();
    info_.seekable = s["seekable"].as_bool(true);
    info_.duration_ms = program["duration"].as_int();
    info_.audio = parse_tracks(program["audioTrackInfo"], true);
    info_.text = parse_tracks(program["subtitleTrackInfo"], false);
    const Json &video = program["videoTrackInfo"][0];
    if (video.is_object()) {
      info_.video_codec = video["codec"].as_string();
      info_.width = static_cast<int>(video["width"].as_int());
      info_.height = static_cast<int>(video["height"].as_int());
      info_.frame_rate = video["frameRate"].as_number();
    }
    // The file's default track if it says so, else the first.
    bool any_default = false;
    for (auto &t : info_.audio) any_default |= t.is_default;
    for (auto &t : info_.audio) t.selected = any_default ? t.is_default : t.index == 0;
    if (!logged_track_keys_) {
      logged_track_keys_ = true;
      if (program["audioTrackInfo"].size()) logf("media", "audio track fields: %s", keys_of(program["audioTrackInfo"][0]).c_str());
      if (program["subtitleTrackInfo"].size()) logf("media", "subtitle track fields: %s", keys_of(program["subtitleTrackInfo"][0]).c_str());
    }
    logf("media", "sourceInfo container=%s duration=%lldms audio=%zu text=%zu video=%s %dx%d@%.3f",
         info_.container.c_str(), info_.duration_ms, info_.audio.size(), info_.text.size(),
         info_.video_codec.c_str(), info_.width, info_.height, info_.frame_rate);
    listener_({MediaEvent::SourceInfo});
  }
  if (json.has("videoInfo")) {
    const Json &v = json["videoInfo"];
    if (v["width"].as_int()) info_.width = static_cast<int>(v["width"].as_int());
    if (v["height"].as_int()) info_.height = static_cast<int>(v["height"].as_int());
    if (v["frameRate"].as_number()) info_.frame_rate = v["frameRate"].as_number();
    std::string hdr = v["hdrType"].as_string();
    if (!hdr.empty()) info_.hdr = hdr;
    logf("media", "videoInfo %dx%d@%.3f hdr=%s", info_.width, info_.height, info_.frame_rate, info_.hdr.c_str());
    listener_({MediaEvent::VideoInfo});
  }
  if (json.has("trackSelected")) {
    const Json &t = json["trackSelected"];
    std::string type = t["type"].as_string();
    int index = static_cast<int>(t["index"].as_int());
    auto &list = type == "audio" ? info_.audio : info_.text;
    for (auto &track : list) track.selected = track.index == index;
    listener_({MediaEvent::TrackSelected, 0, type, index});
  }
  if (json.has("loadCompleted")) listener_({MediaEvent::Loaded});
  if (json.has("playing")) listener_({MediaEvent::Playing});
  if (json.has("paused")) listener_({MediaEvent::Paused});
  if (json.has("bufferingStart")) listener_({MediaEvent::BufferingStart});
  if (json.has("bufferingEnd")) listener_({MediaEvent::BufferingEnd});
  if (json.has("seekDone")) listener_({MediaEvent::SeekDone});
  if (json.has("endOfStream")) listener_({MediaEvent::EndOfStream});
  // The pipeline was taken away (resource conflict, system UI). Not after our own unload:
  // that drops the subscription first.
  // policyAction: the resource manager is taking the decoder for something else.
  if ((json.has("unloadCompleted") || json.has("released") || json.has("suspended") || json.has("policyAction")) && !lost_) {
    lost_ = true;
    logf("media", "pipeline released by the TV (%s)", json.has("policyAction") ? "policyAction" : "released");
    listener_({MediaEvent::Lost, 0, "released"});
  }
  if (json.has("error")) {
    const Json &e = json["error"];
    logf("media", "error %lld %s", e["errorCode"].as_int(), e["errorText"].as_string().c_str());
    listener_({MediaEvent::Error, 0, e["errorText"].as_string("Playback failed."), static_cast<int>(e["errorCode"].as_int())});
  }
}
