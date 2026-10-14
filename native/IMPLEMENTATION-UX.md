# UX implementation map: responsiveness and continuity

This map turns the Elements of A UI studies into concrete changes to Bloob native. The studies are on `bloob-v1`:
- `research/Elements-of-A-UI-UX-code-study.md` (commit `32c2ebc`);
- `research/Elements-of-A-UI-responsiveness.md` (commit `59348e3`).

Every item below has six parts:
1. the reference mechanism, as **static evidence** from shipped code (not a TV trace);
2. what Bloob does today, with file and line;
3. the logic to build;
4. the code;
5. how to prove it on the desktop;
6. what to check on the TV.

**About the code:**
- It is written against `claude/sleepy-ptolemy-o87vgv` at `3fd3b02`, using the real names in `native/src`. It has **not been compiled yet**.
- Each item says how to build and test it.
- The timings are Bloob's own starting values, to tune on the TV. They are not the reference app's values.
- No reference code, assets or text are used.

## Order and dependencies

| # | Item | Depends on | Size | Why this position |
| --- | --- | --- | --- | --- |
| U0 | Bring both research notes onto this branch | — | trivial | `DESIGN.md` already cites them |
| U1 | Key-to-present latency and frame-time stats | — | S | Measure first; U8 uses it |
| U2 | One owner for "Play" | — | M | Removes four copies of the resume rule before more entry points arrive |
| U3 | Home restores focus by title, not position | — | S | Real defect after playback |
| U4 | Fetch title details while focus rests | — | S | Details opens complete |
| U5 | Season row that scrolls (APP-MAP **5.1**); different speeds for focus arriving and leaving | — | S | Real defect on long shows |
| U6 | Player timeout classes and a reduced seek mode | — | M | Controls follow what the viewer is doing |
| U7 | Subtitles move clear of the controls | U6 | S | The TV call is unverified, so it switches itself off if refused |
| U8 | Visual quality ladder and Reduce motion | U1 | M | Effects follow measured capability and preference |
| U9 | Search keyboard gives way when browsing results | — | M | Search as a workspace |

Ship them in small commits, one item each, citing the U-number and any APP-MAP increment. After each item:
- run `native/scripts/build.sh` (unit tests, cross-build, ABI guard);
- run `native/scripts/ui-tour.sh`.

---

## U0. Bring the research notes onto this branch

```sh
git checkout origin/bloob-v1 -- research/Elements-of-A-UI-UX-code-study.md research/Elements-of-A-UI-responsiveness.md
```

Then:
- drop "on the `bloob-v1` branch" from `DESIGN.md`;
- add both files to the research table in `CLAUDE.md`.

**Done when** `DESIGN.md`'s citation resolves on this branch.

---

## U1. Measure key-to-present latency and frame time

**Reference (static evidence).**
- Frame requests are coalesced, and an unchanged tree takes an idle path.
- The responsiveness note's first suggested experiment is to time a key press to the next painted frame.

**Bloob today.**
- The loop already drains every pending event, then draws once (`main.cpp`, `loop()`).
- Nothing is timed.

**Logic.**
- Stamp each key with SDL's own event time. That includes time spent in the queue.
- After `SDL_RenderPresent`, the first key still waiting gets its latency recorded. This measures **key → present returned**, not light leaving the panel; label it that way.
- Record the render cost of every frame.
- Every 5 s, if anything happened, log one line with the median (p50), 95th percentile (p95), maximum, and the count of frames over 33 ms.
- Keep the last window for Diagnostics, and for U8's automatic quality step-down.

**Code.** Add `native/src/perf.h`:

```cpp
#pragma once
// Interaction timing for the log and Diagnostics: key → present, and render cost.
// "Present" is when SDL_RenderPresent returns, not when light leaves the panel.
#include <algorithm>
#include <string>
#include <vector>

class PerfWindow {
 public:
  void key(long long at_ms) { if (!pending_key_) pending_key_ = at_ms; }
  // Call after SDL_RenderPresent. `render_ms` covers draw + present.
  void frame(long long now_ms, long long render_ms) {
    frames_.push_back(render_ms);
    if (pending_key_) {
      keys_.push_back(now_ms - pending_key_);
      pending_key_ = 0;
    }
  }
  bool due(long long now_ms) const { return now_ms - started_ >= 5000 && (!frames_.empty() || !keys_.empty()); }
  // Summarises and starts a new window.
  std::string flush(long long now_ms) {
    std::string line = "keys n=" + std::to_string(keys_.size()) + " key->present " + stats(keys_) +
                       "; frames n=" + std::to_string(frames_.size()) + " render " + stats(frames_) +
                       " slow>33ms=" + std::to_string(std::count_if(frames_.begin(), frames_.end(), [](long long v) { return v > 33; }));
    last_render_p95_ = pct(frames_, 95);
    last_line_ = line;
    keys_.clear();
    frames_.clear();
    started_ = now_ms;
    return line;
  }
  long long last_render_p95() const { return last_render_p95_; }
  const std::string &last_line() const { return last_line_; }

 private:
  static long long pct(std::vector<long long> v, int p) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, v.size() * static_cast<size_t>(p) / 100)];
  }
  static std::string stats(const std::vector<long long> &v) {
    if (v.empty()) return "-";
    return "p50 " + std::to_string(pct(v, 50)) + " p95 " + std::to_string(pct(v, 95)) + " max " +
           std::to_string(*std::max_element(v.begin(), v.end())) + " ms";
  }
  long long pending_key_ = 0, started_ = 0, last_render_p95_ = 0;
  std::vector<long long> keys_, frames_;
  std::string last_line_;
};
```

