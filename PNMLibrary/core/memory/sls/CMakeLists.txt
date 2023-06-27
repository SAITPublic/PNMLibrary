add_subdirectory(axdimm)
add_subdirectory(cxl)

file(GLOB SLS_MEMORY_SRC *.cpp)
add_library(sls_memory STATIC ${SLS_MEMORY_SRC})
target_link_libraries(
  sls_memory
  PUBLIC
    sls_memory_axdimm
    sls_memory_cxl
    sls_device
)
static_dependency_check(sls_memory)
