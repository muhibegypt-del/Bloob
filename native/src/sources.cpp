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
//
// 5.0, Columns: the versions are narrowed in steps, each answering one question,
// side by side on one screen: Quality, then Picture, then which Dolby Vision (or,
// for other pictures, the kind of release), then the releases themselves. Every
// choice shows how many versions it holds and how many are ready; a choice with
// none is never offered; every column starts at Any, so the list opens as the
// whole flat list, best first. Above: Episode or Whole season (season packs)
// and Ready only. Faceted navigation as Hearst's Search User Interfaces (ch. 8)
// describes it: query previews, no dead ends, the trail of what is chosen.
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
constexpr int LX = safe_x, LW = 760;            // title, top left
constexpr int RX = 760, RW = W - safe_x - 760;  // loading and error messages
constexpr int BEST_X = 1000, BEST_Y = 56, BEST_W = W - safe_x - 1000, BEST_H = 150;
constexpr int TOP_Y = 236;                      // Episode / Whole season, Ready only, the trail
constexpr int COLS_Y = 330, COLS_B = H - safe_y; // the columns and the list
constexpr int COL_GAP = 20;
constexpr int COL_W[3] = {300, 290, 370};
constexpr int OPT_H = 62, OPT_NOTE_H = 94, OPT_GAP = 6, COL_HEAD = 64;
constexpr int LIST_X = safe_x + 300 + 290 + 370 + 3 * COL_GAP;
constexpr int LIST_W = W - safe_x - LIST_X;
constexpr int ROW = 96, ROW_GAP = 8;
constexpr int LIST_TOP = COLS_Y + 68;
constexpr int LIST_H = ((COLS_B - 56 - LIST_TOP) / (ROW + ROW_GAP)) * (ROW + ROW_GAP);
constexpr int HEADER_H = 58;

bool is_more(const SourceOption &o) { return o.id.compare(0, 5, "more:") == 0; }

std::string source_words(const std::string &s) {
  if (s == "REMUX") return "Remux";
  if (s == "BluRay") return "Blu-ray";
  if (s == "WEB-DL") return "Web download";
  if (s == "WEBRip") return "Web rip";
  if (s == "HDTV") return "TV recording";
  return s;
}

bool has(const std::string &s, const char *part) { return s.find(part) != std::string::npos; }

// Column 1, Quality. A version can be in two (an SDR remux is also 1080p).
const char *kQualityKeys[] = {"4K", "1080p", "lower", "sdr-remux", "problems"};
std::string quality_name(const std::string &k) {
  if (k == "lower") return "720p and below";
  if (k == "sdr-remux") return "SDR Remux";
  if (k == "problems") return "May not play";
  return k;
}
bool quality_match(const std::string &k, const SourceOption &o, bool problem) {
  if (k == "problems") return problem;
  if (problem) return false;
  if (k.empty()) return true;
  if (k == "4K" || k == "1080p") return o.resolution == k;
  if (k == "lower") return o.resolution != "4K" && o.resolution != "1080p";
  if (k == "sdr-remux") return o.tier == "sdr-remux";
  return false;
}

// Column 2, Picture: what the release carries, whatever this TV makes of it.
const char *kPictureKeys[] = {"dv", "hdr10+", "hdr10", "sdr"};
const char *picture_key_name(const std::string &k) {
  if (k == "dv") return "Dolby Vision";
  if (k == "hdr10+") return "HDR10+";
  if (k == "hdr10") return "HDR10";
  return "SDR";
}
std::string picture_key(const SourceOption &o) {
  if (!o.dv_profile.empty() || o.family == "DV") return "dv";
  if (has(o.picture, "HDR10+") || has(o.picture_name, "HDR10+")) return "hdr10+";
  if (o.family == "HDR") return "hdr10";
  return "sdr";
}

