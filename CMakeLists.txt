cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0075 NEW)
project(datacrumbs-utils LANGUAGES C CXX)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep the original datacrumbs target/output naming for the moved client/explorer code.
set(DATACRUMBS_PACKAGE "datacrumbs")
set(DATACRUMBS_PACKAGE_NAME "datacrumbs")
set(DATACRUMBS_VERSION_MAJOR "1")
set(DATACRUMBS_VERSION_MINOR "0")
set(DATACRUMBS_VERSION_PATCH "0")
set(DATACRUMBS_PACKAGE_VERSION
    "${DATACRUMBS_VERSION_MAJOR}.${DATACRUMBS_VERSION_MINOR}.${DATACRUMBS_VERSION_PATCH}"
)
set(DATACRUMBS_VERSION "(0, 0, 4)")
set(DATACRUMBS_RELEASE_VERSION_STRING "0.0.4")

string(TOUPPER "${DATACRUMBS_PACKAGE_NAME}" UPPER_PROJECT_NAME)
set(DATACRUMBS_LIBDIR "lib" CACHE STRING "Library output directory name")

get_filename_component(_DATACRUMBS_UTILS_PROJECT_PATH_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
get_filename_component(_DATACRUMBS_PROJECT_PATH_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/../datacrumbs" ABSOLUTE)
set(
  DATACRUMBS_UTILS_PROJECT_PATH
  "${_DATACRUMBS_UTILS_PROJECT_PATH_DEFAULT}"
  CACHE PATH "Path to the datacrumbs-utils project root"
)
set(
  DATACRUMBS_PROJECT_PATH
  "${_DATACRUMBS_PROJECT_PATH_DEFAULT}"
  CACHE PATH "Path to the datacrumbs project root"
)

list(APPEND CMAKE_MODULE_PATH "${DATACRUMBS_PROJECT_PATH}/cmake/modules")
include(datacrumbs-utils)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(yaml-cpp REQUIRED)

macro(load_datacrumbs_system_configuration)
  set(_dc_system_configuration_path "")

  if(DEFINED DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE
     AND NOT DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE STREQUAL "")
    if(EXISTS "${DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE}")
      set(_dc_system_configuration_path "${DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE}")
    else()
      message(
        FATAL_ERROR
          "[${UPPER_PROJECT_NAME}] DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE does not exist: ${DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE}"
      )
    endif()
  endif()

  if(_dc_system_configuration_path STREQUAL "")
    set(_dc_install_data_dir "${DATACRUMBS_INSTALL_DIR}/share/datacrumbs/data")
    if(DATACRUMBS_INSTALL_USER
       AND NOT DATACRUMBS_INSTALL_USER STREQUAL ""
       AND NOT DATACRUMBS_INSTALL_USER STREQUAL "NONE"
       AND DATACRUMBS_HOST
       AND NOT DATACRUMBS_HOST STREQUAL ""
       AND NOT DATACRUMBS_HOST STREQUAL "NONE")
      set(
        _dc_named_system_configuration
        "${_dc_install_data_dir}/system-probe-${DATACRUMBS_INSTALL_USER}-${DATACRUMBS_HOST}.json.gz"
      )
      if(EXISTS "${_dc_named_system_configuration}")
        set(_dc_system_configuration_path "${_dc_named_system_configuration}")
      endif()
    endif()

    if(_dc_system_configuration_path STREQUAL "")
      file(GLOB _dc_discovered_system_configurations "${_dc_install_data_dir}/system-probe-*.json.gz")
      list(LENGTH _dc_discovered_system_configurations _dc_discovered_system_configuration_count)
      if(_dc_discovered_system_configuration_count EQUAL 1)
        list(GET _dc_discovered_system_configurations 0 _dc_system_configuration_path)
      elseif(_dc_discovered_system_configuration_count EQUAL 0)
        message(
          FATAL_ERROR
            "[${UPPER_PROJECT_NAME}] No datacrumbs system configuration found under ${_dc_install_data_dir}. Set DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE or DATACRUMBS_INSTALL_DIR."
        )
      else()
        message(
          FATAL_ERROR
            "[${UPPER_PROJECT_NAME}] Multiple datacrumbs system configurations found under ${_dc_install_data_dir}. Set DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE or specify DATACRUMBS_HOST and DATACRUMBS_INSTALL_USER."
        )
      endif()
    endif()
  endif()

  set(_dc_loaded_system_configuration "${CMAKE_BINARY_DIR}/datacrumbs-system-configuration.cmake")
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/load_datacrumbs_system_configuration.py"
      --input
      "${_dc_system_configuration_path}"
      --output
      "${_dc_loaded_system_configuration}"
    RESULT_VARIABLE _dc_load_system_configuration_result
    ERROR_VARIABLE _dc_load_system_configuration_error
  )
  if(NOT _dc_load_system_configuration_result EQUAL 0)
    string(STRIP "${_dc_load_system_configuration_error}" _dc_load_system_configuration_error)
    message(
      FATAL_ERROR
        "[${UPPER_PROJECT_NAME}] Failed to load datacrumbs system configuration from ${_dc_system_configuration_path}: ${_dc_load_system_configuration_error}"
    )
  endif()

  include("${_dc_loaded_system_configuration}")
  message(
    STATUS
      "[${UPPER_PROJECT_NAME}] Loaded datacrumbs system configuration from ${_dc_system_configuration_path}"
  )
endmacro()

macro(datacrumbs_set_default variable_name default_value)
  if(NOT DEFINED ${variable_name} OR "${${variable_name}}" STREQUAL "")
    set(${variable_name} "${default_value}")
  endif()
endmacro()

option(DATACRUMBS_BUILD_ONLY "Build only the datacrumbs library without installation" OFF)
option(DATACRUMBS_BPFTIME_COMPATIBLE "Enable compatibility with bpftime" OFF)
option(DATACRUMBS_ENABLE_OPT "Enable tracing" ON)
option(DATACRUMBS_TRACE_ALL_PROCESSES_OPT "Trace all processes" OFF)
option(DATACRUMBS_BPF_PRINT_ENABLE "Enable BPF print" OFF)
option(DATACRUMBS_DISABLE_PROBE_SIGNING "Disable probe signing and signature verification" OFF)
option(DATACRUMBS_SKIP_PROBE_EXPLORING "Skip running the probe explorer helper target" OFF)
option(DATACRUMBS_SKIP_PROBE_GENERATION "Compatibility toggle retained from datacrumbs" ON)

set(DATACRUMBS_INCLUSION_PATH "NONE" CACHE STRING "Inclusion path for datacrumbs")
set(DATACRUMBS_CONFIGURED_TRACE_DIR "/tmp" CACHE STRING "Trace directory for datacrumbs")
set(DATACRUMBS_CONFIGURED_LOG_DIR "/tmp" CACHE STRING "Log directory for datacrumbs")
set(DATACRUMBS_CONFIGURED_RUN_DIR "NONE" CACHE STRING "Run directory for datacrumbs")
set(DATACRUMBS_MODE_STR "TRACE" CACHE STRING "Mode for datacrumbs (TRACE or PROFILE)")
set_property(CACHE DATACRUMBS_MODE_STR PROPERTY STRINGS TRACE PROFILE)
set(BPFTOOL_EXECUTABLE "NONE" CACHE STRING "Path to bpftool executable")
set(DATACRUMBS_HOST "NONE" CACHE STRING "Host name for datacrumbs")
set(DATACRUMBS_USER "$ENV{USER}" CACHE STRING "User name for datacrumbs")
set(DATACRUMBS_INSTALL_USER "$ENV{USER}" CACHE STRING "Installation user name for datacrumbs")
set(DATACRUMBS_TRACE_RINGBUF_SIZE_MB "16" CACHE STRING "Ring buffer size in MB for BPF tracing")
set(DATACRUMBS_MAX_RUNTIME_FUNCTIONS "16384" CACHE STRING "Maximum number of runtime functions supported in one probes file")
set(DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS
    "1000"
    CACHE STRING "Threshold in nanoseconds for skipping events"
)
set(DATACRUMBS_TIME_INTERVAL_NS "1000000" CACHE STRING "Profiling time interval in nanoseconds")
set(DATACRUMBS_SCHEDULER_TYPE "FLUX" CACHE STRING "Scheduler type for datacrumbs")
set_property(CACHE DATACRUMBS_SCHEDULER_TYPE PROPERTY STRINGS FLUX SLURM OPENMPI NONE)
set(DATACRUMBS_SCHEDULER_JOBID_ENV_VAR "NONE" CACHE STRING "Scheduler job ID environment variable name")
set(DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS "" CACHE STRING "Scheduler run extra args")
set(DATACRUMBS_LOG_LEVEL_STR "INFO" CACHE STRING "Log level for compiling datacrumbs utils")
set_property(CACHE DATACRUMBS_LOG_LEVEL_STR PROPERTY STRINGS ERROR WARN INFO DEBUG TRACE)
set(DATACRUMBS_KERNEL_VERSION "" CACHE STRING "Kernel version")
set(DATACRUMBS_KERNEL_HEADERS_PATH "" CACHE STRING "Path to the kernel headers directory")
set(DATACRUMBS_KERNEL_PATH "" CACHE STRING "Path to the kernel source directory")
set(
  DATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE
  ""
  CACHE FILEPATH "Path to a datacrumbs system configuration artifact generated in a build tree"
)
set(
  DATACRUMBS_INSTALL_DIR
  "${CMAKE_INSTALL_PREFIX}"
  CACHE PATH "Path to the datacrumbs install dir used to discover the installed system configuration"
)
set(DATACRUMBS_INSTALL_PREFIX_PATH "${DATACRUMBS_INSTALL_DIR}")

load_build_variables()
load_datacrumbs_system_configuration()
derive_configurations()
find_system_details()
include_dependencies()

datacrumbs_set_default(DATACRUMBS_INSTALL_HOST "${DATACRUMBS_HOST}")
datacrumbs_set_default(DATACRUMBS_INSTALL_CONFIGS_DIR "${DATACRUMBS_INSTALL_ETC_CONFIGSDIR}")
datacrumbs_set_default(DATACRUMBS_INSTALL_SHARED_DATA_DIR "${DATACRUMBS_INSTALL_PREFIX_PATH}/share/datacrumbs/data")
datacrumbs_set_default(DATACRUMBS_INSTALL_DATA_DIR "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}")
datacrumbs_set_default(
  DATACRUMBS_SYSTEM_PROBE_FILE
  "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}/system-probe-${DATACRUMBS_INSTALL_USER}-${DATACRUMBS_HOST}.json.gz"
)
datacrumbs_set_default(
  DATACRUMBS_PROBE_SECRET_FILE
  "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}/.datacrumbs-probe-secret"
)
datacrumbs_set_default(
  DATACRUMBS_SIGN_PROBES_SOCKET_PATH
  "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}/datacrumbs-sign-probes.sock"
)
datacrumbs_set_default(DATACRUMBS_TRACE_DIR_PATTERN "${DATACRUMBS_CONFIGURED_TRACE_DIR}/%YY%/%MM%/%DD%")
datacrumbs_set_default(DATACRUMBS_CLIENT_LIB "${DATACRUMBS_INSTALL_LIB_DIR}/libdatacrumbs_client.so")
datacrumbs_set_default(DATACRUMBS_JOB_SCHEDULER "${DATACRUMBS_SCHEDULER_TYPE}")
datacrumbs_set_default(DATACRUMBS_JOB_OTHER_FLAGS "${DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS}")
datacrumbs_set_default(DATACRUMBS_JOB_ID_VAR "${DATACRUMBS_SCHEDULER_JOBID_ENV_VAR}")
if(DATACRUMBS_ENABLE_INCLUSION_PATH)
  datacrumbs_set_default(DATACRUMBS_INCLUSION_PATHS "${DATACRUMBS_INCLUSION_PATH}")
else()
  datacrumbs_set_default(DATACRUMBS_INCLUSION_PATHS "")
endif()
datacrumbs_set_default(DATACRUMBS_SERVER_RUN_DIR "${DATACRUMBS_INSTALL_RUNSTATEDIR}/datacrumbs")
datacrumbs_set_default(DATACRUMBS_SERVER_RUN_ID_FILE "${DATACRUMBS_SERVER_RUN_DIR}/datacrumbs.runid")
datacrumbs_set_default(DATACRUMBS_INSTALL_PREFIX "${DATACRUMBS_INSTALL_PREFIX_PATH}")
datacrumbs_set_default(DATACRUMBS_INSTALL_BIN_DIR "${DATACRUMBS_INSTALL_BINARYDIR}")
datacrumbs_set_default(DATACRUMBS_INSTALL_SBIN_DIR "${DATACRUMBS_INSTALL_SBINARYDIR}")
datacrumbs_set_default(DATACRUMBS_INSTALL_LIBEXEC_DIR "${DATACRUMBS_INSTALL_LIBEXEC}")
datacrumbs_set_default(DATACRUMBS_INSTALL_ETC_DIR "${DATACRUMBS_INSTALL_SYSCONFDIR}")
datacrumbs_set_default(DATACRUMBS_INSTALL_MODULES_DIR "${DATACRUMBS_INSTALL_ETC_MODULESDIR}")
datacrumbs_set_default(DATACRUMBS_INSTALL_PROBE_OBJECTS_DIR "${DATACRUMBS_INSTALL_LIBEXEC}/objects")

set(DATACRUMBS_SYSTEM_CONFIGURATION_DEFINE_KEYS
    DATACRUMBS_INSTALL_PREFIX
    DATACRUMBS_INSTALL_HOST
    DATACRUMBS_INSTALL_USER
    DATACRUMBS_INSTALL_BIN_DIR
    DATACRUMBS_INSTALL_SBIN_DIR
    DATACRUMBS_INSTALL_LIB_DIR
    DATACRUMBS_INSTALL_LIBEXEC_DIR
    DATACRUMBS_INSTALL_ETC_DIR
    DATACRUMBS_INSTALL_DATA_DIR
    DATACRUMBS_INSTALL_MODULES_DIR
    DATACRUMBS_INSTALL_PROBE_OBJECTS_DIR
    DATACRUMBS_CLIENT_LIB
    DATACRUMBS_JOB_SCHEDULER
    DATACRUMBS_JOB_OTHER_FLAGS
    DATACRUMBS_JOB_ID_VAR
    DATACRUMBS_SERVER_RUN_DIR
    DATACRUMBS_CONFIGURED_TRACE_DIR
    DATACRUMBS_USER
    DATACRUMBS_INCLUSION_PATHS
    DATACRUMBS_KERNEL_UNAME_R
    DATACRUMBS_KERNEL_HEADERS_PATH
    DATACRUMBS_LIBC_SO
)

set(DATACRUMBS_SYSTEM_CONFIGURATION_DEFINES "")
foreach(_dc_key IN LISTS DATACRUMBS_SYSTEM_CONFIGURATION_DEFINE_KEYS)
  if(NOT DEFINED ${_dc_key})
    continue()
  endif()
  set(_dc_value "${${_dc_key}}")
  if("${_dc_value}" STREQUAL "")
    continue()
  endif()
  if(_dc_value MATCHES "^[+-]?[0-9]+$")
    string(APPEND DATACRUMBS_SYSTEM_CONFIGURATION_DEFINES "#define ${_dc_key} ${_dc_value}\n")
  else()
    string(REPLACE "\\" "\\\\" _dc_escaped "${_dc_value}")
    string(REPLACE "\"" "\\\"" _dc_escaped "${_dc_escaped}")
    string(APPEND DATACRUMBS_SYSTEM_CONFIGURATION_DEFINES "#define ${_dc_key} \"${_dc_escaped}\"\n")
  endif()
endforeach()

set(DATACRUMBS_CONFIG_TEMPLATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/configure_files/datacrumbs_config.h.in"
)
set(DATACRUMBS_LEGACY_SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DATACRUMBS_STAGED_INCLUDE_ROOT "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs")
set(DATACRUMBS_BUILD_SHARE_DIR "${CMAKE_BINARY_DIR}/share/datacrumbs")
set(DATACRUMBS_BUILD_DATA_DIR "${DATACRUMBS_BUILD_SHARE_DIR}/data")
datacrumbs_set_default(DATACRUMBS_INSTALL_SHARE_DIR "${DATACRUMBS_INSTALL_PREFIX_PATH}/share/datacrumbs")
datacrumbs_set_default(DATACRUMBS_INSTALL_DATA_SHARE_DIR "${DATACRUMBS_INSTALL_SHARE_DIR}/data")
datacrumbs_set_default(DATACRUMBS_INSTALL_CONFIGS_DIR "${DATACRUMBS_INSTALL_PREFIX_PATH}/etc/datacrumbs/configs")
datacrumbs_set_default(
  DATACRUMBS_INSTALL_ETC_MODULESDIR
  "${DATACRUMBS_INSTALL_PREFIX_PATH}/${CMAKE_INSTALL_SYSCONFDIR}/${DATACRUMBS_PACKAGE_NAME}/${CMAKE_INSTALL_MODULES_DIR}"
)
set(DATACRUMBS_BUILD_PROJECT_ENV_FILE "${CMAKE_CONFIG_OUTPUT_DIRECTORY}/project.env.local")
set(DATACRUMBS_INSTALL_PROJECT_ENV_FILE "${DATACRUMBS_INSTALL_CONFIGS_DIR}/project.env.local")
set(DATACRUMBS_BUILD_MODULEFILE
    "${CMAKE_MODULES_OUTPUT_DIRECTORY}/${DATACRUMBS_PACKAGE_NAME}/${DATACRUMBS_RELEASE_VERSION_STRING}.lua"
)
set(DATACRUMBS_BUILD_PROBE_WRAPPER "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_probe_configurator")
set(DATACRUMBS_BUILD_TRACK_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_track")
set(DATACRUMBS_BUILD_UNTRACK_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_untrack")
set(DATACRUMBS_BUILD_WRAP_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_wrap")
set(DATACRUMBS_BUILD_SALLOC_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_salloc")
set(DATACRUMBS_BUILD_SBATCH_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_sbatch")
set(DATACRUMBS_BUILD_SETUP_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_setup")
set(DATACRUMBS_BUILD_UTILITY_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_utility")
set(DATACRUMBS_BUILD_RUN_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_run")
set(DATACRUMBS_BUILD_STOP_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_stop")
set(DATACRUMBS_BUILD_SERVICE_WRAPPER_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_service_wrapper")
set(DATACRUMBS_INSTALL_PROBE_WRAPPER "${CMAKE_BINARY_DIR}/datacrumbs_probe_configurator.install")
set(DATACRUMBS_BUILD_ARGS_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_args")
set(DATACRUMBS_BUILD_LOGGING_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_logging")
set(DATACRUMBS_INSTALL_ARGS_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_args.install")
set(DATACRUMBS_INSTALL_LOGGING_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_logging.install")
set(DATACRUMBS_INSTALL_TRACK_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_track.install")
set(DATACRUMBS_INSTALL_UNTRACK_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_untrack.install")
set(DATACRUMBS_INSTALL_WRAP_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_wrap.install")
set(DATACRUMBS_INSTALL_SALLOC_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_salloc.install")
set(DATACRUMBS_INSTALL_SBATCH_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_sbatch.install")
set(DATACRUMBS_INSTALL_SETUP_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_setup.install")
set(DATACRUMBS_INSTALL_UTILITY_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_utility.install")
set(DATACRUMBS_INSTALL_RUN_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_run.install")
set(DATACRUMBS_INSTALL_STOP_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_stop.install")
set(DATACRUMBS_INSTALL_SERVICE_WRAPPER_SCRIPT "${CMAKE_BINARY_DIR}/datacrumbs_service_wrapper.install")
set(DATACRUMBS_DD_PRELOAD_TEST_SCRIPT "${CMAKE_BINARY_DIR}/dd_preload_test.sh")

file(MAKE_DIRECTORY "${DATACRUMBS_BUILD_DATA_DIR}")

function(stage_datacrumbs_header source_path relative_dest)
  get_filename_component(dest_dir "${DATACRUMBS_STAGED_INCLUDE_ROOT}/${relative_dest}" DIRECTORY)
  file(MAKE_DIRECTORY "${dest_dir}")
  configure_file("${source_path}" "${DATACRUMBS_STAGED_INCLUDE_ROOT}/${relative_dest}" COPYONLY)
endfunction()

configure_file(
  "${DATACRUMBS_CONFIG_TEMPLATE}"
  "${DATACRUMBS_STAGED_INCLUDE_ROOT}/datacrumbs_config.h"
  @ONLY
)
set(DATACRUMBS_MOVED_HEADERS
    client/library.h
    common/configuration_manager.h
    common/logging.h
    common/probe_file.h
    common/singleton.h
    explorer/probe_explorer.h
    explorer/mechanism/elf_capture.h
    explorer/mechanism/header_capture.h
    explorer/mechanism/ksym_capture.h
    explorer/mechanism/usdt_functions.h
)

foreach(header IN LISTS DATACRUMBS_MOVED_HEADERS)
  stage_datacrumbs_header("${CMAKE_CURRENT_SOURCE_DIR}/src/${header}" "${header}")
endforeach()

set(DATACRUMBS_SHARED_HEADERS
    common/constants.h
    common/data_structures.h
    common/enumerations.h
    common/typedefs.h
    common/utils.h
)

foreach(header IN LISTS DATACRUMBS_SHARED_HEADERS)
  stage_datacrumbs_header("${DATACRUMBS_LEGACY_SRC_ROOT}/${header}" "${header}")
endforeach()

include_directories("${CMAKE_INCLUDE_OUTPUT_DIRECTORY}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/configs/project.env.local.in"
  "${DATACRUMBS_BUILD_PROJECT_ENV_FILE}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/modulefiles/module.lua.in"
  "${DATACRUMBS_BUILD_MODULEFILE}"
  @ONLY
)

message(STATUS "[${UPPER_PROJECT_NAME}] Configured datacrumbs-utils")
message(STATUS "             - DATACRUMBS_HOST: ${DATACRUMBS_HOST}")
message(STATUS "             - DATACRUMBS_USER: ${DATACRUMBS_USER}")
message(STATUS "             - DATACRUMBS_INSTALL_USER: ${DATACRUMBS_INSTALL_USER}")
message(STATUS "             - DATACRUMBS_LOG_LEVEL: ${DATACRUMBS_LOG_LEVEL}")
message(STATUS "             - DATACRUMBS_MODE: ${DATACRUMBS_MODE}")
message(STATUS "             - DATACRUMBS_ENABLE: ${DATACRUMBS_ENABLE}")
message(STATUS "             - DATACRUMBS_TRACE_ALL_PROCESSES: ${DATACRUMBS_TRACE_ALL_PROCESSES}")
message(STATUS "             - DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS: ${DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS}")
message(STATUS "             - DATACRUMBS_MAX_RUNTIME_FUNCTIONS: ${DATACRUMBS_MAX_RUNTIME_FUNCTIONS}")
message(STATUS "             - DATACRUMBS_TRACE_RINGBUF_SIZE_MB: ${DATACRUMBS_TRACE_RINGBUF_SIZE_MB}")
message(STATUS "             - DATACRUMBS_TIME_INTERVAL_NS: ${DATACRUMBS_TIME_INTERVAL_NS}")
message(STATUS "             - DATACRUMBS_BPF_PRINT_ENABLE_FLAG: ${DATACRUMBS_BPF_PRINT_ENABLE_FLAG}")
message(STATUS "             - DATACRUMBS_BPFTIME_COMPATIBLE_FLAG: ${DATACRUMBS_BPFTIME_COMPATIBLE_FLAG}")
message(STATUS "             - DATACRUMBS_ENABLE_INCLUSION_PATH: ${DATACRUMBS_ENABLE_INCLUSION_PATH}")
message(STATUS "             - DATACRUMBS_INCLUSION_PATH: ${DATACRUMBS_INCLUSION_PATH}")
message(STATUS "             - DATACRUMBS_KERNEL_UNAME_R: ${DATACRUMBS_KERNEL_UNAME_R}")
message(STATUS "             - DATACRUMBS_KERNEL_HEADERS_PATH: ${DATACRUMBS_KERNEL_HEADERS_PATH}")
message(STATUS "             - DATACRUMBS_LIBC_SO: ${DATACRUMBS_LIBC_SO}")
message(STATUS "             - DATACRUMBS_CONFIGURED_TRACE_DIR: ${DATACRUMBS_CONFIGURED_TRACE_DIR}")
message(STATUS "             - DATACRUMBS_CONFIGURED_LOG_DIR: ${DATACRUMBS_CONFIGURED_LOG_DIR}")
message(STATUS "             - DATACRUMBS_SCHEDULER_TYPE: ${DATACRUMBS_SCHEDULER_TYPE}")
message(STATUS "             - DATACRUMBS_SCHEDULER_JOBID_ENV_VAR: ${DATACRUMBS_SCHEDULER_JOBID_ENV_VAR}")
message(STATUS "             - DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS: ${DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS}")
message(STATUS "             - Build project env: ${DATACRUMBS_BUILD_PROJECT_ENV_FILE}")
message(STATUS "             - Build data dir: ${DATACRUMBS_BUILD_DATA_DIR}")
message(STATUS "             - Install configs dir: ${DATACRUMBS_INSTALL_CONFIGS_DIR}")
message(STATUS "             - Install data dir: ${DATACRUMBS_INSTALL_DATA_SHARE_DIR}")
message(STATUS "             - DATACRUMBS_UTILS_PROJECT_PATH: ${DATACRUMBS_UTILS_PROJECT_PATH}")
message(STATUS "             - DATACRUMBS_PROJECT_PATH: ${DATACRUMBS_PROJECT_PATH}")
message(STATUS "             - Client library path: ${DATACRUMBS_BUILD_CLIENT_SO}")
install(FILES "${DATACRUMBS_BUILD_PROJECT_ENV_FILE}" DESTINATION "${DATACRUMBS_INSTALL_CONFIGS_DIR}")
install(
  FILES "${DATACRUMBS_BUILD_MODULEFILE}"
  DESTINATION "${DATACRUMBS_INSTALL_ETC_MODULESDIR}/${DATACRUMBS_PACKAGE_NAME}"
)

set(DATACRUMBS_COMMON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/configuration_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/configuration_manager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/probe_signing_service.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/probe_signing_service.h"
)

set(DATACRUMBS_CLIENT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/client/library.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/client/library.h"
)
add_library(datacrumbs_client SHARED ${DATACRUMBS_CLIENT_SRC})
set(DATACRUMBS_BUILD_CLIENT_SO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdatacrumbs_client.so")
set_target_properties(
  datacrumbs_client
  PROPERTIES
    OUTPUT_NAME datacrumbs_client
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
)
install(TARGETS datacrumbs_client LIBRARY DESTINATION "${DATACRUMBS_INSTALL_LIB_DIR}")

add_custom_target(
  clean_all
  COMMAND "${CMAKE_COMMAND}" -E echo "Dummy clean_all target for datacrumbs-utils"
  COMMENT "No generated BPF artifacts to clean in datacrumbs-utils"
)

set(DATACRUMBS_PROBE_CONFIGURATOR_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/probe_configurator_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/probe_explorer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/probe_explorer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/mechanism/elf_capture.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/mechanism/elf_capture.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/mechanism/header_capture.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/mechanism/header_capture.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/mechanism/ksym_capture.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/mechanism/ksym_capture.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/explorer/mechanism/usdt_functions.h"
)
add_executable(datacrumbs_probe_configurator ${DATACRUMBS_PROBE_CONFIGURATOR_SRC} ${DATACRUMBS_COMMON_SRC})
set_target_properties(datacrumbs_probe_configurator PROPERTIES OUTPUT_NAME datacrumbs_probe_configurator_exec)
target_link_libraries(datacrumbs_probe_configurator ${DEPENDENCY_LIB} yaml-cpp OpenSSL::Crypto)
add_dependencies(datacrumbs_probe_configurator datacrumbs_client clean_all)
install(TARGETS datacrumbs_probe_configurator RUNTIME DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}")


set(DATACRUMBS_PROBE_CONFIGURATOR_BIN "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_probe_configurator_exec")
set(DATACRUMBS_PROBE_CONFIGURATOR_INSTALL_BIN
    "${DATACRUMBS_INSTALL_BINARYDIR}/datacrumbs_probe_configurator_exec"
)

set(_DATACRUMBS_ORIG_INSTALL_BINARYDIR "${DATACRUMBS_INSTALL_BINARYDIR}")
set(DATACRUMBS_INSTALL_BINARYDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_logging.in"
  "${DATACRUMBS_BUILD_LOGGING_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_args.in"
  "${DATACRUMBS_BUILD_ARGS_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/probe_configurator.in"
  "${DATACRUMBS_BUILD_PROBE_WRAPPER}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_track.in"
  "${DATACRUMBS_BUILD_TRACK_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_untrack.in"
  "${DATACRUMBS_BUILD_UNTRACK_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_wrap.in"
  "${DATACRUMBS_BUILD_WRAP_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_salloc.in"
  "${DATACRUMBS_BUILD_SALLOC_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_sbatch.in"
  "${DATACRUMBS_BUILD_SBATCH_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_utility.in"
  "${DATACRUMBS_BUILD_UTILITY_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_setup.in"
  "${DATACRUMBS_BUILD_SETUP_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_run.in"
  "${DATACRUMBS_BUILD_RUN_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_stop.in"
  "${DATACRUMBS_BUILD_STOP_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_service_wrapper.in"
  "${DATACRUMBS_BUILD_SERVICE_WRAPPER_SCRIPT}"
  @ONLY
)
set(DATACRUMBS_INSTALL_BINARYDIR "${_DATACRUMBS_ORIG_INSTALL_BINARYDIR}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_logging.in"
  "${DATACRUMBS_INSTALL_LOGGING_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_args.in"
  "${DATACRUMBS_INSTALL_ARGS_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/probe_configurator_install.in"
  "${DATACRUMBS_INSTALL_PROBE_WRAPPER}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_track.in"
  "${DATACRUMBS_INSTALL_TRACK_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_untrack.in"
  "${DATACRUMBS_INSTALL_UNTRACK_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_wrap.in"
  "${DATACRUMBS_INSTALL_WRAP_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_salloc.in"
  "${DATACRUMBS_INSTALL_SALLOC_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_sbatch.in"
  "${DATACRUMBS_INSTALL_SBATCH_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_utility.in"
  "${DATACRUMBS_INSTALL_UTILITY_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_setup.in"
  "${DATACRUMBS_INSTALL_SETUP_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_run.in"
  "${DATACRUMBS_INSTALL_RUN_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_stop.in"
  "${DATACRUMBS_INSTALL_STOP_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_service_wrapper.in"
  "${DATACRUMBS_INSTALL_SERVICE_WRAPPER_SCRIPT}"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/dd_preload_test.sh.in"
  "${DATACRUMBS_DD_PRELOAD_TEST_SCRIPT}"
  @ONLY
)
file(CHMOD "${DATACRUMBS_BUILD_ARGS_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_LOGGING_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_PROBE_WRAPPER}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_TRACK_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_UNTRACK_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_WRAP_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_SALLOC_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_SBATCH_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_UTILITY_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_SETUP_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_RUN_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_STOP_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_BUILD_SERVICE_WRAPPER_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_ARGS_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_LOGGING_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_PROBE_WRAPPER}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_TRACK_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_UNTRACK_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_WRAP_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_SALLOC_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_SBATCH_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_UTILITY_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_SETUP_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_RUN_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_STOP_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_INSTALL_SERVICE_WRAPPER_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${DATACRUMBS_DD_PRELOAD_TEST_SCRIPT}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(PROGRAMS "${DATACRUMBS_INSTALL_ARGS_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_args")
install(PROGRAMS "${DATACRUMBS_INSTALL_LOGGING_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_logging")
install(PROGRAMS "${DATACRUMBS_INSTALL_PROBE_WRAPPER}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_probe_configurator")
install(PROGRAMS "${DATACRUMBS_INSTALL_TRACK_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_track")
install(PROGRAMS "${DATACRUMBS_INSTALL_UNTRACK_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_untrack")
install(PROGRAMS "${DATACRUMBS_INSTALL_WRAP_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_wrap")
install(PROGRAMS "${DATACRUMBS_INSTALL_SALLOC_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_salloc")
install(PROGRAMS "${DATACRUMBS_INSTALL_SBATCH_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_sbatch")
install(PROGRAMS "${DATACRUMBS_INSTALL_UTILITY_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_utility")
install(PROGRAMS "${DATACRUMBS_INSTALL_SETUP_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_setup")
install(PROGRAMS "${DATACRUMBS_INSTALL_RUN_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_run")
install(PROGRAMS "${DATACRUMBS_INSTALL_STOP_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_stop")
install(PROGRAMS "${DATACRUMBS_INSTALL_SERVICE_WRAPPER_SCRIPT}" DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
        RENAME "datacrumbs_service_wrapper")

if(BUILD_TESTING)
  add_test(
    NAME datacrumbs_utils_dd_preload
    COMMAND "${DATACRUMBS_DD_PRELOAD_TEST_SCRIPT}"
  )
  set_tests_properties(datacrumbs_utils_dd_preload PROPERTIES SKIP_RETURN_CODE 77)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/CMakeLists.txt")
  add_subdirectory(tools)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
