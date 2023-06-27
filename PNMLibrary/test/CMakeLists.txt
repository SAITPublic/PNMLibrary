include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG 03597a01ee50ed33e9dfd640b249b4be3799d395 # https://google.github.io/googletest/quickstart-cmake.html
)

# Disable installation of GTest
option(INSTALL_GTEST OFF)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)
enable_testing()

# [TODO: @e-kutovoi AXDIMM-593] Separate common axdimm and imdb libraries
set(COMMON_PNM_LIBS pnm_context pnm_memory pnm_common)
set(COMMON_DATAGEN_LIBS imdb_datagen sls_datagen)

# Libraries linked to all test targets.
set(COMMON_LIBS ${COMMON_PNM_LIBS})

# Libraries linked to auto-generated test targets, in addition to COMMON_LIBS.
set(EXTRA_AUTO_LIBS ${COMMON_DATAGEN_LIBS} sls_operation)

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

# Helper function for test definition.
#
# Adds the executable target with given sources, sets correct executable file name,
# links common test libraries (and additional user-specified libraries as well) to it,
# and optionally calls `add_test` or `gtest_discover_tests`. By default, it links
# `gtest_main` to the test executable. This could be changed using the `GTEST_NO_MAIN` flag.
#
# Usage:
#
#     add_pnm_test(
#       <test_name>
#       [ADD_TEST] [DISCOVER_TESTS] [GTEST_NO_MAIN]
#       [TARGET_VAR <var_name>]
#       SOURCES [<src1> [<src2> ...]]
#       [LIBS [<lib1> [<lib2> ...]]]
#     )
#
# Parameters and flags:
#     <test_name>:                The name of the test. Same as the output executable file name.
#                                 Always required.
#
#     SOURCES <src1> <src2> ...:  Defines the list of sources for the test executable. Always
#                                 required.
#
#     LIBS <lib1> <lib2> ...:     Link the specified libraries to the test target in addition to
#                                 common test libraries. Optional.
#
#     TARGET_VAR <var_name>:      Set the variable with the specified name to the actual name of
#                                 the test executable target (currently, "${test_name}_test").
#                                 Useful if this function is not flexible enough and additional
#                                 manipulations on the test target are required. Optional.
#
#     ADD_TEST:                   Call `add_test(NAME "${test_name}" COMMAND "${test_target_name}")`.
#
#     DISCOVER_TESTS:             Call `gtest_discover_tests("${test_target_name}")`.
#
#     GTEST_NO_MAIN:              Link `gtest` instead of `gtest_main`.
#
function(add_pnm_test test_name)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "ADD_TEST;DISCOVER_TESTS;GTEST_NO_MAIN" "TARGET_VAR" "SOURCES;LIBS")
  
  # Save the test executable target name to a variable, if requested.
  set(target_name "${test_name}_test")
  if (DEFINED ARG_TARGET_VAR)
    set("${ARG_TARGET_VAR}" "${target_name}" PARENT_SCOPE)
  endif ()

  # Add test executable target.
  if (NOT DEFINED ARG_SOURCES)
    message(FATAL_ERROR "No SOURCES specified for add_pnm_test()")
  endif ()

  add_executable("${target_name}" ${ARG_SOURCES})
  set_target_properties("${target_name}" PROPERTIES OUTPUT_NAME "${test_name}")

  # Link requested libraries to the test executable.
  set(libraries ${COMMON_LIBS})
  if (DEFINED ARG_LIBS)
    list(APPEND libraries ${ARG_LIBS})
  endif ()

  if (ARG_GTEST_NO_MAIN)
    list(APPEND libraries gtest)
  else ()
    list(APPEND libraries gtest_main)
  endif ()

  target_link_libraries("${target_name}" PRIVATE ${libraries})

  # Add/discover tests.
  if (ARG_ADD_TEST)
    add_test(NAME "${test_name}" COMMAND "${target_name}")
  endif ()

  if (ARG_DISCOVER_TESTS)
    gtest_discover_tests("${target_name}")
  endif ()
endfunction()

function(try_add_target subdir_path)
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir_path}/build.cmake)
    message("Add manual test target for: " ${subdir_path})
    include("${subdir_path}/build.cmake")
  else ()
    message("Add test target for: " ${subdir_path})
    file(GLOB_RECURSE SRC_FILES ${subdir_path}/*.cpp)
    if (SRC_FILES)
      add_pnm_test(
        "${subdir_path}"
        DISCOVER_TESTS
        SOURCES ${SRC_FILES}
        LIBS ${EXTRA_AUTO_LIBS}
      )
    endif ()
  endif ()
endfunction()

foreach (subdir ${SUBDIRS})
  try_add_target(${subdir})
endforeach ()
