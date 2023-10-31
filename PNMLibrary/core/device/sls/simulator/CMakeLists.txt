file(GLOB SLS_SIMULATOR_SRC *.cpp)

add_library(sls_simulator STATIC ${SLS_SIMULATOR_SRC})
target_link_libraries(sls_simulator PUBLIC pnm_common pnm_simulator sls_simulator_core)
