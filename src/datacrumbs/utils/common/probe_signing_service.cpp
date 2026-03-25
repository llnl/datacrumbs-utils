#include <datacrumbs/utils/common/probe_signing_service.h>
#include <datacrumbs/datacrumbs_config.h>
#include <json-c/json.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <string>

namespace datacrumbs::probe_signing_service {

namespace {

std::string json_string_or_empty(json_object* root, const char* key) {
  json_object* value = nullptr;
  if (root == nullptr || !json_object_object_get_ex(root, key, &value) ||
      json_object_get_type(value) != json_type_string) {
    return "";
  }
  return json_object_get_string(value);
}

bool read_all_from_fd(int fd, std::string* payload) {
  char buffer[4096];
  payload->clear();
  ssize_t read_bytes = 0;
  for (;;) {
    read_bytes = read(fd, buffer, sizeof(buffer));
    if (read_bytes > 0) {
      payload->append(buffer, static_cast<std::size_t>(read_bytes));
      continue;
    }
    if (read_bytes < 0 && errno == EINTR) {
      continue;
    }
    break;
  }
  return read_bytes == 0;
}

bool write_all_to_fd(int fd, const std::string& payload) {
  std::size_t total_written = 0;
  while (total_written < payload.size()) {
    const ssize_t written =
        write(fd, payload.data() + total_written, payload.size() - total_written);
    if (written < 0 && errno == EINTR) {
      continue;
    }
    if (written <= 0) {
      return false;
    }
    total_written += static_cast<std::size_t>(written);
  }
  return true;
}

bool read_response_payload(const std::string& response_payload, std::string* signed_payload,
                           std::string* error) {
  json_object* root = json_tokener_parse(response_payload.c_str());
  if (root == nullptr || json_object_get_type(root) != json_type_object) {
    if (error != nullptr) {
      *error = "failed to parse signer response";
    }
    if (root != nullptr) {
      json_object_put(root);
    }
    return false;
  }

  json_object* ok_obj = nullptr;
  if (!json_object_object_get_ex(root, "ok", &ok_obj) ||
      json_object_get_type(ok_obj) != json_type_boolean) {
    if (error != nullptr) {
      *error = "signer response missing status";
    }
    json_object_put(root);
    return false;
  }

  const bool ok = json_object_get_boolean(ok_obj);
  if (!ok) {
    if (error != nullptr) {
      *error = json_string_or_empty(root, "error");
      if (error->empty()) {
        *error = "signer rejected request";
      }
    }
    json_object_put(root);
    return false;
  }

  if (signed_payload != nullptr) {
    *signed_payload = json_string_or_empty(root, "payload");
  }
  json_object_put(root);
  return signed_payload != nullptr && !signed_payload->empty();
}

}  // namespace

std::filesystem::path socket_path() {
  if (const char* env_path = std::getenv("DATACRUMBS_SIGN_PROBES_SOCKET_PATH");
      env_path != nullptr && env_path[0] != '\0') {
    return env_path;
  }
  return DATACRUMBS_SIGN_PROBES_SOCKET_PATH;
}

bool request_probe_signature(const std::string& signing_payload, std::string* checksum,
                             std::string* error) {
  const auto path = socket_path();
  const int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (client_fd < 0) {
    if (error != nullptr) {
      *error = "failed to create client socket";
    }
    return false;
  }

  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  const std::string path_string = path.string();
  if (path_string.size() >= sizeof(address.sun_path)) {
    if (error != nullptr) {
      *error = "signer socket path is too long";
    }
    close(client_fd);
    return false;
  }
  std::strncpy(address.sun_path, path_string.c_str(), sizeof(address.sun_path) - 1);

  if (connect(client_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    if (error != nullptr) {
      *error = "failed to connect to datacrumbs_sign_probes service";
    }
    close(client_fd);
    return false;
  }

  json_object* request_root = json_object_new_object();
  json_object_object_add(request_root, "signing_payload",
                         json_object_new_string(signing_payload.c_str()));
  const char* request_json = json_object_to_json_string_ext(request_root, JSON_C_TO_STRING_PLAIN);
  const std::string request_payload = request_json != nullptr ? request_json : "";
  json_object_put(request_root);

  if (!write_all_to_fd(client_fd, request_payload)) {
    if (error != nullptr) {
      *error = "failed to write signing request";
    }
    close(client_fd);
    return false;
  }
  shutdown(client_fd, SHUT_WR);

  std::string response_payload;
  const bool ok = read_all_from_fd(client_fd, &response_payload);
  close(client_fd);
  if (!ok) {
    if (error != nullptr) {
      *error = "failed to read signer response";
    }
    return false;
  }
  if (response_payload.empty()) {
    if (error != nullptr) {
      *error = "signer service closed connection without a response";
    }
    return false;
  }
  return read_response_payload(response_payload, checksum, error);
}

}  // namespace datacrumbs::probe_signing_service
