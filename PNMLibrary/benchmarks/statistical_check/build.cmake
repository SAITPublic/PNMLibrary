add_executable(statistical_benchmark statistical_check/main.cpp)
target_link_libraries(
  statistical_benchmark
  PRIVATE
    ${BENCHMARKS_LIBS}
    pnm_context
    sls_device
    sls_operation
    sls_datagen
)
