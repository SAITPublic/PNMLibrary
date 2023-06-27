file(GLOB PNM_CONTEXT_TEST_MOCK_SRC *.cpp)
add_library(pnm_context_test_mock STATIC ${PNM_CONTEXT_TEST_MOCK_SRC})
target_link_libraries(pnm_context_test_mock PUBLIC ${COMMON_LIBS})
static_dependency_check(pnm_context_test_mock)
