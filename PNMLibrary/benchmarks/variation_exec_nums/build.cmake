add_executable(variation_exec_nums variation_exec_nums/main.cpp)
target_link_libraries(
  variation_exec_nums
  PRIVATE
    pnm_context
    sls_device
    sls_datagen
    sls_operation
    gtest_main
)