// Column 3: which Dolby Vision (profile), else the kind of release.
std::string kind_key(const SourceOption &o, bool dv) {
  if (dv) return o.dv_profile.empty() ? "dv" : o.dv_profile;
  if (!o.tier_label.empty() && o.tier != "pack" && o.tier != "other") return o.tier_label;
  return o.source.empty() ? "Other" : source_words(o.source);
}
std::string kind_name(const std::string &k, bool dv) {
  if (!dv) return k;
  if (k == "p7") return "Disc \xC2\xB7 Profile 7";
  if (k == "p8") return "Profile 8";
  if (k == "p5") return "Profile 5";
  return "Dolby Vision";
}
// How a Dolby Vision kind plays on this TV, from the service's picture label.
std::string kind_note(const SourceOption &o) {
  const std::string &p = o.picture.empty() ? o.picture_name : o.picture;
  if (has(p, "colours may be wrong")) return "Colours may be off here";
  if (has(p, "as 8.1")) return "Plays as Dolby Vision";
  if (has(p, "skipped")) return "Plays as HDR10 (your setting)";
  if (o.family == "DV") return "Plays as Dolby Vision";
  return "Plays as HDR10 here";
}

// Plain words for a version's picture and how easy it is to get.
std::string picture_words(const SourceOption &o) {
  if (o.picture == "SDR" || o.picture.empty()) return "Standard picture";
  return o.picture;
}

