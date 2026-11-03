// Choose how to watch. One best match, stated in plain words (what you will
// see and hear, and why it was picked), then every other version.
//
// 2.5.0: with several indexes a title has a hundred or more versions, so the
// list is structured: filter pills by resolution (with counts), then sections by
// kind of release (remux, Blu-ray encode, web download…), each version on two
// lines: what you get, then source, size and release group. Full release
// details stay on Right. After OK: pairing if needed, then a short, cancellable
// progress panel, and any question (which file, which quality) as a list.
//
// 2.8, the cinephile list: when a film comes in several cuts, an edition row
// (Theatrical, Extended, Director's Cut…) replaces the resolution pills and the
// best card follows the chosen cut, which is remembered. Sections are tiers of
// source (UHD Blu-ray Remux first); each row is one release on one line, then
// how it compares with the best of its cut; repetition folds behind "+N more";
// versions already on the owner's AllDebrid say "Ready now".
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

#include "common.h"
#include "log.h"
#include "pairing.h"
#include "screens.h"

namespace {

using namespace theme;

constexpr long long kPollMs = 700;
// 4.1 Swiss: the title column takes three grid columns, the versions the other nine.
const int LX = safe_x, LW = span_w(3);
const int RX = col_x(3), RW = span_w(9);
constexpr int ROW = 56, ROW_GAP = 0;  // one-line table rows
constexpr int PILLS_Y = 436;                  // the filter strip, under the best version
constexpr int LIST_TOP = 530;
constexpr int LIST_H = 8 * (ROW + ROW_GAP);  // the list's window
constexpr int HEADER_H = 44;                  // a section title in the list

// Source tiers, best first (the service's cinephile.js names them).
// 4.1: SDR remuxes and season packs have sections of their own.
const char *const kTiers[] = {"uhd-remux", "sdr-remux", "bd-remux", "uhd-encode", "web-4k", "bd-encode", "web-hd", "pack", "other"};
constexpr int kTierCount = static_cast<int>(sizeof(kTiers) / sizeof(kTiers[0]));
int tier_index(const std::string &tier) {
  for (int k = 0; k < kTierCount; k++)
    if (tier == kTiers[k]) return k;
  return kTierCount;  // a service without tiers
}
bool is_more(const SourceOption &o) { return o.id.compare(0, 5, "more:") == 0; }

std::string source_words(const std::string &s) {
  if (s == "REMUX") return "Remux";
  if (s == "BluRay") return "Blu-ray";
  if (s == "WEB-DL") return "Web download";
  if (s == "WEBRip") return "Web rip";
  if (s == "HDTV") return "TV recording";
  return s;
}

// Resolution filters; a filter with no versions is not shown.
// 4.1 (owner): SDR remuxes and season packs have tabs of their own; the
// resolution tabs list single files, so a pack is only under Season packs.
enum class Filter { All, UHD, FullHD, Lower, SdrRemux, Packs, Problems };
bool passes(Filter f, const SourceOption &o, bool problem) {
  switch (f) {
    case Filter::All: return true;
    case Filter::UHD: return !problem && !o.pack && o.resolution == "4K";
    case Filter::FullHD: return !problem && !o.pack && o.resolution == "1080p";
    case Filter::Lower: return !problem && !o.pack && o.resolution != "4K" && o.resolution != "1080p";
    case Filter::SdrRemux: return !problem && o.tier == "sdr-remux";
    case Filter::Packs: return !problem && o.pack;
    case Filter::Problems: return problem;
  }
  return true;
}
const char *filter_name(Filter f) {
  switch (f) {
    case Filter::All: return "All";
    case Filter::UHD: return "4K";
    case Filter::FullHD: return "1080p";
    case Filter::Lower: return "720p and below";
    case Filter::SdrRemux: return "SDR Remux";
    case Filter::Packs: return "Season packs";
    case Filter::Problems: return "May not play well";
  }
  return "";
}

// Plain words for a version's picture and how easy it is to get.
std::string picture_words(const SourceOption &o) {
  if (o.picture == "SDR" || o.picture.empty()) return "Standard picture";
  return o.picture;
}

struct Availability {
  std::string words;
  Color dot;
};


class SourcesScreen : public Screen {
 public:
  SourcesScreen(const PlayRequest &request, bool autostart) : req_(request), autostart_(autostart) {
    backdrop_.set(request.item.backdrop, true);
    load();
    refresh_account();
  }

  const char *name() const override { return "sources"; }
  void shell_glass(const GlassSpec &g) const override { backdrop_.glass(g); }

  std::string spoken() const override {
    if (pairing_.active()) return pairing_.spoken();
    switch (modal_) {
      case Modal::Resolving:
        return "Starting " + chosen_.headline + ". " + step_label_ + (sheet_focus_ == 0 ? " Cancel, button" : " Other versions, button");
      case Modal::ChooseFile:
      case Modal::ChooseQuality: {
        if (choices_.empty()) return "";
        const Choice &c = choices_[static_cast<size_t>(choice_)];
        return std::string(modal_ == Modal::ChooseFile ? "Which file? " : "Which quality? ") + c.label + ", " + c.detail + ", " +
               position(choice_, static_cast<int>(choices_.size()));
      }
      case Modal::Failed:
        return "That version didn't work. " + failure_ + ". " + (fail_focus_ == 0 ? "Try the next best" : "Try again") + ", button";
      case Modal::None:
        break;
    }
    if (state_ == State::Loading) return "Finding versions";
    if (state_ != State::Ready) return state_ == State::Empty ? "Nothing to play yet" : error_;
    if (pills_focused_ && !pills_.empty()) {
      const Pill &p = pills_[static_cast<size_t>(pill_)];
      return std::string("Show ") + filter_name(p.filter) + ", " + std::to_string(p.count) + " versions, " +
             position(pill_, static_cast<int>(pills_.size()));
    }
    const SourceOption &o = options_[static_cast<size_t>(focus_index_)];
    std::string head = focus_index_ == 0 && has_recommended() ? "Best version, " : "";
    for (size_t i = 1; i < rows_.size(); i++)
      if (rows_[i].option == focus_index_ && rows_[i - 1].option < 0) head = rows_[i - 1].label + ". ";
    return head + o.headline + ", " + picture_words(o) + ", " + o.audio + ", " + position(focus_index_, static_cast<int>(options_.size()));
  }

  // A glass sheet over this screen's artwork (solid while the frost bakes).
  void sheet(int x, int y, int w, int h, float a) const {
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.alpha = a;
    backdrop_.glass(g);
  }

  void shown() override {
    if (leave_after_play_) {
      leave_after_play_ = false;
      // Back from playback returns to where the viewer came from, unless they
      // asked the player for another source.
      if (!choose_another()) {
        ctx().pop();
        return;
      }
      choose_another() = false;
      direct_ = false;
    }
    refresh_account();
  }

  // Set by the player when the viewer asks for another source.
  static bool &choose_another() {
    static bool flag = false;
    return flag;
  }

  // ---------------------------------------------------------------- data

