#include "model.h"

static std::vector<std::string> strings(const Json &list) {
  std::vector<std::string> out;
  for (size_t i = 0; i < list.size(); i++) {
    std::string s = list[i].as_string();
    if (!s.empty()) out.push_back(s);
  }
  return out;
}

Item parse_item(const Json &j) {
  Item it;
  it.id = j["id"].as_string();
  it.type = j["type"].as_string("movie");
  it.name = j["name"].as_string();
  it.year = j["year"].as_string();
  it.runtime = j["runtime"].as_string();
  it.rating = j["rating"].as_string();
  it.description = j["description"].as_string();
  it.genres = strings(j["genres"]);
  it.tile = j["tile"].as_string();
  it.backdrop = j["backdrop"].as_string();
  it.logo = j["logo"].as_string();
  it.poster = j["poster"].as_string();
  return it;
}

Resume parse_resume(const Json &j) {
  Resume r;
  r.item = parse_item(j);
  r.kind = j["kind"].as_string("resume");
  r.video_id = j["videoId"].as_string();
  r.episode_title = j["episodeTitle"].as_string();
  r.season = static_cast<int>(j["season"].as_int());
  r.episode = static_cast<int>(j["episode"].as_int());
  r.position = j["position"].as_int();
  r.duration = j["duration"].as_int();
  const Json &n = j["next"];
  if (n.is_object()) {
    NextEpisode next;
    next.video_id = n["videoId"].as_string();
    next.title = n["title"].as_string();
    next.season = static_cast<int>(n["season"].as_int());
    next.episode = static_cast<int>(n["episode"].as_int());
    r.next = next;
  }
  return r;
}

Meta parse_meta(const Json &j) {
  Meta m;
  m.item = parse_item(j);
  m.cast = strings(j["cast"]);
  m.director = strings(j["director"]);
  m.released = j["released"].as_string();
  const Json &seasons = j["seasons"];
  for (size_t i = 0; i < seasons.size(); i++) {
    Season s;
    s.season = static_cast<int>(seasons[i]["season"].as_int());
    s.label = seasons[i]["label"].as_string();
    const Json &eps = seasons[i]["episodes"];
    for (size_t k = 0; k < eps.size(); k++) {
      Episode e;
      e.id = eps[k]["id"].as_string();
      e.title = eps[k]["title"].as_string();
      e.overview = eps[k]["overview"].as_string();
      e.released = eps[k]["released"].as_string();
      e.thumbnail = eps[k]["thumbnail"].as_string();
      e.season = static_cast<int>(eps[k]["season"].as_int());
      e.episode = static_cast<int>(eps[k]["episode"].as_int());
      s.episodes.push_back(e);
    }
    if (!s.episodes.empty()) m.seasons.push_back(s);
  }
  return m;
}

std::map<std::string, Watch> parse_progress(const Json &j) {
  std::map<std::string, Watch> out;
  for (auto &entry : j.items()) {
    Watch w;
    w.position = entry.second["position"].as_int();
    w.duration = entry.second["duration"].as_int();
    w.watched = entry.second["watched"].as_bool();
    w.updated = entry.second["updated"].as_int();
    out[entry.first] = w;
  }
  return out;
}

static SourceOption parse_option(const Json &j) {
  SourceOption o;
  o.id = j["id"].as_string();
  o.headline = j["headline"].as_string();
  o.resolution = j["resolution"].as_string();
  o.picture = j["picture"].as_string();
  o.audio = j["audio"].as_string();
  o.size = j["size"].as_string();
  o.availability = j["availability"].as_string();
  o.facts = j["facts"].as_string();
  o.release = j["release"].as_string();
  o.notes = strings(j["notes"]);
  o.warnings = strings(j["warnings"]);
  o.cautions = strings(j["cautions"]);
  o.excluded = j["excluded"].as_bool();
  return o;
}

Ranking parse_ranking(const Json &j) {
  Ranking r;
  if (j["recommended"].is_object()) r.recommended = parse_option(j["recommended"]);
  r.why = j["why"].as_string();
  r.provider = j["provider"].as_string();
  r.total = static_cast<int>(j["total"].as_int());
  const Json &groups = j["groups"];
  for (size_t i = 0; i < groups.size(); i++) {
    SourceGroup g;
    g.label = groups[i]["label"].as_string();
    for (size_t k = 0; k < groups[i]["items"].size(); k++) g.items.push_back(parse_option(groups[i]["items"][k]));
    r.groups.push_back(g);
  }
  return r;
}

std::optional<NextEpisode> next_episode(const Meta &meta, const std::string &video_id) {
  bool found = false;
  for (auto &season : meta.seasons) {
    if (season.season == 0) continue;  // specials are not "next"
    for (auto &e : season.episodes) {
      if (found) return NextEpisode{e.id, e.title, e.season, e.episode};
      if (e.id == video_id) found = true;
    }
  }
  return std::nullopt;
}

std::string episode_code(int season, int episode) {
  return "S" + std::to_string(season) + " \xC2\xB7 E" + std::to_string(episode);
}

std::string PlayRequest::label() const {
  if (!item.series()) return "";
  std::string code = episode_code(season, episode);
  return episode_title.empty() ? code : code + " \xC2\xB7 " + episode_title;
}
