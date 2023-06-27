add_library(sls_context INTERFACE)
target_link_libraries(sls_context INTERFACE sls_memory sls_device sls_operation sls_compute_unit)

add_library(imdb_context INTERFACE)
target_link_libraries(imdb_context INTERFACE imdb_memory imdb_device imdb_operation)

file(GLOB PNM_CONTEXT_SRC api/*.cpp)
add_library(pnm_context STATIC ${PNM_CONTEXT_SRC})
target_link_libraries(
  pnm_context
  PUBLIC
    sls_context
    imdb_context
    pnm_runner
    pnm_device
)
static_dependency_check(pnm_context)
