include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        f5e54359df4c26b6230fc61d38aa294581393084  # v10.1.1
)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        7e635fca68d014934b4af8a1cf874f63989352b7  # 1.12.0
)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    # NOTE: Commit 60f0dca contains a reqiured fix that includes headers
    # as `-isystem`. It is not yet available for the latest release as for
    # 2023-07-14.
    GIT_TAG        60f0dcab3d0bd51d87146ec7601f0aa7dbfe3410  # v2.3.2+.
)

set(FMT_SYSTEM_HEADERS ON)
set(SPDLOG_FMT_EXTERNAL ON)

FetchContent_MakeAvailable(fmt spdlog cli11)

# [TODO: s-lavrentiev] Temporary work around for tidy-18 error
# with 'AnalyzeTemporaryDtors' keyword
file(REMOVE ${spdlog_SOURCE_DIR}/.clang-tidy)

add_library(pnm_common INTERFACE)
target_link_libraries(pnm_common INTERFACE fmt::fmt spdlog::spdlog)
