add_executable(scan_bench imdb_scan/cpu.cpp imdb_scan/imdb.cpp)
target_link_libraries(
  scan_bench
  PRIVATE
    ${BENCHMARKS_LIBS}
    pnm_memory
    pnm_context
    imdb_datagen
    imdb_scan_avx2
    imdb_device
)
