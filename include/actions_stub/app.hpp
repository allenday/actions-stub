#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace actions_stub {

struct Settings {
  std::string app_name = "actions-stub";
  std::string host = "0.0.0.0";
  int port = 8080;
  std::string config_dir = "/config";
};

struct HttpTarget {
  std::string host = "127.0.0.1";
  int port = 8080;
  std::string path = "/health";
};

Settings settings_from_environment();
std::string render_health();
std::string render_home(const Settings& settings);
std::string http_response(int status, const std::string& reason, const std::string& body);
std::vector<std::string> split_mounts(const std::string& mounts);
int inspect_mounts(std::ostream& output, const std::vector<std::string>& directories);
HttpTarget parse_http_target(const std::string& value);
int healthcheck(const HttpTarget& target);
int serve(const Settings& settings);

}  // namespace actions_stub
