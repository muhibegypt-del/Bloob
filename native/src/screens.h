#pragma once
// Screens. Each owns its focus and draws itself; the app shell routes one key
// event to one owner at a time (the tab bar, a root page, or the top screen of
// the stack) and animates pushes and Back.
#include <memory>
#include <string>

#include "keys.h"
#include "model.h"

class Screen {
 public:
  virtual ~Screen() = default;
  // Returns false when the key is not used, so the shell can offer it upwards
  // (Up/Back from a root page reaches the tab bar).
  virtual bool key(Key key, bool repeat) = 0;
  virtual void update(double dt) {}
  virtual void draw() = 0;
  virtual void shown() {}       // became the visible screen again (Back returned here)
  virtual void hidden() {}      // another screen now covers it
  // The app went to the background (Home, input switch) and came back.
  // A system overlay such as the quick settings menu is neither.
  virtual void background() {}
  virtual void foreground() {}
  // Draws over the hardware video plane: transparent where it draws nothing,
  // and never cross-faded (the video must not flash through a fading layer).
  virtual bool over_video() const { return false; }
  // Root pages sit under the tab bar.
  virtual bool root() const { return false; }
  virtual void set_tabs_focus(bool) {}
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
// `autostart`: resolve the best match straight away (up next).
std::unique_ptr<Screen> make_sources(const PlayRequest &request, bool autostart = false);
// Set by the player before it closes to return to the source list instead of leaving it.
bool &player_wants_another_source();
std::unique_ptr<Screen> make_player(const PlayRequest &request, const std::string &url, const std::string &source_label,
                                    const std::string &load_type = "media");
