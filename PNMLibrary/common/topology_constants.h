/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef PNM_TOPOLOGY_CONSTANTS_H
#define PNM_TOPOLOGY_CONSTANTS_H

#include "misc_control.h"

#include <linux/sls_resources.h>

#include <cstdint>
#include <string>

namespace pnm::device {

namespace topology {
struct Constants {
  uint32_t NumOfRanks;
  uint32_t NumOfCS;
  uint32_t NumOfChannels;
  uint32_t NumOfRanksPerCS;

  uint32_t PSumBufSize;
  uint32_t NumOfPSumBuf;

  uint32_t TagsBufSize;
  uint32_t NumOfTagsBuf;

  uint32_t InstBufSize;
  uint32_t NumOfInstBuf;

  uint32_t InstructionSize;

  uint64_t RankInterleavingSize;
  uint64_t ChannelInterleavingSize;
  uint64_t InterleavingStrideLength;

  uint64_t AlignedTagSize;

  uint64_t RegPolling;
  uint64_t RegSLSExec;
  uint64_t RegSLSEn;

  uint64_t DataSize;
};
} // namespace topology

static const topology::Constants &topo();

namespace topology {
class ConstantsHandler {
public:
  const auto &constants() const { return topo_constants_; }

private:
  friend const topology::Constants &pnm::device::topo();

  static constexpr auto *sysfs_root = SLS_SYSFS_ROOT "/";

  //[TODO: y-lavrinenko] Split for declaration and definition
  ConstantsHandler() {
    auto load_value = [](const std::string &path_ending, auto &...args) {
      pnm::control::utils::read_file(sysfs_root + path_ending, args...);
    };

    load_value(ATTR_NUM_OF_RANKS_PATH, topo_constants_.NumOfRanks);
    load_value(ATTR_NUM_OF_CS_PATH, topo_constants_.NumOfCS);
    load_value(ATTR_NUM_OF_CHANNEL_PATH, topo_constants_.NumOfChannels);
    topo_constants_.NumOfRanksPerCS =
        topo_constants_.NumOfRanks / topo_constants_.NumOfCS;

    load_value(ATTR_PSUM_BUFFER_PATH, topo_constants_.NumOfPSumBuf,
               topo_constants_.PSumBufSize);
    load_value(ATTR_TAGS_BUFFER_PATH, topo_constants_.NumOfTagsBuf,
               topo_constants_.TagsBufSize);
    load_value(ATTR_INST_BUFFER_PATH, topo_constants_.NumOfInstBuf,
               topo_constants_.InstBufSize);

    load_value(ATTR_INSTRUCTION_SIZE_PATH, topo_constants_.InstructionSize);

    load_value(ATTR_RANK_INTERLEAVING_SIZE_PATH,
               topo_constants_.RankInterleavingSize);
    load_value(ATTR_CHANNEL_INTERLEAVING_SIZE_PATH,
               topo_constants_.ChannelInterleavingSize);
    load_value(ATTR_INTERLEAVING_STRIDE_PATH,
               topo_constants_.InterleavingStrideLength);

    load_value(ATTR_ALIGNED_TAG_SIZE_PATH, topo_constants_.AlignedTagSize);
    load_value(ATTR_DATA_SIZE_PATH, topo_constants_.DataSize);

    load_value(ATTR_POLLING_REGISTER_OFFSET_PATH, topo_constants_.RegPolling);
    load_value(ATTR_SLS_EXEC_VALUE_PATH, topo_constants_.RegSLSExec);
    load_value(ATTR_REG_SLS_EN_PATH, topo_constants_.RegSLSEn);
  }
  Constants topo_constants_;
};
} // namespace topology

static const topology::Constants &
topo() { // NOLINT(clang-diagnostic-unused-function)
  static const topology::ConstantsHandler handler;
  return handler.constants();
}

} // namespace pnm::device

#endif // PNM_TOPOLOGY_CONSTANTS_H
