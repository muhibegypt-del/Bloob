// Bloob native host (Buna N1): SDL window and remote input, original UI drawn
// by our renderer, video through the TV's media service in a hardware plane
// beneath the UI, and a native Buna session owning playback state.
#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "gfx.h"
#include "json.h"
#include "keys.h"
#include "log.h"
#include "luna.h"
#include "media.h"
#include "platform.h"
#include "session.h"

namespace {

constexpr int W = 1920;
constexpr int H = 1080;
constexpr long long kHudTimeoutMs = 6000;

const Color kBg = rgba(0x07080a);
const Color kText = rgba(0xf2f1ee);
const Color kText2 = rgba(0xb4b8bf);
const Color kText3 = rgba(0x7d828b);
const Color kPanel = rgba(0x15171b);
const Color kAccent = rgba(0xf2b84b);
const Color kWhite = rgba(0xffffff);
const Color kInk = rgba(0x0b0d12);

std::string clock_text(long long ms) {
  if (ms < 0) return "--:--";
  long long s = ms / 1000;
  char buf[32];
  if (s >= 3600) std::snprintf(buf, sizeof buf, "%lld:%02lld:%02lld", s / 3600, (s / 60) % 60, s % 60);
  else std::snprintf(buf, sizeof buf, "%lld:%02lld", s / 60, s % 60);
  return buf;
}

std::string delta_text(long long ms) {
  long long s = (ms < 0 ? -ms : ms) / 1000;
  return std::string(ms < 0 ? "\xE2\x88\x92" : "+") + (s < 60 ? std::to_string(s) + "s" : clock_text(s * 1000));
}

std::string language_name(const std::string &code) {
  static const std::pair<const char *, const char *> names[] = {
      {"en", "English"}, {"eng", "English"}, {"fr", "French"}, {"fre", "French"}, {"fra", "French"},
      {"de", "German"}, {"ger", "German"}, {"deu", "German"}, {"es", "Spanish"}, {"spa", "Spanish"},
      {"it", "Italian"}, {"ita", "Italian"}, {"pt", "Portuguese"}, {"por", "Portuguese"}, {"ru", "Russian"},
      {"rus", "Russian"}, {"ja", "Japanese"}, {"jpn", "Japanese"}, {"ko", "Korean"}, {"kor", "Korean"},
      {"zh", "Chinese"}, {"chi", "Chinese"}, {"zho", "Chinese"}, {"ar", "Arabic"}, {"ara", "Arabic"},
      {"hi", "Hindi"}, {"hin", "Hindi"}, {"nl", "Dutch"}, {"dut", "Dutch"}, {"pl", "Polish"}, {"pol", "Polish"},
      {"tr", "Turkish"}, {"tur", "Turkish"}, {"sv", "Swedish"}, {"swe", "Swedish"}};
  for (auto &n : names) if (code == n.first) return n.second;
  return code.empty() ? "Unknown language" : code;
}

std::string audio_label(const MediaTrack &t) {
  std::string codec = t.codec;
  for (auto &c : codec) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
  std::string ch = t.channels >= 8 ? "7.1" : t.channels == 6 ? "5.1" : t.channels == 2 ? "Stereo" : t.channels ? std::to_string(t.channels) + " ch" : "";
  std::string out = language_name(t.language);
  if (!codec.empty()) out += " \xC2\xB7 " + codec;
  if (!ch.empty()) out += " " + ch;
  return out;
}

enum class Screen { Home, Diagnostics, Player };
enum class Zone { Timeline, Controls };
enum class Menu { None, Audio, Subtitles };

struct LaunchParams {
  std::string uri;
  std::string title = "Launch link";
  std::string load_type = "media";
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

class UmsPort : public PlayerPort {
 public:
  explicit UmsPort(UmsPlayer &player) : player_(player) {}
  void play() override { player_.play(); }
  void pause() override { player_.pause(); }
  void seek(long long ms) override { player_.seek(ms); }
  void select_track(const std::string &type, int index) override { player_.select_track(type, index); }
  void set_subtitles(bool enabled) override { player_.set_subtitles(enabled); }

 private:
  UmsPlayer &player_;
};

struct App {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  Gfx gfx;
  Font regular, semibold, bold;
  WebOS webos;
  std::string dir, id;
  LaunchParams launch;
  Uint32 wake_event = 0;
  bool running = true;
  bool dirty = true;

