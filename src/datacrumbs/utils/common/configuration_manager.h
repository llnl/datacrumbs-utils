#ifndef DATACRUMBS_COMMON_CONFIGURATION_MANAGER_H__
#define DATACRUMBS_COMMON_CONFIGURATION_MANAGER_H__

/**
 * @file configuration_manager.h
 * @brief Internal header for the ConfigurationManager class.
 *
 * This file defines the ConfigurationManager class, which is responsible for
 * managing, validating, and deriving configuration settings for the DataCrumbs
 * library.
 */
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/common/enumerations.h>
#include <datacrumbs/common/logging.h>

// std headers
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace datacrumbs {

/**
 * @class ConfigurationManager
 * @brief Manages configuration settings for the DataCrumbs library.
 *
 * The ConfigurationManager class handles loading, validating, and deriving
 * configuration settings based on command-line arguments. It ensures that all
 * required configurations are set and valid before the library is used.
 */
class ConfigurationManager {
 public:
  // Exact configuration file path provided by the user
  std::filesystem::path config_file_path;

  // Directory for data storage
  std::filesystem::path data_dir;

  // Directory where trace logs will be stored
  std::filesystem::path trace_log_dir;

  // List of capture probes to be used in the session
  std::vector<std::shared_ptr<CaptureProbe>> capture_probes;

  // Runtime probes loaded from a signed probe file.
  std::vector<std::shared_ptr<Probe>> runtime_probes;

  // Event IDs generated at runtime load, keyed by probe/function pair.
  std::unordered_map<std::string, uint64_t> runtime_event_ids;

  // User associated with the configuration
  std::string user;

  std::string inclusion_path;   // Path to the inclusion file
  std::string inclusion_paths;  // Colon-separated runtime inclusion paths

  std::string log_dir;  // Directory for log files

  // Derived configuration: path to the trace file
  std::filesystem::path trace_file_path;

  // Derived configuration: path to the probe file
  std::filesystem::path probe_file_path;

  // Optional override for the exact probe file output path
  std::filesystem::path explicit_probe_file_path;

  // Derived configuration: path to the probe exclusion file
  std::filesystem::path probe_exclusion_file_path;

  // Derived configuration: path to the probe invalid file
  std::filesystem::path probe_invalid_file_path;

  // Derived configuration: path to the category map file
  std::filesystem::path category_map_path;

  // Derived configuration: path to the manual probe file
  std::filesystem::path manual_probe_path;

  // Derived configuration: path to the combined system probe file
  std::filesystem::path system_probe_path;

  // Derived configuration: category map for event IDs
  std::unordered_map<uint64_t, std::pair<std::string, std::string>> category_map;

  // Derived configuration: current hostname
  std::string hostname;

  // Unique run identifier
  std::string run_id;

  // Flag to disable MPI usage
  bool disable_mpi;

  // MPI rank of the current process
  int mpi_rank{0};

  // MPI size (total number of processes)
  int mpi_size{1};

  /**
   * @brief Constructor that initializes the ConfigurationManager with
   * command-line arguments.
   *
   * Parses the command-line arguments to set up the configuration, derives
   * necessary configurations, and validates them. If any required configuration
   * is missing or invalid, logs an error and exits the program.
   *
   * @param argc Number of command-line arguments
   * @param argv Array of command-line argument strings
   */
  ConfigurationManager(int argc, char** argv, bool load_capture_probes = false, bool print = true);
  ConfigurationManager(const std::filesystem::path& runtime_probe_file, bool print = true);

  ConfigurationManager() {
    // Default constructor for internal use
  }

  // For debugging: prints all configuration values to the log
  void print_configurations();

  std::optional<uint64_t> get_runtime_event_id(const std::string& probe_name,
                                               const std::string& function_name) const;

 private:
  /**
   * @brief Derives configurations based on the provided command-line arguments.
   *
   * Sets up paths and other configurations based on the mode of operation.
   */
  void derive_configurations();

  /**
   * @brief Validates the derived configurations.
   *
   * Checks if all required configurations are set and valid. If any
   * configuration is invalid, logs an error and exits the program. This ensures
   * correct operation of the DataCrumbs library.
   */
  void validate_configurations();

  // Loads the category map from the specified JSON file
  void load_category_map();
  void load_runtime_system_configuration();
  void load_runtime_probe_file();
};

class ArgumentParser {
 public:
  std::string config_file_path;                     ///< YAML configuration file to load
  std::optional<std::string> trace_log_dir;         ///< Optional trace log directory
  std::optional<std::string> data_dir;              ///< Optional data directory
  std::optional<std::string> probe_file_path;       ///< Optional probe file path
  std::optional<std::string> user;                  ///< Optional user argument
  std::optional<uint64_t> skip_event_threshold_us;  ///< Optional skip event threshold
  std::optional<std::string> inclusion_path;        ///< Optional inclusion path
  std::optional<std::string> log_dir;               ///< Optional log directory
  std::optional<std::string> run_id;                ///< Optional run_id
  /**
   * @brief Constructor that parses command-line arguments.
   * @param argc Number of command-line arguments
   * @param argv Array of command-line argument strings
   * @throws std::invalid_argument if required arguments are missing or unknown
   * arguments are found
   */
  ArgumentParser(int argc, char** argv);
};

}  // namespace datacrumbs

#endif  // DATACRUMBS_COMMON_CONFIGURATION_MANAGER_H__
