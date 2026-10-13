#pragma once
// Artwork: fetched through the local service, decoded and scaled to the exact
// size it is drawn at on a worker thread (with rounded corners baked in), then
// uploaded as a texture. Least recently drawn textures are released when the
// budget is exceeded. Images fade in when they arrive.
#include <SDL.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>

class Api;
class Tasks;

enum class Fit { Cover, Contain };

struct ImageRef {
  SDL_Texture *texture = nullptr;
  int w = 0, h = 0;      // texture size (Contain: fitted size within the box)
  Uint8 alpha = 0;       // fade-in progress
  bool failed = false;
};

class Images {
 public:
  void init(SDL_Renderer *renderer, Tasks *tasks, Api *api);
  void shutdown();
  // Request and (if ready) return an image drawn at w×h. Call every frame it is visible.
  // `shade` darkens the lower part so a title can sit on the image.
  ImageRef get(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false);
  // Warm the cache without drawing.
  void prefetch(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false) {
    get(url, w, h, radius, fit, shade);
  }
  void begin_frame(long long now) { now_ = now; }
  // Release textures over budget. Returns true while any visible image is fading in.
  bool end_frame();
  void on_loaded() { loaded_ = true; }
  size_t bytes() const { return bytes_; }

 private:
  struct Shared {
    std::atomic<long long> wanted{0};
  };
  struct Entry {
    SDL_Texture *texture = nullptr;
    int w = 0, h = 0;
    size_t bytes = 0;
    enum State { Idle, Loading, Ready, Failed } state = Idle;
    long long ready_at = 0;
    long long used_at = 0;
    long long failed_at = 0;
    std::shared_ptr<Shared> shared = std::make_shared<Shared>();
  };
  void load(const std::string &key, const std::string &url, int w, int h, int radius, Fit fit, bool shade);

  SDL_Renderer *renderer_ = nullptr;
  Tasks *tasks_ = nullptr;
  Api *api_ = nullptr;
  std::map<std::string, Entry> entries_;
  size_t bytes_ = 0;
  size_t budget_ = 192u * 1024 * 1024;
  long long now_ = 0;
  bool fading_ = false;
  bool loaded_ = false;
  std::shared_ptr<int> alive_ = std::make_shared<int>(0);
};

// Scale RGBA pixels (exposed for tests). Cover crops to fill; Contain fits inside.
struct Pixels {
  std::string data;  // RGBA8
  int w = 0, h = 0;
};
Pixels scale_image(const unsigned char *rgba, int sw, int sh, int box_w, int box_h, Fit fit, int radius, bool shade = false);