  Screen screen = Screen::Home;
  int home_focus = 0;
  std::string tv_model = "…";
  std::string last_key = "Press any remote key";

  // Player
  UmsPlayer ums;
  UmsPort port{ums};
  Session session;
  std::string window_id;
  std::string title;
  std::string uri;
  std::string load_type = "media";
  bool hud = true;
  long long hud_until = 0;
  Zone zone = Zone::Timeline;
  int control = 1;
  Menu menu = Menu::None;
  int menu_index = 0;
  int error_focus = 0;
  long long last_render_second = -1;

  // Scripted runs (BLOOB_SCRIPT), used by the desktop UI test build:
  // steps separated by '|': key:<name>, wait:<ms>, media:<event json>, snap:<name>, quit.
  struct Step { std::string kind, arg; };
  std::vector<Step> script;
  size_t script_pos = 0;
  long long script_at = 0;
  std::string snapshot_dir;

  long long now() const { return static_cast<long long>(SDL_GetTicks()); }

  // ---------------------------------------------------------------- lifecycle

  bool init(int argc, char **argv) {
    dir = app_directory();
    id = app_id();
    SDL_SetHint("SDL_WEBOS_ACCESS_POLICY_KEYS_BACK", "true");
    SDL_SetHint("SDL_WEBOS_ACCESS_POLICY_KEYS_EXIT", "true");
    SDL_SetHint("SDL_WEBOS_CURSOR_SLEEP_TIME", "5000");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
      logf("app", "SDL_Init failed: %s", SDL_GetError());
      return false;
    }
    webos.resolve();
    SDL_DisplayMode mode{};
    SDL_GetCurrentDisplayMode(0, &mode);
    int ww = mode.w > 0 ? mode.w : W, wh = mode.h > 0 ? mode.h : H;
    if (const char *forced = std::getenv("BLOOB_WINDOW")) std::sscanf(forced, "%dx%d", &ww, &wh);  // desktop tests
    window = SDL_CreateWindow("Bloob", 0, 0, ww, wh, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
    if (!window) {
      logf("app", "SDL_CreateWindow failed: %s", SDL_GetError());
      return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
      logf("app", "SDL_CreateRenderer failed: %s", SDL_GetError());
      return false;
    }
    SDL_RendererInfo info{};
    SDL_GetRendererInfo(renderer, &info);
    logf("app", "video=%s renderer=%s window=%dx%d display=%dx%d@%d missing=[%s]", SDL_GetCurrentVideoDriver(),
         info.name, ww, wh, mode.w, mode.h, mode.refresh_rate, webos.missing().c_str());
    gfx.init(renderer, W, H);
    std::string fonts = dir + "/assets/fonts/";
    regular.load(renderer, fonts + "Inter-Regular.ttf");
    semibold.load(renderer, fonts + "Inter-SemiBold.ttf");
    bold.load(renderer, fonts + "Inter-Bold.ttf");

    wake_event = SDL_RegisterEvents(1);
    Uint32 wake = wake_event;
    Luna::instance().start([wake] {
      SDL_Event e{};
      e.type = wake;
      SDL_PushEvent(&e);
    });
    Luna::instance().call("luna://com.webos.service.tv.systemproperty/getSystemInfo",
                          "{\"keys\":[\"modelName\",\"firmwareVersion\",\"sdkVersion\"]}", [this](const std::string &reply) {
                            Json j = Json::parse(reply);
                            if (j["returnValue"].as_bool()) {
                              tv_model = j["modelName"].as_string("?") + " \xC2\xB7 webOS " + j["sdkVersion"].as_string("?") +
                                         " \xC2\xB7 firmware " + j["firmwareVersion"].as_string("?");
                            } else {
                              tv_model = "System info unavailable: " + j["errorText"].as_string(reply);
                            }
                            logf("app", "system info: %s", tv_model.c_str());
                            dirty = true;
                          });

    load_script();
    launch = parse_launch(argc, argv);
    if (launch.autoplay && !launch.uri.empty()) open_player(launch.uri, launch.title, launch.load_type);
    return true;
  }

  void shutdown() {
    close_player();
    Luna::instance().stop();
    gfx.shutdown();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
  }

