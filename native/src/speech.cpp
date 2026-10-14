#include "speech.h"

#include <cstdlib>

#include "json.h"
#include "log.h"
#include "luna.h"

namespace {
constexpr long long kSettleMs = 150;  // let quick key repeats pass before speaking
}

Speech &speech() {
  static Speech instance;
  return instance;
}

void Speech::start() {
  const char *forced = std::getenv("BLOOB_AUDIO_GUIDANCE");
  if (forced && *forced == '1') {
    enabled_ = true;
    logf("speech", "audio guidance forced on");
  }
  Luna::instance().call("luna://com.webos.settingsservice/getSystemSettings",
                        "{\"category\":\"option\",\"keys\":[\"audioGuidance\"],\"subscribe\":true}", [this](const std::string &reply) {
    Json j = Json::parse(reply);
    const Json &value = j["settings"]["audioGuidance"];
    if (value.is_null()) return;  // not a reply to this (or the desktop bus)
    bool on = value.as_string() == "on";
    if (on != enabled_) logf("speech", "audio guidance %s", on ? "on" : "off");
    enabled_ = on;
    if (!on) last_.clear();
  }, true);
}

void Speech::focus(const std::string &text, long long now) {
  if (!on() || text.empty()) {
    due_ = 0;
    return;
  }
  if (text == last_) {
    pending_.clear();
    due_ = 0;
    return;
  }
  if (text != pending_) {
    pending_ = text;
    due_ = now + kSettleMs;
    return;
  }
  if (now >= due_) {
    last_ = pending_;
    pending_.clear();
    due_ = 0;
    say(last_);
  }
}

void Speech::say(const std::string &text) {
  if (!on() || text.empty()) return;
  logf("speech", "say: %s", text.c_str());
  Luna::instance().call("luna://com.webos.service.tts/speak", "{\"text\":" + Json::quote(text) + ",\"clear\":true}",
                        [this](const std::string &reply) {
    Json j = Json::parse(reply);
    if (j["returnValue"].as_bool(true)) return;
    supported_ = false;
    logf("speech", "the TV refused speech (%s); spoken focus is off for this session", j["errorText"].as_string("no reason").c_str());
  });
}
