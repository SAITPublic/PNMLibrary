include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG 015d1a091af6937488242b70121858bce8fd40e9 # v1.8.2
)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 03597a01ee50ed33e9dfd640b249b4be3799d395 # https://google.github.io/googletest/quickstart-cmake.html
)

# benchmark fails to build otherwise for some reason
set(BENCHMARK_ENABLE_TESTING OFF)
# don't install benchmark docs with our sources
set(BENCHMARK_ENABLE_INSTALL OFF)

# Disable installation of GTest
option(INSTALL_GTEST OFF)

FetchContent_MakeAvailable(benchmark googletest)

include(GoogleTest)
enable_testing()

# [TODO: @e-kutovoi AXDIMM-593] Separate common axdimm and imdb libraries
set(BENCHMARKS_LIBS pnm_common benchmark::benchmark)

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

foreach (subdir ${SUBDIRS})
  include("${subdir}/build.cmake")
endforeach ()
