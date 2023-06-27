add_executable(bitvector bit_vector/main.cpp)
target_link_libraries(bitvector PRIVATE ${BENCHMARKS_LIBS})
