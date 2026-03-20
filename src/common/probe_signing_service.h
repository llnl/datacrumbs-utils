#ifndef DATACRUMBS_COMMON_PROBE_SIGNING_SERVICE_H__
#define DATACRUMBS_COMMON_PROBE_SIGNING_SERVICE_H__

#include <filesystem>
#include <string>

namespace datacrumbs::probe_signing_service {

std::filesystem::path socket_path();

bool request_probe_signature(const std::string& signing_payload, std::string* checksum,
                             std::string* error = nullptr);

}  // namespace datacrumbs::probe_signing_service

#endif  // DATACRUMBS_COMMON_PROBE_SIGNING_SERVICE_H__
