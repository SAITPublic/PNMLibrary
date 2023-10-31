add_pnm_test(
        simple_sls_app
        GTEST_NO_MAIN
        TARGET_VAR target
        SOURCES simple_sls_app/simple_app.cpp
        LIBS CLI11::CLI11
)
