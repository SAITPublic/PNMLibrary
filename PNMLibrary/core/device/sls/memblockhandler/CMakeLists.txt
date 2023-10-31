add_subdirectory(axdimm)
add_subdirectory(cxl)

file(GLOB SLS_MEMBLOCK_SRC *.cpp)

add_library(sls_memblock OBJECT ${SLS_MEMBLOCK_SRC})
target_link_libraries(
  sls_memblock
  PUBLIC
    $<TARGET_OBJECTS:sls_memblock_axdimm>
    $<TARGET_OBJECTS:sls_memblock_cxl>
    $<TARGET_OBJECTS:sls_simulator_core>
    $<TARGET_OBJECTS:sls_simulator>
    pnm_common
)
