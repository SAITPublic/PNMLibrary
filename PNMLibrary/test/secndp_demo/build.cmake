add_pnm_test(
  secndp_demo
  GTEST_NO_MAIN
  TARGET_VAR target
  SOURCES secndp_demo/main.cpp
  LIBS sls_datagen sls_secure_static sls_operation CLI11::CLI11
)

message("\tTEST ROOT DIRECTORY " ${TEST_TABLES_ROOT})
set(NUM_INSTANCE 2)
set(TEST_EXTRA_ARGS
    "${TEST_TABLES_ROOT}/position_16_6_500000 rand_128"
    "${TEST_TABLES_ROOT}/position_16_6_500000 seq_128"
    "${TEST_TABLES_ROOT}/random_16_6_500000 rand_128"
    "${TEST_TABLES_ROOT}/random_16_6_500000 seq_128"
    # Tests with MAC
    "${TEST_TABLES_ROOT}/position_16_6_500000 rand_128 --tag"
    "${TEST_TABLES_ROOT}/position_16_6_500000 seq_128 --tag"
    "${TEST_TABLES_ROOT}/random_16_6_500000 rand_128 --tag"
    "${TEST_TABLES_ROOT}/random_16_6_500000 seq_128 --tag")

if(ENABLE_HUGE_TESTS)
    set(TEST_EXTRA_ARGS ${TEST_EXTRA_ARGS}
        "${TEST_TABLES_ROOT}/position_16_50_500000 rand_16"
        "${TEST_TABLES_ROOT}/position_16_50_500000 seq_16"
        "${TEST_TABLES_ROOT}/random_16_50_500000 rand_16"
        "${TEST_TABLES_ROOT}/random_16_50_500000 seq_16"
        # Tests with MAC
        "${TEST_TABLES_ROOT}/position_16_50_500000 rand_16 --tag"
        "${TEST_TABLES_ROOT}/position_16_50_500000 seq_16 --tag"
        "${TEST_TABLES_ROOT}/random_16_50_500000 rand_16 --tag"
         "${TEST_TABLES_ROOT}/random_16_50_500000 seq_16 --tag")
endif()


foreach (args ${TEST_EXTRA_ARGS})
  string(REPLACE " " ";" arg_list ${args})

  set(tag "")
  string(REPLACE "${TEST_TABLES_ROOT}/" "" test_prefix ${args})
  string(REPLACE " --tag" "" test_prefix ${test_prefix})
  string(REPLACE " " "_" test_prefix ${test_prefix})
  if (${args} MATCHES "--tag")
    set(tag "_tagged")
  endif ()
  gtest_discover_tests("${target}" EXTRA_ARGS ${NUM_INSTANCE} ${arg_list}
      TEST_PREFIX "${test_prefix}${tag}_"
      DISCOVERY_MODE PRE_TEST)
endforeach ()
