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

#include "wrapper.h"

#include "core/device/sls/base.h"

#include "common/make_error.h"
#include "common/topology_constants.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_cunit_info.h"

#include <gtest/gtest.h>

#include <linux/imdb_resources.h>

#include <cerrno>
#include <cstdint>
#include <type_traits>
#include <variant>

template <typename> inline constexpr bool always_false_v = false;

uint8_t DeviceWrapper::acquire_compute_unit(uint8_t preferred_unit) {
  return std::visit(
      [preferred_unit](auto &&device) {
        using T = std::decay_t<decltype(device)>;

        if constexpr (std::is_same_v<T, IMDBDevice *>) {
          return device->lock_thread();
        } else if constexpr (std::is_same_v<T, SLSDevice *>) {
          uint8_t value;
          const pnm::sls::device::BaseDevice::bit_mask mask = 1
                                                              << preferred_unit;
          const bool acquired =
              device->acquire_cunit_for_write_from_range(&value, mask);
          if (!acquired) {
            pnm::error::throw_from_errno(errno);
          }
          return value;
        } else {
          static_assert(always_false_v<T>, "Unprocessed type");
        }
      },
      device_);
}

void DeviceWrapper::release_compute_unit(uint8_t compute_unit) {
  std::visit(
      [compute_unit](auto &&device) {
        using T = std::decay_t<decltype(device)>;

        if constexpr (std::is_same_v<T, IMDBDevice *>) {
          device->release_thread(compute_unit);
        } else if constexpr (std::is_same_v<T, SLSDevice *>) {
          device->release_cunit_for_write(compute_unit);
        } else {
          static_assert(always_false_v<T>, "Unprocessed type");
        }
      },
      device_);
}

uint64_t DeviceWrapper::compute_unit_count() {
  switch (ctx_->type()) {
  case pnm::Device::Type::IMDB_CXL:
    return IMDB_THREAD_NUM;
  case pnm::Device::Type::SLS_CXL:
    return pnm::device::topo().NumOfChannels;
  case pnm::Device::Type::SLS_AXDIMM:
    return pnm::device::topo().NumOfRanks;
  }
  throw pnm::error::make_inval("Device type.");
}

bool DeviceWrapper::is_compute_unit_busy(uint8_t unit) {
  return std::visit(
      [unit](auto &&device) {
        using T = std::decay_t<decltype(device)>;

        if constexpr (std::is_same_v<T, IMDBDevice *>) {
          return device->is_thread_busy(unit);
        } else if constexpr (std::is_same_v<T, SLSDevice *>) {
          const auto info =
              device->get_compute_unit_info(unit, SlsComputeUnitInfo::State);
          EXPECT_FALSE(info > 1);
          return info == 1;
        } else {
          static_assert(always_false_v<T>, "Unprocessed type");
        }
      },
      device_);
}

DeviceWrapper::VariantType DeviceWrapper::get_device(pnm::ContextHandler &ctx) {
  switch (ctx->type()) {
  case pnm::Device::Type::IMDB_CXL:
    return {ctx->device()->as<IMDBDevice>()};
  case pnm::Device::Type::SLS_CXL:
  case pnm::Device::Type::SLS_AXDIMM:
    return {ctx->device()->as<SLSDevice>()};
  }

  throw pnm::error::make_inval("Device type.");
}

uint64_t DeviceWrapper::memory_size() {
  return std::visit(
      [](auto &&device) -> uint64_t {
        using T = std::decay_t<decltype(device)>;

        if constexpr (std::is_same_v<T, IMDBDevice *>) {
          return device->memory_size();
        } else if constexpr (std::is_same_v<T, SLSDevice *>) {
          return device->get_base_memory_size();
        } else {
          static_assert(always_false_v<T>, "Unprocessed type");
        }
      },
      device_);
}
