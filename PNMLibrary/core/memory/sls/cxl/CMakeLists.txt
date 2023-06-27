file(GLOB SLS_MEMORY_CXL_SRC *.cpp)
add_library(sls_memory_cxl STATIC ${SLS_MEMORY_CXL_SRC})
target_link_libraries(sls_memory_cxl PUBLIC pnm_common sls_device)
static_dependency_check(sls_memory_cxl)
