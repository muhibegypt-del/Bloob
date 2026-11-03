#pragma once
// The TV's own apps (2.6, launcher), from applicationManager/listLaunchPoints:
// used to show the YouTube tab only when the app is installed. Parsing is pure
// and tested; the luna calls live in main.cpp.
#include <string>
#include <vector>

struct LaunchPoint {
  std::string id, title, icon;  // icon: a file on the TV, served by the service's /api/image
  bool input = false;           // an HDMI or other input, shown after the apps
};

// Launch points (a listLaunchPoints reply, or its text from luna-send) in the
// row's order: YouTube first, then apps as the TV lists them, then inputs.
// Leaves out Bloob itself and LG's own home screen.
std::vector<LaunchPoint> parse_launch_points(const std::string &reply, const std::string &self_id);

// The payload for applicationManager/launch.
std::string launch_payload(const std::string &id);
