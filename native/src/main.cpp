// Bloob native: the app shell. SDL window and remote input, the tab bar and
// screen stack with animated transitions, app lifecycle, the bundled local
// service (catalogue, sources, AllDebrid), and scripted desktop runs.
#include <SDL.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "alloc.h"
#include "clock.h"
#include "frames.h"
#include "http.h"
#include "inspect.h"
#include "apps.h"
#include "json.h"
#include "keys.h"
#include "log.h"
#include "perf.h"
#include "luna.h"
#include "screens.h"
#include "speech.h"
#include "ui.h"

namespace {

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

// Tab bar: Search and Settings are icons at the ends, pages in between.
struct Tab {
  const char *label;
  int icon;  // Icon value or -1 for a text tab
};
// YouTube (2.6.1, the owner's only other app) is a tab that opens the ad-free
// YouTube app on OK; it has no page and is shown only when the app is installed.
const Tab kTabs[] = {{"Search", static_cast<int>(Icon::Search)}, {"Home", -1}, {"Films", -1}, {"Series", -1},
                     {"YouTube", -1}, {"Settings", static_cast<int>(Icon::Settings)}};
constexpr int kTabCount = 6;
constexpr int kHomeTab = 1;
constexpr int kYouTubeTab = 4;
const char *kYouTubeId = "youtube.leanback.v4";
const char *kHbExec = "luna://org.webosbrew.hbchannel.service/exec";
constexpr long long kExitConfirmMs = 2500;

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
    // Study G6: let SDL batch compatible draws (SDL 2.0.10+; ignored by older SDL).
    SDL_SetHint("SDL_RENDER_BATCHING", "1");
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
    c.mark("window");
    SDL_RendererInfo info{};
    SDL_GetRendererInfo(c.renderer, &info);
    logf("app", "video=%s renderer=%s window=%dx%d display=%dx%d@%d missing=[%s]", SDL_GetCurrentVideoDriver(), info.name, ww,
         wh, mode.w, mode.h, mode.refresh_rate, c.webos.missing().c_str());
    SDL_version compiled, linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    sdl_version_ = std::to_string(linked.major) + "." + std::to_string(linked.minor) + "." + std::to_string(linked.patch);
    renderer_name_ = info.name ? info.name : "?";
    refresh_hz_ = mode.refresh_rate;
    logf("app", "SDL linked %s (built against %d.%d.%d), renderer flags 0x%x, max texture %dx%d, vsync %s",
         sdl_version_.c_str(), compiled.major, compiled.minor, compiled.patch, info.flags, info.max_texture_width,
         info.max_texture_height, (info.flags & SDL_RENDERER_PRESENTVSYNC) ? "on" : "off");
    pacer_.set_refresh_hz(mode.refresh_rate);
    gate_frames_ = !(info.flags & SDL_RENDERER_PRESENTVSYNC);
    sdl_offset_ = now_ms() - static_cast<long long>(SDL_GetTicks());
    c.gfx.init(c.renderer, W, H);
    std::string fonts = c.dir + "/assets/fonts/";
    c.fonts.regular.load(c.renderer, fonts + "Inter-Regular.ttf");
    c.fonts.medium.load(c.renderer, fonts + "Inter-Medium.ttf");
    c.fonts.semibold.load(c.renderer, fonts + "Inter-SemiBold.ttf");
    c.fonts.bold.load(c.renderer, fonts + "Inter-Bold.ttf");
    c.icons.init(c.renderer);
    c.mark("fonts");

    // Wakes are notifications only (study C2): each worker pool and the bus has
    // its own payload queue; the first producer after a drain posts one SDL
    // event, later ones only count. The loop drains everything it wakes for.
    wake_event_ = SDL_RegisterEvents(1);
    c.api_tasks.start(2, wake_for(kWakeApi), "bl-api");
    c.fetch_tasks.start(4, wake_for(kWakeFetch), "bl-fetch", 256 * 1024);
    c.image_tasks.start(3, wake_for(kWakeImage), "bl-img");
    c.api.init(&c.api_tasks);
    c.images.init(c.renderer, &c.image_tasks, &c.fetch_tasks, &c.api);
    gpu_budget().set_limit(budget_for(MemoryPolicy::Normal));
    inspector_.init();
    Luna::instance().start(wake_for(kWakeLuna));
    speech().start();
    find_youtube();

