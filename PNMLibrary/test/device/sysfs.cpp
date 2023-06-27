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

#include "common.h"
#include "wrapper.h"

#include "core/device/sls/base.h"

#include "common/topology_constants.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_cunit_info.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <sched.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <vector>

const auto PNMDeviceTypes = ::testing::Values(pnm::Device::Type::SLS_AXDIMM,
                                              pnm::Device::Type::IMDB_CXL);

class PNMTypesF : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type DevType) {
    DeviceWrapper dev(DevType);
    auto &ctx = dev.context();
    EXPECT_EQ(ctx->device()->get_device_type(), DevType);
    EXPECT_EQ(ctx->type(), DevType);
  }
};

class EnableCleanupF : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type DevType) {

    DeviceWrapper dev(DevType);

    bool original_cleanup{};
    EXPECT_NO_THROW(original_cleanup = dev.get_resource_cleanup());

    EXPECT_NO_THROW(dev.set_resource_cleanup(false));
    bool is_enabled_cleanup = true;
    EXPECT_NO_THROW(is_enabled_cleanup = dev.get_resource_cleanup());
    EXPECT_FALSE(is_enabled_cleanup);

    EXPECT_NO_THROW(dev.set_resource_cleanup(true));
    EXPECT_NO_THROW(is_enabled_cleanup = dev.get_resource_cleanup());
    EXPECT_TRUE(is_enabled_cleanup);

    EXPECT_NO_THROW(dev.set_resource_cleanup(original_cleanup));
  }
};

class StateF : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type DevType) {
    DeviceWrapper dev(DevType);
    const auto compute_unit_count = dev.compute_unit_count();

    for (uint8_t compute_unit = 0; compute_unit < compute_unit_count;
         ++compute_unit) {
      bool is_busy = true;
      EXPECT_NO_THROW(is_busy = dev.is_compute_unit_busy(compute_unit));
      EXPECT_FALSE(is_busy);
    }

    for (uint8_t i = 0; i < compute_unit_count; ++i) {
      uint8_t compute_unit{};
      EXPECT_NO_THROW(compute_unit = dev.acquire_compute_unit(i));

      bool is_busy = false;
      EXPECT_NO_THROW(is_busy = dev.is_compute_unit_busy(compute_unit));

      EXPECT_TRUE(is_busy);
      EXPECT_NO_THROW(dev.release_compute_unit(compute_unit));
    }

    for (uint8_t compute_unit = 0; compute_unit < compute_unit_count;
         ++compute_unit) {
      bool is_busy = true;
      EXPECT_NO_THROW(is_busy = dev.is_compute_unit_busy(compute_unit));
      EXPECT_FALSE(is_busy);
    }
  }
};

class AcquisitionCountF : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  void run(pnm::Device::Type DevType) {
    // [TODO: s.motov] add/remove acquisition count imdb/sls
    if (DevType == pnm::Device::Type::IMDB_CXL) {
      GTEST_SKIP() << "IMDB doesn't have acquisition count\n";
    }

    auto dev = pnm::sls::device::BaseDevice::make_device(DevType);

    std::default_random_engine gen{std::random_device{}()};
    // acquire and free ranks by [a, b] times
    std::uniform_int_distribution<size_t> distrib(5, 10);

    for (uint8_t rank = 0; rank < pnm::device::topo().NumOfRanks; ++rank) {

      size_t old_aqc_count{};
      EXPECT_NO_THROW(old_aqc_count = dev->get_compute_unit_info(
                          rank, SlsComputeUnitInfo::AcquisitionCount));

      const pnm::sls::device::BaseDevice::bit_mask rank_mask = 1 << rank;

      auto count = distrib(gen);
      for (size_t j = 0; j < count; ++j) {
        EXPECT_NO_THROW(
            dev->acquire_cunit_for_write_from_range(&rank, rank_mask));

        EXPECT_NO_THROW(dev->release_cunit_for_write(rank));
      }

      size_t new_aqc_count = 0;
      EXPECT_NO_THROW(new_aqc_count = dev->get_compute_unit_info(
                          rank, SlsComputeUnitInfo::AcquisitionCount));

      EXPECT_EQ(new_aqc_count - old_aqc_count, count);
    }
  }
};

class LeakedF : public ::testing::TestWithParam<pnm::Device::Type> {
public:
  static constexpr auto children_num = 3;

  void run(pnm::Device::Type dev_type) {

    const bool was_enabled_cleanup = exchange_cleanup(false, dev_type);

    std::vector<pid_t> children_pids(children_num);
    if (auto fork_res =
            fork_children(children_pids)) { // child need to leak some resource
      auto child_index = *fork_res;
      DeviceWrapper leaking_dev(dev_type);
      bool res = true;
      try {
        leaking_dev.acquire_compute_unit(child_index);
      } catch (...) {
        res = false;
      }
      exit(res ? 0 : EXIT_FAILURE);
    } else { // parent wait child complete before start checking
      std::vector<bool> children_msk(children_num);
      children_msk.flip();
      EXPECT_TRUE(handle_children_exits(children_pids, children_msk));

      DeviceWrapper dev(dev_type);
      uint64_t leaked_cnt;
      EXPECT_NO_THROW(leaked_cnt = dev.get_leaked());
      EXPECT_EQ(children_num, leaked_cnt);

      //  enable cleanup
      EXPECT_NO_THROW(dev.set_resource_cleanup(true));

      // check leak
      EXPECT_NO_THROW(leaked_cnt = dev.get_leaked());
      EXPECT_EQ(0, leaked_cnt);
      restore_cleanup(was_enabled_cleanup, dev_type);
    }
  }
};

TEST_P(PNMTypesF, CheckTypes) { this->run(GetParam()); }
TEST_P(EnableCleanupF, EnableCleanup) { this->run(GetParam()); }
TEST_P(StateF, State) { this->run(GetParam()); }
TEST_P(AcquisitionCountF, AcquisitionCount) { this->run(GetParam()); }
TEST_P(LeakedF, Leaked) { this->run(GetParam()); }

INSTANTIATE_TEST_SUITE_P(PNMDeviceTypeCheck, PNMTypesF, PNMDeviceTypes);
INSTANTIATE_TEST_SUITE_P(PNMSysfsAPI, EnableCleanupF, PNMDeviceTypes,
                         generate_param_name<EnableCleanupF>);
INSTANTIATE_TEST_SUITE_P(PNMSysfsAPI, StateF, PNMDeviceTypes);
INSTANTIATE_TEST_SUITE_P(PNMSysfsAPI, AcquisitionCountF, PNMDeviceTypes);
INSTANTIATE_TEST_SUITE_P(PNMSysfsAPI, LeakedF, PNMDeviceTypes,
                         generate_param_name<LeakedF>);
