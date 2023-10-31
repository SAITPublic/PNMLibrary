add_subdirectory(imdb)

file(GLOB PNM_RUNNER_SRC api/*.cpp)
add_library(pnm_runner STATIC ${PNM_RUNNER_SRC})
target_link_libraries(pnm_runner PRIVATE imdb_runner)
static_dependency_check(pnm_runner)
