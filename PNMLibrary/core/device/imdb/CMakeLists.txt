file(GLOB IMDB_DEVICE_SRC *.cpp)
add_library(imdb_device STATIC ${IMDB_DEVICE_SRC})
target_link_libraries(imdb_device PUBLIC imdb_scan_avx2 pnm_common)
static_dependency_check(imdb_device)
