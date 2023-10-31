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

#include "common/log.h"
#include "common/make_error.h"
#include "common/mapped_file.h"
#include "common/topology_constants.h"

#include "pnmlib/sls/control.h"

#include <linux/sls_resources.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace pnm::sls {

using pnm::sls::device::topo;

struct memAddr {
  uint8_t *addr[SLS_BLOCK_MAX];
  size_t size[SLS_BLOCK_MAX];
};

inline constexpr std::array<std::string_view, SLS_BLOCK_MAX>
    sls_mem_blocks_name{
        "BASE", "INST", "CFGR", "TAGS", "PSUM",
    };

constexpr void check_block_type([[maybe_unused]] sls_mem_blocks_e type) {
  assert(SLS_BLOCK_BASE <= type && type < SLS_BLOCK_MAX);
}

class MemMap {
public:
  void init_device_memory(const device::MemInfo &mem_info, int data_fd) {
    const auto cunits_count = topo().NumOfCUnits;

    std::vector<memAddr> addr_infos(cunits_count);
    m_cunitAddrInfo.resize(cunits_count);

    for (size_t cunit = 0; cunit < cunits_count; ++cunit) {
      for (size_t reg = 0; reg < SLS_BLOCK_MAX; ++reg) {
        auto [size, offset, map_size, map_offset] = mem_info[cunit][reg];

        const auto &mapped_data = mapped_data_.emplace_back(
            data_fd, map_size, "Device Memory", map_offset);

        addr_infos[cunit].size[reg] = size;
        addr_infos[cunit].addr[reg] =
            static_cast<uint8_t *>(mapped_data.data());
      }
    }

    init(addr_infos);
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

    const auto bus_to_str = [](device::BusType bus) -> std::string_view {
      switch (bus) {
      case device::BusType::AXDIMM:
        return "AXDIMM";
      case device::BusType::CXL:
        return "CXL";
      }

      throw pnm::error::make_inval("Invalid Bus type");
    };

    pnm::log::debug("------------- SLS-{} Device Memory Map -------------",
                    bus_to_str(device::topo().Bus));
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
  std::vector<pnm::utils::MappedData> mapped_data_{};
  //! \brief Minimal sizes of each block out of all compute units
  std::array<size_t, SLS_BLOCK_MAX> m_minSizeInfo{};
};

} /* namespace pnm::sls */

#endif /* SLS_MEMORY_MAP_H */
