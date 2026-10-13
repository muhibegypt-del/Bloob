#pragma once
// Screens. Each owns its focus and draws itself; the app routes one key event
// to the top screen at a time (root screens share the navigation rail).
#include <memory>
#include <string>

#include "keys.h"
#include "model.h"

class Screen {
 public:
  virtual ~Screen() = default;
  // Returns false when the key is not used, so the app can offer it to the rail.
  virtual bool key(Key key, bool repeat) = 0;
  virtual void update(double dt) {}
  virtual void draw() = 0;
  virtual void shown() {}    // became the visible screen (again)
  virtual void hidden() {}   // another screen covers it, or the app goes to the background
  // Draws over the video plane (the window stays transparent where it draws nothing).
  virtual bool over_video() const { return false; }
  // Screens with the navigation rail. `rail_focus` is true while the rail has focus.
  virtual bool root() const { return false; }
  virtual void set_rail_focus(bool) {}
  // Ends playback before the app exits or is suspended.
  virtual void suspend() {}
  virtual const char *name() const = 0;

 protected:
  // Async callbacks capture this token; they are dropped once the screen is gone.
  std::shared_ptr<int> alive_ = std::make_shared<int>(0);
};

enum class Browse { Home, Movies, Series };

std::unique_ptr<Screen> make_browse(Browse kind);
std::unique_ptr<Screen> make_search();
std::unique_ptr<Screen> make_settings();
std::unique_ptr<Screen> make_diagnostics();
std::unique_ptr<Screen> make_details(const Item &item);
// `autostart`: resolve the recommended source straight away (up next).
std::unique_ptr<Screen> make_sources(const PlayRequest &request, bool autostart = false);
// Set by the player before it closes to return to the source list instead of leaving it.
bool &player_wants_another_source();
std::unique_ptr<Screen> make_player(const PlayRequest &request, const std::string &url, const std::string &source_label,
                                    const std::string &load_type = "media");
