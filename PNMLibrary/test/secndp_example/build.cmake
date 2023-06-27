add_pnm_test(
  secndp_example
  GTEST_NO_MAIN
  SOURCES secndp_example/main.cpp
  LIBS sls_datagen sls_operation sls_secure_static CLI11::CLI11
)