  std::string test_clip_uri() const { return "file://" + dir + "/assets/media/big-buck-bunny-480p-30sec.mp4"; }

  // ---------------------------------------------------------------- player

  void open_player(const std::string &media_uri, const std::string &media_title, const std::string &type) {
    close_player();
    uri = media_uri;
    title = media_title;
    load_type = type;
    screen = Screen::Player;
    menu = Menu::None;
    zone = Zone::Timeline;
    control = 1;
    error_focus = 0;
    show_hud();
    session.open(&port, now());
    if (webos.create_exported_window) {
      const char *created = webos.create_exported_window(0);
      window_id = created ? created : "";
    }
    logf("player", "exported video window: %s", window_id.empty() ? "(none)" : window_id.c_str());
    place_video(0, 0);
    ums.load(uri, id, window_id, load_type, [this](const MediaEvent &event) {
      long long t = now();
      bool had_frame = session.first_frame_ms() >= 0;
      auto pending = session.pending_ms();
      long long seek_started = session.seek_started_ms();
      session.on_event(event, ums.info(), t);
      if (!had_frame && session.first_frame_ms() >= 0) logf("player", "first frame after %lld ms", session.first_frame_ms());
      if (pending && !session.pending_ms()) {
        logf("player", "seek to %lld ms landed at %lld ms after %lld ms", *pending, session.position_ms(t),
             t - seek_started);
      }
      if (event.kind == MediaEvent::VideoInfo || event.kind == MediaEvent::SourceInfo) {
        place_video(ums.info().width, ums.info().height);
      }
      if (event.kind == MediaEvent::Loaded) place_video(ums.info().width, ums.info().height);
      dirty = true;
    });
    dirty = true;
  }

  void place_video(int video_w, int video_h) {
    if (window_id.empty() || !webos.set_exported_window) return;
    int dw = W, dh = H;
    SDL_GetWindowSize(window, &dw, &dh);
    SDL_Rect src{0, 0, video_w > 0 ? video_w : dw, video_h > 0 ? video_h : dh};
    SDL_Rect dst{0, 0, dw, dh};
    webos.set_exported_window(window_id.c_str(), &src, &dst);
  }

  void close_player() {
    if (screen != Screen::Player && window_id.empty()) return;
    ums.unload();
    session.close();
    if (!window_id.empty() && webos.destroy_exported_window) webos.destroy_exported_window(window_id.c_str());
    window_id.clear();
    menu = Menu::None;
    if (screen == Screen::Player) screen = Screen::Home;
    dirty = true;
  }

  void show_hud() {
    hud = true;
    hud_until = now() + kHudTimeoutMs;
  }

  bool hud_pinned() const {
    Phase p = session.phase();
    return menu != Menu::None || p == Phase::Paused || p == Phase::Ended || p == Phase::Loading || p == Phase::Error ||
           session.preview_ms().has_value() || session.pending_ms().has_value();
  }

  struct Control { const char *label; };
  static constexpr int kControls = 5;  // back10, toggle, fwd10, audio, subtitles

  void run_control(int index) {
    switch (index) {
      case 0: session.nudge(-1, now()); break;
      case 1: session.toggle(); break;
      case 2: session.nudge(1, now()); break;
      case 3: menu = Menu::Audio; menu_index = selected_index(Menu::Audio); break;
      case 4: menu = Menu::Subtitles; menu_index = selected_index(Menu::Subtitles); break;
    }
    show_hud();
  }

  int selected_index(Menu which) const {
    const MediaInfo &info = ums.info();
    if (which == Menu::Audio) {
      for (size_t i = 0; i < info.audio.size(); i++) if (info.audio[i].selected) return static_cast<int>(i);
      return 0;
    }
    return session.subtitle_index() + 1;  // row 0 = Off
  }

  int menu_rows() const {
    const MediaInfo &info = ums.info();
    return menu == Menu::Audio ? static_cast<int>(info.audio.size()) : static_cast<int>(info.text.size()) + 1;
  }

