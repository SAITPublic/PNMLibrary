cmake_minimum_required(VERSION 3.18)

project(PNM)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option(PNM_USE_CCACHE "Attempt using CCache to wrap the compilation" OFF)
option(PNM_USE_ASAN "Build with ASan" OFF)
option(PNM_USE_MSAN "Build with MSan" OFF)
option(PNM_USE_TSAN "Build with TSan" OFF)
option(PNM_USE_UBSAN "Build with UBSan" OFF)
option(PNM_ENABLE_TESTS "Build common test apps" OFF)
option(PNM_ENABLE_BENCHMARKS "Build benchmarks" OFF)

set(AVAILABLE_EXECUTION_PLATFORMS HARDWARE FUNCSIM PERFSIM)
set(PNM_EXECUTION_PLATFORM UNDEFINED CACHE STRING "The operation execution mode")
set_property(CACHE PNM_EXECUTION_PLATFORM PROPERTY STRINGS ${AVAILABLE_EXECUTION_PLATFORMS})
if (NOT PNM_EXECUTION_PLATFORM IN_LIST AVAILABLE_EXECUTION_PLATFORMS)
  message(FATAL_ERROR "The execution mode should be one of ${AVAILABLE_EXECUTION_PLATFORMS}.\n"
      "Use -DPNM_EXECUTION_PLATFORM to specify one of them")
endif()

option(PNM_ENABLE_COMMON_CHECKS "Enable additional checks, e. g. format and header checking" OFF)
option(PNM_ENABLE_STATIC_LIB_CHECK "Enable static library dependency check" OFF)

if (PNM_ENABLE_COMMON_CHECKS)
  set(PNM_ENABLE_STATIC_LIB_CHECK ON)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSAN doesn't support allocation of more than 8GB, TSAN increases memory explicitly to x8 times
set(ENABLE_HUGE_TESTS "ON")
if (PNM_USE_MSAN OR PNM_USE_TSAN)
  set(ENABLE_HUGE_TESTS "OFF")
endif()

include(cmake/utils.cmake)

if (PNM_USE_CCACHE)
  message(STATUS "Attempt to use ccache.")
  find_program(CCACHE_PROGRAM ccache)
  if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "C compiler launcher")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "CXX compiler launcher")
  else()
    message(STATUS "Could not find ccache. Consider installing ccache to speed up compilation.")
  endif()
endif()

foreach (PLATFORM ${AVAILABLE_EXECUTION_PLATFORMS})
  list(FIND AVAILABLE_EXECUTION_PLATFORMS ${PLATFORM} PLATFORM_INDEX)
  add_compile_definitions("${PLATFORM}=${PLATFORM_INDEX}")
endforeach()

list(FIND AVAILABLE_EXECUTION_PLATFORMS ${PNM_EXECUTION_PLATFORM} PLATFORM_INDEX)
add_compile_definitions("PNM_PLATFORM=${PLATFORM_INDEX}")

set(ASAN_FLAGS "")
if (PNM_USE_ASAN)
  message(STATUS "Compile with ASAN")
  set(ASAN_FLAGS "-fsanitize=address -g -fno-common -fno-omit-frame-pointer -fsanitize-address-use-after-scope \
                  -D_GLIBCXX_SANITIZE_VECTOR")
endif()

set(UBSAN_FLAGS "")
if (PNM_USE_UBSAN)
  message(STATUS "Compile with UBSAN")
  set(UBSAN_CHECKS "undefined,float-divide-by-zero")
  set(UBSAN_EXCLUDE "")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(UBSAN_CHECKS "${UBSAN_CHECKS},integer,nullability,bounds ")
    #To ignore the third-party code, add ignorelist
    set(UBSAN_EXCLUDE "-fsanitize-blacklist=${PROJECT_SOURCE_DIR}/ignorelist_ubsan.txt")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(UBSAN_CHECKS "${UBSAN_CHECKS},float-cast-overflow")
  endif()
  set(UBSAN_FLAGS "-g -fsanitize=${UBSAN_CHECKS} -fno-sanitize-recover ${UBSAN_EXCLUDE}")
endif()

