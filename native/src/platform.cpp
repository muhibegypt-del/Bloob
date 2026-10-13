#include "platform.h"

#include <dlfcn.h>
#include <unistd.h>

#include <climits>
#include <cstdlib>

template <typename T>
static void bind(T &target, const char *name) {
  target = reinterpret_cast<T>(dlsym(RTLD_DEFAULT, name));
}

void WebOS::resolve() {
  bind(get_panel_resolution, "SDL_webOSGetPanelResolution");
  bind(get_refresh_rate, "SDL_webOSGetRefreshRate");
  bind(create_exported_window, "SDL_webOSCreateExportedWindow");
  bind(set_exported_window, "SDL_webOSSetExportedWindow");
  bind(destroy_exported_window, "SDL_webOSDestroyExportedWindow");
  bind(cursor_visibility, "SDL_webOSCursorVisibility");
}

std::string WebOS::missing() const {
  std::string out;
  if (!get_panel_resolution) out += "PanelResolution ";
  if (!get_refresh_rate) out += "RefreshRate ";
  if (!create_exported_window) out += "CreateExportedWindow ";
  if (!set_exported_window) out += "SetExportedWindow ";
  if (!destroy_exported_window) out += "DestroyExportedWindow ";
  return out;
}

std::string app_directory() {
  char path[PATH_MAX];
  ssize_t n = readlink("/proc/self/exe", path, sizeof path - 1);
  if (n <= 0) return ".";
  std::string exe(path, static_cast<size_t>(n));
  // <app>/bin/bloob → <app>
  auto slash = exe.rfind('/');
  std::string bin = slash == std::string::npos ? "." : exe.substr(0, slash);
  slash = bin.rfind('/');
  return slash == std::string::npos ? "." : bin.substr(0, slash);
}

std::string app_id() {
  const char *id = std::getenv("APPID");
  return id && *id ? id : "com.muhib.bloob";
}
