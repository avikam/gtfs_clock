#pragma once

#include <string>
#include <vector>
#include <json.hpp>

namespace config {

enum Direction { Unknown, South, North };
struct Stop {
    std::string url;
    Direction direction;
};

struct Config {
  int active_stop;
  struct Connection {
    std::string ssid;
    std::string password;
  } connection;

  std::vector<Stop> stops;
};

template <class Inp>
bool parse(Inp&& inp, Config* conf) {
  auto parsed = nlohmann::json::parse(inp);

  conf->active_stop = parsed["active_stop"];
  for (auto& stop_obj : parsed["stops"]) {
    Stop stop;
    stop.url = stop_obj["url"];

    std::string dir_string = stop_obj["direction"];
    if (dir_string == "South") stop.direction = Direction::South;
    else if (dir_string == "North") stop.direction =  Direction::North;

    (conf->stops).push_back(stop);
  }

  const auto& connection = parsed["connection"];
  conf->connection.ssid = connection["ssid"];
  conf->connection.password = connection["password"];

  return true;
}

} // namespace config