file(GLOB PNM_DEVICE_TEST_SRC device/*.cpp)
add_pnm_test(pnm_device TARGET_VAR target SOURCES ${PNM_DEVICE_TEST_SRC})

gtest_discover_tests("${target}"
	             TEST_FILTER -PnmProcessManager*:*Leaked*:*EnableCleanup*
		     PROPERTIES LABELS pnm_device)
gtest_discover_tests("${target}"
	             TEST_FILTER PnmProcessManager*SLS*:*Leaked*SLS*:*EnableCleanup*SLS*
		     PROPERTIES LABELS sls_dev_privileged)
gtest_discover_tests("${target}"
	             TEST_FILTER PnmProcessManager*IMDB*:*Leaked*IMDB*:*EnableCleanup*IMDB*
		     PROPERTIES LABELS imdb_dev_privileged)