Changes in `main.cpp`:

```cpp
// member of the App class
PerfWindow perf_;

// on_key(): right after `dirty_ = true;`
perf_.key(static_cast<long long>(event.timestamp));   // SDL ticks when the key was queued

// render(): at the very top
long long render_started = static_cast<long long>(SDL_GetTicks());
// ... existing body ...
SDL_RenderPresent(c.renderer);
long long presented = static_cast<long long>(SDL_GetTicks());
perf_.frame(presented, presented - render_started);
if (perf_.due(presented)) {
  logf("perf", "%s", perf_.flush(presented).c_str());
  ctx().perf_line = perf_.last_line();          // Diagnostics shows it
  ctx().render_p95_ms = perf_.last_render_p95(); // U8 reads it
}
dirty_ = false;
```

Changes in `ui.h` `Ctx`:

```cpp
std::string perf_line;      // last 5 s of interaction timing, for Diagnostics
long long render_p95_ms = 0;
```

In `settings.cpp`, the Diagnostics page adds one line: `text(Text::Footnote, ctx().perf_line, ...)`.

**Prove it on the desktop.** In the `ui-tour.sh` logs, check that `grep '\[perf\]'` shows lines while the tour moves, and none while it is idle.

**On the TV.** Hold Right across a Home row for 5 s, then copy the `[perf]` lines into `TV-TEST.md` results. This is the baseline for U8 and for APP-MAP **22.1**.

---

## U2. One owner for "Play"

**Reference (static evidence).**
- A card sends a semantic `Play` intent upward.
- The root resolves the episode, the start position and the gates, then pushes a loading page.

**Bloob today.**
- Five places build a `PlayRequest` and push Sources themselves:
  - `browse.cpp:231` and `:246`;
  - `details.cpp:240` and `:248`;
  - `player.cpp:265`.
- The resume rule ("30 s or more, not watched") is written out four times:
  - `details.cpp:69`, `:91` and `:247`;
  - `player.cpp:210` (which adds "below 95%").
- Future gates would have to be copied into every one of them: pairing, the TV format check from audit M2, parental controls.

**Logic.**
- Split the work into **pure decisions**, which are unit-tested, and **one starter**, which applies the gates and pushes the screen.
- Screens say *what the viewer chose*. They never decide resume positions or push Sources themselves.

**Code.** Add `native/src/play.h`. It goes in `buna_core` (no SDL), so the tests can link it.

```cpp
#pragma once
// What to play, decided in one place. Screens describe the viewer's choice; these
// functions turn it into a PlayRequest. Pure: no screens, no I/O, unit-tested.
#include <map>
#include <optional>
#include <string>

#include "model.h"

namespace play {

constexpr long long kResumeMinMs = 30000;   // shorter than this: start from the beginning
constexpr int kResumeMaxPercent = 95;       // the service marks a video watched at 95%

// Where to continue `w` from, or 0 to start at the beginning.
long long resume_point(const Watch &w);
// A film from its details page. `from_start` for "Start over".
PlayRequest film(const Item &item, const std::map<std::string, Watch> &progress, bool from_start);
// One episode, chosen on the episode row.
PlayRequest episode(const Meta &meta, const Episode &e, const std::map<std::string, Watch> &progress, bool from_start);
// The title's main button: resume the latest unfinished episode, else the next one, else the first.
PlayRequest title_default(const Meta &meta, const std::map<std::string, Watch> &progress);
// A Continue watching card (resume, or the episode after a finished one).
PlayRequest continue_item(const Resume &r);
// Up next, from the request now playing.
std::optional<PlayRequest> after(const PlayRequest &current);

}  // namespace play
```

`native/src/play.cpp`. The bodies move existing logic from `details.cpp` and `browse.cpp` without changing behaviour:

```cpp
#include "play.h"

namespace play {

long long resume_point(const Watch &w) {
  if (w.watched || w.position < kResumeMinMs) return 0;
  if (w.duration > 0 && w.position >= w.duration * kResumeMaxPercent / 100) return 0;
  return w.position;
}

static void fill(PlayRequest &req, const Meta &meta, const Episode &e) {
  req.item = meta.item;
  req.video_id = e.id;
  req.season = e.season;
  req.episode = e.episode;
  req.episode_title = e.title;
  req.episode_thumbnail = e.thumbnail;
  req.next = next_episode(meta, e.id);
}

PlayRequest film(const Item &item, const std::map<std::string, Watch> &progress, bool from_start) {
  PlayRequest req;
  req.item = item;
  req.video_id = item.id;
  auto it = progress.find(item.id);
  if (!from_start && it != progress.end()) req.resume_ms = resume_point(it->second);
  return req;
}

PlayRequest episode(const Meta &meta, const Episode &e, const std::map<std::string, Watch> &progress, bool from_start) {
  PlayRequest req;
  fill(req, meta, e);
  auto it = progress.find(e.id);
  if (!from_start && it != progress.end()) req.resume_ms = resume_point(it->second);
  return req;
}

PlayRequest title_default(const Meta &meta, const std::map<std::string, Watch> &progress) {
  if (!meta.item.series()) return film(meta.item, progress, false);
  const Episode *latest = nullptr;
  long long latest_at = -1;
  for (auto &s : meta.seasons)
    for (auto &e : s.episodes) {
      auto it = progress.find(e.id);
      if (it != progress.end() && it->second.updated > latest_at) { latest = &e; latest_at = it->second.updated; }
    }
  if (latest) {
    if (resume_point(progress.at(latest->id)) > 0) return episode(meta, *latest, progress, false);
    if (auto n = next_episode(meta, latest->id))
      for (auto &s : meta.seasons)
        for (auto &e : s.episodes)
          if (e.id == n->video_id) return episode(meta, e, progress, true);
  }
  for (auto &s : meta.seasons)
    if (s.season != 0 && !s.episodes.empty()) return episode(meta, s.episodes[0], progress, true);
  PlayRequest none;
  if (!meta.seasons.empty() && !meta.seasons[0].episodes.empty()) return episode(meta, meta.seasons[0].episodes[0], progress, true);
  return none;
}

PlayRequest continue_item(const Resume &r) {
  PlayRequest req;
  req.item = r.item;
  if (r.kind == "next" && r.next) {
    req.video_id = r.next->video_id;
    req.season = r.next->season;
    req.episode = r.next->episode;
    req.episode_title = r.next->title;
  } else {
    req.video_id = r.video_id;
    req.season = r.season;
    req.episode = r.episode;
    req.episode_title = r.episode_title;
    Watch w{r.position, r.duration, false, 0};
    req.resume_ms = resume_point(w);
    req.next = r.next;
  }
  return req;
}

std::optional<PlayRequest> after(const PlayRequest &current) {
  if (!current.next) return std::nullopt;
  PlayRequest req;
  req.item = current.item;
  req.video_id = current.next->video_id;
  req.season = current.next->season;
  req.episode = current.next->episode;
  req.episode_title = current.next->title;
  return req;
}

}  // namespace play
```

