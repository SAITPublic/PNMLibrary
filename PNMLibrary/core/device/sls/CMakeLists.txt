add_subdirectory(memblockhandler)
add_subdirectory(simulator)

file(GLOB SLS_DEVICE_IMPL_SRC *.cpp)
file(GLOB SLS_DEVICE_SRC api/*.cpp)

add_library(sls_device_impl STATIC ${SLS_DEVICE_IMPL_SRC})
target_link_libraries(sls_device_impl PUBLIC sls_memblock pnm_common)
static_dependency_check(sls_device_impl)

add_library(sls_device STATIC ${SLS_DEVICE_SRC})
target_link_libraries(sls_device PUBLIC sls_device_impl)
static_dependency_check(sls_device)