  void player_key(Key key, bool repeat) {
    long long t = now();
    if (session.phase() == Phase::Error) {
      if (key == Key::Left || key == Key::Right) error_focus = 1 - error_focus;
      if (key == Key::Ok) {
        if (error_focus == 0) open_player(uri, title, load_type);
        else close_player();
      }
      if (key == Key::Back || key == Key::Stop) close_player();
      return;
    }
    if (menu != Menu::None) {
      int rows = menu_rows();
      if (key == Key::Up) menu_index = std::max(0, menu_index - 1);
      if (key == Key::Down) menu_index = std::min(std::max(0, rows - 1), menu_index + 1);
      if (key == Key::Ok && rows > 0) {
        if (menu == Menu::Audio) session.select_audio(menu_index);
        else session.select_text(menu_index - 1);
        menu = Menu::None;
      }
      if (key == Key::Back || key == Key::Left) menu = Menu::None;
      show_hud();
      return;
    }
    switch (key) {
      case Key::Play: session.play(); show_hud(); return;
      case Key::Pause: session.pause(); show_hud(); return;
      case Key::PlayPause: session.toggle(); show_hud(); return;
      case Key::Forward: session.nudge(1, t); show_hud(); return;
      case Key::Rewind: session.nudge(-1, t); show_hud(); return;
      case Key::Stop: close_player(); return;
      default: break;
    }
    if (key == Key::Back) {
      if (session.cancel_preview()) { show_hud(); return; }
      if (hud && session.phase() == Phase::Playing) { hud = false; return; }
      close_player();
      return;
    }
    if (!hud) {
      if (key == Key::Left || key == Key::Right) session.nudge(key == Key::Right ? 1 : -1, t);
      if (key == Key::Ok && !repeat) { session.toggle(); zone = Zone::Controls; control = 1; }
      if (key == Key::Down) { zone = Zone::Controls; control = 1; }
      if (key == Key::Up) zone = Zone::Timeline;
      show_hud();
      return;
    }
    show_hud();
    if (zone == Zone::Timeline) {
      if (key == Key::Left || key == Key::Right) session.nudge(key == Key::Right ? 1 : -1, t);
      if (key == Key::Ok && !repeat) { if (!session.commit_preview(t)) session.toggle(); }
      if (key == Key::Down) { zone = Zone::Controls; control = 1; }
      return;
    }
    if (key == Key::Up) zone = Zone::Timeline;
    if (key == Key::Left) control = std::max(0, control - 1);
    if (key == Key::Right) control = std::min(kControls - 1, control + 1);
    if (key == Key::Ok && !repeat) run_control(control);
  }

  // ---------------------------------------------------------------- input

  void on_key(const SDL_KeyboardEvent &event) {
    Key key = key_from_event(event);
    char buf[160];
    std::snprintf(buf, sizeof buf, "%s  \xC2\xB7  scancode %d  \xC2\xB7  keycode %d", key_name(key),
                  static_cast<int>(event.keysym.scancode), static_cast<int>(event.keysym.sym));
    last_key = buf;
    if (key == Key::None) logf("input", "unmapped key: scancode=%d sym=%d", event.keysym.scancode, event.keysym.sym);
    dirty = true;
    if (key == Key::Exit) { running = false; return; }
    bool repeat = event.repeat != 0;

    switch (screen) {
      case Screen::Home: {
        int cards = launch.uri.empty() ? 2 : 3;
        if (key == Key::Left) home_focus = std::max(0, home_focus - 1);
        if (key == Key::Right) home_focus = std::min(cards - 1, home_focus + 1);
        if (key == Key::Back) running = false;
        if (key == Key::Ok && !repeat) {
          int action = launch.uri.empty() && home_focus >= 1 ? home_focus + 1 : home_focus;
          if (action == 0) open_player(test_clip_uri(), "Big Buck Bunny", "media");
          if (action == 1) open_player(launch.uri, launch.title, launch.load_type);
          if (action == 2) screen = Screen::Diagnostics;
        }
        break;
      }
      case Screen::Diagnostics:
        if (key == Key::Back) screen = Screen::Home;
        break;
      case Screen::Player:
        player_key(key, repeat);
        break;
    }
  }

  void on_event(const SDL_Event &e) {
    if (e.type == SDL_QUIT) running = false;
    else if (e.type == SDL_KEYDOWN) on_key(e.key);
    else if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST || e.window.event == SDL_WINDOWEVENT_HIDDEN) {
        // App moved to the background: pause so we resume where we left off.
        if (screen == Screen::Player && session.intent_play()) session.pause();
        logf("app", "window hidden/focus lost");
      }
      dirty = true;
    } else if (e.type == SDL_APP_WILLENTERBACKGROUND) {
      if (screen == Screen::Player && session.intent_play()) session.pause();
    }
  }

