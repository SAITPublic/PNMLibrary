file(GLOB IMDB_RUNNER_SRC *.cpp)
add_library(imdb_runner STATIC ${IMDB_RUNNER_SRC})
target_link_libraries(imdb_runner PUBLIC imdb_device imdb_operation)
static_dependency_check(imdb_runner)
