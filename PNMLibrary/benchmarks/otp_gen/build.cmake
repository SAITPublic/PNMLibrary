add_executable(otp_gen otp_gen/main.cpp)
target_link_libraries(
  otp_gen
  PRIVATE
    ${BENCHMARKS_LIBS}
    pnm_context
    sls_device
    sls_secure_static
)
