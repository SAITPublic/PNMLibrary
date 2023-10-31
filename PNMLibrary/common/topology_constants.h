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

#include "common/make_error.h"
#include "hw/pnm_addr_constants.h"

#include <linux/imdb_resources.h>
#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace pnm::device {

template <typename ConstantsType> class ConstantsHandler {
public:
  const ConstantsType &constants() const { return topo_constants_; }

private:
  ConstantsType topo_constants_{};
};

template <typename T> static const T &topo() {
  static const ConstantsHandler<T> handler;
  return handler.constants();
}
} // namespace pnm::device

namespace pnm::sls::device {

enum class BusType : uint8_t { AXDIMM = 1, CXL = 2 };

namespace topology {
struct Constants {
  BusType Bus;

  uint32_t NumOfCUnits;

  uint32_t PSumBufSize;
  uint32_t NumOfPSumBuf;

  uint32_t TagsBufSize;
  uint32_t NumOfTagsBuf;

  uint32_t InstBufSize;
  uint32_t NumOfInstBuf;

  uint32_t InstructionSize;

  uint64_t RankInterleavingSize;

  uint64_t AlignedTagSize;

  uint64_t RegPolling;
  uint64_t RegSlsExec;
  uint64_t RegSlsEn;

  uint64_t DataSize;

  Constants() {
    static constexpr auto *sysfs_root = SLS_SYSFS_ROOT "/";

    auto load_value = [](const std::string &path_ending, auto &args) {
      pnm::control::read_file(sysfs_root + path_ending, args);
    };

    size_t bus_type;
    load_value(ATTR_DEV_TYPE_PATH, bus_type);
    Bus = static_cast<BusType>(bus_type);

    load_value(ATTR_NUM_OF_CUNITS_PATH, NumOfCUnits);

    load_value(ATTR_PSUM_BUFFER_PATH, NumOfPSumBuf);
    load_value(ATTR_TAGS_BUFFER_PATH, NumOfTagsBuf);
    load_value(ATTR_INST_BUFFER_PATH, NumOfInstBuf);
    load_value(ATTR_BUFFER_SIZE_PATH, PSumBufSize);
    load_value(ATTR_BUFFER_SIZE_PATH, TagsBufSize);
    load_value(ATTR_BUFFER_SIZE_PATH, InstBufSize);

    load_value(ATTR_INSTRUCTION_SIZE_PATH, InstructionSize);

    load_value(ATTR_ALIGNED_TAG_SIZE_PATH, AlignedTagSize);
    load_value(ATTR_DATA_SIZE_PATH, DataSize);

    load_value(ATTR_POLLING_REGISTER_OFFSET_PATH, RegPolling);
    load_value(ATTR_SLS_EXEC_VALUE_PATH, RegSlsExec);
    load_value(ATTR_REG_SLS_EN_PATH, RegSlsEn);

    RankInterleavingSize = 1ULL << hw::HA_BIT_POS;
  };
};
} // namespace topology

[[maybe_unused]] static const topology::Constants &topo() {
  return pnm::device::topo<topology::Constants>();
}

// [TODO: @nataly.cher] generalize for both imdb/sls
// and move into pnmlib/core/device.h
inline std::string get_device_path() {
  if constexpr (PNM_PLATFORM == FUNCSIM) {
    return "/dev/shm/sls";
  }

  switch (topo().Bus) {
  case BusType::AXDIMM:
    return SLS_MEMDEV_PATH;
  case BusType::CXL:
    return DAX_PATH;
  }

  throw pnm::error::make_inval("SLS Bus type");
}

} // namespace pnm::sls::device

namespace pnm::imdb::device {
namespace topology {
struct Constants {
  uint32_t NumOfRanks;
  uint32_t NumOfThreads;

  Constants() {
    static constexpr auto *sysfs_root = IMDB_SYSFS_PATH "/";

    auto load_value = [](const std::string &path_ending, auto &args) {
      pnm::control::read_file(sysfs_root + path_ending, args);
    };

    load_value(IMDB_ATTR_NUM_OF_RANKS_PATH, NumOfRanks);
    load_value(IMDB_ATTR_NUM_OF_THREADS_PATH, NumOfThreads);
  };
};
} // namespace topology

[[maybe_unused]] static const topology::Constants &topo() {
  return pnm::device::topo<topology::Constants>();
}
} // namespace pnm::imdb::device

#endif // PNM_TOPOLOGY_CONSTANTS_H
