add_pnm_test(
  secure_exec
  DISCOVER_TESTS
  SOURCES secure_exec/main.cpp
  LIBS sls_datagen sls_operation sls_secure_static
)