  void load() {
    state_ = State::Loading;
    loading_since_ = ctx().now;
    std::string body = "{\"type\":" + Json::quote(req_.item.type) + ",\"videoId\":" + Json::quote(req_.video_id) +
                       ",\"language\":" + Json::quote(ctx().language);
    // The service judges bitrate from size over running time.
    if (int minutes = runtime_minutes(req_.item.runtime)) body += ",\"runtimeMinutes\":" + std::to_string(minutes);
    body += "}";
    ctx().api.post("/api/sources/ranked", body, [this](const ApiResult &r) {
      if (!r.ok()) {
        state_ = State::Error;
        error_ = r.error;
        autostart_ = false;
        return;
      }
      ranking_ = parse_ranking(r.json);
      all_ = ranking_;
      expanded_.clear();
      cut_ = initial_cut();
      filter_ = Filter::All;
      pills_focused_ = false;
      arrange();
      state_ = options_.empty() ? State::Empty : State::Ready;
      focus_index_ = 0;
      lift_.snap(0);
      lift_.to(1);
      logf("sources", "%d sources, recommended=%d", ranking_.total, ranking_.recommended ? 1 : 0);
      if (autostart_) {
        autostart_ = false;
        // The version picked on the title page, else the recommended one.
        int pick = ranking_.recommended ? 0 : -1;
        for (size_t i = 0; i < options_.size(); i++)
          if (!req_.candidate.empty() && options_[i].id == req_.candidate) pick = static_cast<int>(i);
        if (pick >= 0) {
          // Straight from the title page: only the progress shows, and
          // cancelling goes back there.
          direct_ = true;
          focus_index_ = pick;
          choose(pick);
        }
      }
    }, alive_);
  }

  bool problem(size_t i) const { return section_of_[i] == "problems"; }

  // The cut the list opens on: the one last chosen, else Theatrical, else the first.
  std::string initial_cut() const {
    if (all_.cuts.size() < 2) return "";
    for (auto &c : all_.cuts)
      if (c.first == ctx().preferred_cut) return c.first;
    for (auto &c : all_.cuts)
      if (c.first == "Theatrical") return c.first;
    return all_.cuts[0].first;
  }
  bool in_cut(const SourceOption &o) const {
    return all_.cuts.size() < 2 || (o.cut.empty() ? std::string("Theatrical") : o.cut) == cut_;
  }

  // The list for the chosen cut: its best version first (the recommended one
  // if it is of this cut, else the highest-scored playable one), then the rest
  // in the service's order, then a "+N more" row for each tier with folded versions.
  void arrange() {
    ranking_ = all_;
    std::vector<std::pair<SourceOption, std::string>> pool;
    if (all_.recommended) pool.push_back({*all_.recommended, "best"});
    for (auto &g : all_.groups)
      for (auto &o : g.items) pool.push_back({o, g.kind});
    int best = -1;
    for (size_t i = 0; i < pool.size() && best < 0; i++)
      if (pool[i].second == "best" && in_cut(pool[i].first)) best = static_cast<int>(i);
    if (best < 0)
      for (size_t i = 0; i < pool.size(); i++) {
        const SourceOption &o = pool[i].first;
        if (in_cut(o) && !o.excluded && pool[i].second != "problems" && (best < 0 || o.score > pool[static_cast<size_t>(best)].first.score))
          best = static_cast<int>(i);
      }
    ranking_.recommended.reset();
    if (best >= 0) {
      ranking_.recommended = pool[static_cast<size_t>(best)].first;
      if (pool[static_cast<size_t>(best)].second != "best") ranking_.why = "The best " + cut_ + " version on offer.";
    }
    options_.clear();
    section_of_.clear();
    if (ranking_.recommended) {
      options_.push_back(*ranking_.recommended);
      section_of_.push_back("best");
    }
    std::map<std::string, int> folded;
    for (size_t i = 0; i < pool.size(); i++) {
      if (static_cast<int>(i) == best || !in_cut(pool[i].first) || pool[i].second == "best") continue;
      options_.push_back(pool[i].first);
      section_of_.push_back(pool[i].second);
      if (folds(options_.size() - 1)) folded[pool[i].first.tier]++;
    }
    for (auto &f : folded) {
      SourceOption more;
      more.id = "more:" + f.first;
      more.tier = f.first;
      bool one = f.second == 1;
      more.headline = "+" + std::to_string(f.second) + (f.first == "other" ? (one ? " lower-quality version" : " lower-quality versions") : (one ? " more like this" : " more like these"));
      options_.push_back(more);
      section_of_.push_back("more");
    }
    build_pills();
    build_rows();
  }

  // Folded behind "+N more" until that row is opened.
  bool folds(size_t i) const {
    return options_[i].collapsed && section_of_[i] == "others" && !expanded_.count(options_[i].tier);
  }

  // The edition row when there are several cuts, else one pill per resolution
  // that has versions, with its count. Focus stays where it was.
  void build_pills() {
    pills_.clear();
    if (all_.cuts.size() > 1) {
      for (auto &c : all_.cuts) pills_.push_back({Filter::All, c.second, c.first});
      // 4.1: the SDR Remux and Season packs tabs follow the editions (within the cut shown).
      size_t first_cut = ranking_.recommended ? 1 : 0;
      for (Filter f : {Filter::SdrRemux, Filter::Packs}) {
        int count = 0;
        for (size_t i = first_cut; i < options_.size(); i++) count += section_of_[i] != "more" && passes(f, options_[i], problem(i)) ? 1 : 0;
        if (count) pills_.push_back({f, count, ""});
      }
      pill_ = 0;
      for (size_t i = 0; i < pills_.size(); i++)
        if (filter_ == Filter::All ? pills_[i].cut == cut_ : pills_[i].filter == filter_ && pills_[i].cut.empty()) pill_ = static_cast<int>(i);
      return;
    }
    size_t first = ranking_.recommended ? 1 : 0;
    for (Filter f : {Filter::All, Filter::UHD, Filter::FullHD, Filter::Lower, Filter::SdrRemux, Filter::Packs, Filter::Problems}) {
      int count = 0;
      for (size_t i = first; i < options_.size(); i++) count += section_of_[i] != "more" && passes(f, options_[i], problem(i)) ? 1 : 0;
      if (count || f == Filter::All) pills_.push_back({f, count, ""});
    }
    // Nothing to choose between: no strip.
    if (pills_.size() <= 2 && (pills_.size() < 2 || pills_[1].count == pills_[0].count)) pills_.clear();
    pill_ = 0;
    for (size_t i = 0; i < pills_.size(); i++)
      if (pills_[i].filter == filter_) pill_ = static_cast<int>(i);
  }

