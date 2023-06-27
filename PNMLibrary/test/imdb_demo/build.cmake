add_pnm_test(
  imdb_demo
  GTEST_NO_MAIN
  TARGET_VAR target
  SOURCES imdb_demo/main.cpp
  LIBS pnm_memory imdb_datagen CLI11::CLI11
)
target_compile_options("${target}" PRIVATE -Wcast-align -mavx2 -mavx -fno-strict-aliasing)