  // ---------------------------------------------------------------- scripted runs

  void load_script() {
    const char *spec = std::getenv("BLOOB_SCRIPT");
    const char *dir_env = std::getenv("BLOOB_SNAPSHOT_DIR");
    snapshot_dir = dir_env ? dir_env : ".";
    if (!spec) return;
    std::string all = spec;
    size_t start = 0;
    while (start <= all.size()) {
      size_t end = all.find('|', start);
      std::string step = all.substr(start, end == std::string::npos ? std::string::npos : end - start);
      size_t colon = step.find(':');
      if (!step.empty()) script.push_back({step.substr(0, colon), colon == std::string::npos ? "" : step.substr(colon + 1)});
      if (end == std::string::npos) break;
      start = end + 1;
    }
    logf("script", "%zu steps", script.size());
  }

  void press(const std::string &name) {
    static const std::pair<const char *, int> codes[] = {
        {"up", SDL_SCANCODE_UP}, {"down", SDL_SCANCODE_DOWN}, {"left", SDL_SCANCODE_LEFT}, {"right", SDL_SCANCODE_RIGHT},
        {"ok", SDL_SCANCODE_RETURN}, {"back", kScancodeWebosBack}, {"play", SDL_SCANCODE_AUDIOPLAY},
        {"pause", SDL_SCANCODE_PAUSE}};
    SDL_KeyboardEvent key{};
    key.type = SDL_KEYDOWN;
    for (auto &c : codes) if (name == c.first) key.keysym.scancode = static_cast<SDL_Scancode>(c.second);
    on_key(key);
  }

  void snapshot(const std::string &name) {
    render();
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    if (gfx.canvas()) {  // capture the 1920×1080 frame itself
      SDL_SetRenderTarget(renderer, gfx.canvas());
      w = W;
      h = H;
    }
    SDL_Surface *surface = SDL_CreateRGBSurface(0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!surface) return;
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
    SDL_SetRenderTarget(renderer, nullptr);
    std::string path = snapshot_dir + "/" + name + ".bmp";
    SDL_SaveBMP(surface, path.c_str());
    SDL_FreeSurface(surface);
    logf("script", "snapshot %s", path.c_str());
  }

  void run_script(long long t) {
    while (script_pos < script.size() && t >= script_at) {
      const Step &step = script[script_pos++];
      if (step.kind == "wait") { script_at = t + std::atoll(step.arg.c_str()); return; }
      if (step.kind == "key") press(step.arg);
      else if (step.kind == "media") Luna::instance().inject(step.arg);
      else if (step.kind == "pump") Luna::instance().pump();
      else if (step.kind == "snap") snapshot(step.arg);
      else if (step.kind == "quit") running = false;
      dirty = true;
    }
  }

  // ---------------------------------------------------------------- drawing

  void text(Font &font, const std::string &s, int x, int y, int size, Color color) { font.draw(s, x, y, size, color); }
  void text_right(Font &font, const std::string &s, int right, int y, int size, Color color) {
    font.draw(s, right - font.measure(s, size), y, size, color);
  }
  void text_center(Font &font, const std::string &s, int cx, int y, int size, Color color) {
    font.draw(s, cx - font.measure(s, size) / 2, y, size, color);
  }

