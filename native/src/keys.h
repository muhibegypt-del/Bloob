#pragma once
// Remote input: SDL key events → semantic keys. webOS scancodes come from
// SDL-webOS (Back = 482 etc.); Back reaches the app only when the
// SDL_WEBOS_ACCESS_POLICY_KEYS_BACK hint is set.
#include <SDL.h>

#include <string>

enum class Key { None, Up, Down, Left, Right, Ok, Back, Exit, Play, Pause, PlayPause, Stop, Forward, Rewind, Red, Green, Yellow, Blue };

constexpr int kScancodeWebosBack = 482;
constexpr int kScancodeWebosExit = 505;
constexpr int kScancodeWebosRed = 486;
constexpr int kScancodeWebosGreen = 487;
constexpr int kScancodeWebosYellow = 488;
constexpr int kScancodeWebosBlue = 489;

inline Key key_from_event(const SDL_KeyboardEvent &event) {
  switch (static_cast<int>(event.keysym.scancode)) {
    case SDL_SCANCODE_UP: return Key::Up;
    case SDL_SCANCODE_DOWN: return Key::Down;
    case SDL_SCANCODE_LEFT: return Key::Left;
    case SDL_SCANCODE_RIGHT: return Key::Right;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
    case SDL_SCANCODE_SELECT: return Key::Ok;
    case kScancodeWebosBack:
    case SDL_SCANCODE_AC_BACK:
    case SDL_SCANCODE_ESCAPE:
    case SDL_SCANCODE_BACKSPACE: return Key::Back;
    case kScancodeWebosExit: return Key::Exit;
    case SDL_SCANCODE_AUDIOPLAY: return Key::Play;
    case SDL_SCANCODE_PAUSE: return Key::Pause;
    case SDL_SCANCODE_SPACE: return Key::PlayPause;
    case SDL_SCANCODE_AUDIOSTOP: return Key::Stop;
    case SDL_SCANCODE_AUDIONEXT:
    case SDL_SCANCODE_AUDIOFASTFORWARD: return Key::Forward;
    case SDL_SCANCODE_AUDIOPREV:
    case SDL_SCANCODE_AUDIOREWIND: return Key::Rewind;
    case kScancodeWebosRed: return Key::Red;
    case kScancodeWebosGreen: return Key::Green;
    case kScancodeWebosYellow: return Key::Yellow;
    case kScancodeWebosBlue: return Key::Blue;
    default: return Key::None;
  }
}

inline const char *key_name(Key key) {
  switch (key) {
    case Key::Up: return "Up";
    case Key::Down: return "Down";
    case Key::Left: return "Left";
    case Key::Right: return "Right";
    case Key::Ok: return "OK";
    case Key::Back: return "Back";
    case Key::Exit: return "Exit";
    case Key::Play: return "Play";
    case Key::Pause: return "Pause";
    case Key::PlayPause: return "Play/Pause";
    case Key::Stop: return "Stop";
    case Key::Forward: return "Fast-forward";
    case Key::Rewind: return "Rewind";
    case Key::Red: return "Red";
    case Key::Green: return "Green";
    case Key::Yellow: return "Yellow";
    case Key::Blue: return "Blue";
    default: return "Unmapped";
  }
}
