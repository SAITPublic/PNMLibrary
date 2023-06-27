file(GLOB SLS_COMPUTE_UNIT_SRC *.cpp)

add_library(sls_compute_unit STATIC ${SLS_COMPUTE_UNIT_SRC})
target_link_libraries(sls_compute_unit PUBLIC sls_device pnm_common)