  void draw_home() {
    gfx.clear(kBg);
    gfx.fill_round(96, 72, 56, 56, 16, kAccent);
    text_center(bold, "b", 124, 78, 34, kInk);
    text(bold, "bloob", 172, 76, 40, kText);
    text(semibold, "NATIVE PREVIEW \xC2\xB7 BUNA N1", 96, 250, 20, kText3);
    text(bold, "Native playback, our own UI.", 96, 290, 76, kText);
    text(regular, "Video plays through the TV's media pipeline in a hardware plane under this interface.", 96, 400, 28, kText2);

    struct Card { std::string title, detail; };
    std::vector<Card> cards = {{"Play test clip", "Big Buck Bunny \xC2\xB7 bundled \xC2\xB7 30 seconds"}};
    if (!launch.uri.empty()) cards.push_back({"Play launch link", launch.title});
    cards.push_back({"Diagnostics", "Display, services and a remote key tester"});
    int x = 96;
    for (size_t i = 0; i < cards.size(); i++) {
      bool focused = static_cast<int>(i) == home_focus;
      int w = 520, h = 220, y = 540;
      if (focused) { x -= 8; y -= 8; w += 16; h += 16; }
      gfx.fill_round(x, y, w, h, 28, focused ? kWhite : kPanel);
      Color fg = focused ? kInk : kText;
      Color fg2 = focused ? rgba(0x3a3f47) : kText2;
      if (i == 0) gfx.triangle_right(x + 44, y + 52, 36, fg);
      text(bold, cards[i].title, x + 44, y + (i == 0 ? 104 : 60), 36, fg);
      text(regular, cards[i].detail, x + 44, y + (i == 0 ? 154 : 112), 22, fg2);
      if (focused) { x += 8; }
      x += 560;
    }
    text(regular, tv_model, 96, 960, 22, kText3);
    text(regular, "\xE2\x86\x90 \xE2\x86\x92 choose  \xC2\xB7  OK open  \xC2\xB7  Back exit", 96, 1000, 22, kText3);
  }

  void draw_diagnostics() {
    gfx.clear(kBg);
    text(bold, "Diagnostics", 96, 72, 56, kText);
    SDL_DisplayMode mode{};
    SDL_GetCurrentDisplayMode(0, &mode);
    SDL_RendererInfo info{};
    SDL_GetRendererInfo(renderer, &info);
    int pw = 0, ph = 0, refresh = 0;
    if (webos.get_panel_resolution) webos.get_panel_resolution(&pw, &ph);
    if (webos.get_refresh_rate) webos.get_refresh_rate(&refresh);
    int ww = 0, wh = 0;
    SDL_GetWindowSize(window, &ww, &wh);
    std::string missing = webos.missing();
    std::vector<std::pair<std::string, std::string>> rows = {
        {"TV", tv_model},
        {"SDL video driver", SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "?"},
        {"Renderer", std::string(info.name ? info.name : "?") + ((info.flags & SDL_RENDERER_ACCELERATED) ? " (accelerated)" : " (software)")},
        {"Window / display", std::to_string(ww) + "\xC3\x97" + std::to_string(wh) + " / " + std::to_string(mode.w) + "\xC3\x97" +
                                 std::to_string(mode.h) + " @ " + std::to_string(mode.refresh_rate) + " Hz"},
        {"Panel", pw ? std::to_string(pw) + "\xC3\x97" + std::to_string(ph) + " @ " + std::to_string(refresh) + " Hz" : "unknown"},
        {"LG SDL extensions", missing.empty() ? "all present" : "missing: " + missing},
        {"App", id + "  \xC2\xB7  " + dir},
        {"Log file", "/tmp/bloob-native.log"},
        {"Last key", last_key},
    };
    int y = 180;
    for (auto &row : rows) {
      text(regular, row.first, 96, y, 24, kText3);
      text(semibold, row.second, 520, y, 24, kText);
      y += 46;
    }
    text(semibold, "Recent log", 96, y + 20, 24, kText2);
    y += 64;
    for (auto &line : log_tail(9)) {
      text(regular, line.substr(0, 150), 96, y, 18, kText3);
      y += 28;
    }
    text(regular, "Press any remote key to see its code  \xC2\xB7  Back returns", 96, 1020, 22, kText3);
  }

