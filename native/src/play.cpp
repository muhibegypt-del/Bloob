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

static const Episode *find_episode(const Meta &meta, const std::string &id) {
  for (auto &s : meta.seasons)
    for (auto &e : s.episodes)
      if (e.id == id) return &e;
  return nullptr;
}

PlayRequest title_default(const Meta &meta, const std::map<std::string, Watch> &progress, const Target &target,
                          const std::string &today) {
  if (target.valid()) {
    bool resume = target.action == "resume";
    if (!meta.item.series()) {
      PlayRequest req = film(meta.item, progress, !resume);
      if (resume && target.position > 0) req.resume_ms = target.position;
      return req;
    }
    std::string id = target.action == "caughtUp" ? target.again_id : target.video_id;
    if (const Episode *e = find_episode(meta, id)) {
      PlayRequest req = episode(meta, *e, progress, !resume);
      if (resume && target.position > 0) req.resume_ms = target.position;
      return req;
    }
  }
  return title_default(meta, progress, today);
}

PlayRequest title_default(const Meta &meta, const std::map<std::string, Watch> &progress, const std::string &today) {
  if (!meta.item.series()) return film(meta.item, progress, false);
  const Episode *latest = nullptr;
  long long latest_at = -1;
  for (auto &s : meta.seasons)
    for (auto &e : s.episodes) {
      auto it = progress.find(e.id);
      if (it != progress.end() && it->second.updated > latest_at) {
        latest = &e;
        latest_at = it->second.updated;
      }
    }
  if (latest) {
    if (resume_point(progress.at(latest->id)) > 0) return episode(meta, *latest, progress, false);
    if (auto n = next_episode(meta, latest->id))
      if (aired(n->released, today))
        if (const Episode *e = find_episode(meta, n->video_id)) return episode(meta, *e, progress, true);
  }
  for (auto &s : meta.seasons)
    if (s.season != 0 && !s.episodes.empty()) return episode(meta, s.episodes[0], progress, true);
  if (!meta.seasons.empty() && !meta.seasons[0].episodes.empty()) return episode(meta, meta.seasons[0].episodes[0], progress, true);
  return PlayRequest{};
}

PlayRequest continue_item(const Resume &r) {
  PlayRequest req;
  req.item = r.item;
  if (r.kind == "upcoming") return req;
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
    Watch w;
    w.position = r.position;
    w.duration = r.duration;
    req.resume_ms = resume_point(w);
    req.next = r.next;
  }
  return req;
}

std::optional<PlayRequest> after(const PlayRequest &current, const std::string &today) {
  if (!current.next || !aired(current.next->released, today)) return std::nullopt;
  PlayRequest req;
  req.item = current.item;
  req.video_id = current.next->video_id;
  req.season = current.next->season;
  req.episode = current.next->episode;
  req.episode_title = current.next->title;
  return req;
}

}  // namespace play
