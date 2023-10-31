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

#include "common/log.h"
#include "common/topology_constants.h"

#include <test/utils/pnm_fmt.h> // NOLINT(misc-include-cleaner)

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"
#include "pnmlib/core/shared_region.h"

#include "pnmlib/common/misc_utils.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string_view>
#include <vector>

pnm::ContextHandler make_context_by_typename(std::string_view name) {
  if (name == "SLS") {
    return pnm::make_context(pnm::Device::Type::SLS);
  }

  if (name == "IMDB") {
    return pnm::make_context(pnm::Device::Type::IMDB);
  }

  return nullptr;
}

auto make_serialized_by_typename(std::string_view name) {
  if (name == "SLS") {
    return pnm::memory::SharedRegion::SerializedRegion(
        pnm::sls::device::topo().NumOfCUnits);
  }

  if (name == "IMDB") {
    return pnm::memory::SharedRegion::SerializedRegion(1);
  }

  return pnm::memory::SharedRegion::SerializedRegion(1);
}

template <typename T> void print_buffer_info(pnm::memory::Buffer<T> &buffer) {
  pnm::log::debug("================ Buffer Info ================");
  pnm::log::debug("------------------ Region ------------------");

  std::visit(
      pnm::utils::visitor::overload{
          [](const pnm::memory::SequentialRegion &sequential) {
            pnm::log::debug("Addr: {}, Size: {}, Rank: {}, Is global: {}",
                            sequential.start, sequential.size,
                            sequential.location.value_or(255),
                            sequential.is_global);
          },
          [](const pnm::memory::RankedRegion &ranked) {
            for (const auto &region : ranked.regions) {
              pnm::log::debug("Addr: {}, Size: {}, Rank: {}, Is global: {}",
                              region.start, region.size,
                              region.location.value_or(255), region.is_global);
            }
          }},
      buffer.device_region());

  auto accessor = buffer.direct_access();
  pnm::log::debug("------------- Content of buffer -------------");
  pnm::log::debug("({})", fmt::join(accessor, ", "));
  pnm::log::debug("=============================================");
}

int main(int argc, char *argv[]) {
  int sock, listener;
  int status = 0;
  struct sockaddr_un addr;
  const std::filesystem::path receiver_file{
      std::filesystem::temp_directory_path().string() + "/shared.socket"};

  if (argc < 2) {
    pnm::log::debug("Insufficient amount of arguments. Amount: {}/2", argc);
    return 1;
  }

  auto context = make_context_by_typename(argv[1]);
  if (!context) {
    pnm::log::debug("Can't create context of type: {}", argv[1]);
    return 1;
  }

  auto buf = make_serialized_by_typename(argv[1]);

  listener = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listener < 0) {
    pnm::log::debug("Can't create listener");
    return 1;
  }

  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, receiver_file.string().c_str(),
               sizeof(addr.sun_path) - 1);
  if (bind(listener, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) <
      0) {
    pnm::log::debug("Can't bind socket with {}", addr.sun_path);
    return 1;
  }

  listen(listener, 1);

  while (true) {
    sock = accept(listener, nullptr, nullptr);
    if (sock < 0) {
      pnm::log::debug("Can't open socket");
      return 1;
    }

    // [TODO: @b.palkin] Receive size of serialized region at first
    if (recv(sock, buf.data(), buf.size() * sizeof(uint64_t), 0) <= 0) {
      break;
    }

    std::vector<int> original_vector{4, 8, 28, 42};
    const pnm::memory::SharedRegion shared_in_receiver(buf);
    pnm::memory::Buffer<int> shared_buffer(shared_in_receiver, context);
    print_buffer_info(shared_buffer);

    // [TODO: @b.palkin] Make checks more convenient
    if (original_vector.size() != shared_buffer.size()) {
      pnm::log::info("Size of original vector and vector from shared buffer "
                     "aren't equal.");
      status = 1;
      break;
    }

    auto accessor = shared_buffer.direct_access();
    for (size_t i = 0; i < original_vector.size(); ++i) {
      if (accessor[i] != original_vector[i]) {
        pnm::log::info(
            "Original vector and vector from shared buffer aren't equal.");
        status = 1;
      }
    }
    break;
  }

  send(sock, &status, sizeof(status), 0);
  close(sock);

  if (std::filesystem::exists(receiver_file)) {
    std::filesystem::remove(receiver_file);
  }
  return 0;
}
