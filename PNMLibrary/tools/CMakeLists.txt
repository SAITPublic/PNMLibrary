file(GLOB_RECURSE IMDB_DATAGEN_SRC datagen/imdb/*.cpp)

add_library(imdb_datagen ${IMDB_DATAGEN_SRC})
target_link_libraries(imdb_datagen PUBLIC pnm_common)

file(GLOB_RECURSE SLS_DATAGEN_SRC datagen/sls/*.cpp)

add_library(sls_datagen ${SLS_DATAGEN_SRC})
target_link_libraries(sls_datagen PUBLIC pnm_common)

add_executable(create_tables create_tables.cpp)
target_link_libraries(create_tables PRIVATE sls_datagen CLI11::CLI11)

add_executable(create_indices create_indices.cpp)
target_link_libraries(create_indices PRIVATE sls_datagen CLI11::CLI11)

file(GLOB PNM_CTL_SRC pnm_ctl/*.cpp)
add_executable(pnm_ctl ${PNM_CTL_SRC})
target_link_libraries(pnm_ctl PRIVATE pnm_device ndctl.so daxctl.so fmt::fmt spdlog::spdlog CLI11::CLI11 pthread rt)