The starter goes in `screens.h` and `sources.cpp`: one function, and the only caller of `make_sources`.

```cpp
// screens.h
enum class PlayOrigin { HomeHero, HomeRow, Details, Episodes, UpNext };
// The single way playback starts: gates, logging, then Choose how to watch.
void start_playback(const PlayRequest &req, PlayOrigin origin, bool replace_top = false, bool autostart = false);

// sources.cpp
void start_playback(const PlayRequest &req, PlayOrigin origin, bool replace_top, bool autostart) {
  if (req.video_id.empty()) return;
  static const char *names[] = {"home-hero", "home-row", "details", "episodes", "up-next"};
  logf("play", "intent from %s: %s resume=%lld", names[static_cast<int>(origin)], req.label().c_str(), req.resume_ms);
  // Gates belong here and nowhere else (pairing, TV format profile, household limits).
  auto screen = make_sources(req, autostart);
  if (replace_top) ctx().replace(std::move(screen));
  else ctx().push(std::move(screen));
}
```

**Call sites after the change:**
- `browse.cpp`:
  - `play_resume(r)` becomes `start_playback(play::continue_item(r), PlayOrigin::HomeRow)`.
  - The hero's "Play" for a film becomes `start_playback(play::film(shown_.item, {}, false), PlayOrigin::HomeHero)`.
- `details.cpp`:
  - `choose_start` keeps its *labels* and focus logic, but sets `start_ = play::title_default(meta_, progress_)`.
  - `press_button(1)` uses `play::film`/`play::episode` with `from_start = true`.
  - `play_episode(e)` becomes `start_playback(play::episode(meta_, e, progress_, false), PlayOrigin::Episodes)`.
- `player.cpp`:
  - `play_next()` becomes `if (auto n = play::after(req_)) { close(); start_playback(*n, PlayOrigin::UpNext, true, true); }`.
  - Line 210 becomes `if (play::resume_point({start_at_, d, false, 0}) > 0)`.
- `CMakeLists.txt`: add `src/play.cpp` to `buna_core`, and `tests/play_test.cpp` to `native_tests`.

**Tests.** Add `native/tests/play_test.cpp`, in the same style as `session_test.cpp`:
- `resume_point`: 29 s gives 0; 30 s gives 30 s; 96% gives 0; watched gives 0.
- `title_default`: the latest unfinished episode is resumed; after a finished episode the next one plays from 0; a season finale moves to the next season's first episode; specials are never chosen as the default.
- `continue_item`: "next" never carries a resume position.
- `after`: the last episode returns `nullopt`.

**Prove it.** The `ui-tour.sh` log shows exactly one `[play] intent from …` line per start, and the acceptance task still passes.

---

## U3. Home restores focus by title, not position

**Reference (static evidence).** Selection-mimic helpers match the previously selected widget **by ID**, and fall back only when it has gone.

**Bloob today.**
- `refresh_continue()` (`browse.cpp:83-111`) replaces the Continue watching items and keeps `cols_[0]` as a number.
- The service sorts by last watched, so the title you just watched jumps to the front, and your column now points at a different title.
- `build_featured()` likewise keeps `feature_` as an index.

**Logic.** Before the refresh, remember the *identity* of what is focused (and of the featured page). Afterwards, find it again. Fall back to the nearest valid position only if it has gone.

**Code** in `browse.cpp`:

