#pragma once
// LG SDL extensions, resolved at runtime so a TV whose SDL lacks one still
// starts (and reports it) instead of failing to load.
#include <SDL.h>

#include <string>

struct WebOS {
  SDL_bool (*get_panel_resolution)(int *, int *) = nullptr;
  SDL_bool (*get_refresh_rate)(int *) = nullptr;
  const char *(*create_exported_window)(int) = nullptr;
  SDL_bool (*set_exported_window)(const char *, SDL_Rect *, SDL_Rect *) = nullptr;
  void (*destroy_exported_window)(const char *) = nullptr;
  SDL_bool (*cursor_visibility)(SDL_bool) = nullptr;

  void resolve();
  std::string missing() const;
};

std::string app_directory();
std::string app_id();
