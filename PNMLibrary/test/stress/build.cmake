set(STRESS_TEST_SRC stress/allocator.cpp)
set(STRESS_MULTITHREAD_TEST_SRC stress/secndp.cpp stress/multithread_allocator.cpp)

add_pnm_test(stress ADD_TEST SOURCES ${STRESS_TEST_SRC})
add_pnm_test(
  stress_multithread
  ADD_TEST
  SOURCES ${STRESS_MULTITHREAD_TEST_SRC}
  LIBS sls_datagen sls_operation sls_secure_static
)
