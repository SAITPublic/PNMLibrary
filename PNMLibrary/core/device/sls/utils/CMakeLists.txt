file(GLOB SLS_UTILS_SRC *.cpp)

add_library(sls_utils STATIC ${SLS_UTILS_SRC})
target_link_libraries(sls_utils PUBLIC pnm_common pnm_simulator)
