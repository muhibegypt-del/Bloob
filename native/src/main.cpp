// Bloob native (Buna N2): SDL window and remote input, our own renderer and
// screens, the bundled local service for catalogue/sources/AllDebrid, and LG's
// media pipeline in a hardware plane beneath the UI for playback.
#include <SDL.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "http.h"
#include "json.h"
#include "keys.h"
#include "log.h"
#include "luna.h"
#include "screens.h"
#include "ui.h"

namespace {

constexpr int RAIL_W = 132;
constexpr long long kServiceBusWaitMs = 8000;
constexpr long long kServiceTimeoutMs = 30000;

struct LaunchParams {
  std::string uri, title = "Launch link", load_type = "media";
  bool autoplay = false;
};

LaunchParams parse_launch(int argc, char **argv) {
  LaunchParams params;
  if (argc < 2) return params;
  bool ok = false;
  Json json = Json::parse(argv[1], &ok);
  if (!ok) return params;
  params.uri = json["uri"].as_string();
  params.title = json["title"].as_string(params.title);
  params.load_type = json["loadType"].as_string(params.load_type);
  params.autoplay = json["autoplay"].as_bool(false);
  logf("app", "launch params: uri=%s type=%s autoplay=%d", params.uri.empty() ? "-" : redact_uri(params.uri).c_str(),
       params.load_type.c_str(), params.autoplay);
  return params;
}

struct RailItem {
  const char *label;
  Icon icon;
};
const RailItem kRail[] = {{"Home", Icon::Home}, {"Search", Icon::Search}, {"Films", Icon::Film}, {"Series", Icon::Tv},
                          {"Settings", Icon::Settings}};
constexpr int kRailCount = 5;

class App {
 public:
  bool init(int argc, char **argv) {
    Ctx &c = ctx();
    c.dir = app_directory();
    c.app_id = app_id();
    SDL_SetHint("SDL_WEBOS_ACCESS_POLICY_KEYS_BACK", "true");
    SDL_SetHint("SDL_WEBOS_ACCESS_POLICY_KEYS_EXIT", "true");
    SDL_SetHint("SDL_WEBOS_CURSOR_SLEEP_TIME", "5000");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
      logf("app", "SDL_Init failed: %s", SDL_GetError());
      return false;
    }
    c.webos.resolve();
    SDL_DisplayMode mode{};
    SDL_GetCurrentDisplayMode(0, &mode);
    int ww = mode.w > 0 ? mode.w : W, wh = mode.h > 0 ? mode.h : H;
    if (const char *forced = std::getenv("BLOOB_WINDOW")) std::sscanf(forced, "%dx%d", &ww, &wh);  // desktop tests
    c.window = SDL_CreateWindow("Bloob", 0, 0, ww, wh, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
    if (!c.window) {
      logf("app", "SDL_CreateWindow failed: %s", SDL_GetError());
      return false;
    }
    c.renderer = SDL_CreateRenderer(c.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!c.renderer) c.renderer = SDL_CreateRenderer(c.window, -1, 0);
    if (!c.renderer) {
      logf("app", "SDL_CreateRenderer failed: %s", SDL_GetError());
      return false;
    }
    SDL_RendererInfo info{};
    SDL_GetRendererInfo(c.renderer, &info);
    logf("app", "video=%s renderer=%s window=%dx%d display=%dx%d@%d missing=[%s]", SDL_GetCurrentVideoDriver(), info.name, ww,
         wh, mode.w, mode.h, mode.refresh_rate, c.webos.missing().c_str());
    c.gfx.init(c.renderer, W, H);
    std::string fonts = c.dir + "/assets/fonts/";
    c.fonts.regular.load(c.renderer, fonts + "Manrope-Regular.ttf");
    c.fonts.semibold.load(c.renderer, fonts + "Manrope-SemiBold.ttf");
    c.fonts.bold.load(c.renderer, fonts + "Manrope-Bold.ttf");
    c.fonts.heavy.load(c.renderer, fonts + "Manrope-ExtraBold.ttf");
    c.icons.init(c.renderer);

    wake_event_ = SDL_RegisterEvents(1);
    Uint32 wake = wake_event_;
    auto push_wake = [wake] {
      SDL_Event e{};
      e.type = wake;
      SDL_PushEvent(&e);
    };
    c.api_tasks.start(2, push_wake);
    c.image_tasks.start(3, push_wake);
    c.api.init(&c.api_tasks);
    c.images.init(c.renderer, &c.image_tasks, &c.api);
    Luna::instance().start(push_wake);

    c.push = [this](std::unique_ptr<Screen> s) { ops_.push_back({Op::Push, std::move(s)}); };
    c.pop = [this] { ops_.push_back({Op::Pop, nullptr}); };
    c.replace = [this](std::unique_ptr<Screen> s) { ops_.push_back({Op::Replace, std::move(s)}); };
    c.toast = [this](const std::string &text) {
      toast_ = text;
      toast_until_ = ctx().now + 3500;
    };
    c.quit = [this] { running_ = false; };
    c.refresh_home = [] {};

    Luna::instance().call("luna://com.webos.service.tv.systemproperty/getSystemInfo",
                          "{\"keys\":[\"modelName\",\"firmwareVersion\",\"sdkVersion\"]}", [](const std::string &reply) {
                            Json j = Json::parse(reply);
                            Ctx &cx = ctx();
                            if (j["returnValue"].as_bool()) {
                              cx.tv_model = j["modelName"].as_string("?") + " \xC2\xB7 webOS " + j["sdkVersion"].as_string("?");
                            } else {
                              cx.tv_model = "System info unavailable";
                            }
                            logf("app", "system info: %s", cx.tv_model.c_str());
                          });
    start_service();

    roots_.resize(kRailCount);
    root_ = 0;
    roots_[0] = make_browse(Browse::Home);

    load_script();
    LaunchParams launch = parse_launch(argc, argv);
    if (!launch.uri.empty() && launch.autoplay) {
      PlayRequest req;
      req.item.name = launch.title;
      req.video_id = "launch";
      ops_.push_back({Op::Push, make_player(req, launch.uri, "", launch.load_type)});
    }
    c.now = static_cast<long long>(SDL_GetTicks());
    return true;
  }