  // The list for the current filter: versions that continue the show's last
  // episode first, then a section per source tier (each ending with its
  // "+N more" row), then those likely to fail. The service's order holds within.
  void build_rows() {
    rows_.clear();
    int y = 0;
    size_t first = ranking_.recommended ? 1 : 0;
    auto section = [&](const std::string &title, const std::string &kind, const std::vector<int> &items, int count) {
      if (items.empty()) return;
      rows_.push_back({-1, title + " \xC2\xB7 " + std::to_string(count), kind, y});
      y += HEADER_H;
      for (int i : items) {
        rows_.push_back({i, "", kind, y});
        y += ROW + ROW_GAP;
      }
    };
    std::vector<int> continuing, problems;
    std::vector<std::vector<int>> tiers(kTierCount + 1);
    std::vector<int> counts(kTierCount + 1, 0);
    std::vector<std::string> titles(kTierCount + 1, "Other versions");
    std::map<std::string, int> more_row;
    for (size_t i = first; i < options_.size(); i++) {
      if (section_of_[i] == "more") {
        more_row[options_[i].tier] = static_cast<int>(i);
        continue;
      }
      if (!passes(filter_, options_[i], problem(i))) continue;
      if (section_of_[i] == "continue") continuing.push_back(static_cast<int>(i));
      else if (problem(i)) problems.push_back(static_cast<int>(i));
      else {
        int t = tier_index(options_[i].tier);
        counts[static_cast<size_t>(t)]++;
        if (!options_[i].tier_label.empty()) titles[static_cast<size_t>(t)] = options_[i].tier_label;
        if (!folds(i)) tiers[static_cast<size_t>(t)].push_back(static_cast<int>(i));
      }
    }
    for (auto &g : ranking_.groups)
      if (g.kind == "continue") section(g.label, "continue", continuing, static_cast<int>(continuing.size()));
    for (int t = 0; t <= kTierCount; t++) {
      auto &items = tiers[static_cast<size_t>(t)];
      auto more = more_row.find(t < kTierCount ? kTiers[t] : "");
      if (more != more_row.end()) items.push_back(more->second);
      section(titles[static_cast<size_t>(t)], "others", items, counts[static_cast<size_t>(t)]);
    }
    section("May not play well on this TV", "problems", problems, static_cast<int>(problems.size()));
    list_h_ = y;
    scroll_.snap(0);
  }

  // Versions in list order (for Up and Down), the best version first.
  std::vector<int> order() const {
    std::vector<int> list;
    if (has_recommended()) list.push_back(0);
    for (auto &r : rows_)
      if (r.option >= 0) list.push_back(r.option);
    return list;
  }

  void set_filter(int index) {
    if (pills_.empty()) return;
    pill_ = std::max(0, std::min(static_cast<int>(pills_.size()) - 1, index));
    if (!pills_[static_cast<size_t>(pill_)].cut.empty()) {
      cut_ = pills_[static_cast<size_t>(pill_)].cut;
      filter_ = Filter::All;  // an edition shows all its versions
      arrange();  // the best card and the list follow the cut
      focus_index_ = 0;
    } else {
      filter_ = pills_[static_cast<size_t>(pill_)].filter;
      build_rows();
    }
    lift_.snap(0);
    lift_.to(1);
  }

  void focus_pills() {
    pills_focused_ = true;
    details_ = false;
    lift_.snap(0);
    lift_.to(1);
  }

  void refresh_account() {
    ctx().api.get("/api/account", [this](const ApiResult &r) {
      if (!r.ok()) return;
      ctx().paired = r.json["paired"].as_bool();
      account_known_ = true;
      durable_ = r.json["durable"].as_bool();
    }, alive_);
  }

  bool has_recommended() const { return ranking_.recommended.has_value(); }

  // ---------------------------------------------------------------- resolving

  // What the audio bridge said for the resolved file (3.0); empty: play the file itself.
  struct Bridge {
    std::string url, note;
    long long duration_ms = 0;
  };

  // The resolved version, into the player.
  void launch(const Json &j, const Bridge &bridge) {
    std::string url = j["url"].as_string();
    modal_ = Modal::None;
    logf("sources", "resolved (%s)", redact_uri(url).c_str());
    leave_after_play_ = true;
    choose_another() = false;
    PlayRequest played = req_;
    played.candidate = chosen_.id;  // the edition, for progress and the show's next episode
    PlaySourceInfo info;
    info.headline = chosen_.headline;
    info.release = chosen_.release;
    info.source = chosen_.source;
    info.group = chosen_.group;
    info.found_by = chosen_.sources;
    info.file_name = j["fileName"].as_string();
    info.file_bytes = j["size"].as_int(0);
    info.dolby_vision = chosen_.dolby_vision;
    info.bridge_url = bridge.url;
    info.bridge_note = bridge.note;
    info.bridge_duration_ms = bridge.duration_ms;
    ctx().push(make_player(played, url, info));
  }

  void choose(int index) {
    if (index < 0 || index >= static_cast<int>(options_.size())) return;
    if (is_more(options_[static_cast<size_t>(index)])) {  // "+N more": unfold that tier, focus its first new row
      std::string tier = options_[static_cast<size_t>(index)].tier;
      expanded_.insert(tier);
      arrange();
      for (auto &r : rows_)
        if (r.option >= 0 && options_[static_cast<size_t>(r.option)].tier == tier && options_[static_cast<size_t>(r.option)].collapsed) {
          move_to(r.option);
          break;
        }
      return;
    }
    chosen_ = options_[static_cast<size_t>(index)];
    // The cut chosen here is where the next film's list opens.
    if (all_.cuts.size() > 1 && cut_ != ctx().preferred_cut) {
      ctx().preferred_cut = cut_;
      ctx().save_settings();
    }
    // Picking another version of a show by hand: its next episodes keep to it.
    if (!direct_ && req_.item.series() && !(index == 0 && has_recommended()))
      ctx().api.post("/api/intent", "{\"id\":" + Json::quote(req_.item.id) + ",\"candidate\":" + Json::quote(chosen_.id) + "}");
    if (!ctx().paired) {
      pairing_.start([this](bool paired) {
        if (paired) start_job();
      });
      return;
    }
    start_job();
  }

  void start_job() {
    modal_ = Modal::Resolving;
    sheet_focus_ = 0;
    modal_since_ = ctx().now;
    details_ = false;
    job_id_.clear();
    step_ = "starting";
    step_label_ = "Starting\xE2\x80\xA6";
    fraction_ = -1;
    saw_caching_ = false;
    // Up next already started this one during the credits: carry on with that job.
    Ctx::Prepared &prep = ctx().prepared_next;
    if (!prep.job_id.empty() && prep.video_id == req_.video_id && chosen_.headline == prep.headline &&
        ctx().now - prep.at < 10 * 60 * 1000) {
      job_id_ = prep.job_id;
      prep = Ctx::Prepared{};
      logf("sources", "continuing the job prepared during the credits");
      poll();
      return;
    }
    std::string body = "{\"candidate\":" + Json::quote(chosen_.id);
    if (req_.item.series()) body += ",\"season\":" + std::to_string(req_.season) + ",\"episode\":" + std::to_string(req_.episode);
    body += "}";
    ctx().api.post("/api/resolve/start", body, [this](const ApiResult &r) { on_job(r); }, alive_);
  }

