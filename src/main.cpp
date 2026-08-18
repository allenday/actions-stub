#include "actions_stub/app.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void usage() {
  std::cout << "usage: actions-stub <serve|healthcheck|inspect-mounts>\n";
}

}  // namespace

int main(int argc, char** argv) {
  const std::string command = argc >= 2 ? argv[1] : "serve";

  if (command == "serve") {
    return actions_stub::serve(actions_stub::settings_from_environment());
  }

  if (command == "healthcheck") {
    const std::string target = argc >= 3 ? argv[2] : "http://127.0.0.1:8080/health";
    return actions_stub::healthcheck(actions_stub::parse_http_target(target));
  }

  if (command == "inspect-mounts") {
    std::vector<std::string> directories;
    if (argc > 2) {
      for (int index = 2; index < argc; ++index) {
        directories.emplace_back(argv[index]);
      }
    } else {
      const char* mounts = std::getenv("ACTIONS_STUB_MOUNT_DIRS");
      directories = actions_stub::split_mounts(mounts == nullptr ? "/config" : mounts);
    }
    return actions_stub::inspect_mounts(std::cout, directories);
  }

  usage();
  return 2;
}
