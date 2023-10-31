file(GLOB SLS_SIMULATOR_CORE_SRC *.cpp)

add_library(sls_simulator_core STATIC ${SLS_SIMULATOR_CORE_SRC})
target_link_libraries(sls_simulator_core PUBLIC pnm_common)
