add_executable(imdb_allocator_bench allocator/imdb.cpp)
target_link_libraries(
  imdb_allocator_bench
  PRIVATE
    pnm_context
    ${BENCHMARKS_LIBS}
)
add_executable(ranked_allocator_bench allocator/ranked.cpp)
target_link_libraries(
  ranked_allocator_bench
  PRIVATE
    pnm_context
    ${BENCHMARKS_LIBS}
)
