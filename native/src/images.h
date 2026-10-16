#pragma once
// Artwork: fetched through the local service, decoded once per source picture,
// scaled to the exact size it is drawn at (rounded corners, blur and frost
// baked in) on worker threads, then uploaded as a texture on the UI thread.
//
// 2.4.0 pipeline (R5b, R5c; Elements of A resource study A1-A6, M3-M8, G12):
//  - two stages on separate pools: fetch (I/O) and decode/scale (CPU);
//  - one decode per source URL feeds every variant (sharp, blurred, frost);
//  - every request carries a relevance (visible, near, prefetch) that orders
//    the queues, so visible work never waits behind prefetch;
//  - staged cancellation: a request nobody drew for 30 frames is retired, and
//    workers check again after the fetch and before decoding and scaling;
//  - uploads are paced (a few per frame, highest relevance first) and admitted
//    against the app's GPU budget BEFORE a texture is created;
//  - eviction takes the least relevant, oldest texture not drawn in the last
//    3 frames; an evicted image keeps its record and reloads without a fade;
//  - in the background every texture and decoded source is released and no
//    new fetch starts until the app is back.
#include <SDL.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "resources.h"

class Api;
class Tasks;

enum class Fit { Cover, Contain };

struct ImageRef {
  SDL_Texture *texture = nullptr;
  int w = 0, h = 0;      // texture size (Contain: fitted size within the box)
  Uint8 alpha = 0;       // fade-in progress
  bool failed = false;
  float luma = -1;       // full-screen art: brightness where hero text sits (0..1), -1 unknown
};

struct GlassRect {
  int x = 0, y = 0, w = 0, h = 0;
};

// RGBA8 pixels.
struct Pixels {
  std::string data;  // RGBA8
  int w = 0, h = 0;
};

class Images {
 public:
  void init(SDL_Renderer *renderer, Tasks *decode_tasks, Tasks *fetch_tasks, Api *api);
  void shutdown();
  // Request and (if ready) return an image drawn at w×h. Call every frame it is visible.
  // `shade` darkens the lower part so a title can sit on the image.
  // `blur` (px at w×h) returns a soft, quarter-resolution texture for frosted
  // backgrounds; draw it stretched to w×h.
  ImageRef get(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false, int blur = 0,
               Relevance relevance = Relevance::Visible);
  // Glass: the frosted crop of `url` (drawn full-screen, cover) under `rect`,
  // blurred, tinted and lit at the top, corners rounded. Baked on a worker; half
  // resolution, drawn stretched to the rect.
  ImageRef frost(const std::string &url, GlassRect rect, int radius);
  // Warm the cache without drawing (lowest relevance).
  void prefetch(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false, int blur = 0,
                Relevance relevance = Relevance::Prefetch) {
    get(url, w, h, radius, fit, shade, blur, relevance);
  }
  // True once the image is decoded (or failed), so a transition can wait for it.
  bool settled(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false, int blur = 0);
  void begin_frame(long long now) { now_ = now; }
  // Creates textures for decoded images, paced and admitted against the GPU
  // budget. Returns the number uploaded.
  int upload();
  // Retires stale work, enforces the budget, advances the frame counter.
  // Returns true while any visible image is fading in.
  bool end_frame();
  bool pending_uploads() const { return !pending_.empty(); }
  // Background: release every texture and decoded source; no new fetches.
  void release_handles();
  void set_suspended(bool suspended);
  size_t bytes() const { return gpu_budget().used(GpuClass::Images); }

  struct Stats {
    size_t entries = 0, resident = 0, waiting = 0, making = 0, uploading = 0, failed = 0;
    size_t resident_bytes[3] = {0, 0, 0};  // by relevance
    size_t sources = 0, sources_ready = 0, sources_fetching = 0, sources_decoding = 0;
    long long source_bytes = 0;
    long long uploads = 0, upload_bytes = 0, deferred = 0, evictions = 0, cancelled = 0, overruns = 0, fetches = 0, decodes = 0,
              variants = 0;
    long long frame = 0;
  };
  Stats stats() const;

 private:
  struct Decoded;
  using Make = std::function<Pixels(const unsigned char *rgba, int sw, int sh)>;
  struct Entry {
    std::string url;
    Make make;
    SDL_Texture *texture = nullptr;
    int w = 0, h = 0;
    enum State { Idle, Waiting, Making, Uploading, Ready, Failed } state = Idle;
    Evictable ev;
    bool seen = false;           // was resident once: reloads without fading in
    long long ready_at = 0;
    long long failed_at = 0;
    long long deferred_since = -1;
    float luma = -1;
    std::shared_ptr<Pixels> pending;
    std::shared_ptr<std::atomic<long long>> wanted = std::make_shared<std::atomic<long long>>(0);
  };
  struct Source {
    enum State { Idle, Fetching, Decoding, Ready, Failed } state = Idle;
    std::shared_ptr<const Decoded> decoded;
    long long bytes = 0;
    long long last_frame = 0;
    long long failed_at = 0;
    int lane = 2;
    std::vector<std::string> waiters;
    std::shared_ptr<std::atomic<long long>> wanted = std::make_shared<std::atomic<long long>>(0);
  };

  ImageRef lookup(const std::string &key, const std::string &url, const Make &make, Relevance relevance);
  void request_source(const std::string &url, const std::string &key, int lane);
  void start_fetch(const std::string &url);
  void decode(const std::string &url, std::shared_ptr<std::string> bytes);
  void source_ready(const std::string &url);
  void source_failed(const std::string &url, bool cancelled);
  void make_variant(const std::string &key, const std::shared_ptr<const Decoded> &decoded);
  void evict(const std::string &key);
  void drop_source(const std::string &url);
  static std::string key_for(const std::string &url, int w, int h, int radius, Fit fit, bool shade, int blur);

  SDL_Renderer *renderer_ = nullptr;
  Tasks *decode_tasks_ = nullptr;
  Tasks *fetch_tasks_ = nullptr;
  Api *api_ = nullptr;
  std::map<std::string, Entry> entries_;
  std::map<std::string, Source> sources_;
  std::vector<std::string> pending_;  // keys with decoded pixels awaiting upload
  long long now_ = 0;
  long long frame_ = 1;
  std::shared_ptr<std::atomic<long long>> frame_atomic_ = std::make_shared<std::atomic<long long>>(1);
  bool fading_ = false;
  bool loaded_ = false;
  bool suspended_ = false;
  long long uploads_ = 0, upload_bytes_ = 0, deferred_ = 0, evictions_ = 0, cancelled_ = 0, fetches_ = 0, decodes_ = 0,
            variants_ = 0;
  std::shared_ptr<int> alive_ = std::make_shared<int>(0);
};

// Scale RGBA pixels (exposed for tests). Cover crops to fill; Contain fits inside.
Pixels scale_image(const unsigned char *rgba, int sw, int sh, int box_w, int box_h, Fit fit, int radius, bool shade = false);
// Three box-blur passes (≈ Gaussian) in place.
void blur_image(Pixels &p, int radius);
// Glass frost for `rect` of an image drawn cover at screen_w×screen_h (exposed for tests).
Pixels frost_image(const unsigned char *rgba, int sw, int sh, int screen_w, int screen_h, GlassRect rect, int radius);
// Mean brightness (0..1) of the lower left of a picture, where hero text sits.
float text_luma(const Pixels &p);
