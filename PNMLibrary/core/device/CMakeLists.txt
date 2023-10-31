add_subdirectory(simulator)
add_subdirectory(sls)
add_subdirectory(imdb)

file(GLOB PNM_DEVICE_SRC *.cpp)

add_library(pnm_device STATIC ${PNM_DEVICE_SRC})
target_link_libraries(pnm_device PUBLIC sls_device imdb_device pnm_common pnm_simulator)
static_dependency_check(pnm_device)
