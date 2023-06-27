add_pnm_test(
  sample_app
  TARGET_VAR target
  SOURCES sls_app/sample_app/main.cpp
  LIBS
    sls_operation
    sls_datagen
    pnm_context_test_mock
    CLI11::CLI11
)

gtest_discover_tests(
  "${target}"
  EXTRA_ARGS 10
  TEST_SUFFIX "__num_engines_10"
  NO_PRETTY_VALUES
)
