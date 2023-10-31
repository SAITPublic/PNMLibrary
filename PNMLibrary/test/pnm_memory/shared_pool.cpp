/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#include "test/utils/pnm_fmt.h" // NOLINT(misc-include-cleaner)

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"
#include "pnmlib/core/shared_region.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace {
std::filesystem::path path_to_test;
} // namespace

class SharedPoolTest : public testing::TestWithParam<pnm::Device::Type> {};

INSTANTIATE_TEST_SUITE_P(SharedPoolParamSuite, SharedPoolTest,
                         ::testing::Values(pnm::Device::Type::IMDB,
                                           pnm::Device::Type::SLS),
                         print_fmt_test_params);

TEST_P(SharedPoolTest, MakeShared) {
  auto context = pnm::make_context(GetParam());
  std::vector input{4, 8, 28, 42};
  pnm::memory::Buffer buffer(pnm::views::make_view(input), context);
  ASSERT_EQ(buffer.size(), input.size());

  auto shared = buffer.make_shared();
  auto dev_region_from_shared_region = shared.get_region();
  auto dev_region_from_buffer = buffer.device_region();

  std::visit(pnm::utils::visitor::overload{
                 [&](const pnm::memory::SequentialRegion &sequential_shared) {
                   ASSERT_EQ(std::get<pnm::memory::SequentialRegion>(
                                 dev_region_from_buffer),
                             sequential_shared);
                 },
                 [&](const pnm::memory::RankedRegion &ranked_shared) {
                   auto shared_regs = ranked_shared.regions;
                   auto original_regs = std::get<pnm::memory::RankedRegion>(
                                            dev_region_from_buffer)
                                            .regions;
                   ASSERT_EQ(shared_regs.size(), original_regs.size());

                   for (size_t i = 0; i < shared_regs.size(); ++i) {
                     ASSERT_EQ(shared_regs[i], original_regs[i]);
                   }
                 },
             },
             dev_region_from_shared_region);
}

// We serialize only start(addr) and location(memory_pool) fields. So, in
// "SerializeDeserializeValid" and "SerializeDeserializeInvalid" we check
// serialization and deserialization only of these fields, because other
// fields unnecessary for this sharing mechanism.
TEST_P(SharedPoolTest, SerializeDeserializeValid) {
  pnm::memory::SequentialRegion reg{
      .start = 0xABCDEFU,
      .size = 100,
      .location = 1,
      .is_global = 1,
  };

  const pnm::memory::SharedRegion original_shared(reg);
  auto serialized = original_shared.get_serialized();
  const pnm::memory::SharedRegion shared_from_serialized(serialized);

  auto orig_seq_region =
      std::get<pnm::memory::SequentialRegion>(original_shared.get_region());
  auto from_serialized_seq_region = std::get<pnm::memory::SequentialRegion>(
      shared_from_serialized.get_region());

  ASSERT_EQ(orig_seq_region.start, from_serialized_seq_region.start);
  ASSERT_EQ(orig_seq_region.location, from_serialized_seq_region.location);
}

TEST_P(SharedPoolTest, SerializeDeserializeInvalid) {
  pnm::memory::SequentialRegion reg{
      .start = 0x0, .size = 0, .location = std::nullopt, .is_global = 0};

  const pnm::memory::SharedRegion original_shared(reg);
  auto serialized = original_shared.get_serialized();
  const pnm::memory::SharedRegion shared_from_serialized(serialized);

  auto orig_seq_region =
      std::get<pnm::memory::SequentialRegion>(original_shared.get_region());
  auto from_serialized_seq_region = std::get<pnm::memory::SequentialRegion>(
      shared_from_serialized.get_region());

  ASSERT_EQ(orig_seq_region.start, from_serialized_seq_region.start);
  ASSERT_EQ(orig_seq_region.location, from_serialized_seq_region.location);
}

TEST_P(SharedPoolTest, MakeBufferFromShared) {
  auto context = pnm::make_context(GetParam());
  std::vector input{4, 8, 28, 42};
  pnm::memory::Buffer buffer(pnm::views::make_view(input), context);
  ASSERT_EQ(buffer.size(), input.size());

  auto shared = buffer.make_shared();
  auto dev_region_from_original = buffer.device_region();
  auto serialized = shared.get_serialized();
  const pnm::memory::SharedRegion from_serialized(serialized);

  auto new_context = pnm::make_context(GetParam());
  const pnm::memory::Buffer<int> shared_buffer(from_serialized, new_context);
  auto dev_region_from_shared_buffer = shared_buffer.device_region();

  std::visit(
      pnm::utils::visitor::overload{
          [&](const pnm::memory::SequentialRegion &sequential_shared) {
            ASSERT_EQ(std::get<pnm::memory::SequentialRegion>(
                          dev_region_from_original),
                      sequential_shared);
          },
          [&](const pnm::memory::RankedRegion &ranked_shared) {
            auto original_regs =
                std::get<pnm::memory::RankedRegion>(dev_region_from_original)
                    .regions;

            auto from_shared_buffer_regs = ranked_shared.regions;
            ASSERT_EQ(original_regs.size(), from_shared_buffer_regs.size());

            for (size_t i = 0; i < original_regs.size(); ++i) {
              ASSERT_EQ(original_regs[i], from_shared_buffer_regs[i]);
            }
          }},
      dev_region_from_shared_buffer);
}

// [TODO: @b.palkin] Tidy up this test
TEST_P(SharedPoolTest, MakeBufferFromSharedMP) {
  int receiver;
  int child_status;
  int buffer_check_status = -1;

  const std::filesystem::path tests_folder = path_to_test.parent_path();
  static const std::filesystem::path receiver_process_path{
      tests_folder.string() + "/shared_region_receiver"};

  auto pid = fork();
  ASSERT_NE(pid, -1);

  if (pid == 0) {
    auto type = fmt::format("{}", GetParam());
    static const char *args[] = {receiver_process_path.c_str(), type.c_str(),
                                 nullptr};
    static auto *new_args = const_cast<char **>(args);
    receiver = execv(args[0], new_args);
    EXPECT_EQ(receiver, 0);
    if (receiver != 0) {
      std::exit(receiver);
    }
  } else {
    // [TODO: @b.palkin] Use some sort of IPC to inform parent that
    // process-receiver is ready to accept inbound connection
    std::this_thread::sleep_for(std::chrono::seconds(2));

    int sock;
    struct sockaddr_un addr;
    auto context = pnm::make_context(GetParam());

    std::vector input{4, 8, 28, 42};
    pnm::memory::Buffer buffer(pnm::views::make_view(input), context);
    ASSERT_EQ(buffer.size(), input.size());

    auto shared = buffer.make_shared();
    auto serialized = shared.get_serialized();

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(sock, 0);

    const std::filesystem::path socket_file{
        std::filesystem::temp_directory_path().string() + "/shared.socket"};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_file.string().c_str(),
                 sizeof(addr.sun_path) - 1);
    ASSERT_GE(
        connect(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)),
        0);

    ASSERT_GE(
        send(sock, serialized.data(), serialized.size() * sizeof(uint64_t), 0),
        0);

    while (buffer_check_status < 0) {

      if (recv(sock, &buffer_check_status, sizeof(buffer_check_status), 0) <=
          0) {
        break;
      }

      ASSERT_EQ(buffer_check_status, 0);
    }

    close(sock);

    auto child_pid = wait(&child_status);
    if (child_status != 0) {
      kill(child_pid, SIGKILL);
      waitpid(child_pid, nullptr, 0);
    }
  }
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  path_to_test /= argv[0];

  return RUN_ALL_TESTS();
}
