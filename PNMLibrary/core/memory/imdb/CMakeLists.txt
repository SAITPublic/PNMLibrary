file(GLOB IMDB_MEMORY_SRC *.cpp)
add_library(imdb_memory STATIC ${IMDB_MEMORY_SRC})
target_link_libraries(imdb_memory PUBLIC pnm_common)
static_dependency_check(imdb_memory)