class SourcesScreen : public Screen {
 public:
  // 5.0, Columns.
  enum class Zone { Best, Top, Column, List };
  struct Facet {
    std::string key, label, note;
    int count = 0, ready = 0;
  };
  struct Toggle {
    std::string label;
    bool on;
    int id;  // 0 Episode, 1 Whole season, 2 Ready only
  };
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
    if (zone_ == Zone::Column) {
      const Facet &f = columns_[col_][static_cast<size_t>(sel_[col_])];
      return std::string(column_title(col_)) + ": " + f.label + ", " + std::to_string(f.count) + " versions" +
             (f.ready ? ", " + std::to_string(f.ready) + " ready now" : "") + (f.note.empty() ? "" : ". " + f.note);
    }
    if (zone_ == Zone::Top && !toggles_.empty()) {
      const Toggle &t = toggles_[static_cast<size_t>(toggle_)];
      return t.label + (t.on ? ", on" : ", off");
    }
    const SourceOption &o = options_[static_cast<size_t>(zone_ == Zone::Best ? 0 : focus_index_)];
    std::string head = zone_ == Zone::Best ? "Best for this TV, " : "";
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
      sel_[0] = sel_[1] = sel_[2] = 0;
      keys_[0] = keys_[1] = keys_[2] = "";
      ready_only_ = false;
      arrange();
      state_ = options_.empty() ? State::Empty : State::Ready;
      focus_index_ = 0;
      zone_ = has_recommended() ? Zone::Best : Zone::Column;
      col_ = 0;
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
    return "";  // 4.2: no edition tabs; the cut is a chip on each row
  }
  bool in_cut(const SourceOption &o) const {
    (void)o;
    return true;  // 4.2: every cut is listed; each row says which it is
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
    // 4.2: one flat list, best first: versions that continue the show's last
    // episode, then by score. Nothing is folded away.
    std::vector<size_t> rest;
    for (size_t i = 0; i < pool.size(); i++)
      if (static_cast<int>(i) != best && pool[i].second != "best") rest.push_back(i);
    std::stable_sort(rest.begin(), rest.end(), [&](size_t a, size_t b) {
      bool ca = pool[a].second == "continue", cb = pool[b].second == "continue";
      if (ca != cb) return ca;
      return pool[a].first.score > pool[b].first.score;
    });
    for (size_t i : rest) {
      options_.push_back(pool[i].first);
      section_of_.push_back(pool[i].second);
    }
    // Episode or Whole season: offered when this episode comes both ways.
    bool singles = false, packs = false;
    for (auto &o : options_) (o.pack ? packs : singles) = true;
    both_scopes_ = req_.item.series() && singles && packs;
    if (!both_scopes_) season_ = packs && !singles;
    build_columns();
  }

  // Folded behind "+N more" until that row is opened.
  bool folds(size_t i) const {
    (void)i;
    return false;  // 4.2: nothing folds
  }

  // ---------------------------------------------------------------- columns (5.0)

  static const char *column_title(int c) { return c == 0 ? "Quality" : c == 1 ? "Picture" : "Source"; }
  bool dv_mode() const { return keys_[1] == "dv"; }
  const char *third_title() const { return dv_mode() ? "Which Dolby Vision" : "Source"; }

  // Episode or Whole season, then Ready only.
  bool in_scope(const SourceOption &o) const {
    if (both_scopes_ && o.pack != season_) return false;
    return !ready_only_ || o.ready == 1;
  }
  bool passes(size_t i, int upto) const {
    const SourceOption &o = options_[i];
    if (is_more(o) || !in_scope(o)) return false;
    if (upto >= 0 && !quality_match(keys_[0], o, problem(i))) return false;
    if (upto >= 1 && !keys_[1].empty() && picture_key(o) != keys_[1]) return false;
    if (upto >= 2 && !keys_[2].empty() && kind_key(o, dv_mode()) != keys_[2]) return false;
    return true;
  }

  // Each column's choices from what the columns to its left leave: Any first,
  // then every choice that holds at least one version, with its counts. A
  // choice that no longer exists falls back to Any.
  void build_columns() {
    for (int c = 0; c < 3; c++) {
      std::vector<Facet> col;
      Facet any;
      any.label = "Any";
      std::vector<std::string> keys;
      if (c == 0) for (const char *k : kQualityKeys) keys.push_back(k);
      if (c == 1) for (const char *k : kPictureKeys) keys.push_back(k);
      std::map<std::string, Facet> found;
      for (size_t i = 0; i < options_.size(); i++) {
        if (!passes(i, c - 1)) continue;
        const SourceOption &o = options_[i];
        bool prob = problem(i);
        if (!prob || (c > 0 && keys_[0] == "problems")) {
          any.count++;
          any.ready += o.ready == 1;
        }
        auto add = [&](const std::string &k) {
          Facet &f = found[k];
          if (f.count == 0) {
            f.key = k;
            f.label = c == 0 ? quality_name(k) : c == 1 ? picture_key_name(k) : kind_name(k, dv_mode());
            if (c == 2 && dv_mode()) f.note = kind_note(o);
            if (c == 2) keys.push_back(k);  // in order of their best version
          }
          f.count++;
          f.ready += o.ready == 1;
        };
        if (c == 0) {
          for (const char *k : kQualityKeys)
            if (quality_match(k, o, prob)) add(k);
        } else if (!prob || keys_[0] == "problems") {
          add(c == 1 ? picture_key(o) : kind_key(o, dv_mode()));
        }
      }
      col.push_back(any);
      for (auto &k : keys) {
        auto it = found.find(k);
        if (it != found.end() && it->second.count > 0) col.push_back(it->second);
      }
      // One kind only under Any: nothing to choose, so just Any.
      if (c > 0 && col.size() == 2 && col[1].count == col[0].count) col.resize(1);
      columns_[c] = col;
      int at = 0;
      for (size_t i = 0; i < col.size(); i++)
        if (col[i].key == keys_[c]) at = static_cast<int>(i);
      sel_[c] = at;
      keys_[c] = col[static_cast<size_t>(at)].key;
    }
    toggles_.clear();
    if (both_scopes_) {
      toggles_.push_back({"Episode", !season_, 0});
      toggles_.push_back({"Whole season", season_, 1});
    }
    bool any_ready = false;
    for (auto &o : options_) any_ready |= o.ready == 1;
    if (any_ready) toggles_.push_back({"Ready only", ready_only_, 2});
    toggle_ = std::min(toggle_, std::max(0, static_cast<int>(toggles_.size()) - 1));
    build_rows();
  }

  // The releases the columns leave, best first.
  void build_rows() {
    rows_.clear();
    int y = 0;
    for (size_t i = 0; i < options_.size(); i++) {
      if (!passes(i, 2)) continue;
      rows_.push_back({static_cast<int>(i), "", section_of_[i], y});
      y += ROW + ROW_GAP;
    }
    list_h_ = y;
    scroll_.snap(0);
    if (!rows_.empty() && zone_ != Zone::List) focus_index_ = rows_[0].option;
  }

  // Versions in list order (for Up and Down).
  std::vector<int> order() const {
    std::vector<int> list;
    for (auto &r : rows_)
      if (r.option >= 0) list.push_back(r.option);
    return list;
  }

  // Moving within a column chooses: the columns to its right and the list follow.
  void choose_facet(int c, int index) {
    const std::vector<Facet> &col = columns_[c];
    index = std::max(0, std::min(static_cast<int>(col.size()) - 1, index));
    if (index == sel_[c]) return;
    sel_[c] = index;
    keys_[c] = col[static_cast<size_t>(index)].key;
    for (int r = c + 1; r < 3; r++) keys_[r] = "";
    build_columns();
    lift_.snap(0);
    lift_.to(1);
  }

  void flip(const Toggle &t) {
    if (t.id == 0) season_ = false;
    if (t.id == 1) season_ = true;
    if (t.id == 2) ready_only_ = !ready_only_;
    build_columns();
  }

  void focus(Zone z, int col = 0) {
    zone_ = z;
    col_ = col;
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
    if (false) {  // 4.2: no edition tabs, so no cut to remember
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
    // Where Up from the columns and the list goes: the toggles, else the best version.
    auto up_from_body = [&]() {
      if (!toggles_.empty()) focus(Zone::Top);
      else if (has_recommended()) focus(Zone::Best);
    };
    auto into_list = [&]() {
      if (list.empty()) return;
      focus(Zone::List, 2);
      move_to(list[0]);
    };
    switch (zone_) {
      case Zone::Best:
        if (key == Key::Down) {
          if (!toggles_.empty()) focus(Zone::Top);
          else focus(Zone::Column, 0);
        } else if ((key == Key::Ok || key == Key::Play || key == Key::PlayPause) && !repeat) {
          focus_index_ = 0;
          choose(0);
        } else if (key == Key::Back) {
          ctx().pop();
        }
        return true;
      case Zone::Top: {
        int n = static_cast<int>(toggles_.size());
        if (key == Key::Left) toggle_ = std::max(0, toggle_ - 1);
        if (key == Key::Right) toggle_ = std::min(n - 1, toggle_ + 1);
        if (key == Key::Up && has_recommended()) focus(Zone::Best);
        if (key == Key::Down) focus(Zone::Column, 0);
        if (key == Key::Ok && !repeat && n) flip(toggles_[static_cast<size_t>(toggle_)]);
        if (key == Key::Back) ctx().pop();
        return true;
      }
      case Zone::Column: {
        int n = static_cast<int>(columns_[col_].size());
        switch (key) {
          case Key::Up:
            if (sel_[col_] == 0) up_from_body();
            else choose_facet(col_, sel_[col_] - 1);
            return true;
          case Key::Down:
            if (sel_[col_] + 1 < n) choose_facet(col_, sel_[col_] + 1);
            return true;
          case Key::Right:
          case Key::Ok: {
            if (repeat && key == Key::Ok) return true;
            // In: the next column that has something to choose, else the list.
            int next = col_ + 1;
            while (next < 3 && columns_[next].size() < 2) next++;
            if (next < 3) focus(Zone::Column, next);
            else into_list();
            return true;
          }
          case Key::Left: {
            int prev = col_ - 1;
            while (prev > 0 && columns_[prev].size() < 2) prev--;
            if (prev >= 0) focus(Zone::Column, prev);
            return true;
          }
          case Key::Back: {
            // Out one column; from the first, leave.
            int prev = col_ - 1;
            while (prev > 0 && columns_[prev].size() < 2) prev--;
            if (prev >= 0) focus(Zone::Column, prev);
            else ctx().pop();
            return true;
          }
          default:
            return true;
        }
      }
      case Zone::List:
        break;
    }
    int last_col = 2;
    while (last_col > 0 && columns_[last_col].size() < 2) last_col--;
    switch (key) {
      case Key::Up:
        if (at == 0) up_from_body();
        else move_to(list[static_cast<size_t>(at - 1)]);
        return true;
      case Key::Down:
        if (at + 1 < static_cast<int>(list.size())) move_to(list[static_cast<size_t>(at + 1)]);
        return true;
      case Key::Right:
        details_ = true;  // release details, on request
        return true;
      case Key::Left:
        if (details_) details_ = false;
        else focus(Zone::Column, last_col);
        return true;
      case Key::Ok:
      case Key::Play:
      case Key::PlayPause:
        if (!repeat) choose(focus_index_);
        return true;
      case Key::Back:
        if (details_) details_ = false;
        else focus(Zone::Column, last_col);
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
        text(Text::Title3, state_ == State::Empty ? "Nothing to play yet" : "Couldn\xE2\x80\x99t find versions", LX, COLS_Y, label(Label::Primary));
        paragraph(Text::Body, state_ == State::Empty ? "No versions of this are available right now. Try again later." : error_, LX, COLS_Y + 70,
                  label(Label::Secondary), W - 2 * LX, 3);
        GlassButton b;
        b.label = "Try again";
        b.focused = true;
        b.lift = lift_.value;
        draw_glass_button(LX, COLS_Y + 200, b);
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

  // Top left: the title's logo and what is being played (5.0: compact, so the
  // columns get the room).
  void draw_title_column() {
    Ctx &c = ctx();
    int y = draw_title(req_.item, LX, BEST_Y, LW - 120, 88);
    y += 16;
    std::string what;
    if (req_.item.series()) {
      what = episode_code(req_.season, req_.episode);
      if (!req_.episode_title.empty()) what += " \xC2\xB7 " + req_.episode_title;
    } else {
      what = meta_line(req_.item);
    }
    if (req_.resume_ms > 0) what += " \xC2\xB7 from " + clock_text(req_.resume_ms);
    text(Text::Callout, what, LX, y, label(Label::Primary), BEST_X - LX - 40);
    // AllDebrid, quietly, only when it needs saying.
    if (account_known_ && !c.paired) {
      int ay = y + line_for(Text::Callout) + 10;
      c.gfx.fill_circle(LX + 8, ay + 15, 6, caution);
      text(Text::Caption1, "AllDebrid not connected: you\xE2\x80\x99ll pair once when you press Play", LX + 26, ay, label(Label::Secondary),
           BEST_X - LX - 66);
    }
  }

  void draw_options() {
    if (has_recommended()) draw_best(*ranking_.recommended, zone_ == Zone::Best);
    draw_top();
    int x = LX;
    for (int col = 0; col < 3; col++) {
      draw_column(col, x);
      x += COL_W[col] + COL_GAP;
    }
    draw_list();
  }

  // Episode / Whole season and Ready only, then the trail of what is chosen.
  void draw_top() {
    int x = LX;
    for (size_t i = 0; i < toggles_.size(); i++) {
      const Toggle &t = toggles_[i];
      GlassButton b;
      b.label = t.label;
      b.pill = true;
      b.focused = zone_ == Zone::Top && static_cast<int>(i) == toggle_;
      b.candidate = t.on && !b.focused;
      b.lift = lift_.value;
      x += draw_glass_button(x, TOP_Y, b) + (i + 1 < toggles_.size() && toggles_[i + 1].id == 2 ? 36 : theme::control_gap);
    }
    if (!toggles_.empty()) x += 24;
    // The trail: the choices made, in order; the count when nothing is chosen.
    std::vector<std::string> trail;
    for (int col = 0; col < 3; col++)
      if (sel_[col] > 0) trail.push_back(columns_[col][static_cast<size_t>(sel_[col])].label);
    int ty = TOP_Y + (theme::pill_h - line_for(Text::CalloutBold)) / 2;
    if (trail.empty()) {
      text(Text::CalloutBold, "All versions", x, ty, label(Label::Secondary));
      return;
    }
    for (size_t i = 0; i < trail.size(); i++) {
      if (i) x += text(Text::Callout, "\xE2\x80\xBA", x + 12, ty, label(Label::Tertiary)) + 24;
      x += text(Text::CalloutBold, trail[i], x, ty, label(i + 1 == trail.size() ? Label::Primary : Label::Secondary), W - safe_x - x);
    }
  }

  void draw_column(int col, int x) {
    Ctx &c = ctx();
    const std::vector<Facet> &facets = columns_[col];
    int w = COL_W[col];
    sheet(x, COLS_Y, w, COLS_B - COLS_Y, 1.0f);
    const char *title = col == 2 ? third_title() : column_title(col);
    text(Text::Caption1Semi, title, x + 24, COLS_Y + 22, label(Label::Secondary), w - 48);
    bool active = zone_ == Zone::Column && col_ == col;
    int y = COLS_Y + COL_HEAD;
    for (size_t i = 0; i < facets.size(); i++) {
      const Facet &f = facets[i];
      bool chosen = static_cast<int>(i) == sel_[col];
      bool focused = active && chosen;
      int h = f.note.empty() ? OPT_H : OPT_NOTE_H;
      int ox = x + 12, ow = w - 24;
      if (y + h > COLS_B - 12) break;
      if (focused) {
        float k = 1.0f + 0.04f * lift_.value;
        int fw = static_cast<int>(ow * k), fh = static_cast<int>(h * k);
        c.gfx.shadow(ox - (fw - ow) / 2, y - (fh - h) / 2, fw, fh, 18, 18, 8, 110);
        c.gfx.fill_round(ox - (fw - ow) / 2, y - (fh - h) / 2, fw, fh, 18, rgba(0xffffff));
      } else if (chosen && (i > 0 || active)) {
        c.gfx.fill_round(ox, y, ow, h, 18, candidate);
      }
      Color fg = focused ? rgba(0x000000) : label(chosen ? Label::Primary : Label::Primary);
      Color fg2 = focused ? rgba(0x000000, 160) : label(Label::Secondary);
      Color ready = focused ? rgba(0x0a7a3d) : good;
      // Counts on the right: "3 ready · 6", the ready part green.
      std::string total = std::to_string(f.count);
      int tw = measure(Text::Caption1, total);
      int cy = y + (OPT_H - line_for(Text::Caption1)) / 2;
      text(Text::Caption1, total, ox + ow - 18 - tw, cy, fg2);
      int right = ox + ow - 18 - tw;
      if (f.ready > 0 && f.ready < f.count) {
        std::string r = std::to_string(f.ready) + (active || col == 2 ? " ready \xC2\xB7 " : " \xC2\xB7 ");
        int rw = measure(Text::Caption1, r);
        text(Text::Caption1, r, right - rw, cy, ready);
        right -= rw;
      } else if (f.ready > 0) {
        int rw = measure(Text::Caption1, "all ready \xC2\xB7 ");
        text(Text::Caption1, "all ready \xC2\xB7 ", right - rw, cy, ready);
        right -= rw;
      }
      bool warn = f.key == "problems";
      int lx = ox + 18;
      if (warn) {
        c.icons.draw(Icon::Warning, lx, y + (OPT_H - 26) / 2, 26, focused ? caution_ink : caution);
        lx += 36;
      }
      text(Text::BodySemi, f.label, lx, y + (OPT_H - line_for(Text::BodySemi)) / 2, fg, right - lx - 10);
      if (!f.note.empty()) {
        bool off = has(f.note, "off");
        text(Text::Caption2, f.note, ox + 18, y + OPT_H - 6, off ? (focused ? caution_ink : caution) : fg2, ow - 36);
      }
      y += h + OPT_GAP;
    }
  }

  void draw_list() {
    Ctx &c = ctx();
    sheet(LIST_X, COLS_Y, LIST_W, COLS_B - COLS_Y, 1.0f);
    int n = static_cast<int>(rows_.size());
    text(Text::Caption1Semi, std::to_string(n) + (n == 1 ? " version" : " versions"), LIST_X + 28, COLS_Y + 22, label(Label::Secondary));
    if (n > 1) text_right(Text::Caption1, "Best first", LIST_X + LIST_W - 28, COLS_Y + 22, label(Label::Tertiary));
    float sv = scroll_.value;
    for (auto &row : rows_) {
      float ry = LIST_TOP + row.y - sv;
      if (ry < LIST_TOP - 8 || ry + ROW > LIST_TOP + LIST_H + 8) continue;  // only whole rows, inside the window
      draw_version(options_[static_cast<size_t>(row.option)], static_cast<int>(ry), zone_ == Zone::List && row.option == focus_index_);
    }
    if (rows_.empty()) text(Text::Callout, "Nothing here.", LIST_X + 28, LIST_TOP + 14, label(Label::Secondary));
    if (sv + LIST_H < list_h_ - ROW_GAP) c.icons.draw(Icon::ChevronDown, LIST_X + LIST_W / 2 - 18, LIST_TOP + LIST_H - 2, 36, label(Label::Tertiary));
    if (zone_ == Zone::List) draw_hint(LIST_X + 28, COLS_B - 50, "\xE2\x86\x92", details_ ? "Hide details" : "Release details");
  }

  // The best version: frosted glass at rest, white only when focused (5.0:
  // no white slab on an OLED unless it is where you are).
  void draw_best(const SourceOption &o, bool focused) {
    Ctx &c = ctx();
    int x = BEST_X, y = BEST_Y, w = BEST_W, h = BEST_H;
    float lift = focused ? lift_.value : 0;
    sheet(x, y, w, h, 1.0f);
    if (focused) {
      float k = 1.0f + 0.03f * lift;
      int fw = static_cast<int>(w * k), fh = static_cast<int>(h * k);
      int fx = x - (fw - w) / 2, fy = y - (fh - h) / 2;
      c.gfx.shadow(fx, fy, fw, fh, glass_radius, 30, 14, static_cast<Uint8>(150 * lift));
      c.gfx.fill_round(fx, fy, fw, fh, glass_radius, rgba(0xffffff));
    }
    Color fg = focused ? rgba(0x000000) : label(Label::Primary);
    Color fg2 = focused ? rgba(0x000000, 166) : label(Label::Secondary);
    std::string kind = o.match == "same" ? "Same as your last episode" : o.match == "like" ? "Like your last episode" : "Best for this TV";
    text(Text::Caption1Semi, kind, x + 32, y + 22, fg2, w - 220);
    text(Text::Title3, o.headline, x + 32, y + 52, fg, w - 220);
    std::vector<std::string> facts;
    if (o.ready == 1) facts.push_back("Ready now");
    facts.push_back(o.size);
    facts.push_back(o.tier_label.empty() ? source_words(o.source) : o.tier_label);
    if (!o.cautions.empty()) facts.push_back(o.cautions[0]);
    text(Text::Caption1, join(facts), x + 32, y + 108, o.cautions.empty() ? fg2 : (focused ? caution_ink : caution), w - 220);
    // Play.
    int r = 40, cx = x + w - 32 - r, cy = y + h / 2;
    c.gfx.fill_circle(cx, cy, r, focused ? rgba(0x000000) : candidate);
    c.gfx.triangle_right(cx - 12, cy - 16, 32, focused ? rgba(0xffffff) : label(Label::Primary));
  }

  void draw_version(const SourceOption &o, int y, bool focused) {
    Ctx &c = ctx();
    int x = LIST_X + 14, w = LIST_W - 28;
    if (focused) {
      c.gfx.shadow(x, y, w, ROW, 20, 18, 8, 120);
      c.gfx.fill_round(x, y, w, ROW, 20, rgba(0xffffff));
    }
    Color fg = focused ? rgba(0x000000) : label(o.excluded ? Label::Disabled : Label::Primary);
    Color fg2 = focused ? rgba(0x000000, 166) : label(o.excluded ? Label::Disabled : Label::Secondary);
    int right = x + w - 18;
    // 5.0: line one says only what the columns have not already said.
    int block = line_for(Text::BodySemi) + 8 + 36;
    int ty = y + (ROW - block) / 2;
    std::string picture = o.picture_name.empty() ? (o.picture.empty() ? "SDR" : o.picture) : o.picture_name;
    std::vector<std::string> what;
    if (keys_[0].empty() || keys_[0] == "sdr-remux" || keys_[0] == "problems" || keys_[0] == "lower") what.push_back(o.resolution);
    if (keys_[1].empty()) what.push_back(picture);
    what.push_back(o.audio);
    int sw = o.size.empty() ? 0 : measure(Text::Caption1, o.size);
    text(Text::BodySemi, join(what), x + 18, ty, fg, right - x - 18 - sw - 24);
    if (!o.size.empty()) text_right(Text::Caption1, o.size, right, ty + 3, fg2);
    struct Chip {
      std::string text;
      int kind;  // 0 plain, 1 ready, 2 caution
    };
    std::vector<Chip> chips;
    if (o.ready == 1) chips.push_back({"Ready now", 1});
    if (!o.cautions.empty()) chips.push_back({o.cautions[0], 2});
    if (o.match == "same") chips.push_back({"Same as your last episode", 0});
    if (o.pack) chips.push_back({"Season pack", 0});
    if (keys_[2].empty() || dv_mode()) {
      if (o.tier != "pack" && o.tier != "other" && !o.tier_label.empty()) chips.push_back({o.tier_label, 0});
      else if (!o.source.empty()) chips.push_back({source_words(o.source), 0});
    }
    if (!o.cut.empty()) chips.push_back({o.cut, 0});
    for (auto &t : o.tags) chips.push_back({t, 0});
    if (!o.group.empty()) chips.push_back({o.group, 0});
    int cx = x + 18, cy = ty + line_for(Text::BodySemi) + 8;
    for (auto &chip : chips) {
      int cw = measure(Text::Caption2, chip.text) + 24;
      if (cx + cw > right) break;
      Color bg = chip.kind == 1 ? (focused ? rgba(0xd6f7e4) : rgba(0x50d28c, 60)) : focused ? rgba(0x000000, 28) : rgba(0xffffff, 34);
      Color ink = chip.kind == 1 ? (focused ? rgba(0x0a7a3d) : rgba(0xa6f3c6)) : chip.kind == 2 ? (focused ? caution_ink : caution)
                                                                        : focused ? rgba(0x000000, 200) : label(Label::Secondary);
      c.gfx.fill_round(cx, cy, cw, 36, 18, bg);
      text(Text::Caption2, chip.text, cx + 12, cy + (36 - line_for(Text::Caption2)) / 2, ink);
      cx += cw + 8;
    }
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
      struct StepRow { const char *id; const char *label; };
      std::vector<StepRow> steps = {{"adding", "Sending to AllDebrid"}};
      if (saw_caching_) steps.push_back({"caching", "AllDebrid is fetching it"});
      steps.push_back({"files", "Finding the right file"});
      steps.push_back({"unlocking", "Getting the stream"});
      int h = 330 + static_cast<int>(steps.size()) * 58 + (saw_caching_ ? 34 : 0), y = (H - h) / 2;
      sheet(x, y, w, h, a);
      std::string starting = chosen_.match == "same" ? "Starting \xC2\xB7 same version as your last episode"
                             : chosen_.match == "like" ? "Starting \xC2\xB7 like your last episode" : "Starting";
      text(Text::Caption1, starting, x + 56, y + 48, label(Label::Secondary, a));
      text(Text::Title3, chosen_.headline, x + 56, y + 84, label(Label::Primary, a), w - 112);
      static const char *order[] = {"starting", "adding", "caching", "files", "unlocking", "preparing", "ready"};
      auto rank = [](const std::string &id) {
        for (int i = 0; i < 7; i++) if (id == order[i]) return i;
        return 0;
      };
      int now_rank = rank(step_);
      int sy = y + 162;
      for (auto &st : steps) {
        std::string id = st.id;
        int r = rank(id);
        bool preparing = id == "unlocking" && step_ == "preparing";
        bool done = now_rank > r && !preparing;
        bool current = now_rank == r || preparing;
        if (done) c.icons.draw(Icon::Check, x + 56, sy, 30, fade(good, a));
        else if (current) draw_spinner(x + 71, sy + 15, 30, label(Label::Primary, a));
        else c.gfx.fill_circle(x + 71, sy + 15, 5, label(Label::Tertiary, a));
        text(Text::Body, st.label, x + 108, sy - 2, label(done ? Label::Secondary : current ? Label::Primary : Label::Tertiary, a));
        if (id == "caching" && fraction_ >= 0 && current) {
          draw_progress(x + 108, sy + 44, 520, 8, fraction_);
          text(Text::Callout, std::to_string(static_cast<int>(fraction_ * 100)) + "%", x + 648, sy + 32, label(Label::Secondary, a));
        }
        sy += id == "caching" ? 92 : 58;
      }
      GlassButton cancel;
      cancel.label = "Cancel";
      cancel.focused = sheet_focus_ == 0;
      cancel.variant = ButtonStyle::Default;
      cancel.alpha = a;
      int bx = x + 56 + draw_glass_button(x + 56, y + h - 100, cancel) + theme::stack_gap;
      if (can_show_others()) {
        GlassButton others;
        others.label = "Other versions";
        others.focused = sheet_focus_ == 1;
        others.variant = ButtonStyle::Default;
        others.alpha = a;
        draw_glass_button(bx, y + h - 100, others);
      }
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
  Ranking all_;  // the service's whole answer; ranking_ is the chosen cut's view of it
  std::string cut_;
  std::set<std::string> expanded_;  // tiers whose folded versions are shown
  Zone zone_ = Zone::Best;
  int col_ = 0;
  std::vector<Facet> columns_[3];
  int sel_[3] = {0, 0, 0};
  std::string keys_[3];
  std::vector<Toggle> toggles_;
  int toggle_ = 0;
  bool both_scopes_ = false, season_ = false, ready_only_ = false;
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
