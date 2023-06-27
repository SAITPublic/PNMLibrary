file(GLOB_RECURSE SLS_SECURE_SRC *.cpp)

# We want to create both a static and a shared version of `sls_secure`.
# The shared version is going to be exported and installed on the user's system.
# The static version is going to be linked to by tests and benchmarks, which also link
# to various other intermediate static libraries. Such tests cannot link to the shared
# version of `sls_secure`, because then they would also link to shared `pnm` library, AND
# also to some of its constituent parts (as static libraries). This will likely cause
# conflicts and mismatches of symbols and should be avoided.
#
# To create a dual shared/static library, we first collect all source files into an
# object library, and then construct a shared and a static version from the resulting
# object files.

# Add the object library.

add_library(sls_secure_obj OBJECT ${SLS_SECURE_SRC})

target_compile_options(sls_secure_obj PRIVATE -maes -msse2)
target_link_libraries(sls_secure_obj PUBLIC pnm_common)

# [TODO: @a.korzun] propagate interface usage requirements of the object lib.

# Add the static library.

add_library(sls_secure_static STATIC $<TARGET_OBJECTS:sls_secure_obj>)

# Add the shared library.

add_library(sls_secure SHARED $<TARGET_OBJECTS:sls_secure_obj>)
# [TODO: @e-kutovoi] Don't make these "system" headers
# [TODO: @e-kutovoi] Include only a couple of "public" headers instead of the whole root directory
target_include_directories(sls_secure SYSTEM INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

target_link_libraries(
  sls_secure
  PRIVATE
    pnm
    fmt::fmt
    spdlog::spdlog
)