  void draw_player() {
    long long t = now();
    Phase phase = session.phase();
    // Transparent: the hardware video plane under the window shows through.
    gfx.clear({0, 0, 0, 0});

    if (phase == Phase::Loading && !session.started()) {
      gfx.fill_rect(0, 0, W, H, kBg);
      text_center(bold, title, W / 2, 440, 56, kText);
      text_center(regular, "Starting\xE2\x80\xA6", W / 2, 530, 26, kText2);
      return;
    }

    if (phase == Phase::Error) {
      gfx.fill_rect(0, 0, W, H, rgba(0x000000, 220));
      text_center(bold, "Playback stopped", W / 2, 380, 52, kText);
      text_center(regular, session.error(), W / 2, 470, 26, kText2);
      const char *labels[2] = {"Try again", "Back"};
      int x = W / 2 - 230;
      for (int i = 0; i < 2; i++) {
        bool f = error_focus == i;
        gfx.fill_round(x, 560, 220, 72, 36, f ? kWhite : rgba(0xffffff, 36));
        text_center(semibold, labels[i], x + 110, 580, 26, f ? kInk : kText);
        x += 240;
      }
      return;
    }

    bool show = hud || hud_pinned();
    if (!show) {
      if (phase == Phase::Buffering) draw_chip("Buffering\xE2\x80\xA6", W / 2 - 90, H / 2 - 30);
      return;
    }

    // Top: title and wall clock. Bottom: status, timeline, controls.
    gfx.fill_rect(0, 0, W, 180, rgba(0x000000, 120));
    gfx.fill_rect(0, H - 330, W, 330, rgba(0x000000, 150));
    text(bold, title, 96, 56, 38, kText);
    const MediaInfo &info = ums.info();
    std::string stream;
    if (!info.container.empty()) {
      std::string container = info.container;
      for (auto &c : container) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
      stream = container;
    }
    if (!info.video_codec.empty()) {
      char buf[96];
      char fps[16];
      std::snprintf(fps, sizeof fps, "%.3f", info.frame_rate);
      std::string f = fps;
      while (!f.empty() && f.back() == '0') f.pop_back();
      if (!f.empty() && f.back() == '.') f.pop_back();
      std::snprintf(buf, sizeof buf, " \xC2\xB7 %s %d\xC3\x97%d %s fps", info.video_codec.c_str(), info.width, info.height, f.c_str());
      stream += buf;
    }
    if (!info.hdr.empty() && info.hdr != "none" && info.hdr != "NONE") stream += " \xC2\xB7 " + info.hdr;
    for (auto &a : info.audio) if (a.selected) stream += " \xC2\xB7 " + audio_label(a);
    if (!stream.empty()) text(regular, stream, 96, 108, 22, kText2);

    long long display = session.display_ms(t);
    long long duration = session.duration_ms();
    long long position = session.position_ms(t);
    std::string status = phase == Phase::Seeking ? "Seeking\xE2\x80\xA6" : phase == Phase::Buffering ? "Buffering\xE2\x80\xA6"
                         : phase == Phase::Ended ? "Finished" : phase == Phase::Paused ? "Paused" : "";
    if (!status.empty()) draw_chip(status, 96, H - 300);

    int tx = 96, tw = W - 192, ty = H - 218;
    bool timeline_focus = zone == Zone::Timeline;
    int th = timeline_focus ? 8 : 6;
    gfx.fill_round(tx, ty - th / 2, tw, th, th / 2, rgba(0xffffff, 60));
    double frac = duration > 0 ? std::min(1.0, std::max(0.0, static_cast<double>(display) / duration)) : 0;
    gfx.fill_round(tx, ty - th / 2, static_cast<int>(tw * frac), th, th / 2, kWhite);
    if (timeline_focus || session.preview_ms() || session.pending_ms()) {
      gfx.fill_circle(tx + static_cast<int>(tw * frac), ty, 12, kWhite);
    }
    if (session.preview_ms() || session.pending_ms()) {
      std::string label = clock_text(display) + "   " + delta_text(display - session.seek_origin_ms(position));
      int lw = bold.measure(label, 28) + 40;
      int lx = std::max(tx, std::min(tx + tw - lw, tx + static_cast<int>(tw * frac) - lw / 2));
      gfx.fill_round(lx, ty - 92, lw, 60, 14, kWhite);
      text(bold, label, lx + 20, ty - 82, 28, kInk);
    }
    text(regular, clock_text(display), tx, ty + 22, 24, kText2);
    text_right(regular, duration > 0 ? "\xE2\x88\x92" + clock_text(std::max(0LL, duration - display)) : "--:--", tx + tw, ty + 22, 24, kText2);

    // Controls row.
    const char *names[kControls] = {"Back 10s", "Play/Pause", "Forward 10s", "Audio", "Subtitles"};
    int cx = 96;
    for (int i = 0; i < kControls; i++) {
      bool f = zone == Zone::Controls && control == i;
      int size = i == 1 ? 84 : 72;
      int x = i >= 3 ? W - 96 - (kControls - i) * 92 + (92 - size) : cx;
      int y = H - 140 + (84 - size) / 2;
      gfx.fill_circle(x + size / 2, y + size / 2, size / 2, f ? kWhite : rgba(0xffffff, 40));
      Color fg = f ? kInk : kText;
      if (i == 1) {
        if (session.intent_play() && phase != Phase::Ended) gfx.bars(x + 28, y + 26, 30, fg);
        else gfx.triangle_right(x + 32, y + 24, 36, fg);
      } else if (i == 0 || i == 2) {
        text_center(bold, i == 0 ? "\xE2\x88\x92" "10" : "+10", x + size / 2, y + 22, 22, fg);
      } else {
        text_center(bold, i == 3 ? "A" : "CC", x + size / 2, y + 20, 26, fg);
      }
      if (f) text_center(semibold, names[i], x + size / 2, y + size + 10, 20, kText);
      if (i < 3) cx = x + size + 20;
    }

    if (menu != Menu::None) draw_menu();
  }