```cpp
// Identity of a card: the title, plus the episode for Continue watching.
static std::string card_id(const Row &row, int col) {
  if (row.continue_row()) {
    const Resume &r = row.resume[static_cast<size_t>(col)];
    return r.item.id + "|" + r.video_id;
  }
  return row.items[static_cast<size_t>(col)].id;
}
static int find_card(const Row &row, const std::string &id, int fallback) {
  for (int i = 0; i < static_cast<int>(row.size()); i++)
    if (card_id(row, i) == id) return i;
  // The episode changed (finished → next) but the title is the same: match the title.
  std::string title = id.substr(0, id.find('|'));
  for (int i = 0; i < static_cast<int>(row.size()); i++)
    if (card_id(row, i).compare(0, title.size(), title) == 0) return i;
  return std::max(0, std::min(fallback, static_cast<int>(row.size()) - 1));
}

void refresh_continue() {
  ctx().api.get("/api/progress", [this](const ApiResult &r) {
    if (!r.ok()) return;
    // Remember what the viewer was on, by identity.
    bool on_continue = !hero_ && row_ == 0 && !rows_.empty() && rows_[0].continue_row();
    std::string was = on_continue ? card_id(rows_[0], cols_[0]) : "";
    std::string feature_was = featured_.empty() ? "" : featured_[static_cast<size_t>(feature_)].item.id;

    std::vector<Resume> items;
    for (size_t i = 0; i < r.json["items"].size(); i++) items.push_back(parse_resume(r.json["items"][i]));
    bool had = !rows_.empty() && rows_[0].continue_row();
    if (had && items.empty()) {
      rows_.erase(rows_.begin()); cols_.erase(cols_.begin()); hscroll_.erase(hscroll_.begin());
      row_ = std::max(0, row_ - 1);
    } else if (had) {
      rows_[0].resume = items;
      cols_[0] = was.empty() ? std::min<int>(cols_[0], static_cast<int>(items.size()) - 1) : find_card(rows_[0], was, cols_[0]);
      int count = static_cast<int>(items.size());
      hscroll_[0].snap(static_cast<float>(std::min(cols_[0], std::max(0, count - 4))));
    } else if (!items.empty()) {
      Row row; row.id = "continue"; row.title = "Continue watching"; row.resume = items;
      rows_.insert(rows_.begin(), row); cols_.insert(cols_.begin(), 0); hscroll_.insert(hscroll_.begin(), Anim(scroll_rate));
      if (!hero_) row_++;
    }
    vscroll_.snap(static_cast<float>(row_));
    build_featured();
    // Keep the same featured title on screen if it is still featured.
    for (int i = 0; i < static_cast<int>(featured_.size()); i++)
      if (featured_[static_cast<size_t>(i)].item.id == feature_was) feature_ = i;
    if (hero_) show_featured(true);
    else show_focused(true);
  }, alive_);
}
```

**Prove it.** Extend `ui-tour.sh` with a Continue watching row that has two items:
1. Focus the second card and play it.
2. Return to Home.
3. Snap and check that the focused card's title is the same as before.

To assert it, log `focused id` in `show_focused` and grep for it.

---

## U4. Fetch title details while focus rests

**Reference (static evidence).**
- The app prefetches the product-page route.
- `preloadContentImages` runs before the page fades in.
- `LibraryPage` delays browsing on a category change, so passing focus doesn't fire requests.

**Bloob today.**
- Details asks for `/api/meta` only when you press OK (`details.cpp:36`), so the buttons show a spinner first.
- The backdrop and logo are already warm, because they're the same images as Home.

**Logic.**
- When focus has rested on a card for the dwell time, Home already calls `apply_pending()`. That moment also asks the service for the title's details.
- The result goes into a small cache: 24 entries, 10 minutes, with requests already in flight de-duplicated.
- Details uses a cached entry at once, then refreshes **progress only**. This is stale-while-revalidate: the meta is stable, the progress is fresh.
- Passing focus never fetches, because the dwell gates it.

**Code.** `Ctx` in `ui.h`:

```cpp
// Title details fetched ahead of need (Home dwell), reused by Details.
struct MetaCache {
  struct Entry { Json json; long long at = 0; };
  std::map<std::string, Entry> entries;
  std::set<std::string> in_flight;
  const Json *get(const std::string &key, long long now) const {
    auto it = entries.find(key);
    return it != entries.end() && now - it->second.at < 10 * 60 * 1000 ? &it->second.json : nullptr;
  }
  void put(const std::string &key, Json json, long long now) {
    if (entries.size() >= 24) {
      auto oldest = entries.begin();
      for (auto it = entries.begin(); it != entries.end(); ++it) if (it->second.at < oldest->second.at) oldest = it;
      entries.erase(oldest);
    }
    entries[key] = {std::move(json), now};
  }
} meta_cache;
```

`common.h` and `common.cpp`:

```cpp
std::string meta_key(const Item &item);   // "series:tt…"
void prefetch_meta(const Item &item);

std::string meta_key(const Item &item) { return item.type + ":" + item.id; }

void prefetch_meta(const Item &item) {
  Ctx &c = ctx();
  std::string key = meta_key(item);
  if (item.id.empty() || c.meta_cache.get(key, c.now) || c.meta_cache.in_flight.count(key)) return;
  c.meta_cache.in_flight.insert(key);
  // Background priority: a later focus supersedes it in the worker queue.
  c.api.get("/api/meta?type=" + item.type + "&id=" + item.id, [key](const ApiResult &r) {
    Ctx &x = ctx();
    x.meta_cache.in_flight.erase(key);
    if (r.ok()) x.meta_cache.put(key, r.json, x.now);
  }, nullptr);
}
```

In `browse.cpp` `apply_pending()`, add after the logo prefetch:

```cpp
prefetch_meta(shown_.item);
```

In `details.cpp`, split the `load()` callback body into `apply_meta(const Json &)` and use the cache:

