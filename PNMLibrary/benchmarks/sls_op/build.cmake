add_executable(sls_bench sls_op/main.cpp)
target_link_libraries(
  sls_bench
  PRIVATE
    ${BENCHMARKS_LIBS}
    CLI11::CLI11
    pnm_memory
    pnm_context
    sls_secure_static
    sls_datagen
)
