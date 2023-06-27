file(GLOB_RECURSE IMDB_OPERATION_SRC *.cpp)
add_library(imdb_operation STATIC ${IMDB_OPERATION_SRC})
target_link_libraries(imdb_operation PUBLIC pnm_common)
static_dependency_check(imdb_operation)