```cpp
explicit DetailsScreen(const Item &item) {
  meta_.item = item;
  backdrop_.set(item.backdrop, true);
  opened_at_ = ctx().now;
  if (const Json *cached = ctx().meta_cache.get(meta_key(item), ctx().now)) {
    apply_meta(*cached);   // complete on the first frame
    load_progress();       // then refresh only what changes between visits
  } else {
    load();
  }
}

void apply_meta(const Json &j) {
  Item original = meta_.item;
  meta_ = parse_meta(j["meta"]);
  if (meta_.item.description.empty()) meta_.item.description = original.description;
  progress_ = parse_progress(j["progress"]);
  loaded_ = true;
  choose_start(true);
}

void load() {
  ctx().api.get("/api/meta?type=" + meta_.item.type + "&id=" + meta_.item.id, [this](const ApiResult &r) {
    if (!r.ok()) { error_ = r.error; return; }
    ctx().meta_cache.put(meta_key(meta_.item), r.json, ctx().now);
    apply_meta(r.json);
  }, alive_);
}
```

**Check first:** `ApiResult::json` must be copyable. `Json` stores `std::map`, `std::vector` and `std::string` by value, so it is.

**Prove it.** The tour log shows Details reaching `loaded` in the same frame as `push details`: add `logf("details", "ready %s", cached ? "cached" : "fetched")`. The snapshot of Details taken 200 ms after OK shows buttons, not a spinner.

**On the TV.** Rapid Right across a row must not raise the request count. Watch `[api]` lines, or the service log.

---

## U5. Season row that scrolls; focus arrives fast and leaves gently

**Reference (static evidence).**
- Season tabs have idle, focus-candidate, focused and disabled styles.
- Focus gain and loss have *different* animation durations.
- The row stays navigable whatever the number of seasons.

**Bloob today.**
- `draw_episodes()` stops drawing pills at the screen edge (`details.cpp:325`, `if (sx + w > W - margin) break;`). On The Simpsons, only seasons 1 to 11 are visible, yet Right still moves the selection to unseen pills (audit M4).
- Lift and unlift share `focus_rate` (`details.cpp:402`, `browse.cpp:573`).

**Logic.**
- Lay out every pill on a strip. Keep a horizontal scroll value that eases, so the selected pill always sits inside the safe area, with one pill of look-ahead.
- Draw only the pills that fall on screen.
- Show a fading chevron on any side that has more.
- Add a `focus_out_rate` token (slower), used by every `prev_lift_`.

**Code.** `theme.h`:

```cpp
constexpr float focus_out_rate = 16.0f;  // ≈290 ms: focus leaves gently; it arrives in ≈140 ms
```

In `details.cpp`, add members and replace the pill block in `draw_episodes()`:

```cpp
Anim sscroll_{scroll_rate};   // season strip offset, px

// Left edge of each pill on an unscrolled strip.
std::vector<int> season_xs() const {
  std::vector<int> xs;
  int x = 0;
  for (auto &s : meta_.seasons) { xs.push_back(x); x += pill_width(s.label) + 12; }
  xs.push_back(x);  // strip end
  return xs;
}

void keep_season_visible() {
  auto xs = season_xs();
  int view = W - 2 * margin;
  size_t i = static_cast<size_t>(season_);
  int look = i + 1 < meta_.seasons.size() ? pill_width(meta_.seasons[i + 1].label) / 2 : 0;  // a glimpse of the next
  float target = sscroll_.target;
  if (xs[i] - target < 0) target = static_cast<float>(xs[i]);
  if (xs[i + 1] + look - target > view) target = static_cast<float>(xs[i + 1] + look - view);
  sscroll_.to(std::max(0.0f, std::min(target, static_cast<float>(std::max(0, xs.back() - view)))));
}

// in select_season(): add keep_season_visible();
// in update(): add sscroll_.step(dt) to the animating expression

// draw_episodes(): replaces the `if (meta_.seasons.size() > 1) { ... }` block
if (meta_.seasons.size() > 1) {
  auto xs = season_xs();
  int off = static_cast<int>(std::lround(sscroll_.value));
  for (size_t i = 0; i < meta_.seasons.size(); i++) {
    int x = margin + xs[i] - off;
    int w = xs[i + 1] - xs[i] - 12;
    if (x + w < margin - 40 || x > W - margin + 40) continue;   // off screen
    bool selected = static_cast<int>(i) == season_;
    draw_pill(x, sy, meta_.seasons[i].label, zone_ == Zone::Seasons && selected, selected);
  }
  if (off > 0) c.icons.draw(Icon::ChevronLeft, margin - 52, sy + 10, 36, tertiary);
  if (xs.back() - off > W - 2 * margin) c.icons.draw(Icon::ChevronRight, W - margin + 16, sy + 10, 36, tertiary);
}
```

Change `prev_lift_{focus_rate}` to `prev_lift_{focus_out_rate}` in `details.cpp` and `browse.cpp`.

**Prove it.** Add a tour fixture series with 30 seasons, then:
1. Down to the season row.
2. Press Right ×20 and snap.
3. Check that "Season 21" is visible and focused.
4. Press Left ×20 and check the strip returns to 0.

**On the TV.** The Simpsons: every season is reachable, and Specials is last and visible.

---

## U6. Player: timeouts follow what the viewer is doing, and a reduced seek mode

**Reference (static evidence).**
- `UnifiedPlayerWidget` has **Full, Reduced and Deck** modes.
- Its show, hide and timer methods use separate timeout classes: ordinary or automatic display; paused or focused controls; an open tab.
- Scrubbing and trick play suppress dismissal.
- Scrubbing can reduce the UI.
- The hide is a short fade.

**Bloob today.**
- One timeout (`hud_timeout_ms` = 5 s). Panels, pause, seeking and errors pin the controls (`hud_pinned()`, `player.cpp:278`).
- Resting focus on Audio or Next times out like doing nothing.
- Left/Right from a hidden screen shows the full controls.

**Logic.**

