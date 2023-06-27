file(GLOB SLS_MEMORY_AXDIMM_SRC *.cpp)
add_library(sls_memory_axdimm STATIC ${SLS_MEMORY_AXDIMM_SRC})
target_link_libraries(sls_memory_axdimm PUBLIC
    pnm_common
    sls_device
    )
static_dependency_check(sls_memory_axdimm)
