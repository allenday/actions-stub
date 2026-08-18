#include "actions_stub/app.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

namespace actions_stub {
namespace {

volatile std::sig_atomic_t keep_running = 1;

void stop_server(int) {
  keep_running = 0;
}

void install_signal_handlers() {
  struct sigaction action {};
  action.sa_handler = stop_server;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGTERM, &action, nullptr);
  sigaction(SIGINT, &action, nullptr);
}

std::string getenv_or(const char* key, std::string fallback) {
  const char* value = std::getenv(key);
  if (value == nullptr || std::string_view(value).empty()) {
    return fallback;
  }
  return value;
}

int getenv_int_or(const char* key, int fallback) {
  const char* value = std::getenv(key);
  if (value == nullptr || std::string_view(value).empty()) {
    return fallback;
  }
  try {
    return std::stoi(value);
  } catch (const std::exception&) {
    return fallback;
  }
}

std::string request_path(const std::string& request) {
  std::istringstream input(request);
  std::string method;
  std::string path;
  input >> method >> path;
  if (path.empty()) {
    return "/";
  }
  return path;
}

int connect_to(const HttpTarget& target) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* raw_results = nullptr;
  const std::string port = std::to_string(target.port);
  if (getaddrinfo(target.host.c_str(), port.c_str(), &hints, &raw_results) != 0) {
    return -1;
  }

  int socket_fd = -1;
  for (addrinfo* item = raw_results; item != nullptr; item = item->ai_next) {
    socket_fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
    if (socket_fd == -1) {
      continue;
    }
    if (connect(socket_fd, item->ai_addr, item->ai_addrlen) == 0) {
      break;
    }
    close(socket_fd);
    socket_fd = -1;
  }

  freeaddrinfo(raw_results);
  return socket_fd;
}

}  // namespace

Settings settings_from_environment() {
  Settings settings;
  settings.app_name = getenv_or("APP_NAME", settings.app_name);
  settings.host = getenv_or("APP_HOST", settings.host);
  settings.port = getenv_int_or("APP_PORT", settings.port);
  settings.config_dir = getenv_or("APP_CONFIG_DIR", settings.config_dir);
  return settings;
}

std::string render_health() {
  return "ok\n";
}

std::string render_home(const Settings& settings) {
  std::ostringstream output;
  output << "hello from " << settings.app_name << "\n";
  output << "config_dir=" << settings.config_dir << "\n";
  return output.str();
}

std::string http_response(int status, const std::string& reason, const std::string& body) {
  std::ostringstream response;
  response << "HTTP/1.1 " << status << ' ' << reason << "\r\n";
  response << "Content-Type: text/plain; charset=utf-8\r\n";
  response << "Content-Length: " << body.size() << "\r\n";
  response << "Connection: close\r\n\r\n";
  response << body;
  return response.str();
}

std::vector<std::string> split_mounts(const std::string& mounts) {
  std::vector<std::string> result;
  std::string current;
  std::istringstream input(mounts);
  while (std::getline(input, current, ':')) {
    if (!current.empty()) {
      result.push_back(current);
    }
  }
  return result;
}

int inspect_mounts(std::ostream& output, const std::vector<std::string>& directories) {
  int missing = 0;
  for (const std::string& directory : directories) {
    const std::filesystem::path readme = std::filesystem::path(directory) / "README.md";
    output << "== " << readme.string() << " ==\n";
    std::ifstream input(readme);
    if (!input) {
      output << "missing README.md\n\n";
      ++missing;
      continue;
    }
    output << input.rdbuf();
    output << "\n";
  }
  return missing == 0 ? 0 : 1;
}

HttpTarget parse_http_target(const std::string& value) {
  std::string input = value;
  constexpr std::string_view prefix = "http://";
  if (input.rfind(prefix, 0) == 0) {
    input = input.substr(prefix.size());
  }

  HttpTarget target;
  const std::size_t slash = input.find('/');
  std::string host_port = slash == std::string::npos ? input : input.substr(0, slash);
  target.path = slash == std::string::npos ? "/" : input.substr(slash);

  const std::size_t colon = host_port.rfind(':');
  if (colon == std::string::npos) {
    target.host = host_port;
    target.port = 80;
  } else {
    target.host = host_port.substr(0, colon);
    target.port = std::stoi(host_port.substr(colon + 1));
  }
  if (target.path.empty()) {
    target.path = "/";
  }
  return target;
}

int healthcheck(const HttpTarget& target) {
  const int socket_fd = connect_to(target);
  if (socket_fd == -1) {
    return 1;
  }

  const std::string request = "GET " + target.path + " HTTP/1.1\r\nHost: " + target.host +
                              "\r\nConnection: close\r\n\r\n";
  if (send(socket_fd, request.data(), request.size(), 0) == -1) {
    close(socket_fd);
    return 1;
  }

  char buffer[512]{};
  const ssize_t received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
  close(socket_fd);
  if (received <= 0) {
    return 1;
  }
  const std::string response(buffer, static_cast<std::size_t>(received));
  return response.find("HTTP/1.1 200") == 0 || response.find("HTTP/1.0 200") == 0 ? 0 : 1;
}

int serve(const Settings& settings) {
  install_signal_handlers();

  const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    std::perror("socket");
    return 1;
  }

  int reuse = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<uint16_t>(settings.port));
  if (inet_pton(AF_INET, settings.host.c_str(), &address.sin_addr) != 1) {
    address.sin_addr.s_addr = INADDR_ANY;
  }

  if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
    std::perror("bind");
    close(server_fd);
    return 1;
  }
  if (listen(server_fd, 16) == -1) {
    std::perror("listen");
    close(server_fd);
    return 1;
  }

  std::cout << settings.app_name << " listening on " << settings.host << ':' << settings.port
            << std::endl;

  while (keep_running != 0) {
    sockaddr_in client{};
    socklen_t client_length = sizeof(client);
    const int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client), &client_length);
    if (client_fd == -1) {
      if (keep_running == 0) {
        break;
      }
      if (errno == EINTR) {
        continue;
      }
      continue;
    }

    char buffer[4096]{};
    const ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    const std::string request = received > 0 ? std::string(buffer, static_cast<std::size_t>(received)) : "";
    const std::string path = request_path(request);
    const std::string body = path == "/health" ? render_health() : render_home(settings);
    const std::string response = http_response(200, "OK", body);
    send(client_fd, response.data(), response.size(), 0);
    close(client_fd);
  }

  close(server_fd);
  return 0;
}

}  // namespace actions_stub
