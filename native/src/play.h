#pragma once
// What to play, decided in one place. Screens describe the viewer's choice; these
// functions turn it into a PlayRequest. Pure: no screens, no I/O, unit-tested.
#include <map>
#include <optional>
#include <string>

#include "model.h"

namespace play {

constexpr long long kResumeMinMs = 30000;  // shorter than this: start from the beginning
constexpr int kResumeMaxPercent = 95;      // the service marks a video watched at 95%

// Where to continue `w` from, or 0 to start at the beginning.
long long resume_point(const Watch &w);
// A film from its page. `from_start` for Start over.
PlayRequest film(const Item &item, const std::map<std::string, Watch> &progress, bool from_start);
// One episode, chosen on the episode row.
PlayRequest episode(const Meta &meta, const Episode &e, const std::map<std::string, Watch> &progress, bool from_start);
// The title's main button, as the service decided it (UX-PLAN X2). Caught up: watch
// again from the first episode. Without a target (an older service, or offline):
// the local fallback below.
PlayRequest title_default(const Meta &meta, const std::map<std::string, Watch> &progress, const Target &target,
                          const std::string &today);
// Fallback: resume the latest unfinished episode, else the aired one after the
// latest finished one, else the first regular episode.
PlayRequest title_default(const Meta &meta, const std::map<std::string, Watch> &progress, const std::string &today);
// A Continue watching card (resume it, or the episode after a finished one). An
// "upcoming" card has nothing to play yet: the result has no video ID.
PlayRequest continue_item(const Resume &r);
// Up next, from the request now playing; nothing while the next episode hasn't aired.
std::optional<PlayRequest> after(const PlayRequest &current, const std::string &today);

}  // namespace play