  void on_job(const ApiResult &r) {
    if (modal_ != Modal::Resolving && modal_ != Modal::ChooseFile && modal_ != Modal::ChooseQuality) return;
    if (!r.ok()) {
      if (r.status == 401) {  // the service lost or never had the pairing
        ctx().paired = false;
        modal_ = Modal::None;
        pairing_.start([this](bool paired) { if (paired) start_job(); });
        return;
      }
      fail(r.error);
      return;
    }
    const Json &j = r.json;
    job_id_ = j["id"].as_string(job_id_);
    std::string state = j["state"].as_string();
    step_ = j["step"].as_string(step_);
    step_label_ = j["label"].as_string(step_label_);
    fraction_ = j["fraction"].is_null() ? -1.0 : j["fraction"].as_number(-1);
    if (step_ == "caching") saw_caching_ = true;
    next_poll_ = ctx().now + kPollMs;
    if (state == "ready") {
      // 3.0: Bloob's engine plays every sound; if it can't, the player falls back
      // to LG's player and asks for the audio bridge itself.
      launch(j, Bridge{});
    } else if (state == "error") {
      fail(j["error"].as_string("Could not get this source."));
    } else if (state == "choose-file") {
      modal_ = Modal::ChooseFile;
      choices_.clear();
      for (size_t i = 0; i < j["files"].size(); i++)
        choices_.push_back({std::to_string(j["files"][i]["index"].as_int()), j["files"][i]["name"].as_string(), j["files"][i]["size"].as_string()});
      choice_ = 0;
    } else if (state == "choose-quality") {
      modal_ = Modal::ChooseQuality;
      choices_.clear();
      for (size_t i = 0; i < j["qualities"].size(); i++)
        choices_.push_back({j["qualities"][i]["id"].as_string(), j["qualities"][i]["label"].as_string(), ""});
      choice_ = 0;
    }
  }

  void poll() {
    if (job_id_.empty()) return;
    next_poll_ = ctx().now + 60000;  // until the reply arrives
    ctx().api.get("/api/resolve/status?id=" + job_id_, [this](const ApiResult &r) { on_job(r); }, alive_);
  }

  void answer(int index) {
    const Choice &c = choices_[static_cast<size_t>(index)];
    std::string field = modal_ == Modal::ChooseFile ? "\"file\":" + c.id : "\"quality\":" + Json::quote(c.id);
    modal_ = Modal::Resolving;
    step_ = "unlocking";
    step_label_ = "Getting the stream\xE2\x80\xA6";
    ctx().api.post("/api/resolve/choose", "{\"id\":" + Json::quote(job_id_) + "," + field + "}", [this](const ApiResult &r) { on_job(r); }, alive_);
  }

  void cancel_job() {
    if (!job_id_.empty()) ctx().api.post("/api/resolve/cancel", "{\"id\":" + Json::quote(job_id_) + "}");
    job_id_.clear();
    modal_ = Modal::None;
    if (direct_) ctx().pop();  // back to the title page it was started from
  }

  // From the starting sheet: stop this one and show every version instead.
  bool can_show_others() const { return direct_ && options_.size() > 1; }
  void show_others() {
    if (!job_id_.empty()) ctx().api.post("/api/resolve/cancel", "{\"id\":" + Json::quote(job_id_) + "}");
    job_id_.clear();
    modal_ = Modal::None;
    direct_ = false;
    sheet_focus_ = 0;
  }

  // Started from the title page and still on the way: only the progress shows.
  bool quiet() const { return (autostart_ && state_ == State::Loading) || (direct_ && (modal_ == Modal::Resolving || pairing_.active())); }

  void fail(const std::string &message) {
    if (!job_id_.empty()) ctx().api.post("/api/resolve/cancel", "{\"id\":" + Json::quote(job_id_) + "}");
    job_id_.clear();
    modal_ = Modal::Failed;
    failure_ = message;
    fail_focus_ = 0;
    logf("sources", "resolve failed: %s", message.c_str());
  }

  // ---------------------------------------------------------------- input

  bool key(Key key, bool repeat) override {
    if (pairing_.active()) return pairing_.key(key);
    switch (modal_) {
      case Modal::Resolving:
        if ((key == Key::Left || key == Key::Right) && can_show_others()) sheet_focus_ = 1 - sheet_focus_;
        if (key == Key::Back || (key == Key::Ok && sheet_focus_ == 0)) cancel_job();
        else if (key == Key::Ok && !repeat) show_others();
        return true;
      case Modal::ChooseFile:
      case Modal::ChooseQuality: {
        int n = static_cast<int>(choices_.size());
        if (key == Key::Up) choice_ = std::max(0, choice_ - 1);
        if (key == Key::Down) choice_ = std::min(n - 1, choice_ + 1);
        if (key == Key::Ok && !repeat && n) answer(choice_);
        if (key == Key::Back) cancel_job();
        return true;
      }
      case Modal::Failed:
        if (key == Key::Left || key == Key::Right) fail_focus_ = 1 - fail_focus_;
        if (key == Key::Ok && !repeat) {
          modal_ = Modal::None;
          if (fail_focus_ == 1) start_job();
          else if (int next = next_in_order(); next >= 0) {
            move_to(next);  // the next best version, straight away
            choose(focus_index_);
          }
        }
        if (key == Key::Back) {
          modal_ = Modal::None;
          direct_ = false;  // a failure shows the other versions
        }
        return true;
      case Modal::None:
        break;
    }
    if (state_ != State::Ready) {
      if (key == Key::Back) ctx().pop();
      else if (key == Key::Ok && state_ != State::Loading) load();
      return true;
    }
    std::vector<int> list = order();
    int at = 0;
    for (size_t i = 0; i < list.size(); i++)
      if (list[i] == focus_index_) at = static_cast<int>(i);
    int first_row = has_recommended() ? 1 : 0;
    if (pills_focused_) {
      switch (key) {
        case Key::Left: set_filter(pill_ - 1); return true;
        case Key::Right: set_filter(pill_ + 1); return true;
        case Key::Up:
          if (has_recommended()) {
            pills_focused_ = false;
            move_to(0);
          }
          return true;
        case Key::Down:
        case Key::Ok:
          if (static_cast<int>(list.size()) > first_row) {
            pills_focused_ = false;
            move_to(list[static_cast<size_t>(first_row)]);
          }
          return true;
        case Key::Back: ctx().pop(); return true;
        default: return true;
      }
    }
    switch (key) {
      case Key::Up:
        // The list's first version goes up to the filters, the filters to the best version.
        if (!pills_.empty() && at == first_row) focus_pills();
        else if (at > 0) move_to(list[static_cast<size_t>(at - 1)]);
        return true;
      case Key::Down:
        if (!pills_.empty() && has_recommended() && at == 0) focus_pills();
        else if (at + 1 < static_cast<int>(list.size())) move_to(list[static_cast<size_t>(at + 1)]);
        return true;
      case Key::Right:
        details_ = true;  // release details, on request
        return true;
      case Key::Left:
        details_ = false;
        return true;
      case Key::Ok:
      case Key::Play:
      case Key::PlayPause:
        if (!repeat) choose(focus_index_);
        return true;
      case Key::Back:
        if (details_) details_ = false;
        else ctx().pop();
        return true;
      default:
        return true;
    }
  }

  // The version after the focused one in list order, or -1.
  int next_in_order() const {
    std::vector<int> list = order();
    for (size_t i = 0; i + 1 < list.size(); i++)
      if (list[i] == focus_index_) return list[i + 1];
    return -1;
  }