  void start_service() {
    Ctx &c = ctx();
    service_started_at_ = static_cast<long long>(SDL_GetTicks());
    std::string uri = "luna://" + c.app_id + ".service/start";
    logf("app", "starting service %s.service", c.app_id.c_str());
    Luna::instance().call(uri, "{}", [this](const std::string &reply) {
      Ctx &cx = ctx();
      Json j = Json::parse(reply);
      if (j["ready"].as_bool() && j["port"].as_int() > 0) {
        cx.api.set_port(static_cast<int>(j["port"].as_int()));
        logf("app", "service ready on port %d (version %s)", cx.api.port(), j["version"].as_string("?").c_str());
        cx.api.get("/api/settings", [](const ApiResult &r) {
          if (!r.ok()) return;
          Ctx &x = ctx();
          x.language = r.json["language"].as_string("English");
          x.recent_searches.clear();
          for (size_t i = 0; i < r.json["recentSearches"].size(); i++) x.recent_searches.push_back(r.json["recentSearches"][i].as_string());
        }, nullptr);
        cx.api.get("/api/account", [](const ApiResult &r) {
          if (r.ok()) ctx().paired = r.json["paired"].as_bool();
        }, nullptr);
      } else if (!j["returnValue"].as_bool(true) || j.has("errorText")) {
        logf("app", "service start over the bus failed: %s", reply.c_str());
        start_service_fallback();
      }
    });
  }