| Situation | Mode | Hides after |
| --- | --- | --- |
| Any key from hidden except Left/Right | Full | 5 s (`hud_timeout_ms`) |
| Focus on the controls row (Audio, Subtitles, Next…) | Full | 12 s (`hud_controls_timeout_ms`) |
| Left/Right from hidden (seek) | **Reduced**: timeline, bubble and time only, lighter scrim | 2 s after the seek lands (`hud_reduced_timeout_ms`) |
| Up, Down or OK while Reduced | Promote to Full | — |
| Paused, panel open, seek preview or pending, loading, error | Pinned | — |

Back in Reduced cancels a preview if there is one, otherwise it hides.

**Code.** `theme.h`:

```cpp
constexpr long long hud_controls_timeout_ms = 12000;
constexpr long long hud_reduced_timeout_ms = 2000;
```

In `player.cpp`:

```cpp
enum class HudMode { Full, Reduced };
HudMode hud_mode_ = HudMode::Full;

long long hud_timeout() const {
  if (hud_mode_ == HudMode::Reduced) return hud_reduced_timeout_ms;
  if (zone_ == Zone::Controls) return hud_controls_timeout_ms;
  return hud_timeout_ms;
}

void show_hud(HudMode mode = HudMode::Full) {
  // A reduced request never downgrades controls already shown in full.
  if (!hud_ || mode == HudMode::Full) hud_mode_ = mode;
  hud_ = true;
  hud_until_ = ctx().now + hud_timeout();
}
```

In `key()`, the hidden-HUD branch:

```cpp
if (!hud_) {
  if (key == Key::Left || key == Key::Right) {
    session_.nudge(key == Key::Right ? 1 : -1, t);
    zone_ = Zone::Timeline;
    show_hud(HudMode::Reduced);   // just the timeline while seeking from nothing
    return true;
  }
  if (key == Key::Ok && !repeat) session_.toggle();
  zone_ = key == Key::Down ? Zone::Controls : Zone::Timeline;
  control_ = 0;
  show_hud();
  return true;
}
if (hud_mode_ == HudMode::Reduced && (key == Key::Up || key == Key::Down || key == Key::Ok)) {
  show_hud(HudMode::Full);        // the viewer wants more than the timeline
  if (key != Key::Down) return true;
}
if (hud_mode_ == HudMode::Reduced && (key == Key::Left || key == Key::Right)) {
  session_.nudge(key == Key::Right ? 1 : -1, t);
  show_hud(HudMode::Reduced);
  return true;
}
```

For Back, before the existing checks: `if (hud_mode_ == HudMode::Reduced && !session_.preview_ms()) { hud_ = false; return true; }`.

In `update()`, the timeout starts counting from the landing, not from the last key:

```cpp
if (hud_mode_ == HudMode::Reduced && (session_.preview_ms() || session_.pending_ms())) hud_until_ = t + hud_reduced_timeout_ms;
```

In `draw_hud(a, phase)`, branch at the top:

```cpp
bool reduced = hud_mode_ == HudMode::Reduced;
c.gfx.gradient_v(0, H - (reduced ? 300 : 440), W, reduced ? 300 : 440, rgba(0x000000), 0, static_cast<Uint8>((reduced ? 170 : 225) * a));
if (!reduced) {
  c.gfx.gradient_v(0, 0, W, 160, rgba(0x000000), static_cast<Uint8>(120 * a), 0);
  // title, subtitle, badges (unchanged)
}
// timeline, seek bubble, times (unchanged)
if (reduced) return;   // no controls row
// controls row (unchanged)
```

**Prove it.** Two new tour steps:
1. With the controls hidden, press `key:right`, wait 120 ms, snap → only the timeline.
2. Inject `seekDone`, wait 2500 ms, snap → the controls are gone.

Plus: with focus on the Audio pill, wait 6000 ms, snap → the controls are still there.

---

## U7. Subtitles move clear of the controls (unverified TV call; switches itself off)

**Reference (static evidence).** The player calculates bottom space for subtitles from the visible controls and thumbnails.

**Bloob today.**
- uMediaServer draws embedded subtitles on the video plane, under Bloob's window.
- Bloob never moves them, so they can sit under the controls.

**Platform fact (community documentation, unverified on this TV).**
- `luna://com.webos.media/setSubtitlePosition {"mediaId", "position"}`: position runs from −3 (lowest) to 4 (highest), and 0 is the default.
- There is also `setSubtitleFontSize {"mediaId", "fontSize": 0..4}`.
- Source: https://gist.github.com/aabytt/bddbb1bcf031a050d89a89aeee3a6737.
- In `media.cpp`, a refused command only becomes `Lost` for play, pause and seek, so a refused subtitle call cannot trigger a reload.

**Logic.**
- The desired position follows the HUD:
  - hidden: 0;
  - Reduced: 2;
  - Full: 4.
- Send only on change, only while subtitles are enabled, and with a short settle time (300 ms) so a flickering HUD doesn't spam the bus.
- If the TV refuses the call once, stop trying for the rest of the session, and log it.

**Code.** In `media.h` and `media.cpp`, add an optional result callback to `command`, and two methods:

```cpp
// media.h
void set_subtitle_position(int position, std::function<void(bool ok)> done);
void command(const char *method, const std::string &extra = "", std::function<void(bool)> done = nullptr);

// media.cpp
void UmsPlayer::set_subtitle_position(int position, std::function<void(bool)> done) {
  command("setSubtitlePosition", ",\"position\":" + std::to_string(position), std::move(done));
}
// inside command()'s reply lambda, capture `done` and call it:
//   bool ok = json["returnValue"].as_bool(true);
//   if (done && !alive.expired()) done(ok);
//   if (ok) return;  ...existing failure handling...
```

