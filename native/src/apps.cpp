#include "apps.h"

#include "json.h"

static bool is_input(const std::string &id) {
  return id.compare(0, 18, "com.webos.app.hdmi") == 0 || id == "com.webos.app.livetv" || id == "com.webos.app.externalinput.av1" ||
         id == "com.webos.app.externalinput.component";
}

std::vector<App> parse_launch_points(const std::string &reply, const std::string &self_id) {
  bool ok = false;
  Json j = Json::parse(reply, &ok);
  std::vector<App> apps, inputs;
  if (!ok) return apps;
  const Json &list = j["launchPoints"];
  for (size_t i = 0; i < list.size(); i++) {
    const Json &p = list[i];
    App a;
    a.id = p["id"].as_string();
    a.title = p["title"].as_string();
    a.icon = p["largeIcon"].as_string(p["icon"].as_string());
    if (a.id.empty() || a.title.empty() || a.id == self_id || a.id == "com.webos.app.home") continue;
    a.input = is_input(a.id);
    if (a.input) inputs.push_back(a);
    else if (a.id == "youtube.leanback.v4") apps.insert(apps.begin(), a);
    else apps.push_back(a);
  }
  apps.insert(apps.end(), inputs.begin(), inputs.end());
  return apps;
}

std::string launch_payload(const std::string &id) { return "{\"id\":" + Json::quote(id) + "}"; }