  // If the system bus will not start the service for us, run it with the TV's
  // own Node directly, as a child of the app, and stop it when the app exits.
  void start_service_fallback() {
    Ctx &c = ctx();
    if (fallback_tried_ || c.api.ready()) return;
    fallback_tried_ = true;
    std::string script = c.dir + "/../../services/" + c.app_id + ".service/server.js";
    const char *nodes[] = {std::getenv("BLOOB_NODE"), "/usr/bin/node", "/usr/bin/nodejs", "/usr/local/bin/node"};
    const char *node = nullptr;
    for (auto *n : nodes) if (n && access(n, X_OK) == 0) { node = n; break; }
    struct stat st{};
    if (!node || stat(script.c_str(), &st) != 0) {
      logf("app", "service fallback unavailable (node=%s, script %s)", node ? node : "none", stat(script.c_str(), &st) == 0 ? "found" : "missing");
      if (c.service_error.empty()) c.service_error = "Bloob's background service could not start.";
      return;
    }
    logf("app", "starting service directly with %s", node);
    // Everything the child needs is prepared before fork(): after it, only
    // async-signal-safe calls (other threads may hold locks).
    std::vector<std::string> env_strings;
    for (char **e = environ; *e; e++)
      if (std::strncmp(*e, "TV_APP_PORT=", 12) != 0) env_strings.push_back(*e);
    env_strings.push_back("TV_APP_PORT=8790");
    std::vector<char *> envp;
    for (auto &e : env_strings) envp.push_back(const_cast<char *>(e.c_str()));
    envp.push_back(nullptr);
    char *args[] = {const_cast<char *>(node), const_cast<char *>(script.c_str()), nullptr};
    pid_t pid = fork();
    if (pid == 0) {
      setsid();
      int log = open("/tmp/bloob-service.log", O_WRONLY | O_CREAT | O_TRUNC, 0600);
      if (log >= 0) {
        dup2(log, 1);
        dup2(log, 2);
      }
      execve(node, args, envp.data());
      _exit(127);
    }
    if (pid < 0) {
      c.service_error = "Bloob's background service could not start.";
      return;
    }
    fallback_pid_ = pid;
    // Wait for it to answer.
    c.api_tasks.run([]() -> std::function<void()> {
      for (int i = 0; i < 50; i++) {
        HttpResponse res = http_request("GET", 8790, "/api/health", "", 1000);
        if (res.status == 200) {
          return [] {
            ctx().api.set_port(8790);
            ctx().service_error.clear();
            logf("app", "service ready on port 8790 (started directly)");
            ctx().api.get("/api/account", [](const ApiResult &r) { if (r.ok()) ctx().paired = r.json["paired"].as_bool(); }, nullptr);
          };
        }
        usleep(300000);
      }
      return [] {
        ctx().service_error = "Bloob's background service did not start. Close Bloob and open it again.";
        logf("app", "direct service start timed out");
      };
    });
  }

  void shutdown() {
    Ctx &c = ctx();
    for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) (*it)->suspend();
    stack_.clear();
    roots_.clear();
    ops_.clear();
    // Let the last progress save reach the service.
    for (int i = 0; i < 75 && c.api_tasks.pending(); i++) SDL_Delay(20);
    // Stop the bundled service so nothing keeps running after Bloob closes.
    if (fallback_pid_ > 0) kill(fallback_pid_, SIGTERM);
    else if (c.api.ready()) Luna::instance().call("luna://" + c.app_id + ".service/stop", "{}", nullptr);
    SDL_Delay(50);
    Luna::instance().pump();
    c.api_tasks.stop();
    c.image_tasks.stop();
    Luna::instance().stop();
    c.images.shutdown();
    c.icons.shutdown();
    c.gfx.shutdown();
    if (c.renderer) SDL_DestroyRenderer(c.renderer);
    if (c.window) SDL_DestroyWindow(c.window);
    SDL_Quit();
  }

  // ---------------------------------------------------------------- navigation

  Screen *top() {
    if (!stack_.empty()) return stack_.back().get();
    return roots_[static_cast<size_t>(root_)].get();
  }

  void ensure_root(int index) {
    if (roots_[static_cast<size_t>(index)]) return;
    switch (index) {
      case 0: roots_[0] = make_browse(Browse::Home); break;
      case 1: roots_[1] = make_search(); break;
      case 2: roots_[2] = make_browse(Browse::Movies); break;
      case 3: roots_[3] = make_browse(Browse::Series); break;
      case 4: roots_[4] = make_settings(); break;
    }
  }

