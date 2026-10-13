#include "media.h"

#include "json.h"
#include "log.h"
#include "luna.h"

static const char *kService = "luna://com.webos.media/";

bool UmsPlayer::load(const std::string &uri, const std::string &app_id, const std::string &window_id,
                     const std::string &load_type, Listener listener) {
  listener_ = std::move(listener);
  info_ = MediaInfo{};
  std::string option = "{\"appId\":" + Json::quote(app_id);
  if (!window_id.empty()) option += ",\"windowId\":" + Json::quote(window_id);
  option += "}";
  std::string payload = "{\"uri\":" + Json::quote(uri) + ",\"type\":" + Json::quote(load_type) +
                        ",\"payload\":{\"option\":" + option + "}}";
  logf("media", "load %s type=%s window=%s", redact_uri(uri).c_str(), load_type.c_str(), window_id.c_str());

  int generation = ++generation_;
  int id = Luna::instance().call(std::string(kService) + "load", payload, [this, generation](const std::string &reply) {
    bool ok = false;
    Json json = Json::parse(reply, &ok);
    if (generation != generation_) {
      // The viewer left before the pipeline answered: release it immediately.
      std::string stale = json["mediaId"].as_string();
      if (!stale.empty()) {
        logf("media", "discarding late load %s", stale.c_str());
        Luna::instance().call(std::string(kService) + "unload", "{\"mediaId\":" + Json::quote(stale) + "}", nullptr);
      }
      return;
    }
    if (!ok || !json["returnValue"].as_bool()) {
      logf("media", "load rejected: %s", reply.c_str());
      if (listener_) listener_({MediaEvent::Error, 0, json["errorText"].as_string("The TV media service refused the video."),
                                static_cast<int>(json["errorCode"].as_int(-1))});
      return;
    }
    media_id_ = json["mediaId"].as_string();
    logf("media", "loaded mediaId=%s", media_id_.c_str());
    subscription_ = Luna::instance().call(std::string(kService) + "subscribe",
                                          "{\"mediaId\":" + Json::quote(media_id_) + "}",
                                          [this](const std::string &event) { on_event(event); }, true);
    if (listener_) listener_({MediaEvent::Loaded});
  });
  if (!id && listener_) listener_({MediaEvent::Error, 0, "Could not reach the TV media service.", -2});
  return id != 0;
}

void UmsPlayer::command(const char *method, const std::string &extra) {
  if (media_id_.empty()) return;
  std::string payload = "{\"mediaId\":" + Json::quote(media_id_) + extra + "}";
  std::string name = method;
  logf("media", "-> %s%s", method, extra.c_str());
  Luna::instance().call(std::string(kService) + method, payload, [name](const std::string &reply) {
    Json json = Json::parse(reply);
    if (!json["returnValue"].as_bool(true)) logf("media", "%s failed: %s", name.c_str(), reply.c_str());
  });
}

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

static std::vector<MediaTrack> parse_tracks(const Json &list, bool audio) {
  std::vector<MediaTrack> tracks;
  for (size_t i = 0; i < list.size(); i++) {
    const Json &t = list[i];
    MediaTrack track;
    track.index = static_cast<int>(i);
    track.language = t["language"].as_string();
    track.codec = audio ? t["codec"].as_string() : t["type"].as_string();
    track.channels = audio ? static_cast<int>(t["channels"].as_int()) : 0;
    tracks.push_back(track);
  }
  return tracks;
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
    if (!info_.audio.empty()) info_.audio[0].selected = true;
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
  if (json.has("error")) {
    const Json &e = json["error"];
    logf("media", "error %lld %s", e["errorCode"].as_int(), e["errorText"].as_string().c_str());
    listener_({MediaEvent::Error, 0, e["errorText"].as_string("Playback failed."), static_cast<int>(e["errorCode"].as_int())});
  }
}
