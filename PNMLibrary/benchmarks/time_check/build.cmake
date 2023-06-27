add_executable(time_benchmark time_check/main.cpp)
target_link_libraries(
  time_benchmark
  PRIVATE
    ${BENCHMARKS_LIBS}
    pnm_context
    sls_device
    sls_operation
    sls_datagen
)