  void apply_ops() {
    while (!ops_.empty()) {
      Op op = std::move(ops_.front());
      ops_.pop_front();
      Screen *before = top();
      switch (op.kind) {
        case Op::Push:
          before->hidden();
          stack_.push_back(std::move(op.screen));
          logf("nav", "push %s", stack_.back()->name());
          break;
        case Op::Pop:
          if (stack_.empty()) break;
          logf("nav", "pop %s", stack_.back()->name());
          stack_.pop_back();
          top()->shown();
          break;
        case Op::Replace:
          if (!stack_.empty()) {
            logf("nav", "replace %s with %s", stack_.back()->name(), op.screen->name());
            stack_.back() = std::move(op.screen);
          } else {
            stack_.push_back(std::move(op.screen));
          }
          break;
      }
      dirty_ = true;
    }
  }

  void select_root(int index) {
    if (index == root_) return;
    roots_[static_cast<size_t>(root_)]->hidden();
    ensure_root(index);
    root_ = index;
    roots_[static_cast<size_t>(root_)]->set_rail_focus(rail_focus_);
    roots_[static_cast<size_t>(root_)]->shown();
  }

  void set_rail_focus(bool focus) {
    rail_focus_ = focus;
    rail_index_ = root_;
    roots_[static_cast<size_t>(root_)]->set_rail_focus(focus);
  }

  // ---------------------------------------------------------------- input

  void on_key(const SDL_KeyboardEvent &event) {
    Ctx &c = ctx();
    Key key = key_from_event(event);
    char buf[160];
    std::snprintf(buf, sizeof buf, "%s  \xC2\xB7  scancode %d  \xC2\xB7  keycode %d", key_name(key),
                  static_cast<int>(event.keysym.scancode), static_cast<int>(event.keysym.sym));
    c.last_key = buf;
    if (key == Key::None) {
      logf("input", "unmapped key: scancode=%d sym=%d", event.keysym.scancode, event.keysym.sym);
      return;
    }
    dirty_ = true;
    if (key == Key::Exit) {
      running_ = false;
      return;
    }
    bool repeat = event.repeat != 0;
    if (!stack_.empty()) {
      stack_.back()->key(key, repeat);
      return;
    }
    Screen *root = roots_[static_cast<size_t>(root_)].get();
    if (rail_focus_) {
      switch (key) {
        case Key::Up:
          if (rail_index_ > 0) select_root(--rail_index_);
          break;
        case Key::Down:
          if (rail_index_ + 1 < kRailCount) select_root(++rail_index_);
          break;
        case Key::Right:
        case Key::Ok:
          set_rail_focus(false);
          break;
        case Key::Back:
          if (root_ != 0) {
            select_root(0);
            rail_index_ = 0;
          } else {
            running_ = false;
          }
          break;
        default:
          break;
      }
      return;
    }
    if (!root->key(key, repeat) && (key == Key::Left || key == Key::Back)) set_rail_focus(true);
  }

