#pragma once
// Artwork: fetched through the local service, decoded and scaled to the exact
// size it is drawn at on a worker thread (with rounded corners baked in), then
// uploaded as a texture. Least recently drawn textures are released when the
// budget is exceeded. Images fade in when they arrive.
#include <SDL.h>

#include <atomic>
#include <functional>
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
  void init(SDL_Renderer *renderer, Tasks *tasks, Api *api);
  void shutdown();
  // Request and (if ready) return an image drawn at w×h. Call every frame it is visible.
  // `shade` darkens the lower part so a title can sit on the image.
  // `blur` (px at w×h) returns a soft, quarter-resolution texture for frosted
  // backgrounds; draw it stretched to w×h.
  ImageRef get(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false, int blur = 0);
  // Glass: the frosted crop of `url` (drawn full-screen, cover) under `rect`,
  // blurred, tinted and lit at the top, corners rounded. Baked on a worker; half
  // resolution, drawn stretched to the rect.
  ImageRef frost(const std::string &url, GlassRect rect, int radius);
  // Warm the cache without drawing.
  void prefetch(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false, int blur = 0) {
    get(url, w, h, radius, fit, shade, blur);
  }
  // True once the image is decoded (or failed), so a transition can wait for it.
  bool settled(const std::string &url, int w, int h, int radius = 0, Fit fit = Fit::Cover, bool shade = false, int blur = 0);
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
    float luma = -1;
    std::shared_ptr<Shared> shared = std::make_shared<Shared>();
  };
  // `make` turns the decoded RGBA into the pixels to upload (on a worker thread).
  using Make = std::function<Pixels(const unsigned char *rgba, int sw, int sh)>;
  ImageRef lookup(const std::string &key, const std::string &url, const Make &make);
  void load(const std::string &key, const std::string &url, Make make);
  static std::string key_for(const std::string &url, int w, int h, int radius, Fit fit, bool shade, int blur);

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
Pixels scale_image(const unsigned char *rgba, int sw, int sh, int box_w, int box_h, Fit fit, int radius, bool shade = false);
// Three box-blur passes (≈ Gaussian) in place.
void blur_image(Pixels &p, int radius);
// Glass frost for `rect` of an image drawn cover at screen_w×screen_h (exposed for tests).
Pixels frost_image(const unsigned char *rgba, int sw, int sh, int screen_w, int screen_h, GlassRect rect, int radius);
// Mean brightness (0..1) of the lower left of a picture, where hero text sits.
float text_luma(const Pixels &p);
