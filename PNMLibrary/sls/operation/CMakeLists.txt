file(GLOB SLS_OPERATION_SRC api/*.cpp)
file(GLOB SLS_OPERATION_IMPL_SRC *.cpp)

add_library(sls_operation_impl STATIC ${SLS_OPERATION_IMPL_SRC})
target_link_libraries(sls_operation_impl PUBLIC sls_device pnm_common)
static_dependency_check(sls_operation_impl)

add_library(sls_operation STATIC ${SLS_OPERATION_SRC})
target_link_libraries(
  sls_operation
  PUBLIC
    sls_operation_impl
        sls_device
    pnm_memory
)
static_dependency_check(sls_operation)