set(MSAN_FLAGS "")
if (PNM_USE_MSAN)
  message(STATUS "Compile with MSAN")

  # MSanitized libcxx location
  if ("${LIBCXX_INSTALL_DIR}" STREQUAL "")
    set(LIBCXX_INSTALL_DIR "/usr/local/libcxx-msan/")
  endif()

  set(MSAN_FLAGS "-g -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins -g -nostdinc++ \
                  -isystem ${LIBCXX_INSTALL_DIR}/include/c++/v1")
  set(CMAKE_SHARED_LINKER_FLAGS "-stdlib=libc++ -lc++abi -Wl,-rpath ${LIBCXX_INSTALL_DIR}/lib -L${LIBCXX_INSTALL_DIR}/lib \
                                 -Wno-error=unused-command-line-argument -fsanitize=memory")
  set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++ -lc++abi -Wl,-rpath ${LIBCXX_INSTALL_DIR}/lib -L${LIBCXX_INSTALL_DIR}/lib \
                                 -Wno-error=unused-command-line-argument -fsanitize=memory")
endif()

set(TSAN_FLAGS "")
if (PNM_USE_TSAN)
  message(STATUS "Compile with TSAN")

  set(TSAN_FLAGS "-fsanitize=thread -g -fno-omit-frame-pointer -pthread")
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
function(check_flags_notify TYPE FLAGS)
  set(CMAKE_REQUIRED_FLAGS ${FLAGS})
  check_c_compiler_flag("" check_c_${TYPE}_flags)
  check_cxx_compiler_flag("" check_cpp_${TYPE}_flags)
  if (NOT check_c_${TYPE}_flags OR NOT check_cpp_${TYPE}_flags)
    message(FATAL_ERROR "${TYPE} flags are not supported! Flags: ${FLAGS}")
  endif()
endfunction(check_flags_notify)

function(check_kernel_header HEADER PATH)
  find_path(RES ${PATH} NO_CACHE)
  if ("${RES}" STREQUAL "RES-NOTFOUND")
    message(FATAL_ERROR "Cannot enable ${HEADER} on system where ${HEADER} is not supported")
  else()
    message(STATUS "${HEADER} is supported, enabling ${HEADER} build path")
  endif()
endfunction(check_kernel_header)

# Find kernel release
execute_process(
  COMMAND uname -r
  OUTPUT_VARIABLE KERNEL_RELEASE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find SLS headers
check_kernel_header(SLS linux/sls_resources.h)
check_kernel_header(SLS_MEM_TOPOLOGY linux/pnm_sls_mem_topology.h)

# Find IMDB headers
check_kernel_header(IMDB linux/imdb_resources.h)

message(STATUS "Kernel release: ${KERNEL_RELEASE}")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Sanitizers compiler flags
set(SANITIZERS_FLAGS "${ASAN_FLAGS} ${MSAN_FLAGS} ${TSAN_FLAGS} ${UBSAN_FLAGS}")
# Check sanitizers flags compatibility
check_flags_notify(sanitizer ${SANITIZERS_FLAGS})

# Default compiler flags
set(DEFAULT_FLAGS "-Wall -Wextra -D_REENTRANT -fno-strict-aliasing -DLINUX \
                   -fvisibility=hidden -fPIC -fno-omit-frame-pointer -Werror")
# Check default flags compatibility
check_flags_notify(default ${DEFAULT_FLAGS})

# Final compiler flags
set(FINAL_FLAGS "${DEFAULT_FLAGS} ${SANITIZERS_FLAGS}")
# Check final flags compatibility
check_flags_notify(final ${FINAL_FLAGS})

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${FINAL_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FINAL_FLAGS}")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(sls)
add_subdirectory(secure)
add_subdirectory(imdb)

add_subdirectory(tools)

if (PNM_ENABLE_TESTS OR PNM_ENABLE_BENCHMARKS)
  set(TEST_TABLES_ROOT "" CACHE STRING "Path to test tables")
  if ("${TEST_TABLES_ROOT}" STREQUAL "")
    message(FATAL_ERROR "Сannot be build with tests or benchmarks without a path to test tables")
  else ()
    add_compile_definitions(PATH_TO_TABLES="${TEST_TABLES_ROOT}")
  endif ()

  if (PNM_ENABLE_TESTS)
    add_subdirectory(test)
  endif()

  if (PNM_ENABLE_BENCHMARKS)
    add_subdirectory(benchmarks)
  endif()
endif ()

include(cmake/install.cmake)

function(get_all_targets var)
  set(targets)
  get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
  set(${var} ${targets} PARENT_SCOPE)
endfunction()

macro(get_all_targets_recursive targets dir)
  get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
  foreach(subdir ${subdirectories})
    get_all_targets_recursive(${targets} ${subdir})
  endforeach()

  get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
  list(APPEND ${targets} ${current_targets})
endmacro()

if (PNM_ENABLE_COMMON_CHECKS)
  add_custom_target(Checker ALL "${PROJECT_SOURCE_DIR}/scripts/common_checks.sh" USES_TERMINAL)
  get_all_targets(targets)
  add_dependencies(Checker ${targets})
endif()
