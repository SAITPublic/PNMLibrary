add_pnm_test(
  imdb_test_app
  GTEST_NO_MAIN
  TARGET_VAR target
  SOURCES imdb_app/main.cpp
  LIBS pnm_memory imdb_datagen CLI11::CLI11
)

gtest_discover_tests(
  "${target}"
  EXTRA_ARGS 4
  TEST_SUFFIX "__num_threads_4"
  NO_PRETTY_VALUES
)