In `player.cpp`:

```cpp
int sub_pos_sent_ = 0, sub_pos_want_ = 0;
long long sub_pos_since_ = 0;
bool sub_pos_supported_ = true;

// update(): after hud_anim_ is set
int want = !(hud_ || hud_pinned()) ? 0 : hud_mode_ == HudMode::Reduced ? 2 : 4;
if (want != sub_pos_want_) { sub_pos_want_ = want; sub_pos_since_ = t; }
if (sub_pos_supported_ && session_.subtitle_index() >= 0 && sub_pos_want_ != sub_pos_sent_ && t - sub_pos_since_ >= 300) {
  sub_pos_sent_ = sub_pos_want_;
  std::weak_ptr<int> alive = alive_;
  ums_.set_subtitle_position(sub_pos_sent_, [this, alive](bool ok) {
    if (alive.expired() || ok) return;
    sub_pos_supported_ = false;
    logf("player", "subtitle position not supported here; leaving subtitles where the TV puts them");
  });
} else if (sub_pos_want_ != sub_pos_sent_) {
  c.redraw_at(sub_pos_since_ + 300);
}
```

**Prove it on the desktop.** `luna_host.cpp` answers every call with `returnValue: true`. The tour log shows `-> setSubtitlePosition,"position":4` after the controls show with subtitles on, and `…0` after they hide.

**On the TV (TV-TEST §3).** With English subtitles on, press Down and watch the subtitle line rise above the pills. If the log says "not supported", record that, and keep the feature off.

---

## U8. Visual quality ladder and Reduce motion

**Reference (static evidence).**
- `app.js` has performance-level flags for:
  - focus and page animations;
  - product-card view;
  - real-time blur;
  - image placeholders;
  - slideshow advance;
  - player-position updates.
- `ProductPage` checks the blur capability and the reduced-transparency preference.
- Search and the player can make motion instant.

**Bloob today.**
- Shadows, blur, page transitions, hero auto-advance (9 s) and the 4×/s HUD clock are always on.
- The audit found the hero violates WCAG 2.2.2 (no way to stop it).

**Logic.** There are two independent switches.

1. **Reduce motion**, a user setting saved by the service. It turns off:
   - hero auto-advance;
   - page transitions (instant);
   - focus lift and scale (the ring stays);
   - backdrop crossfades (cut instead).
2. **Effects tier**, `Full` or `Lite`, automatic.
   - Bloob starts at Full.
   - After two consecutive U1 windows with render p95 above 25 ms, it drops to **Lite** for the session and logs it once.
   - Lite turns off soft shadows, swaps blurred backdrops for a plain dimmed copy, and updates the HUD clock once a second.
   - It never steps back up automatically during a session; that avoids flapping.

**Code.** `ui.h` `Ctx`:

```cpp
bool reduce_motion = false;          // user setting
enum class Effects { Full, Lite } effects = Effects::Full;
int slow_windows = 0;                // consecutive slow U1 windows
```

`main.cpp`, where U1 flushes a window:

```cpp
if (ctx().effects == Ctx::Effects::Full) {
  ctx().slow_windows = perf_.last_render_p95() > 25 ? ctx().slow_windows + 1 : 0;
  if (ctx().slow_windows >= 2) {
    ctx().effects = Ctx::Effects::Lite;
    logf("perf", "effects -> lite (render p95 %lld ms)", perf_.last_render_p95());
  }
}
```

Where it applies:

```cpp
// gfx.cpp  Gfx::shadow(): first line
if (ctx().effects == Ctx::Effects::Lite) return;   // (or pass a flag into Gfx to avoid the ctx() dependency)

// common.cpp  Backdrop::draw(dim, blur): use the sharp texture, dimmed harder, instead of the blurred one
if (ctx().effects == Ctx::Effects::Lite) blur = 0.0f, dim = std::max(dim, 0.45f);
// Backdrop::set(): cut instead of crossfade
if (ctx().reduce_motion) immediate = true;

// main.cpp  apply_ops(): wherever entering_ / page_fade_ start their animation
entering_.snap(animate && !ctx().reduce_motion ? 0.0f : 1.0f);
// select_root(): page_fade_.snap(ctx().reduce_motion ? 1.0f : 0.0f);

// browse.cpp  update(): auto-advance only when allowed
if (state_ == State::Ready && hero_ && featured_.size() > 1 && !ctx().reduce_motion) { ... }

// browse.cpp / details.cpp  focus lift: keep the ring, drop the scale
cs.focus = ctx().reduce_motion ? (focused ? 1.0f : 0.0f) : lift_.value;   // and draw_card skips scale when reduce_motion

// player.cpp  update(): the clock redraw interval
long long tick = ctx().effects == Ctx::Effects::Lite ? 1000 : 250;
else if (show) c.redraw_at((t / tick + 1) * tick);
```

Settings, through the service: `progress.js` `getSettings()`/`saveSettings()` accept `reduceMotion: boolean`.

```js
// progress.js (no ?. or ??: the TV's Node is old)
if (!settings) settings = Object.assign({ language: 'English', recentSearches: [], reduceMotion: false }, (await settingsStore.read().catch(() => null)) || {});
// saveSettings():
if (typeof input.reduceMotion === 'boolean') current.reduceMotion = input.reduceMotion;
```

