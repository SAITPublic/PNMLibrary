file(GLOB IMDB_SIMULATOR_CORE_TEST_SRC imdb_simulator_core/*.cpp)
add_pnm_test(
  imdb_simulator_core
  ADD_TEST
  SOURCES ${IMDB_SIMULATOR_CORE_TEST_SRC}
  LIBS imdb_device imdb_datagen sls_operation
)
