add_subdirectory(device)
add_subdirectory(memory)
add_subdirectory(operation_runner)
add_subdirectory(context)

add_library(pnm_runner INTERFACE)
target_link_libraries(pnm_runner INTERFACE imdb_runner)

set(PNM_API_LIBS
  pnm_device
  pnm_memory
  sls_device
  sls_operation
  pnm_context
)

# `/dev/null` is a hack to compile a shared library without sources.
# The contents of the library will be specified at link step using the
# `--whole-archive` linker flag.
add_library(pnm SHARED /dev/null)

target_link_libraries(pnm PRIVATE
  # Link PNM API libraries in `--whole-archive` mode.
  -Wl,--whole-archive
  ${PNM_API_LIBS}
  -Wl,--no-whole-archive

  # Link other libraries in ordinary mode.
  fmt::fmt
  spdlog::spdlog
  rt
)
