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

/** @file control.h
    @brief Low level abstraction for SLS device control calls.
*/

#ifndef SLS_CONTROL_H
#define SLS_CONTROL_H

#include "pnmlib/core/sls_cunit_info.h"

#include "pnmlib/common/compiler.h"

#include <linux/sls_resources.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace pnm::sls::device {

struct RegionInfo {
  uint64_t size;
  uint64_t offset;
  uint64_t map_size;
  uint64_t map_offset;
};

using MemInfo = std::vector<std::array<RegionInfo, SLS_BLOCK_MAX>>;

/** @brief Used to interact with the device - query low-level statistics,
 * set/unset configuration flags, reset
 */
class PNM_API Control {
public:
  Control();

  size_t get_compute_unit_info(uint8_t compute_unit,
                               pnm::sls::ComputeUnitInfo key) const;

  RegionInfo get_region_info(uint8_t compute_unit, sls_mem_blocks_e key) const;

  void sysfs_reset() const;

  uint64_t get_acquisition_timeout() const;

  void set_acquisition_timeout(uint64_t value) const;

  uint64_t get_leaked() const;

  uint64_t get_resource_cleanup() const;

  void set_resource_cleanup(uint64_t value) const;

  MemInfo get_mem_info() const;

  size_t memory_size() const;

private:
  /**
     @brief Keys to access specific device information.
     Used in @ref sls_cunit_get_info
   */
  static constexpr std::array<std::string_view, cunit_info_length>
      cunit_info_paths_{
          CUNIT_STATE_PATH,
          CUNIT_ACQUISITION_COUNT_PATH,
          CUNIT_FREE_SIZE_PATH,
          CUNIT_REGION_BASE_SIZE_PATH,
          CUNIT_REGION_BASE_OFFSET_PATH,
          CUNIT_REGION_BASE_MAP_SIZE_PATH,
          CUNIT_REGION_BASE_MAP_OFFSET_PATH,
          CUNIT_REGION_INST_SIZE_PATH,
          CUNIT_REGION_INST_OFFSET_PATH,
          CUNIT_REGION_INST_MAP_SIZE_PATH,
          CUNIT_REGION_INST_MAP_OFFSET_PATH,
          CUNIT_REGION_CFGR_SIZE_PATH,
          CUNIT_REGION_CFGR_OFFSET_PATH,
          CUNIT_REGION_CFGR_MAP_SIZE_PATH,
          CUNIT_REGION_CFGR_MAP_OFFSET_PATH,
          CUNIT_REGION_TAGS_SIZE_PATH,
          CUNIT_REGION_TAGS_OFFSET_PATH,
          CUNIT_REGION_TAGS_MAP_SIZE_PATH,
          CUNIT_REGION_TAGS_MAP_OFFSET_PATH,
          CUNIT_REGION_PSUM_SIZE_PATH,
          CUNIT_REGION_PSUM_OFFSET_PATH,
          CUNIT_REGION_PSUM_MAP_SIZE_PATH,
          CUNIT_REGION_PSUM_MAP_OFFSET_PATH,
      };

  const std::filesystem::path sls_sysfs_root_;
};

} // namespace pnm::sls::device
#endif // SLS_CONTROL_H
