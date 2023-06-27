file(GLOB TABLE_MANIPULATION_TEST_SRC table_manipulation/*.cpp)
add_pnm_test(
  table_manipulation
  DISCOVER_TESTS
  SOURCES ${TABLE_MANIPULATION_TEST_SRC}
  LIBS sls_secure_static sls_datagen
)