```cpp
// main.cpp settings load: x.reduce_motion = r.json["reduceMotion"].as_bool(false);
// ui.cpp Ctx::save_settings(): add ",\"reduceMotion\":" + std::string(reduce_motion ? "true" : "false")
// settings.cpp: a fourth row "Reduce motion" (On/Off, Left/Right/OK toggles, then save_settings()),
//   with the value line "Stops the rotating banner and page movement". row_ max becomes 3.
// Diagnostics: show "Effects: Full|Lite" next to the U1 perf line.
```

**Tests.**
- Vitest: `saveSettings({reduceMotion: true})` round-trips, and non-booleans are ignored.
- Tour: set `reduceMotion` in the fixture's settings, then:
  1. idle for 10 s on Home;
  2. snap;
  3. check the featured title has not changed. Add a `[browse] feature` log line to grep.

**On the TV.** Read the `[perf]` lines and see whether Lite triggers in normal browsing. If it does, investigate the slowest frames before relying on Lite.

---

## U9. Search: the keyboard gives way to results

**Reference (static evidence).**
- `SearchPage` sets the keyboard beside the content.
- Left/Right moves focus between them.
- `displayKeyboard` shifts the layout when focus moves, instantly on low tiers.
- Hints, landing, results and empty are distinct states.
- The landing view is kept while obsolete results are disposed.

**Bloob today.**
- A fixed split: the keyboard at `KX=96, KW=640`; results at `RX=840` in 3 columns (`search.cpp:17-25`).
- There is no "no results" state distinct from "type something".

**Logic.**
- A value `kb_` eases from 1 (keyboard open) to 0 (browsing results).
- **Open:** today's layout.
- **Browsing:** the keyboard slides left and fades. The query stays visible as a compact chip at the top left, so the viewer never loses what they typed. Results start at `margin`, in 4 columns.
- Left from the first column, or Back, brings the keyboard back. The query and the results are kept.
- The content area has four states, each with its own copy:
  - **Landing:** recent searches, plus a hint.
  - **Waiting:** under the 450 ms debounce, or a request in flight. Keep the old results dimmed rather than blanking them.
  - **Results.**
  - **Empty:** "No titles match ‘…’. Try fewer letters." It keeps focus on the keyboard.

**Code** in `search.cpp`. Replace the constants with functions of `kb_`, and draw with an x-offset:

```cpp
Anim kb_{page_rate, 1};   // 1 = keyboard open, 0 = browsing results

int results_x() const { return static_cast<int>(lerp(static_cast<float>(RX), static_cast<float>(margin), 1.0f - kb_.value)); }
int results_cols() const { return kb_.value > 0.5f ? 3 : 4; }
int tile_w() const { int rw = W - margin - results_x(); return (rw - (results_cols() - 1) * RGAP) / results_cols(); }

// whenever zone_ changes:
void set_zone(Zone z) {
  zone_ = z;
  if (ctx().reduce_motion) kb_.snap(z == Zone::Results ? 0.0f : 1.0f);
  else kb_.to(z == Zone::Results ? 0.0f : 1.0f);
}

// draw(): keyboard block translated and faded; compact query chip when browsing
float k = kb_.value;
int kx = KX - static_cast<int>((1.0f - k) * (KW + 80));
if (k > 0.02f) { /* existing keyboard drawing, with KX replaced by kx and colours faded by k */ }
if (k < 0.98f) {
  int w = std::min(560, measure(Text::Headline, query_) + 96);
  c.gfx.fill_round(margin, TOP - 70, w, 56, 28, fade(raised, 1.0f - k));
  c.icons.draw(Icon::Search, margin + 18, TOP - 58, 30, fade(tertiary, 1.0f - k));
  text(Text::Headline, query_, margin + 60, TOP - 60, fade(primary, 1.0f - k), w - 80);
}
// draw_results(): use results_x(), results_cols(), tile_w() instead of RX, RCOLS, RTW,
// and render the four states:
//   query empty            -> landing (recent + hint)
//   due_at_ || in_flight_  -> previous results at 50% alpha, delayed spinner top-right
//   results_.empty()       -> "No titles match …" + hint
//   else                   -> grid
```

Key changes:
- Results keys use `results_cols()` instead of `RCOLS`.
- Left at column 0 calls `set_zone(Zone::Keys)`.
- Back from Results calls `set_zone(Zone::Keys)` and keeps `results_`.
- Every `zone_ =` assignment becomes `set_zone(...)`.
- `update()` steps `kb_`.

**Prove it.** Add tour snaps:
- typing ("toy"), which shows three columns with the keyboard;
- Right into the results, which shows four columns with the query chip;
- Left back;
- a nonsense query, which shows the empty state.

---

## Not doing, and why

- **Category-change delay on tab switch.** Bloob builds each root page once and keeps it (`ensure_root`), so flicking past Films or Series costs one request each, once. Not worth a delay that would slow the deliberate case.
- **Carrying trailer video into Details.** There are no trailers. Home and Details already share the same decoded backdrop texture, so there is no blank frame.
- **Episode-detail windowing (±10 around focus).** Cinemeta returns a series' full episode list in one response. Bloob already limits the expensive part: thumbnails are decoded only for visible cards, with LRU eviction.

## Where this lands in the docs

When an item ships:
- tick it here;
- update the matching APP-MAP layer;
- re-score that layer in `QUALITY.md`:
  - U1, U8 → layers 22, 2, 12;
  - U2 → 9 and 10;
  - U3, U4 → 3, 5;
  - U5 → 5;
  - U6, U7 → 7, 9;
  - U9 → 4.

`DESIGN.md` gains:
- the four-state rule (ready, loading, empty, failed) for every content surface;
- the per-screen focus maps that U3, U5, U6 and U9 implement.