  void on_event(const SDL_Event &e) {
    if (e.type == SDL_QUIT) running_ = false;
    else if (e.type == SDL_KEYDOWN) on_key(e.key);
    else if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST || e.window.event == SDL_WINDOWEVENT_HIDDEN) {
        // Moved to the background (Home button, input switch): pause and save.
        top()->suspend();
        logf("app", "window hidden/focus lost");
      }
      dirty_ = true;
    } else if (e.type == SDL_APP_WILLENTERBACKGROUND) {
      top()->suspend();
    }
  }

  // ---------------------------------------------------------------- drawing

  void draw_rail() {
    Ctx &c = ctx();
    rail_open_.to(rail_focus_ ? 1.0f : 0.0f);
    float o = rail_open_.value;
    int w = RAIL_W + static_cast<int>(188 * o);
    c.gfx.fill_rect(0, 0, w, H, with_alpha(rgba(0x07080a), static_cast<int>(140 + 100 * o)));
    if (o > 0.01f) c.gfx.gradient_h(w, 0, 160, H, rgba(0x07080a), static_cast<Uint8>(200 * o), 0);
    c.gfx.fill_round(40, 56, 52, 52, 16, theme::accent);
    draw_text_center(c.fonts.heavy, "b", 66, 61, 30, theme::ink);
    for (int i = 0; i < kRailCount; i++) {
      int y = i == kRailCount - 1 ? H - 128 : 168 + i * 82;
      bool active = i == root_;
      bool focused = rail_focus_ && i == rail_index_;
      int bw = 64 + static_cast<int>(172 * o);
      if (focused) c.gfx.fill_round(34, y, bw, 64, 20, theme::white);
      else if (active) c.gfx.fill_round(34, y, bw, 64, 20, with_alpha(theme::white, 30));
      Color fg = focused ? theme::ink : active ? theme::white : rgba(0x8d929b);
      c.icons.draw(kRail[i].icon, 51, y + 17, 30, fg);
      if (o > 0.05f) c.fonts.bold.draw(kRail[i].label, 100, y + 18, 24, with_alpha(fg, static_cast<int>(255 * o)));
    }
  }

  void render() {
    Ctx &c = ctx();
    c.images.begin_frame(c.now);
    c.gfx.begin_frame();
    Screen *screen = top();
    screen->draw();
    if (stack_.empty()) draw_rail();
    if (!toast_.empty() && c.now < toast_until_) {
      int w = c.fonts.semibold.measure(toast_, 24) + 64;
      c.gfx.fill_round(W / 2 - w / 2, H - 140, w, 64, 32, with_alpha(theme::panel2, 240));
      draw_text_center(c.fonts.semibold, toast_, W / 2, H - 123, 24, theme::text);
      c.redraw_at(toast_until_);
    }
    c.gfx.end_frame();
    if (c.images.end_frame()) c.animating = true;
    SDL_RenderPresent(c.renderer);
    dirty_ = false;
  }

  // ---------------------------------------------------------------- scripted runs

  // BLOOB_SCRIPT: steps separated by '|': key:<name>, wait:<ms>, media:<event json>, pump, snap:<name>, quit.
  void load_script() {
    const char *spec = std::getenv("BLOOB_SCRIPT");
    const char *dir_env = std::getenv("BLOOB_SNAPSHOT_DIR");
    snapshot_dir_ = dir_env ? dir_env : ".";
    if (!spec) return;
    std::string all = spec;
    size_t start = 0;
    while (start <= all.size()) {
      size_t end = all.find('|', start);
      std::string step = all.substr(start, end == std::string::npos ? std::string::npos : end - start);
      size_t colon = step.find(':');
      if (!step.empty()) script_.push_back({step.substr(0, colon), colon == std::string::npos ? "" : step.substr(colon + 1)});
      if (end == std::string::npos) break;
      start = end + 1;
    }
    logf("script", "%zu steps", script_.size());
  }

  void press(const std::string &name) {
    static const std::pair<const char *, int> codes[] = {
        {"up", SDL_SCANCODE_UP}, {"down", SDL_SCANCODE_DOWN}, {"left", SDL_SCANCODE_LEFT}, {"right", SDL_SCANCODE_RIGHT},
        {"ok", SDL_SCANCODE_RETURN}, {"back", kScancodeWebosBack}, {"play", SDL_SCANCODE_AUDIOPLAY},
        {"pause", SDL_SCANCODE_PAUSE}, {"red", kScancodeWebosRed}};
    SDL_KeyboardEvent key{};
    key.type = SDL_KEYDOWN;
    for (auto &c : codes) if (name == c.first) key.keysym.scancode = static_cast<SDL_Scancode>(c.second);
    on_key(key);
  }

  void snapshot(const std::string &name) {
    Ctx &c = ctx();
    render();
    if (!c.gfx.canvas()) return;
    SDL_SetRenderTarget(c.renderer, c.gfx.canvas());
    SDL_Surface *surface = SDL_CreateRGBSurface(0, W, H, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (surface) {
      SDL_RenderReadPixels(c.renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
      std::string path = snapshot_dir_ + "/" + name + ".bmp";
      SDL_SaveBMP(surface, path.c_str());
      SDL_FreeSurface(surface);
      logf("script", "snapshot %s", path.c_str());
    }
    SDL_SetRenderTarget(c.renderer, nullptr);
  }

  void run_script(long long t) {
    while (script_pos_ < script_.size() && t >= script_at_) {
      const Step &step = script_[script_pos_++];
      if (step.kind == "wait") {
        script_at_ = t + std::atoll(step.arg.c_str());
        return;
      }
      if (step.kind == "key") press(step.arg);
      else if (step.kind == "media") Luna::instance().inject(step.arg);
      else if (step.kind == "pump") Luna::instance().pump();
      else if (step.kind == "snap") snapshot(step.arg);
      else if (step.kind == "quit") running_ = false;
      apply_ops();
      dirty_ = true;
    }
  }

  // ---------------------------------------------------------------- loop

  void loop() {
    Ctx &c = ctx();
    long long last = static_cast<long long>(SDL_GetTicks());
    while (running_) {
      long long now = static_cast<long long>(SDL_GetTicks());
      int timeout;
      if (c.animating || dirty_ || !ops_.empty()) timeout = 0;
      else if (script_pos_ < script_.size()) timeout = 10;
      else if (c.next_redraw) timeout = static_cast<int>(std::max(0LL, std::min(1000LL, c.next_redraw - now)));
      else timeout = 1000;
      SDL_Event e;
      if (SDL_WaitEventTimeout(&e, timeout)) {
        if (e.type == wake_event_) dirty_ = true;
        else on_event(e);
        while (SDL_PollEvent(&e)) {
          if (e.type == wake_event_) dirty_ = true;
          else on_event(e);
        }
      }
      Luna::instance().pump();
      c.api_tasks.pump();
      c.image_tasks.pump();
      now = static_cast<long long>(SDL_GetTicks());
      c.dt = std::min(0.1, (now - last) / 1000.0);
      last = now;
      c.now = now;
      if (!script_.empty()) run_script(now);
      apply_ops();
      if (!c.api.ready() && !fallback_tried_ && now - service_started_at_ > kServiceBusWaitMs) {
        logf("app", "no reply from the service over the bus");
        start_service_fallback();
      }
      if (!c.api.ready() && c.service_error.empty() && now - service_started_at_ > kServiceTimeoutMs) {
        c.service_error = "Bloob's background service did not start. Close Bloob and open it again.";
        logf("app", "service start timed out");
      }
      bool was_animating = c.animating;
      bool redraw_due = c.next_redraw && now >= c.next_redraw;
      c.animating = false;
      c.next_redraw = 0;
      top()->update(c.dt);
      if (stack_.empty() && rail_open_.step(c.dt)) c.animating = true;
      apply_ops();
      if (dirty_ || was_animating || c.animating || redraw_due) render();
    }
  }

 private:
  struct Step {
    std::string kind, arg;
  };
  struct Op {
    enum Kind { Push, Pop, Replace } kind;
    std::unique_ptr<Screen> screen;
  };

  Uint32 wake_event_ = 0;
  bool running_ = true;
  bool dirty_ = true;
  std::vector<std::unique_ptr<Screen>> roots_;
  std::vector<std::unique_ptr<Screen>> stack_;
  std::deque<Op> ops_;
  int root_ = 0;
  bool rail_focus_ = false;
  int rail_index_ = 0;
  Anim rail_open_;
  std::string toast_;
  long long toast_until_ = 0;
  long long service_started_at_ = 0;
  bool fallback_tried_ = false;
  pid_t fallback_pid_ = -1;
  std::vector<Step> script_;
  size_t script_pos_ = 0;
  long long script_at_ = 0;
  std::string snapshot_dir_;
};

}  // namespace

int main(int argc, char **argv) {
  log_init("/tmp/bloob-native.log");
  logf("app", "Bloob native starting");
  App app;
  if (!app.init(argc, argv)) {
    logf("app", "startup failed");
    app.shutdown();
    return 1;
  }
  app.loop();
  logf("app", "exiting");
  app.shutdown();
  return 0;
}
