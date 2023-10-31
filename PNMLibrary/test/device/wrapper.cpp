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

#include "core/device/sls/utils/constants.h"

#include "common/make_error.h"
#include "common/topology_constants.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_cunit_info.h"

#include <gtest/gtest.h>

#include <cerrno>
#include <cstdint>
#include <type_traits>
#include <variant>

template <typename> inline constexpr bool always_false_v = false;

uint8_t DeviceWrapper::acquire_compute_unit(uint8_t preferred_unit) {
  return std::visit(
      [preferred_unit](auto &&device) {
        using T = std::decay_t<decltype(device)>;

        if constexpr (std::is_same_v<T, ImdbDevice *>) {
          return device->lock_thread();
        } else if constexpr (std::is_same_v<T, SlsDevice *>) {
          uint8_t value;
          const pnm::sls::device::cunit mask = 1 << preferred_unit;
          const bool acquired = device->acquire_cunit_from_range(&value, mask);
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

        if constexpr (std::is_same_v<T, ImdbDevice *>) {
          device->release_thread(compute_unit);
        } else if constexpr (std::is_same_v<T, SlsDevice *>) {
          device->release_cunit(compute_unit);
        } else {
          static_assert(always_false_v<T>, "Unprocessed type");
        }
      },
      device_);
}

uint64_t DeviceWrapper::compute_unit_count() {
  switch (ctx_->type()) {
  case pnm::Device::Type::IMDB:
    return pnm::imdb::device::topo().NumOfThreads;
  case pnm::Device::Type::SLS:
    return pnm::sls::device::topo().NumOfCUnits;
  }
  throw pnm::error::make_inval("Device type.");
}

bool DeviceWrapper::is_compute_unit_busy(uint8_t unit) {
  return std::visit(
      [unit](auto &&device) {
        using T = std::decay_t<decltype(device)>;

        if constexpr (std::is_same_v<T, ImdbDevice *>) {
          return device->is_thread_busy(unit);
        } else if constexpr (std::is_same_v<T, SlsDevice *>) {
          const auto info = device->get_compute_unit_info(
              unit, pnm::sls::ComputeUnitInfo::State);
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
  case pnm::Device::Type::IMDB:
    return {ctx->device()->as<ImdbDevice>()};
  case pnm::Device::Type::SLS:
    return {ctx->device()->as<SlsDevice>()};
  }

  throw pnm::error::make_inval("Device type.");
}

uint64_t DeviceWrapper::memory_size() {
  return std::visit(
      [](auto &&device) -> uint64_t {
        using T = std::decay_t<decltype(device)>;

        if constexpr (std::is_same_v<T, ImdbDevice *>) {
          return device->memory_size();
        } else if constexpr (std::is_same_v<T, SlsDevice *>) {
          return device->get_base_memory_size();
        } else {
          static_assert(always_false_v<T>, "Unprocessed type");
        }
      },
      device_);
}
