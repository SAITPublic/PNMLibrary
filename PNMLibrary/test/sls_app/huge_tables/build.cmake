if (ENABLE_HUGE_TESTS)
  add_pnm_test(
    huge_tables
    ADD_TEST
    SOURCES sls_app/huge_tables/main.cpp
    LIBS sls_operation sls_datagen pnm_context_test_mock
  )
endif ()
