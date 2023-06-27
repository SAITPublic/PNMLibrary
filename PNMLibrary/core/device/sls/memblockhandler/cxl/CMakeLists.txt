file(GLOB SLS_MEMBLOCK_CXL_SRC *.cpp)
add_library(sls_memblock_cxl OBJECT ${SLS_MEMBLOCK_CXL_SRC})
target_link_libraries(sls_memblock_cxl PUBLIC pnm_common sls_simulator)
