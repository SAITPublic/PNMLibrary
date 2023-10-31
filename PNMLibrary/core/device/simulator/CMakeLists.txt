file(GLOB PNM_SIMULATOR_SRC *.cpp)

add_library(pnm_simulator STATIC ${PNM_SIMULATOR_SRC})
target_link_libraries(pnm_simulator PUBLIC pnm_common)
static_dependency_check(pnm_simulator)
