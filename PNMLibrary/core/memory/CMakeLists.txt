add_subdirectory(imdb)
add_subdirectory(sls)

file(GLOB PNM_MEMORY_IMPL_SRC *.cpp)
file(GLOB PNM_MEMORY_SRC api/*.cpp)

add_library(pnm_memory_impl STATIC ${PNM_MEMORY_IMPL_SRC})
add_library(pnm_memory STATIC ${PNM_MEMORY_SRC})

target_link_libraries(
  pnm_memory_impl
  PUBLIC
    sls_memory
    imdb_memory
    sls_device
    imdb_device
)
static_dependency_check(pnm_memory_impl)

target_link_libraries(pnm_memory PUBLIC pnm_memory_impl)
static_dependency_check(pnm_memory)
