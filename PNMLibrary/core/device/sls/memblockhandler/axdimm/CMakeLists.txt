file(GLOB SLS_MEMBLOCK_AXDIMM_SRC *.cpp)
add_library(sls_memblock_axdimm OBJECT ${SLS_MEMBLOCK_AXDIMM_SRC})
target_link_libraries(sls_memblock_axdimm PUBLIC pnm_common sls_simulator)
