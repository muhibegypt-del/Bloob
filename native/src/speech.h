#pragma once
// Spoken interface (G7). When LG Audio Guidance is on, the focused control is
// read out through the TV's text-to-speech service as "label, role, position"
// (for example "Play, button" or "Sintel, 3 of 12"), newest first: a change of
// focus interrupts speech that is still going.
//
// Evidence: the Audio Guidance setting (settingsservice getSystemSettings,
// category "option", key "audioGuidance") is documented for webOS TV apps. The
// speak call (com.webos.service.tts/speak with "text" and "clear") is documented
// for webOS OSE and used by web apps on TVs; it is unverified for a native app on
// this TV, so one refusal switches speech off for the session and is logged.
#include <string>

class Speech {
 public:
  // Subscribes to the Audio Guidance setting. BLOOB_AUDIO_GUIDANCE=1 forces it on
  // (desktop checks: the text is logged as "[speech] say").
  void start();
  bool on() const { return enabled_ && supported_; }
  // Called every frame with what is focused now; speaks it once it has settled.
  void focus(const std::string &text, long long now);
  // Speaks at once (announcements such as an error), interrupting.
  void say(const std::string &text);
  // When focus() wants another frame (0 = none).
  long long due() const { return due_; }

 private:
  bool enabled_ = false;
  bool supported_ = true;
  std::string last_, pending_;
  long long due_ = 0;
};

Speech &speech();
