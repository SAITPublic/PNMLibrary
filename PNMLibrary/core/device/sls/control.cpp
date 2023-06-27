/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#include "control.h"

#include "common/make_error.h"
#include "common/misc_control.h"
#include "common/topology_constants.h"

#include "pnmlib/core/sls_cunit_info.h"

#include <linux/sls_resources.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#define BLOCK(str) SLS_BLOCK_##str
#define INFO(str) SlsComputeUnitInfo::Region##str

using namespace pnm::sls::device;

Control::Control() : sls_sysfs_root_(SLS_SYSFS_ROOT "/ranks") {}

uint64_t Control::get_compute_unit_info(uint8_t compute_unit,
                                        SlsComputeUnitInfo key) const {
  uint64_t value;
  const auto path = sls_sysfs_root_ / std::to_string(compute_unit) /
                    cunit_info_paths[cunit_info_to_index(key)];
  pnm::control::utils::read_file(path, value);
  return value;
}

RegionInfo Control::get_region_info(uint8_t compute_unit,
                                    sls_mem_blocks_e key) const {
  static const std::unordered_map<
      sls_mem_blocks_e, std::pair<SlsComputeUnitInfo, SlsComputeUnitInfo>>
      key_matching{{BLOCK(BASE), {INFO(BaseSize), INFO(BaseOffset)}},
                   {BLOCK(CFGR), {INFO(CfgrSize), INFO(CfgrOffset)}},
                   {BLOCK(INST), {INFO(InstSize), INFO(InstOffset)}},
                   {BLOCK(TAGS), {INFO(TagsSize), INFO(TagsOffset)}},
                   {BLOCK(PSUM), {INFO(PsumSize), INFO(PsumOffset)}}};

  const auto it = key_matching.find(key);

  if (it == key_matching.end()) {
    throw pnm::error::make_inval(
        "Unable to find matching 'sls_mem_blocks_e' and 'SlsComputeUnitInfo'.");
  }

  auto [size, offset] = it->second;

  const RegionInfo info{
      .size = get_compute_unit_info(compute_unit, size),
      .offset = get_compute_unit_info(compute_unit, offset),
  };

  return info;
}

void Control::sysfs_reset() const {
  const auto path = sls_sysfs_root_ / DEVICE_RESET_PATH;
  pnm::control::utils::write_file<std::string_view>(path, "1");
}

uint64_t Control::get_leaked() const {
  uint64_t value;
  const auto path = sls_sysfs_root_ / DEVICE_LEAKED_PATH;
  pnm::control::utils::read_file<size_t>(path, value);
  return value;
}

void Control::set_resource_cleanup(uint64_t value) const {
  const auto path = sls_sysfs_root_ / DEVICE_RESOURCE_CLEANUP_PATH;
  pnm::control::utils::write_file<size_t>(path, value);
}

uint64_t Control::get_resource_cleanup() const {
  uint64_t value;
  const auto path = sls_sysfs_root_ / DEVICE_RESOURCE_CLEANUP_PATH;
  pnm::control::utils::read_file<size_t>(path, value);
  return value;
}

uint64_t Control::get_acquisition_timeout() const {
  uint64_t value;
  const auto path = sls_sysfs_root_ / DEVICE_ACQUISITION_TIMEOUT_PATH;
  pnm::control::utils::read_file<size_t>(path, value);
  return value;
}

void Control::set_acquisition_timeout(uint64_t value) const {
  const auto path = sls_sysfs_root_ / DEVICE_ACQUISITION_TIMEOUT_PATH;
  pnm::control::utils::write_file<size_t>(path, value);
}

MemInfo Control::get_mem_info() const {
  MemInfo meminfo(pnm::device::topo().NumOfRanks);

  for (size_t compute_unit = 0; compute_unit < pnm::device::topo().NumOfRanks;
       ++compute_unit) {
    for (size_t reg = 0; reg < SLS_BLOCK_MAX; ++reg) {
      meminfo[compute_unit][reg] =
          get_region_info(compute_unit, static_cast<sls_mem_blocks_e>(reg));
    }
  }

  return meminfo;
}
