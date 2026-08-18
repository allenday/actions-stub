#include "actions_stub/app.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

int main() {
  actions_stub::Settings settings;
  settings.app_name = "unit-test";
  settings.config_dir = "/tmp/config";

  assert(actions_stub::render_health() == "ok\n");
  assert(actions_stub::render_home(settings).find("hello from unit-test") != std::string::npos);
  assert(actions_stub::http_response(200, "OK", "ok\n").find("Content-Length: 3") !=
         std::string::npos);

  const auto mounts = actions_stub::split_mounts("/a:/b::/c");
  assert(mounts.size() == 3);
  assert(mounts[0] == "/a");
  assert(mounts[2] == "/c");

  const auto temp = std::filesystem::temp_directory_path() / "actions-stub-test-readme";
  std::filesystem::create_directories(temp);
  {
    std::ofstream readme(temp / "README.md");
    readme << "mounted readme\n";
  }

  std::ostringstream output;
  assert(actions_stub::inspect_mounts(output, {temp.string()}) == 0);
  assert(output.str().find("mounted readme") != std::string::npos);

  std::filesystem::remove_all(temp);
}
