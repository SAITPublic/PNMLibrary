add_subdirectory(simulator_core)
add_subdirectory(simulator)
add_subdirectory(memblockhandler)
add_subdirectory(utils)

file(GLOB SLS_DEVICE_IMPL_SRC *.cpp)

add_library(sls_device STATIC ${SLS_DEVICE_IMPL_SRC})
target_link_libraries(sls_device PUBLIC sls_memblock pnm_common sls_simulator sls_utils)
static_dependency_check(sls_device)
