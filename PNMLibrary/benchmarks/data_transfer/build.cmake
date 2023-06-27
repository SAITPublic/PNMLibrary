add_executable(data_transfer_bench data_transfer/main.cpp)
target_link_libraries(data_transfer_bench PRIVATE ${BENCHMARKS_LIBS})
target_compile_options(data_transfer_bench PRIVATE -mavx2 -mavx -msse2 -mbmi2 -march=native)