    c.push = [this](std::unique_ptr<Screen> s) { ops_.push_back({Op::Push, std::move(s)}); };
    c.pop = [this] { ops_.push_back({Op::Pop, nullptr}); };
    c.glass = [this](const GlassSpec &g) { top()->shell_glass(g); };
    c.replace = [this](std::unique_ptr<Screen> s) { ops_.push_back({Op::Replace, std::move(s)}); };
    c.toast = [this](const std::string &text) {
      toast_ = text;
      toast_until_ = ctx().now + 3000;  // notification hold [EoA N1]
    };
    c.quit = [this] { running_ = false; };
    c.focus_tabs = [this] { set_tabs_focus(true); };
    c.show_tabs = [this](bool show) { tabs_wanted_ = show; };

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
                            cx.invalidate();
                          });
    watch_network();
    watch_sound_output();
    watch_memory();
    start_service();

    roots_.resize(kTabCount);
    root_ = kHomeTab;
    roots_[kHomeTab] = make_browse(Browse::Home);

    load_script();
    LaunchParams launch = parse_launch(argc, argv);
    if (!launch.uri.empty() && launch.autoplay) {
      PlayRequest req;
      req.item.name = launch.title;
      req.video_id = "launch";
      ops_.push_back({Op::Push, make_player(req, launch.uri, PlaySourceInfo{}, launch.load_type)});
    }
    c.now = now_ms();
    return true;
  }

  // The system's memory pressure (study C8). webOS's memory manager reports a
  // state such as "normal", "low" or "critical"; this was not verified on the
  // TV, so the reply is logged once and anything unexpected is ignored. Low or
  // critical memory moves browsing onto the smaller playback budget.
  void watch_memory() {
    Luna::instance().call("luna://com.webos.memorymanager/getCurrentMemState", "{\"subscribe\":true}",
                          [this](const std::string &reply) {
      Json j = Json::parse(reply);
      static bool logged = false;
      if (!logged) {
        logged = true;
        logf("memory", "system memory state: %s", reply.substr(0, 200).c_str());
      }
      std::string state = j["currentMemState"].as_string(j["memState"].as_string());
      if (state.empty()) return;
      bool low = state == "low" || state == "critical";
      if (low != system_memory_low_) {
        system_memory_low_ = low;
        logf("memory", "system memory %s", state.c_str());
        apply_memory_policy();
      }
    }, true);
  }

  // Online/offline and wired/Wi-Fi, pushed by the TV whenever they change.
  void watch_network() {
    Luna::instance().call("luna://com.webos.service.connectionmanager/getStatus", "{\"subscribe\":true}", [](const std::string &reply) {
      Json j = Json::parse(reply);
      if (!j.has("isInternetConnectionAvailable")) return;
      Ctx &c = ctx();
      bool online = j["isInternetConnectionAvailable"].as_bool(true);
      std::string network = j["wired"]["state"].as_string() == "connected" ? "wired"
                            : j["wifi"]["state"].as_string() == "connected" ? "wifi" : "";
      if (online != c.online || network != c.network)
        logf("net", "%s%s", online ? "online" : "offline", network.empty() ? "" : (" via " + network).c_str());
      c.online = online;
      c.network = network;
      c.invalidate();
    }, true);
  }

  // Where the TV sends its sound: Headphones mode turns itself on for Bluetooth
  // ("bt_soundbar" with the XM5 earbuds, "tv_speaker" without, 25 October 2026).
  void watch_sound_output() {
    Luna::instance().call("luna://com.webos.service.settings/getSystemSettings",
                          "{\"category\":\"sound\",\"keys\":[\"soundOutput\"],\"subscribe\":true}", [](const std::string &reply) {
      Json j = Json::parse(reply);
      std::string out = j["settings"]["soundOutput"].as_string();
      if (out.empty()) return;
      Ctx &c = ctx();
      if (out != c.sound_output) logf("sound", "the TV's sound goes to %s", out.c_str());
      c.sound_output = out;
      c.invalidate();
    }, true);
  }

  void start_service() {
    Ctx &c = ctx();
    service_started_at_ = now_ms();
    std::string uri = "luna://" + c.app_id + ".service/start";
    logf("app", "starting service %s.service", c.app_id.c_str());
    Luna::instance().call(uri, "{}", [this](const std::string &reply) {
      Ctx &cx = ctx();
      cx.invalidate();
      Json j = Json::parse(reply);
      if (j["ready"].as_bool() && j["port"].as_int() > 0) {
        cx.api.set_port(static_cast<int>(j["port"].as_int()));
        logf("app", "service ready on port %d (version %s)", cx.api.port(), j["version"].as_string("?").c_str());
        cx.mark("service");
        cx.api.get("/api/settings", [](const ApiResult &r) {
          if (!r.ok()) return;
          Ctx &x = ctx();
          x.language = r.json["language"].as_string("English");
          x.subtitle_language = r.json["subtitleLanguage"].as_string("Off");
          x.subtitle_size = static_cast<int>(std::max<long long>(0, std::min<long long>(4, r.json["subtitleSize"].as_int(0))));
          x.subtitle_background = r.json["subtitleBackground"].as_bool(false);
          x.picture = r.json["picture"].as_string("best");
          x.sources = r.json["sources"].as_string("all");
          x.preferred_cut = r.json["cut"].as_string("Theatrical");
          x.autoplay_next = r.json["autoplayNext"].as_bool(true);
          x.reduce_motion = r.json["reduceMotion"].as_bool(false);
          x.reduce_transparency = r.json["reduceTransparency"].as_bool(false);
          x.high_contrast = r.json["highContrast"].as_bool(false);
          x.headphones = r.json["headphones"].as_string("auto");
          x.headphone_room = r.json["headphoneRoom"].as_string("lounge");
          x.headphone_boost = static_cast<int>(std::max<long long>(0, std::min<long long>(15, r.json["headphoneBoost"].as_int(9))));
          x.headphone_night = r.json["headphoneNight"].as_bool(false);
          x.headphone_xm5 = r.json["headphoneEq"].as_string("xm5") == "xm5";
          x.headphone_lead_ms = static_cast<int>(std::max<long long>(0, std::min<long long>(500, r.json["headphoneLeadMs"].as_int(0))));
          x.welcome_seen = r.json["welcomeSeen"].as_bool(false);
          x.settings_loaded = true;
          x.refresh_headphone_responses();
          x.recent_searches.clear();
          for (size_t i = 0; i < r.json["recentSearches"].size(); i++) x.recent_searches.push_back(r.json["recentSearches"][i].as_string());
        }, nullptr);
        cx.api.get("/api/account", [](const ApiResult &r) {
          if (!r.ok()) return;
          ctx().paired = r.json["paired"].as_bool();
          ctx().account_known = true;
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
    for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) (*it)->background();
    leaving_.reset();
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
    c.fetch_tasks.stop();
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

  Screen *below_top() {
    if (stack_.size() >= 2) return stack_[stack_.size() - 2].get();
    if (stack_.size() == 1) return roots_[static_cast<size_t>(root_)].get();
    return nullptr;
  }

  void ensure_root(int index) {
    if (roots_[static_cast<size_t>(index)]) return;
    switch (index) {
      case 0: roots_[0] = make_search(); break;
      case 1: roots_[1] = make_browse(Browse::Home); break;
      case 2: roots_[2] = make_browse(Browse::Movies); break;
      case 3: roots_[3] = make_browse(Browse::Series); break;
      case 5: roots_[5] = make_settings(); break;
    }
  }

  void apply_ops() {
    while (!ops_.empty()) {
      Op op = std::move(ops_.front());
      ops_.pop_front();
      Screen *before = top();
      switch (op.kind) {
        case Op::Push: {
          before->hidden();
          bool animate = !before->over_video() && !op.screen->over_video();
          stack_.push_back(std::move(op.screen));
          logf("nav", "push %s", stack_.back()->name());
          finish_leaving();
          transition_id_++;
          entering_.snap(animate && !ctx().reduce_motion ? 0.0f : 1.0f);
          entering_.to(1);
          break;
        }
        case Op::Pop: {
          if (stack_.empty()) break;
          logf("nav", "pop %s", stack_.back()->name());
          std::unique_ptr<Screen> gone = std::move(stack_.back());
          stack_.pop_back();
          finish_leaving();
          entering_.snap(1);
          if (!gone->over_video() && !top()->over_video() && !ctx().reduce_motion) {
            leaving_ = std::move(gone);  // fades out over the screen it returns to
            transition_id_++;
            leaving_fade_.snap(1);
            leaving_fade_.to(0);
          }
          top()->shown();
          break;
        }
        case Op::Replace:
          finish_leaving();
          if (!stack_.empty()) {
            logf("nav", "replace %s with %s", stack_.back()->name(), op.screen->name());
            stack_.back() = std::move(op.screen);
          } else {
            stack_.push_back(std::move(op.screen));
          }
          entering_.snap(1);
          break;
      }
      dirty_ = true;
    }
  }

  void finish_leaving() { leaving_.reset(); }
  PerfWindow perf_;

  void select_root(int index) {
    if (index == root_) return;
    roots_[static_cast<size_t>(root_)]->hidden();
    ensure_root(index);
    root_ = index;
    tabs_wanted_ = true;
    roots_[static_cast<size_t>(root_)]->set_tabs_focus(tabs_focus_);
    roots_[static_cast<size_t>(root_)]->shown();
    page_fade_.snap(ctx().reduce_motion ? 1.0f : 0.0f);
    page_fade_.to(1);
  }

  void set_tabs_focus(bool focus) {
    tabs_focus_ = focus;
    roots_[static_cast<size_t>(root_)]->set_tabs_focus(focus);
    if (focus) tabs_wanted_ = true;
  }

  // ---------------------------------------------------------------- input

  void on_key(const SDL_KeyboardEvent &event) {
    Ctx &c = ctx();
    Key key = key_from_event(event);
    char buf[160];
    std::snprintf(buf, sizeof buf, "%s  \xC2\xB7  scancode %d  \xC2\xB7  keycode %d", key_name(key),
                  static_cast<int>(event.keysym.scancode), static_cast<int>(event.keysym.sym));
    c.last_key = buf;
    int sc = static_cast<int>(event.keysym.scancode);
    if (sc == kScancodeWebosCursorShow || sc == kScancodeWebosCursorHide) {
      logf("input", "pointer %s", sc == kScancodeWebosCursorShow ? "shown" : "hidden");
      return;
    }
    if (key == Key::None) {
      logf("input", "unmapped key: scancode=%d sym=%d", event.keysym.scancode, event.keysym.sym);
      return;
    }
    dirty_ = true;
    input_ = true;
    // Scripted presses carry no timestamp; real ones are stamped when queued.
    perf_.key(event.timestamp ? static_cast<long long>(event.timestamp) + sdl_offset_ : now_ms());
    if (key == Key::Exit) {
      running_ = false;
      return;
    }
    bool repeat = event.repeat != 0;
    // A key during a transition lands on the destination at once.
    if (leaving_) finish_leaving();
    entering_.snap(1);
    if (!stack_.empty()) {
      stack_.back()->key(key, repeat);
      return;
    }
    Screen *root = roots_[static_cast<size_t>(root_)].get();
    if (tabs_focus_) {
      switch (key) {
        case Key::Left:
        case Key::Right: {
          int t = focused_tab() + (key == Key::Left ? -1 : 1);
          if (t == kYouTubeTab && !youtube_) t += key == Key::Left ? -1 : 1;
          if (t < 0 || t >= kTabCount) break;
          youtube_focus_ = t == kYouTubeTab;
          if (!youtube_focus_) select_root(t);
          break;
        }
        case Key::Ok:
          if (youtube_focus_) {
            launch_youtube();
            break;
          }
          set_tabs_focus(false);
          break;
        case Key::Down:
          youtube_focus_ = false;
          set_tabs_focus(false);
          break;
        case Key::Back:
          if (youtube_focus_) {
            youtube_focus_ = false;
          } else if (root_ != kHomeTab) {
            select_root(kHomeTab);
          } else if (launcher_) {
            // Bloob is the TV's home screen: there is nothing behind it to go back to.
          } else if (exit_armed_at_ && c.now - exit_armed_at_ < kExitConfirmMs) {
            running_ = false;
          } else {
            exit_armed_at_ = c.now;
            c.toast("Press Back again to close Bloob");
          }
          break;
        default:
          break;
      }
      return;
    }
    if (!root->key(key, repeat) && (key == Key::Up || key == Key::Back)) set_tabs_focus(true);
  }

  // The Magic Remote's pointer (TV, 2.2.0: with the cursor on screen the app did
  // nothing, because a click arrives as a mouse button, not as OK). Until Bloob
  // has pointer hit-testing, the pointer drives focus like the arrows: a click
  // is OK on whatever is focused, the wheel moves up and down, and the right
  // button is Back. Each kind is logged once so a TV run shows what arrives.
  void on_pointer(const SDL_Event &e) {
    static bool seen[3] = {false, false, false};
    int kind = e.type == SDL_MOUSEBUTTONDOWN ? 0 : e.type == SDL_MOUSEWHEEL ? 1 : 2;
    if (!seen[kind]) {
      seen[kind] = true;
      logf("input", "pointer %s", kind == 0 ? "button" : kind == 1 ? "wheel" : "motion");
    }
    if (kind == 2) return;  // moving the cursor is not a command
    SDL_Scancode code = SDL_SCANCODE_UNKNOWN;
    if (kind == 0) code = e.button.button == SDL_BUTTON_RIGHT ? static_cast<SDL_Scancode>(kScancodeWebosBack)
                          : e.button.button == SDL_BUTTON_LEFT ? SDL_SCANCODE_RETURN : SDL_SCANCODE_UNKNOWN;
    else if (e.wheel.y != 0) code = e.wheel.y > 0 ? SDL_SCANCODE_UP : SDL_SCANCODE_DOWN;
    if (code == SDL_SCANCODE_UNKNOWN) return;
    SDL_KeyboardEvent key{};
    key.type = SDL_KEYDOWN;
    key.timestamp = kind == 0 ? e.button.timestamp : e.wheel.timestamp;
    key.keysym.scancode = code;
    on_key(key);
  }

  // What Audio Guidance should say for the current focus (G7).
  std::string spoken_focus() {
    if (stack_.empty() && tabs_focus_) return std::string(kTabs[focused_tab()].label) + ", tab, " + position(focused_tab(), kTabCount);
    return top()->spoken();
  }

  void on_event(const SDL_Event &e) {
    if (e.type == SDL_QUIT) {
      running_ = false;
    } else if (e.type == SDL_KEYDOWN) {
      on_key(e.key);
    } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEWHEEL || e.type == SDL_MOUSEMOTION) {
      on_pointer(e);
    } else if (e.type == SDL_WINDOWEVENT) {
      // Focus loss alone is a system overlay (quick settings, volume): the app
      // is still on screen and playback must carry on untouched.
      switch (e.window.event) {
        case SDL_WINDOWEVENT_FOCUS_LOST: logf("app", "focus lost (overlay or background)"); break;
        case SDL_WINDOWEVENT_FOCUS_GAINED: logf("app", "focus gained"); foreground(); break;
        case SDL_WINDOWEVENT_HIDDEN:
        case SDL_WINDOWEVENT_MINIMIZED: logf("app", "window hidden"); background(); break;
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_RESTORED: logf("app", "window shown"); foreground(); break;
        default: break;
      }
      dirty_ = true;
    } else if (e.type == SDL_APP_WILLENTERBACKGROUND || e.type == SDL_APP_DIDENTERBACKGROUND) {
      logf("app", "entering background");
      background();
    } else if (e.type == SDL_APP_DIDENTERFOREGROUND) {
      logf("app", "back in the foreground");
      foreground();
    }
  }

  void background() {
    if (in_background_) return;
    in_background_ = true;
    top()->background();
    apply_memory_policy();
  }

  void foreground() {
    bool was = in_background_;
    in_background_ = false;
    if (!youtube_) find_youtube();  // installed while Bloob was away
    top()->foreground();
    if (was) apply_memory_policy();
  }

  // ---------------------------------------------------------------- drawing

  static constexpr int kTabGap = 8;

  void draw_tabs() {
    float a = tabs_alpha_.value;
    if (a <= 0.01f) return;
    int dy = 0;  // glass fades in place: a moving pane would need a new frost each frame
    // Glass: a floating frosted bar on the tvOS safe line; the current tab is
    // white when the bar has focus, a soft candidate otherwise.
    // Tabs are pills on the glass bar [EoA P1-P6], 16 apart [EoA F3].
    auto pill = [&](int i) {
      GlassButton t;
      t.pill = true;
      t.label = kTabs[i].icon >= 0 ? "" : kTabs[i].label;
      t.icon = kTabs[i].icon;
      t.candidate = i == root_;
      t.focused = tabs_focus_ && i == focused_tab();
      t.alpha = a;
      return t;
    };
    int total = 0;
    for (int i = 0; i < kTabCount; i++)
      if (i != kYouTubeTab || youtube_) total += glass_button_width(pill(i)) + (i ? kTabGap : 0);
    int pad = 9, h = theme::pill_h;  // segment platter: padding 9, buttons 8 apart [EoA N2]
    int bw = total + pad * 2, bh = h + pad * 2, bx = (W - bw) / 2, by = theme::safe_y + dy;
    GlassSpec bar;
    bar.x = bx;
    bar.y = by;
    bar.w = bw;
    bar.h = bh;
    bar.radius = bh / 2;
    bar.alpha = a;
    top()->shell_glass(bar);
    int x = bx + pad;
    for (int i = 0; i < kTabCount; i++)
      if (i != kYouTubeTab || youtube_) x += draw_glass_button(x, by + pad, pill(i)) + kTabGap;
  }

  void draw_screen(Screen *s, bool layer, float alpha, int dy) {
    Ctx &c = ctx();
    if (layer && c.gfx.begin_layer()) {
      s->draw();
      c.gfx.end_layer(alpha, dy);
    } else {
      s->draw();
    }
  }

  void render() {
    Ctx &c = ctx();
    long long render_started = now_ms();
    long long allocs_before = alloc_stats().count.load(std::memory_order_relaxed);
    PerfWindow::Stages stages;
    stages.update = last_update_ms_;
    c.images.begin_frame(c.now);
    // Uploads first, paced and admitted against the GPU budget, so new art
    // appears in this frame (study G12, M4).
    c.images.upload();
    long long uploaded = now_ms();
    stages.upload = uploaded - render_started;
    c.gfx.shadows = !c.lite();
    c.gfx.begin_frame();
    Screen *screen = top();
    bool entering = entering_.value < 0.999f && below_top();
    if (entering) {
      // The new screen rises and fades in over the one it came from. The screen
      // underneath does not change during the transition: paint it once into a
      // retained layer and composite it (R6r, study G4), instead of every frame.
      Screen *below = below_top();
      std::string key = "below#" + std::to_string(transition_id_);
      if (c.gfx.cached_valid("below", key)) {
        c.gfx.composite_cached("below", 1.0f, 0);
        layer_reuses_++;
      } else if (c.gfx.begin_cached("below")) {
        below->draw();
        c.gfx.end_cached("below", key);
        c.gfx.composite_cached("below", 1.0f, 0);
        layer_paints_++;
      } else {
        below->draw();
      }
      draw_screen(screen, true, entering_.value, 0);  // a plain crossfade [EoA M10]
    } else {
      if (c.gfx.cached_count()) c.gfx.drop_cached("below");
      if (stack_.empty() && page_fade_.value < 0.999f) {
        c.gfx.clear(theme::black);
        draw_screen(screen, true, page_fade_.value, 0);
      } else {
        screen->draw();
      }
      if (stack_.empty()) draw_tabs();
      if (leaving_) {
        // The screen going away is frozen: paint it once, then fade the layer.
        std::string key = "leaving#" + std::to_string(transition_id_);
        if (c.gfx.cached_valid("leaving", key)) {
          c.gfx.composite_cached("leaving", leaving_fade_.value, 0);
          layer_reuses_++;
        } else if (c.gfx.begin_cached("leaving")) {
          leaving_->draw();
          c.gfx.end_cached("leaving", key);
          c.gfx.composite_cached("leaving", leaving_fade_.value, 0);
          layer_paints_++;
        } else {
          draw_screen(leaving_.get(), true, leaving_fade_.value, 0);
        }
      }
    }
    if (!toast_.empty() && c.now < toast_until_) {
      // Notification [EoA N1]: 200–600 wide, at least 112 high, radius 30,
      // SystemGray5 with a hairline and a soft shadow, text padding 30 / 40,
      // Standard.Caption1; top right inside the safe area, 57 from the top.
      auto lines = wrap(font_for(Text::Caption1), toast_, size_for(Text::Caption1), 600 - 80, 3);
      int tw = 0;
      for (auto &l : lines) tw = std::max(tw, measure(Text::Caption1, l));
      int w = std::max(200, std::min(600, tw + 80));
      int th = std::max(112, static_cast<int>(lines.size()) * line_for(Text::Caption1) + 60);
      int tx = W - theme::safe_x - w, ty = 57;
      if (!c.lite()) c.gfx.shadow(tx, ty, w, th, 30, 36, 15, 38);
      c.gfx.fill_round(tx, ty, w, th, 30, rgba(0x2c2c2e));
      c.gfx.stroke_round(tx, ty, w, th, 30, 1, c.high_contrast ? theme::hairline_high : theme::hairline);
      int ly = ty + (th - static_cast<int>(lines.size()) * line_for(Text::Caption1)) / 2;
      for (auto &l : lines) {
        text(Text::Caption1, l, tx + 40, ly, label(Label::Primary));
        ly += line_for(Text::Caption1);
      }
      c.redraw_at(toast_until_);
    }
    long long painted = now_ms();
    stages.paint = painted - uploaded;
    c.gfx.end_frame();
    SDL_RenderPresent(c.renderer);
    long long presented = now_ms();
    stages.present = presented - painted;
    if (c.images.end_frame()) c.animating = true;
    if (c.images.pending_uploads()) c.animating = true;  // paced uploads continue next frame
    advance_frame();
    c.gfx.housekeeping();
    c.icons.housekeeping();
    pacer_.presented(presented);
    dirty_ = false;
    input_ = false;
    c.dirty = false;
    frames_++;
    perf_.frame(presented, presented - render_started, stages);
    frame_allocs_.push_back(alloc_stats().count.load(std::memory_order_relaxed) - allocs_before);
    if (c.marks.count("home-data") && !c.marks.count("home-frame")) c.mark("home-frame");
    if (perf_.due(presented)) {
      std::string line = perf_.flush(presented);
      if (!frame_allocs_.empty()) {
        std::sort(frame_allocs_.begin(), frame_allocs_.end());
        alloc_p95_ = frame_allocs_[std::min(frame_allocs_.size() - 1, frame_allocs_.size() * 95 / 100)];
        line += "; allocations/frame p95 " + std::to_string(alloc_p95_);
        frame_allocs_.clear();
      }
      c.perf_line = line;
      if (perf_.last_frames() > 0) logf("perf", "%s", line.c_str());
      // Two slow windows in a row: lighter effects for the rest of the session.
      // BLOOB_EFFECTS=full pins the tier (desktop renders are software and always slow).
      static const bool pinned = std::getenv("BLOOB_EFFECTS") && std::string(std::getenv("BLOOB_EFFECTS")) == "full";
      if (!pinned && c.effects == Ctx::Effects::Full && perf_.last_frames() >= 20) {
        c.slow_windows = perf_.last_render_p95() > 25 ? c.slow_windows + 1 : 0;
        if (c.slow_windows >= 2) {
          c.effects = Ctx::Effects::Lite;
          logf("perf", "effects -> lite (render p95 %lld ms)", perf_.last_render_p95());
        }
      }
    }
  }

  // ---------------------------------------------------------------- memory policy

  // Study C8, M13: Normal while browsing, Playback while the player is on top
  // (video decoding needs the memory), Background while another app is in front
  // (every texture and decoded source released, no new fetches).
  void apply_memory_policy() {
    Ctx &c = ctx();
    Screen *t = top();
    MemoryPolicy p = in_background_ ? MemoryPolicy::Background
                     : (t && t->over_video()) || system_memory_low_ ? MemoryPolicy::Playback
                                                                    : MemoryPolicy::Normal;
    if (p == c.memory_policy) return;
    MemoryPolicy was = c.memory_policy;
    c.memory_policy = p;
    gpu_budget().set_limit(budget_for(p));
    if (p == MemoryPolicy::Background) {
      c.images.set_suspended(true);
      c.images.release_handles();
      c.gfx.release_caches();
      c.icons.release();
    } else if (was == MemoryPolicy::Background) {
      c.images.set_suspended(false);
    }
    logf("memory", "policy %s, GPU budget %zu MB, in use %zu MB", memory_policy_name(p), gpu_budget().limit() >> 20,
         gpu_budget().used() >> 20);
    // The service trims its own image byte cache to match.
    if (c.api.ready()) c.api.post("/api/memory", std::string("{\"policy\":\"") + memory_policy_name(p) + "\"}");
    c.invalidate();
  }

  // ---------------------------------------------------------------- inspector

  void inspect(long long now) {
    if (!inspector_.due(now)) return;
    Ctx &c = ctx();
    JsonOut j;
    j.begin();
    j.str("version", BLOOB_VERSION).num("uptime_ms", now).str("sdl", sdl_version_).str("renderer", renderer_name_);
    j.num("refresh_hz", refresh_hz_).str("memory_policy", memory_policy_name(c.memory_policy));
    j.str("effects", c.lite() ? "lite" : "full").boolean("background", in_background_);
    j.boolean("system_memory_low", system_memory_low_);
    j.obj("allocations").num("count", alloc_stats().count.load()).num("bytes", alloc_stats().bytes.load())
        .num("per_frame_p95", alloc_p95_).end();
    j.arr("screens");
    for (auto &r : stack_) j.item_str(r->name());
    j.end_arr();
    j.str("top", top() ? top()->name() : "");
    const PerfWindow::Summary &ps = perf_.last();
    j.obj("frames")
        .num("total", frames_)
        .num("window_frames", static_cast<long long>(ps.frames))
        .num("window_ms", ps.window_ms)
        .num("render_p50", ps.render_p50)
        .num("render_p95", ps.render_p95)
        .num("key_to_submit_p50", ps.key_p50)
        .num("key_to_submit_p95", ps.key_p95)
        .num("update_p95", ps.update_p95)
        .num("paint_p95", ps.paint_p95)
        .num("upload_p95", ps.upload_p95)
        .num("present_p95", ps.present_p95)
        .num("cause_input", cause_[0])
        .num("cause_invalidate", cause_[1])
        .num("cause_animation", cause_[2])
        .num("cause_deadline", cause_[3])
        .num("deferred_to_refresh", deferred_frames_)
        .boolean("vsync_paced", !gate_frames_)
        .num("layer_paints", layer_paints_)
        .num("layer_reuses", layer_reuses_)
        .end();
    j.obj("wakes")
        .num("api", wakes_[kWakeApi].load())
        .num("fetch", wakes_[kWakeFetch].load())
        .num("image", wakes_[kWakeImage].load())
        .num("luna", wakes_[kWakeLuna].load())
        .num("events_posted", wake_events_.load())
        .num("loop_wakes", loop_wakes_)
        .end();
    auto task_stats = [&j](const char *name, const Tasks &t) {
      Tasks::Stats s = t.stats();
      j.obj(name)
          .num("visible", static_cast<long long>(s.queued[0]))
          .num("near", static_cast<long long>(s.queued[1]))
          .num("prefetch", static_cast<long long>(s.queued[2]))
          .num("busy", s.busy)
          .num("completions_waiting", static_cast<long long>(s.completions_waiting))
          .num("ran", s.ran)
          .num("dropped_owner", s.dropped_owner)
          .end();
    };
    j.obj("tasks");
    task_stats("api", c.api_tasks);
    task_stats("fetch", c.fetch_tasks);
    task_stats("image", c.image_tasks);
    j.end();
    Luna::Stats ls = Luna::instance().stats();
    j.obj("luna")
        .num("live", static_cast<long long>(ls.live))
        .num("retired", static_cast<long long>(ls.retired))
        .num("queued", static_cast<long long>(ls.queued))
        .num("delivered", ls.delivered)
        .end();
    GpuBudget &gb = gpu_budget();
    j.obj("gpu").num("limit", static_cast<long long>(gb.limit())).num("used", static_cast<long long>(gb.used()));
    for (int i = 0; i < static_cast<int>(GpuClass::Count); i++)
      j.num(gpu_class_name(static_cast<GpuClass>(i)), static_cast<long long>(gb.used(static_cast<GpuClass>(i))));
    j.num("overruns", static_cast<long long>(gb.overruns)).end();
    Images::Stats is = c.images.stats();
    j.obj("images")
        .num("entries", static_cast<long long>(is.entries))
        .num("resident", static_cast<long long>(is.resident))
        .num("resident_visible_bytes", static_cast<long long>(is.resident_bytes[0]))
        .num("resident_near_bytes", static_cast<long long>(is.resident_bytes[1]))
        .num("resident_prefetch_bytes", static_cast<long long>(is.resident_bytes[2]))
        .num("waiting", static_cast<long long>(is.waiting))
        .num("making", static_cast<long long>(is.making))
        .num("uploading", static_cast<long long>(is.uploading))
        .num("failed", static_cast<long long>(is.failed))
        .num("sources", static_cast<long long>(is.sources))
        .num("sources_ready", static_cast<long long>(is.sources_ready))
        .num("sources_fetching", static_cast<long long>(is.sources_fetching))
        .num("sources_decoding", static_cast<long long>(is.sources_decoding))
        .num("fetches", is.fetches)
        .num("decodes", is.decodes)
        .num("variants", is.variants)
        .num("uploads", is.uploads)
        .num("upload_bytes", is.upload_bytes)
        .num("deferred", is.deferred)
        .num("evictions", is.evictions)
        .num("cancelled", is.cancelled)
        .end();
    Gfx::Stats gs = c.gfx.stats();
    j.obj("shapes")
        .num("discs", static_cast<long long>(gs.discs))
        .num("rings", static_cast<long long>(gs.rings))
        .num("shadows", static_cast<long long>(gs.shadows))
        .num("sheens", static_cast<long long>(gs.sheens))
        .num("triangles", static_cast<long long>(gs.triangles))
        .num("icons", static_cast<long long>(c.icons.count()))
        .num("layers", static_cast<long long>(gs.layers))
        .num("target_pool", static_cast<long long>(gs.pool))
        .num("state_calls_skipped", gs.state_calls_skipped)
        .end();
    ProcMemory pm = read_proc_memory();
    j.obj("memory")
        .num("rss_kb", pm.rss_kb)
        .num("peak_rss_kb", pm.hwm_kb)
        .num("swap_kb", pm.swap_kb)
        .num("threads", pm.threads)
        .num("decoded_sources_bytes", cpu_domains().decoded_sources.load())
        .num("pixels_in_flight_bytes", cpu_domains().pixels_in_flight.load())
        .end();
    j.obj("service").boolean("ready", c.api.ready()).str("memory", service_memory_).end();
    j.end();
    inspector_.write(j.text(), now);
    // The service's own memory, every 10 s, for the next snapshot.
    if (c.api.ready() && now - service_polled_at_ >= 10000) {
      service_polled_at_ = now;
      c.api.get("/api/health", [this](const ApiResult &r) {
        if (!r.ok()) return;
        const Json &m = r.json["memory"];
        if (!m.is_object()) return;
        service_memory_ = "rss " + std::to_string(m["rss"].as_int() >> 20) + " MB, heap " +
                          std::to_string(m["heapUsed"].as_int() >> 20) + " MB, image cache " +
                          std::to_string(r.json["imageCacheBytes"].as_int() >> 20) + " MB";
      }, nullptr);
    }
  }

  // ---------------------------------------------------------------- scripted runs

  // BLOOB_SCRIPT: steps separated by '|': key:<name>, wait:<ms>, media:<event json>, window:<event>,
  // background, foreground, pump, snap:<name>, open:calibrate (straight to a screen, for its tests), quit.
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
        {"pause", SDL_SCANCODE_PAUSE}, {"red", kScancodeWebosRed}, {"blue", kScancodeWebosBlue}, {"info", SDL_SCANCODE_I}};
    SDL_KeyboardEvent key{};
    key.type = SDL_KEYDOWN;
    for (auto &c : codes) if (name == c.first) key.keysym.scancode = static_cast<SDL_Scancode>(c.second);
    on_key(key);
  }

  // A system window event, as the TV would send it: focus-lost (quick menu),
  // focus-gained, hidden (Home or another app), shown.
  void window_event(const std::string &name) {
    static const std::pair<const char *, Uint8> events[] = {
        {"focus-lost", SDL_WINDOWEVENT_FOCUS_LOST}, {"focus-gained", SDL_WINDOWEVENT_FOCUS_GAINED},
        {"hidden", SDL_WINDOWEVENT_HIDDEN}, {"shown", SDL_WINDOWEVENT_SHOWN}};
    SDL_Event e{};
    e.type = SDL_WINDOWEVENT;
    for (auto &ev : events)
      if (name == ev.first) e.window.event = ev.second;
    if (e.window.event) on_event(e);
  }

  void snapshot(const std::string &name) {
    Ctx &c = ctx();
    render();
    if (!c.gfx.canvas()) return;
    SDL_SetRenderTarget(c.renderer, c.gfx.canvas());
    SDL_Surface *surface = SDL_CreateRGBSurface(0, W, H, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (surface) {
      SDL_RenderReadPixels(c.renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
      // A frame with nothing over the video is fully transparent, and BMP readers
      // take an all-zero alpha channel to mean "no alpha" (opaque black). One
      // barely-visible pixel keeps the channel meaningful.
      Uint32 *px = static_cast<Uint32 *>(surface->pixels);
      bool any_alpha = false;
      for (int i = 0; i < W * H && !any_alpha; i++) any_alpha = (px[i] >> 24) != 0;
      if (!any_alpha) px[0] |= 0x01000000u;
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
      else if (step.kind == "background") background();
      else if (step.kind == "foreground") foreground();
      else if (step.kind == "window") window_event(step.arg);
      else if (step.kind == "open" && step.arg == "calibrate") ctx().push(make_calibrate());
      else if (step.kind == "quit") running_ = false;
      apply_ops();
      dirty_ = true;
    }
  }

  // ---------------------------------------------------------------- loop

  void loop() {
    Ctx &c = ctx();
    bool was_animating = false;
    while (running_) {
      long long now = now_ms();
      int timeout;
      if (!ops_.empty() || pump_more_) timeout = 0;
      else if (c.animating || dirty_ || c.dirty || deferred_render_) timeout = gate_frames_ ? pacer_.wait_ms(now) : 0;
      else if (script_pos_ < script_.size()) timeout = 10;
      else if (c.next_redraw) timeout = static_cast<int>(std::max(0LL, std::min(1000LL, c.next_redraw - now)));
      else timeout = 1000;
      SDL_Event e;
      if (SDL_WaitEventTimeout(&e, timeout)) {
        // A wake only says "work is queued": clear the flag before draining so
        // a producer that finishes during the drain posts a fresh event.
        if (e.type == wake_event_) {
          wake_pending_.store(false);
          loop_wakes_++;
        } else {
          on_event(e);
        }
        while (SDL_PollEvent(&e)) {
          if (e.type == wake_event_) {
            wake_pending_.store(false);
            loop_wakes_++;
          } else {
            on_event(e);
          }
        }
      }
      // Bounded work per iteration (study C5): what remains is handled next turn.
      bool more_luna = Luna::instance().pump(kPumpBudgetMs);
      bool more_api = false, more_fetch = false, more_image = false;
      if (c.api_tasks.pump(kPumpBudgetMs, &more_api)) c.invalidate();  // API replies change what screens show
      c.fetch_tasks.pump(kPumpBudgetMs, &more_fetch);
      c.image_tasks.pump(kPumpBudgetMs, &more_image);
      pump_more_ = more_luna || more_api || more_fetch || more_image;
      now = now_ms();
      c.dt = pacer_.step_seconds(now, was_animating);
      c.now = now;
      if (!script_.empty()) run_script(now);
      apply_ops();
      apply_memory_policy();
      if (!c.api.ready() && !fallback_tried_ && now - service_started_at_ > kServiceBusWaitMs) {
        logf("app", "no reply from the service over the bus");
        start_service_fallback();
      }
      if (!c.api.ready() && c.service_error.empty() && now - service_started_at_ > kServiceTimeoutMs) {
        c.service_error = "Bloob's background service did not start. Close Bloob and open it again.";
        logf("app", "service start timed out");
        c.invalidate();
      }
      bool redraw_due = c.next_redraw && now >= c.next_redraw;
      bool prev_animating = was_animating;
      c.animating = false;
      c.next_redraw = 0;
      long long update_started = now_ms();
      top()->update(c.dt);
      if (leaving_) {
        leaving_->update(c.dt);
        if (!leaving_fade_.step(c.dt)) {
          finish_leaving();
          c.gfx.drop_cached("leaving");
        }
        c.animating = true;
      }
      if (entering_.step(c.dt)) c.animating = true;
      if (page_fade_.step(c.dt)) c.animating = true;
      tabs_alpha_.to(stack_.empty() && (tabs_wanted_ || tabs_focus_) ? 1.0f : 0.0f);
      if (tabs_alpha_.step(c.dt)) c.animating = true;
      apply_ops();
      if (speech().on()) {
        speech().focus(spoken_focus(), now);
        if (speech().due()) c.redraw_at(speech().due());
      }
      last_update_ms_ = now_ms() - update_started;
      was_animating = c.animating;
      bool want = dirty_ || c.dirty || prev_animating || c.animating || redraw_due || deferred_render_ ||
                  c.images.pending_uploads();
      if (want) {
        // At most one frame per refresh period unless input is waiting (study C1).
        // With vsync, SDL_RenderPresent paces frames; gating as well would start
        // each frame late in the refresh and miss the next one.
        if (!gate_frames_ || pacer_.due(now_ms(), input_)) {
          if (input_ || dirty_) cause_[0]++;
          else if (c.dirty) cause_[1]++;
          else if (prev_animating || c.animating || c.images.pending_uploads()) cause_[2]++;
          else cause_[3]++;
          deferred_render_ = false;
          render();
        } else {
          deferred_render_ = true;
          deferred_frames_++;
        }
      }
      inspect(now_ms());
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

  // Wake sources, for the coalesced wake and the inspector's counts.
  static constexpr int kWakeApi = 0, kWakeFetch = 1, kWakeImage = 2, kWakeLuna = 3;
  static constexpr int kPumpBudgetMs = 6;
  std::function<void()> wake_for(int source) {
    return [this, source] {
      wakes_[source]++;
      if (!wake_pending_.exchange(true)) {
        wake_events_++;
        SDL_Event e{};
        e.type = wake_event_;
        SDL_PushEvent(&e);
      }
    };
  }

  Uint32 wake_event_ = 0;
  std::atomic<bool> wake_pending_{false};
  std::atomic<long long> wakes_[4] = {{0}, {0}, {0}, {0}};
  std::atomic<long long> wake_events_{0};
  long long loop_wakes_ = 0;
  bool running_ = true;
  bool dirty_ = true;      // input or navigation since the last frame
  bool input_ = false;     // input waiting: a frame may start before the refresh is due
  bool pump_more_ = false;
  bool deferred_render_ = false;
  bool gate_frames_ = false;  // no vsync: at most one frame per refresh period
  bool system_memory_low_ = false;
  FramePacer pacer_;
  Inspector inspector_;
  long long sdl_offset_ = 0;
  std::string sdl_version_ = "?", renderer_name_ = "?", service_memory_;
  int refresh_hz_ = 0;
  long long service_polled_at_ = 0;
  long long frames_ = 0, deferred_frames_ = 0, layer_paints_ = 0, layer_reuses_ = 0;
  long long cause_[4] = {0, 0, 0, 0};  // input, invalidate, animation, deadline
  long long last_update_ms_ = 0;
  std::vector<long long> frame_allocs_;
  long long alloc_p95_ = 0;
  long long transition_id_ = 0;
  std::vector<std::unique_ptr<Screen>> roots_;
  std::vector<std::unique_ptr<Screen>> stack_;
  std::unique_ptr<Screen> leaving_;
  std::deque<Op> ops_;
  int root_ = kHomeTab;
  bool tabs_focus_ = false;
  bool tabs_wanted_ = true;
  bool in_background_ = false;
  long long exit_armed_at_ = 0;
  bool youtube_ = false, youtube_focus_ = false;
  int focused_tab() const { return youtube_focus_ ? kYouTubeTab : root_; }

  // The YouTube tab appears once the TV lists the app. A native app may be refused
  // this call; the Homebrew Channel's root service then asks for it.
  void find_youtube() {
    auto take = [this](const std::string &reply) {
      for (auto &a : parse_launch_points(reply, ctx().app_id))
        if (a.id == kYouTubeId) youtube_ = true;
      if (youtube_) ctx().redraw_at(ctx().now);
      return youtube_;
    };
    Luna::instance().call("luna://com.webos.applicationManager/listLaunchPoints", "{}", [take](const std::string &reply) {
      if (take(reply)) return;
      Luna::instance().call(kHbExec, "{\"command\":\"luna-send -n 1 luna://com.webos.applicationManager/listLaunchPoints '{}'\"}",
                            [take](const std::string &r) { take(Json::parse(r)["stdoutString"].as_string()); });
    });
  }

  void launch_youtube() {
    std::string payload = launch_payload(kYouTubeId);
    logf("apps", "launch %s", kYouTubeId);
    Luna::instance().call("luna://com.webos.applicationManager/launch", payload, [payload](const std::string &reply) {
      if (Json::parse(reply)["returnValue"].as_bool()) return;
      Luna::instance().call(kHbExec, "{\"command\":" + Json::quote("luna-send -n 1 luna://com.webos.applicationManager/launch '" + payload + "'") + "}", nullptr);
    });
  }
  // The launcher hook is installed (native/launcher): Bloob is the home screen.
  bool launcher_ = access("/var/lib/webosbrew/init.d/60-bloob-home", F_OK) == 0;
  Anim entering_{theme::fade_rate, 1};
  Anim leaving_fade_{theme::fade_rate, 0};
  Anim page_fade_{theme::fade_rate, 1};
  Anim tabs_alpha_{theme::fade_rate, 1};
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