  void move_to(int index) {
    prev_index_ = focus_index_;
    prev_lift_.snap(lift_.value);
    prev_lift_.to(0);
    focus_index_ = index;
    lift_.snap(0);
    lift_.to(1);
    // Keep the focused version, and its section title when it's the first, in view.
    float target = scroll_.target;
    for (size_t i = 0; i < rows_.size(); i++) {
      if (rows_[i].option != index) continue;
      int top = rows_[i].y, bottom = top + ROW;
      if (i > 0 && rows_[i - 1].option < 0) top = rows_[i - 1].y;
      if (top < target) target = static_cast<float>(top);
      if (bottom > target + LIST_H) target = static_cast<float>(bottom - LIST_H);
    }
    if (index == 0 && has_recommended()) target = 0;
    scroll_.to(target);
  }

  // ---------------------------------------------------------------- frame

  void update(double dt) override {
    Ctx &c = ctx();
    pairing_.update();
    backdrop_.update(dt);
    if (modal_ == Modal::Resolving && !job_id_.empty() && c.now >= next_poll_) poll();
    if (modal_ == Modal::Resolving) c.redraw_at(next_poll_);
    bool moving = lift_.step(dt) | prev_lift_.step(dt) | scroll_.step(dt);
    details_anim_.to(details_ ? 1.0f : 0.0f);
    moving |= details_anim_.step(dt);
    // Pairing given up on the way from the title page: go back there.
    if (direct_ && modal_ == Modal::None && !pairing_.active() && !leave_after_play_) {
      direct_ = false;
      ctx().pop();
      return;
    }
    modal_anim_.to(modal_ != Modal::None || pairing_.active() ? 1.0f : 0.0f);
    moving |= modal_anim_.step(dt);
    if (moving) c.animating = true;
  }

  void draw() override {
    backdrop_.draw(0.4f, 1.0f);
    float sc = scrim_for(backdrop_.luma());
    draw_scrims(std::max(0.6f, sc), 600, 0.8f);
    if (quiet()) {
      if (modal_ == Modal::None && !pairing_.active()) draw_spinner(W / 2, H / 2, 56, label(Label::Secondary), loading_since_);
      draw_modal();
      pairing_.draw();
      return;
    }
    draw_title_column();
    switch (state_) {
      case State::Loading:
        draw_spinner(RX + RW / 2, 420, 56, label(Label::Secondary), loading_since_);
        if (ctx().now - loading_since_ > spinner_delay_ms) text_center(Text::Callout, "Finding versions\xE2\x80\xA6", RX + RW / 2, 480, label(Label::Secondary));
        break;
      case State::Error:
      case State::Empty: {
        text(Text::Title3, state_ == State::Empty ? "Nothing to play yet" : "Couldn\xE2\x80\x99t find versions", RX, 140, label(Label::Primary));
        paragraph(Text::Body, state_ == State::Empty ? "No versions of this are available right now. Try again later." : error_, RX, 210,
                  label(Label::Secondary), RW, 3);
        GlassButton b;
        b.label = "Try again";
        b.focused = true;
        b.lift = lift_.value;
        draw_glass_button(RX, 340, b);
        break;
      }
      case State::Ready:
        draw_options();
        break;
    }
    draw_details();
    draw_modal();
    pairing_.draw();
  }

  // 4.1 Swiss title column: the logo, what is being chosen, then the count of
  // versions as a big number on a white rule, and AllDebrid's state at the foot.
  void draw_title_column() {
    Ctx &c = ctx();
    int y = draw_title(req_.item, LX, 120, LW, 130) + 28;
    if (req_.item.series()) {
      std::string ep = episode_code(req_.season, req_.episode);
      int w = text(Text::Caption1Semi, ep, LX, y, label(Label::Primary));
      if (!req_.episode_title.empty()) text(Text::Caption1Semi, req_.episode_title, LX + w + 14, y, label(Label::Primary), LW - w - 14);
    } else {
      text(Text::Caption1, meta_line(req_.item), LX, y, label(Label::Secondary), LW);
    }
    y += line_for(Text::Caption1Semi) + 6;
    text(Text::Caption1, "Choose how to watch", LX, y, label(Label::Secondary), LW);
    if (req_.resume_ms > 0) text(Text::Caption1, "Continues from " + clock_text(req_.resume_ms), LX, y + 34, label(Label::Primary), LW);
    if (state_ == State::Ready && ranking_.total > 0) {
      int ready = 0;
      for (auto &o : options_) ready += o.ready == 1 ? 1 : 0;
      if (ranking_.recommended && ranking_.recommended->ready == 1) ready++;
      int ry = 560;
      c.gfx.fill_rect(LX, ry, LW, rule, rgba(0xffffff));
      c.fonts.bold.draw(std::to_string(ranking_.total), LX, ry + 14, 88, rgba(0xffffff));
      text(Text::Caption1, ready > 0 ? "versions found, " + std::to_string(ready) + " ready" : "versions found", LX, ry + 118, label(Label::Secondary), LW);
    }
    // AllDebrid connection, quietly.
    int ay = H - safe_y - 52;
    bool paired = c.paired;
    c.gfx.fill_rect(LX, ay + 10, 12, 12, !account_known_ ? label(Label::Tertiary) : paired ? rgba(0xffffff) : theme::signal);
    std::string status = !account_known_ ? "Checking AllDebrid\xE2\x80\xA6"
                         : paired ? (durable_ ? "AllDebrid connected" : "AllDebrid connected until restart")
                                  : "AllDebrid not connected. You\xE2\x80\x99ll pair once when you press Play.";
    paragraph(Text::Caption2, status, LX + 24, ay, label(Label::Secondary), LW - 24, 2);
  }

  void draw_options() {
    Ctx &c = ctx();
    if (has_recommended()) draw_best(*ranking_.recommended, focus_index_ == 0 && !pills_focused_);
    draw_pills();
    // 4.1 Swiss: the table's column names on a white rule above the list.
    {
      int hy = LIST_TOP - 26;
      c.gfx.fill_rect(RX, hy - 6, RW, rule, rgba(0xffffff));
      const int cols[6] = {0, 1, 3, 5, 6, 8};
      const char *names[6] = {"Resolution", "Picture", "Sound", "Size", "Group", "Ready"};
      for (int k = 0; k < 5; k++) text(Text::Caption3, names[k], RX + 16 + cols[k] * (span_w(1) + grid_gutter), hy, label(Label::Secondary));
      text_right(Text::Caption3, names[5], RX + RW - 16, hy, label(Label::Secondary));
    }
    float sv = scroll_.value;
    for (auto &row : rows_) {
      float ry = LIST_TOP + row.y - sv;
      int h = row.option < 0 ? HEADER_H : ROW;
      if (ry < LIST_TOP - 8 || ry + h > LIST_TOP + LIST_H + 8) continue;  // only whole rows, inside the window
      if (row.option < 0) {
        bool problems = row.kind == "problems";
        if (problems) c.icons.draw(Icon::Warning, RX, static_cast<int>(ry) + 14, 24, caution);
        text(Text::Caption2Bold, row.label, RX + (problems ? 34 : 0) + 16, static_cast<int>(ry) + 14, problems ? caution : label(Label::Primary));
      } else {
        draw_version(options_[static_cast<size_t>(row.option)], static_cast<int>(ry), row.option == focus_index_ && !pills_focused_);
      }
    }
    if (rows_.empty()) text(Text::Callout, "No other versions.", RX, LIST_TOP + 14, label(Label::Secondary));
    if (sv + LIST_H < list_h_ - ROW_GAP) c.icons.draw(Icon::ChevronDown, RX + RW / 2 - 18, LIST_TOP + LIST_H - 4, 36, label(Label::Tertiary));
    // What the keys do here, and where the versions came from.
    int hx = RX;
    if (pills_focused_) {
      hx += draw_hint(hx, H - 82, "\xE2\x86\x90 \xE2\x86\x92", all_.cuts.size() > 1 ? "Edition" : "Filter") + 32;
      hx += draw_hint(hx, H - 82, "\xE2\x86\x93", "Versions") + 32;
    } else {
      hx += draw_hint(hx, H - 82, "OK", "Play") + 32;
      hx += draw_hint(hx, H - 82, "\xE2\x86\x92", details_ ? "Hide details" : "Release details") + 32;
    }
    std::string found = ranking_.provider.empty() ? "" : "Found by " + ranking_.provider;
    if (ranking_.total > 0) found += (found.empty() ? "" : " \xC2\xB7 ") + std::to_string(ranking_.total) + " versions";
    text(Text::Caption2, found, hx + 8, H - 78, label(Label::Tertiary), std::max(0, RX + RW - hx - 8));
  }

