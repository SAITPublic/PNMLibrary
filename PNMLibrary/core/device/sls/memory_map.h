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

#ifndef SLS_MEMORY_MAP_H
#define SLS_MEMORY_MAP_H

#include "core/device/sls/control.h"

#include "common/log.h"
#include "common/mapped_file.h"
#include "common/topology_constants.h"

#include "pnmlib/core/device.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace pnm::sls {

using namespace pnm::device;

struct memAddr {
  uint8_t *addr[SLS_BLOCK_MAX];
  size_t size[SLS_BLOCK_MAX];
};

inline constexpr const char *sls_mem_blocks_name[SLS_BLOCK_MAX] = {
    "BASE", "INST", "CFGR", "TAGS", "PSUM",
};

constexpr void check_block_type([[maybe_unused]] sls_mem_blocks_e type) {
  assert(SLS_BLOCK_BASE <= type && type < SLS_BLOCK_MAX);
}

class MemMap {
public:
  MemMap() = default;

  void init_device_memory(const device::MemInfo &mem_info, int data_fd,
                          pnm::Device::Type device_type) {
    const auto compute_units_count =
        (device_type == pnm::Device::Type::SLS_AXDIMM) ? topo().NumOfRanks
                                                       : topo().NumOfChannels;

    std::vector<memAddr> addr_infos(compute_units_count);
    mapped_data_.resize(compute_units_count);
    m_cunitAddrInfo.resize(compute_units_count);

    for (uint8_t cs = 0; cs < topo().NumOfCS; cs++) {
      for (uint8_t segment = 0; segment < SLS_BLOCK_MAX; segment++) {
        auto [block_size, block_addr] = mem_info[cs][segment];

        size_t mapped_size = block_size;
        // In AXDIMM case we should to multiply mapped region size by
        // interleaved rank count.
        // In CXL case we leave it as is.
        if (device_type == pnm::Device::Type::SLS_AXDIMM) {
          mapped_size *= topo().NumOfRanksPerCS;
        }

        mapped_data_[cs][segment] = pnm::utils::MappedData(
            data_fd, mapped_size, "Device Memory", block_addr);

        addr_infos[cs].addr[segment] =
            static_cast<uint8_t *>(mapped_data_[cs][segment].data());
        addr_infos[cs].size[segment] = block_size;
      }
    }

    if (device_type == pnm::Device::Type::SLS_AXDIMM) {
      apply_axdimm_stride(addr_infos);
    }

    init(addr_infos);
  }

  void apply_axdimm_stride(std::vector<memAddr> &cunit_addr_infos) {
    // Second Rank @ each CS
    for (uint8_t cs = 0; cs < topo().NumOfCS; cs++) {
      const uint8_t cunit = cs + 2; // 2, 3 rank
      for (uint8_t segment = 0; segment < SLS_BLOCK_MAX; segment++) {
        // + stride length as offset 0x80, cs == first rank number
        cunit_addr_infos[cunit].addr[segment] =
            cunit_addr_infos[cs].addr[segment];
        cunit_addr_infos[cunit].size[segment] =
            cunit_addr_infos[cs].size[segment];
      }
    }
  }

  // Make sure that this function must be called after open sls device
  void init(const std::vector<memAddr> &cunitAddrInfos) {
    m_minSizeInfo.fill(std::numeric_limits<size_t>::max());
    for (size_t i = 0; i < m_cunitAddrInfo.size(); ++i) {
      for (size_t j = 0; j < m_minSizeInfo.size(); ++j) {
        m_minSizeInfo[j] =
            std::min(m_minSizeInfo[j], cunitAddrInfos[i].size[j]);
      }
      m_cunitAddrInfo[i] = cunitAddrInfos[i];
    }
    pnm::log::info("buffer addresses of each compute unit are set.");
  }

  size_t get_min_block_size(sls_mem_blocks_e type) const {
    check_block_type(type);
    return m_minSizeInfo[type];
  }

  size_t get_all_cunits_block_size(sls_mem_blocks_e type) const {
    check_block_type(type);
    size_t size = 0;
    for (const auto &cunit_addr_info : m_cunitAddrInfo) {
      size += cunit_addr_info.size[type];
    }
    return size;
  }

  const memAddr &operator[](uint8_t compute_unit) const {
    return m_cunitAddrInfo[compute_unit];
  }

  memAddr &operator[](uint8_t compute_unit) {
    return m_cunitAddrInfo[compute_unit];
  }

  void print() {
    pnm::log::debug("------------- SLS Device Memory Map -------------");
    for (uint8_t cunit = 0; cunit < m_cunitAddrInfo.size(); cunit++) {
      pnm::log::debug("COMPUTE UNIT {}", cunit);
      for (int type = SLS_BLOCK_BASE; type < SLS_BLOCK_MAX; type++) {
        pnm::log::debug("\t BLOCK {} addr: {:p} (size: {:10x})",
                        sls_mem_blocks_name[type],
                        static_cast<void *>(m_cunitAddrInfo[cunit].addr[type]),
                        m_cunitAddrInfo[cunit].size[type]);
      }
    }
  }

private:
  //! \brief Memory info for each compute unit
  std::vector<memAddr> m_cunitAddrInfo{};
  std::vector<std::array<pnm::utils::MappedData, SLS_BLOCK_MAX>> mapped_data_{};
  //! \brief Minimal sizes of each block out of all compute units
  std::array<size_t, SLS_BLOCK_MAX> m_minSizeInfo{};
};

} /* namespace pnm::sls */

#endif /* SLS_MEMORY_MAP_H */