  void draw_chip(const std::string &label, int x, int y) {
    int w = semibold.measure(label, 22) + 36;
    gfx.fill_round(x, y, w, 48, 24, rgba(0xffffff, 44));
    text(semibold, label, x + 18, y + 11, 22, kText);
  }

  void draw_menu() {
    const MediaInfo &info = ums.info();
    int x = W - 96 - 600, y = 200, w = 600;
    int rows = menu_rows();
    int h = 120 + std::max(1, rows) * 64 + 60;
    gfx.fill_round(x, y, w, h, 26, rgba(0x121418, 245));
    text(bold, menu == Menu::Audio ? "Audio" : "Subtitles", x + 36, y + 32, 32, kText);
    int ry = y + 100;
    if (rows == 0) {
      text(regular, "This video reports no audio tracks.", x + 36, ry, 22, kText2);
    }
    for (int i = 0; i < rows; i++) {
      bool f = i == menu_index;
      std::string label;
      bool selected = false;
      if (menu == Menu::Audio) {
        label = audio_label(info.audio[static_cast<size_t>(i)]);
        selected = info.audio[static_cast<size_t>(i)].selected;
      } else if (i == 0) {
        label = "Off";
        selected = session.subtitle_index() < 0;
      } else {
        const MediaTrack &tr = info.text[static_cast<size_t>(i - 1)];
        label = language_name(tr.language) + (tr.codec.empty() ? "" : " \xC2\xB7 " + tr.codec);
        selected = session.subtitle_index() == i - 1;
      }
      if (f) gfx.fill_round(x + 16, ry - 12, w - 32, 56, 14, kWhite);
      text(regular, label, x + 36, ry, 24, f ? kInk : kText);
      if (selected) text_right(bold, "\xE2\x9C\x93", x + w - 36, ry, 24, f ? kInk : kText);
      ry += 64;
    }
    text(regular, "Back to close", x + 36, y + h - 48, 18, kText3);
  }

  void render() {
    gfx.begin_frame();
    switch (screen) {
      case Screen::Home: draw_home(); break;
      case Screen::Diagnostics: draw_diagnostics(); break;
      case Screen::Player: draw_player(); break;
    }
    gfx.end_frame();
    SDL_RenderPresent(renderer);
    dirty = false;
  }

  // ---------------------------------------------------------------- loop

  int wait_timeout() const {
    if (script_pos < script.size()) return 10;
    if (screen != Screen::Player) return 1000;
    // Redraw often enough for a moving clock only when it is on screen.
    if (session.preview_ms()) return 50;
    bool show = hud || hud_pinned();
    return show ? 250 : 1000;
  }

  void loop() {
    while (running) {
      SDL_Event e;
      if (SDL_WaitEventTimeout(&e, wait_timeout())) {
        if (e.type != wake_event) on_event(e);
        while (SDL_PollEvent(&e)) if (e.type != wake_event) on_event(e);
      }
      Luna::instance().pump();
      long long t = now();
      if (!script.empty()) run_script(t);
      if (screen == Screen::Player) {
        if (session.tick(t)) dirty = true;
        if (hud && t > hud_until && !hud_pinned()) { hud = false; dirty = true; }
        bool show = hud || hud_pinned();
        long long second = session.display_ms(t) / 250;
        if (show && second != last_render_second) { last_render_second = second; dirty = true; }
      }
      if (dirty) render();
    }
  }
};

}  // namespace

int main(int argc, char **argv) {
  log_init("/tmp/bloob-native.log");
  logf("app", "Bloob native host starting");
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