  // The filter strip: All, 4K, 1080p, 720p and below, May not play well.
  void draw_pills() {
    if (pills_.empty()) return;
    std::vector<GlassButton> bs;
    std::vector<int> at;
    int total = 0;
    for (size_t i = 0; i < pills_.size(); i++) {
      bool selected = static_cast<int>(i) == pill_;
      GlassButton b;
      b.label = (pills_[i].cut.empty() ? std::string(filter_name(pills_[i].filter)) : pills_[i].cut) + " \xC2\xB7 " + std::to_string(pills_[i].count);
      b.pill = true;
      b.focused = pills_focused_ && selected;
      b.candidate = selected && !b.focused;
      b.lift = lift_.value;
      at.push_back(total);
      total += glass_button_width(b) + theme::control_gap;
      bs.push_back(b);
    }
    // 4.1: more tabs than fit (SDR Remux, Season packs): the row slides so the
    // chosen tab is always in view; tabs outside the column are not drawn.
    int shift = 0;
    if (pill_ >= 0 && pill_ < static_cast<int>(bs.size())) {
      int right = at[static_cast<size_t>(pill_)] + glass_button_width(bs[static_cast<size_t>(pill_)]);
      if (right > RW) shift = right - RW;
    }
    for (size_t i = 0; i < bs.size(); i++) {
      int x = RX + at[i] - shift, w = glass_button_width(bs[i]);
      if (x < RX || x + w > RX + RW) continue;
      draw_glass_button(x, PILLS_Y, bs[i]);
    }
  }

  // A value set large in Bold, shrunk until it fits `w`.
  static void big(const std::string &v, int x, int y, int w, int size, Color color) {
    Font &f = ctx().fonts.bold;
    while (size > 28 && f.measure(v, size) > w) size -= 4;
    f.draw(ellipsize(f, v, size, w), x, y, size, color);
  }

  // 4.1 Swiss best version: a white rule, what it is and whether it's ready, the
  // facts as big numbers in columns with small words under them, and Play.
  void draw_best(const SourceOption &o, bool focused) {
    Ctx &c = ctx();
    int y = 120;
    c.gfx.fill_rect(RX, y, RW, rule, rgba(0xffffff));
    std::string kind = o.match == "same" ? "Same as your last episode" : o.match == "like" ? "Like your last episode"
                       : all_.cuts.size() > 1 ? "Best " + cut_ + " version" : "Best for this TV";
    text(Text::Caption1Semi, kind, RX, y + 14, label(Label::Primary));
    if (o.ready == 1) {
      int rw = text_right(Text::Caption1, "Ready", RX + RW, y + 14, label(Label::Primary));
      c.gfx.fill_rect(RX + RW - rw - 26, y + 22, 14, 14, theme::signal);
    } else if (!o.cautions.empty()) {
      text_right(Text::Caption1, o.cautions[0], RX + RW, y + 14, caution);
    }
    // Columns on the nine-column grid: resolution 2, picture 2, sound 3, size 2.
    int cx[4] = {col_x(3), col_x(5), col_x(7), col_x(10)}, cw[4] = {span_w(2), span_w(2), span_w(3), span_w(2)};
    std::string picture = o.family == "DV" ? "Dolby Vision" : o.family == "HDR" ? (o.picture.empty() ? "HDR" : o.picture.substr(0, o.picture.find(" ("))) : "SDR";
    std::string sound = o.audio;
    for (auto &r : std::vector<std::pair<std::string, std::string>>{{"Dolby Digital Plus", "DD+"}, {"Dolby Digital", "DD"}}) {
      size_t at = sound.find(r.first);
      if (at != std::string::npos) sound.replace(at, r.first.size(), r.second);
    }
    std::string vals[4] = {o.resolution, picture, sound, o.size};
    std::string subs[4] = {o.tier_label.empty() ? source_words(o.source) : o.tier_label, o.picture_name, o.cut.empty() ? "" : o.cut, o.group};
    for (int k = 0; k < 4; k++) {
      big(vals[k], cx[k], y + 54, cw[k], 56, k == 3 ? label(Label::Secondary) : label(Label::Primary));
      text(Text::Caption2, subs[k], cx[k], y + 126, label(Label::Secondary), cw[k]);
    }
    GlassButton b;
    b.label = "Play";
    b.icon = kPlayIcon;
    b.h = 60;
    b.focused = focused;
    b.lift = lift_.value;
    draw_glass_button(RX, y + 176, b);
    if (!ranking_.why.empty()) text(Text::Caption2, ranking_.why, RX + glass_button_width(b) + 24, y + 176 + (60 - line_for(Text::Caption2)) / 2,
                                    label(Label::Secondary), RW - glass_button_width(b) - 24);
  }

  // 4.1 Swiss: one line per version in table columns; the focused row is a
  // white block, the others sit on a hairline.
  void draw_version(const SourceOption &o, int y, bool focused) {
    Ctx &c = ctx();
    int x = RX, w = RW;
    if (focused) c.gfx.fill_rect(x, y, w, ROW, rgba(0xffffff));
    else c.gfx.fill_rect(x, y + ROW - 1, w, 1, rgba(0xffffff, 51));
    Color fg = focused ? rgba(0x000000) : label(o.excluded ? Label::Disabled : Label::Primary);
    Color fg2 = focused ? rgba(0x000000, 166) : label(o.excluded ? Label::Disabled : Label::Secondary);
    int ty = y + (ROW - line_for(Text::Caption1)) / 2;
    if (is_more(o)) {  // "+N more": one quiet line
      text(Text::Caption1, o.headline, x + 16, ty, focused ? fg : label(Label::Secondary), w - 32);
      return;
    }
    std::string picture = o.picture_name.empty() ? o.picture : o.picture_name;
    std::string group = o.group;
    if (req_.item.series() && o.pack) group = join({group, "whole season"});
    const int cols[6] = {0, 1, 3, 5, 6, 8};  // resolution, picture, sound, size, group, ready
    std::string cells[5] = {o.resolution, picture, o.audio, o.size, group};
    for (int k = 0; k < 5; k++) {
      int cxk = x + 16 + (cols[k] * (span_w(1) + grid_gutter)), cwk = (cols[k + 1] - cols[k]) * (span_w(1) + grid_gutter) - grid_gutter;
      text(Text::Caption1, cells[k], cxk, ty, k == 0 || k == 2 ? fg : fg2, cwk);
    }
    if (o.ready == 1) c.gfx.fill_rect(x + w - 16 - 12, y + (ROW - 12) / 2, 12, 12, theme::signal);
    else if (!o.cautions.empty()) text_right(Text::Caption2, o.cautions[0], x + w - 16, y + (ROW - line_for(Text::Caption2)) / 2, focused ? caution_ink : caution);
  }

  // Release details for the focused version: everything technical, on request.
  void draw_details() {
    float a = details_anim_.value;
    if (a <= 0.01f || state_ != State::Ready || options_.empty()) return;
    Ctx &c = ctx();
    const SourceOption &o = options_[static_cast<size_t>(focus_index_)];
    int w = 640, x = W - safe_x - w + static_cast<int>((1.0f - a) * 60), y = 96, h = H - 192;
    // It slides, so it is solid glass (a moving frost would re-bake every frame).
    GlassSpec g;
    g.x = x;
    g.y = y;
    g.w = w;
    g.h = h;
    g.kind = GlassKind::Solid;
    g.alpha = a;
    // Opaque underneath: it lies over text, which must not ghost through.
    c.gfx.fill_round(x, y, w, h, g.radius, fade(rgba(0x2c2c2e), a));
    draw_glass(g);
    int px = x + 44, pw = w - 88, py = y + 44;
    text(Text::Caption1, "Release details", px, py, label(Label::Secondary, a));
    py += 38;
    text(Text::Title3, o.headline, px, py, label(Label::Primary, a), pw);
    py += 70;
    auto row = [&](const std::string &name, const std::string &value, int lines = 1) {
      if (value.empty()) return;
      text(Text::Caption2, name, px, py, label(Label::Secondary, a));
      py += 30;
      py = paragraph(Text::Callout, value, px, py, label(Label::Primary, a), pw, lines) + 18;
    };
    if (is_more(o)) return;
    std::string edition = o.cut.empty() ? "Theatrical (or not stated)" : o.cut;
    for (auto &t : o.tags) edition += " \xC2\xB7 " + t;
    row("Edition", edition);
    row("Picture", o.picture_name.empty() ? picture_words(o) : o.picture_name);
    row("Sound", o.audio);
    row("Source", join({o.tier_label, o.size, o.bitrate_mbps > 0 ? "about " + std::to_string(o.bitrate_mbps) + " Mbit/s" : "",
                        o.group.empty() ? "" : o.group + (o.respected ? " (trusted group)" : "")}), 2);
    row("Compared with the best", o.compare, 2);
    row("On AllDebrid", o.ready == 1 ? "Ready now: plays straight away" : o.ready == 0 ? "Not downloaded yet: AllDebrid fetches it first" : "");
    row("Found by", join({join(o.sources, ", ") + (o.dupes > 0 ? " (listed " + std::to_string(o.dupes + 1) + " times)" : ""),
                          o.seeders >= 0 ? std::to_string(o.seeders) + " seeders" : ""}));
    row("Release name", o.release, 2);
    for (auto &warning : o.warnings) {
      c.icons.draw(Icon::Warning, px, py, 26, fade(caution, a));
      py = paragraph(Text::Caption1, warning, px + 38, py, fade(caution, a), pw - 38, 2) + 8;
    }
    paragraph(Text::Caption2, "These details come from the release name, not from the file itself.", px, y + h - 92, label(Label::Tertiary, a), pw, 2);
  }

  void draw_modal() {
    float a = modal_anim_.value;
    if (modal_ == Modal::None || a <= 0.01f) return;
    Ctx &c = ctx();
    c.gfx.fill_rect(0, 0, W, H, with_alpha(black, static_cast<int>(200 * a)));
    int w = 920, x = (W - w) / 2;
    if (modal_ == Modal::Resolving) {
      // 4.1 Swiss starting sheet: the whole screen, dimmed; the logo top left;
      // at the foot how far along it is as a big number over a red bar, the
      // steps as numbered columns on rules, the version, and the buttons.
      c.gfx.fill_rect(0, 0, W, H, with_alpha(black, static_cast<int>(40 * a)));
      draw_title(req_.item, LX, 80, span_w(4), 120, a);
      struct StepRow { const char *id; const char *label; };
      std::vector<StepRow> steps = {{"adding", "Sent to AllDebrid"}};
      if (saw_caching_) steps.push_back({"caching", "AllDebrid is fetching it"});
      steps.push_back({"files", "Finding the right file"});
      steps.push_back({"unlocking", "Getting the stream"});
      static const char *order[] = {"starting", "adding", "caching", "files", "unlocking", "preparing", "ready"};
      auto rank = [](const std::string &id) {
        for (int i = 0; i < 7; i++) if (id == order[i]) return i;
        return 0;
      };
      int now_rank = rank(step_), done_count = 0;
      std::string current_label = "Starting";
      for (auto &st : steps) {
        bool preparing = std::string(st.id) == "unlocking" && step_ == "preparing";
        if (now_rank > rank(st.id) && !preparing) done_count++;
        else if (now_rank == rank(st.id) || preparing) current_label = st.label;
      }
      double frac = (done_count + (step_ == "caching" && fraction_ >= 0 ? fraction_ : 0.5)) / static_cast<double>(steps.size() + 1);
      int pct = static_cast<int>(std::min(99.0, frac * 100));
      int y = H - safe_y - 360;
      int pw = c.fonts.bold.measure(std::to_string(pct) + "%", 120);
      c.fonts.bold.draw(std::to_string(pct) + "%", LX, y, 120, fade(rgba(0xffffff), a));
      text(Text::Callout, current_label, LX + pw + 28, y + 70, label(Label::Primary, a), span_w(8));
      int by = y + 140;
      c.gfx.fill_rect(LX, by, span_w(12), 8, fade(rgba(0xffffff, 64), a));
      c.gfx.fill_rect(LX, by, static_cast<int>(span_w(12) * frac), 8, fade(theme::signal, a));
      int n = static_cast<int>(steps.size()) + 1, cw = (span_w(12) - (n - 1) * grid_gutter) / n;
      for (int k = 0; k < n; k++) {
        bool is_last = k == n - 1;
        std::string id = is_last ? "ready" : steps[static_cast<size_t>(k)].id;
        std::string lab = is_last ? "First picture" : steps[static_cast<size_t>(k)].label;
        bool preparing = id == "unlocking" && step_ == "preparing";
        bool done = !is_last && now_rank > rank(id) && !preparing;
        bool current = !is_last && (now_rank == rank(id) || preparing);
        int sx = LX + k * (cw + grid_gutter), sy = by + 40;
        c.gfx.fill_rect(sx, sy, cw, current ? 3 : 2, fade(current ? theme::signal : done ? rgba(0xffffff) : rgba(0xffffff, 77), a));
        char num[16];
        std::snprintf(num, sizeof num, "%02d", k + 1);
        int nw = text(Text::Caption1Semi, num, sx, sy + 14, label(done || current ? Label::Primary : Label::Tertiary, a));
        text(current ? Text::Caption1Semi : Text::Caption1, lab, sx + nw + 12, sy + 14, label(done || current ? Label::Primary : Label::Tertiary, a), cw - nw - 12);
      }
      std::string facts = join({chosen_.resolution, chosen_.picture, chosen_.audio, chosen_.size});
      if (chosen_.match == "same") facts = join({"Same version as your last episode", facts});
      text(Text::Caption1, facts, LX, by + 120, label(Label::Secondary, a), span_w(7));
      GlassButton cancel;
      cancel.label = "Cancel";
      cancel.focused = sheet_focus_ == 0;
      cancel.alpha = a;
      cancel.h = 60;
      GlassButton others;
      others.label = "Other versions";
      others.focused = sheet_focus_ == 1;
      others.alpha = a;
      others.h = 60;
      int total = glass_button_width(cancel) + (can_show_others() ? 12 + glass_button_width(others) : 0);
      int bx = W - safe_x - total;
      bx += draw_glass_button(bx, by + 106, cancel) + 12;
      if (can_show_others()) draw_glass_button(bx, by + 106, others);
      c.redraw_at(c.now + 100);
      return;
    }
    if (modal_ == Modal::ChooseFile || modal_ == Modal::ChooseQuality) {
      int n = static_cast<int>(choices_.size());
      int shown = std::min(n, 7);
      int h = 210 + shown * 76, y = (H - h) / 2;
      sheet(x, y, w, h, a);
      text(Text::Title3, modal_ == Modal::ChooseFile ? "Which file?" : "Which quality?", x + 56, y + 46, label(Label::Primary, a));
      text(Text::Callout, modal_ == Modal::ChooseFile ? "This release holds several videos." : "AllDebrid offers these versions.", x + 56, y + 104,
           label(Label::Secondary, a));
      int first = std::max(0, std::min(choice_ - 3, n - shown));
      int ry = y + 160;
      for (int i = first; i < first + shown; i++) {
        bool f = i == choice_;
        if (f) {
          c.gfx.shadow(x + 32, ry, w - 64, 66, 20, 18, 8, 120);
          c.gfx.fill_round(x + 32, ry, w - 64, 66, 20, rgba(0xffffff));
        }
        text(Text::Callout, choices_[static_cast<size_t>(i)].label, x + 56, ry + 14, f ? rgba(0x000000) : label(Label::Primary, a), w - 320);
        text_right(Text::Callout, choices_[static_cast<size_t>(i)].detail, x + w - 56, ry + 14, f ? rgba(0x000000, 166) : label(Label::Secondary, a));
        ry += 76;
      }
      return;
    }
    if (modal_ == Modal::Failed) {
      int h = 420, y = (H - h) / 2;
      sheet(x, y, w, h, a);
      c.icons.draw(Icon::Warning, x + 56, y + 56, 40, fade(caution, a));
      text(Text::Title3, "That version didn\xE2\x80\x99t work", x + 112, y + 50, label(Label::Primary, a));
      paragraph(Text::Body, failure_, x + 56, y + 130, label(Label::Secondary, a), w - 112, 3);
      int bx = x + 56;
      bool has_next = next_in_order() >= 0;
      GlassButton next;
      next.label = has_next ? "Try the next best" : "Back to versions";
      next.focused = fail_focus_ == 0;
      next.lift = lift_.value;
      next.alpha = a;
      next.variant = ButtonStyle::Default;
      bx += draw_glass_button(bx, y + h - 112, next) + theme::stack_gap;
      GlassButton again;
      again.label = "Try again";
      again.focused = fail_focus_ == 1;
      again.lift = lift_.value;
      again.alpha = a;
      again.variant = ButtonStyle::Default;
      draw_glass_button(bx, y + h - 112, again);
    }
  }

 private:
  enum class State { Loading, Ready, Empty, Error };
  enum class Modal { None, Resolving, ChooseFile, ChooseQuality, Failed };
  struct Choice {
    std::string id, label, detail;
  };
  // One line of the list: a section title (option < 0) or a version.
  struct ListRow {
    int option;
    std::string label, kind;
    int y;
  };

  PlayRequest req_;
  bool autostart_;
  Backdrop backdrop_;
  State state_ = State::Loading;
  std::string error_;
  long long loading_since_ = 0;
  Ranking ranking_;
  std::vector<SourceOption> options_;
  std::vector<ListRow> rows_;
  std::vector<std::string> section_of_;  // per option: the service's section ("best", "continue", "others", "problems")
  struct Pill {
    Filter filter;
    int count;
    std::string cut;  // an edition pill (2.8), else ""
  };
  Ranking all_;  // the service's whole answer; ranking_ is the chosen cut's view of it
  std::string cut_;
  std::set<std::string> expanded_;  // tiers whose folded versions are shown
  std::vector<Pill> pills_;
  int pill_ = 0;
  bool pills_focused_ = false;
  Filter filter_ = Filter::All;
  int list_h_ = 0;
  int sheet_focus_ = 0;  // starting sheet: 0 Cancel, 1 Other versions
  int focus_index_ = 0, prev_index_ = -1;
  Anim lift_{focus_in_rate, 1}, prev_lift_{focus_leave_rate}, scroll_{scroll_rate}, details_anim_{fade_rate}, modal_anim_{fade_rate};
  bool details_ = false;
  bool account_known_ = false, durable_ = false;
  Pairing pairing_;
  Modal modal_ = Modal::None;
  long long modal_since_ = 0;
  SourceOption chosen_;
  std::string job_id_, step_, step_label_;
  double fraction_ = -1;
  bool saw_caching_ = false;
  long long next_poll_ = 0;
  std::vector<Choice> choices_;
  int choice_ = 0;
  std::string failure_;
  int fail_focus_ = 0;
  bool leave_after_play_ = false;
  bool direct_ = false;
};

}  // namespace

bool &player_wants_another_source() { return SourcesScreen::choose_another(); }

std::unique_ptr<Screen> make_sources(const PlayRequest &request, bool autostart) {
  return std::make_unique<SourcesScreen>(request, autostart);
}

void start_playback(const PlayRequest &req, PlayOrigin origin, bool replace_top, bool autostart) {
  if (req.video_id.empty()) return;
  static const char *names[] = {"home-hero", "home-row", "details", "episodes", "up-next"};
  logf("play", "intent from %s: %s resume=%lld", names[static_cast<int>(origin)], req.label().c_str(), req.resume_ms);
  // Gates belong here and nowhere else (pairing, TV format profile, household limits).
  // The owner chooses the version (2.7.3): Play opens the versions list with the
  // best one focused. Only Up next continues by itself, with the show's version.
  auto screen = make_sources(req, autostart && origin == PlayOrigin::UpNext);
  if (replace_top) ctx().replace(std::move(screen));
  else ctx().push(std::move(screen));
}
