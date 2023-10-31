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

#ifndef _SLS_AXDIMM_CUNIT_H_

#include "sls/compute_unit/compute_unit.h"

#include "core/device/sls/base.h"
#include "core/device/sls/utils/constants.h"

#include "common/make_error.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/views.h"

#include <cstdint>
#include <type_traits>

namespace pnm::sls {

template <typename ComputeDevice>
class AxdimmComputeUnit : public ComputeUnit<AxdimmComputeUnit<ComputeDevice>> {
  using Parent = ComputeUnit<AxdimmComputeUnit<ComputeDevice>>;
  using DeviceImpl = ComputeDevice;

public:
  AxdimmComputeUnit(device::BaseDevice *device, uint8_t pack)
      : device_(device) {
    const device::cunit ranks_range(pack);
    while (!device_.acquire_cunit_from_range(&cunit_id_, ranks_range)) {
    }

    enable();
  }

  uint8_t id() const { return cunit_id_; }

  void disable() {
    device_.write_control_switch_register(cunit_id_, device::SLS_DISABLE_VALUE);
  }

  void enable() {
    device_.write_control_switch_register(cunit_id_, device::SLS_ENABLE_VALUE);
  }

  ~AxdimmComputeUnit() {
    disable();
    device_.release_cunit(cunit_id_);
  }

  void run() {
    run_async();
    poll(PollType::FINISH);
  }

  void run_async() {
    device_.write_exec_sls_register(cunit_id_, device::SLS_ENABLE_VALUE);
  }

  void wait() { poll(PollType::FINISH); }

  void poll(PollType type) {
    switch (type) {
    case PollType::FINISH:
      Parent::poll_register(device_.psum_poll_register(cunit_id_),
                            device::POLL_FINISH_VALUE);
      break;
    default:
      throw pnm::error::make_not_sup(
          "Poll type: {},  device {}", std::underlying_type_t<PollType>(type),
          std::underlying_type_t<Device::Type>(device_.get_device_type()));
    }
  }

  void read_buffer(BlockType type, pnm::views::common<uint8_t> data) {
    Parent::read_buffer(device_, cunit_id_, type, data);
  }

  void write_buffer(BlockType type, pnm::views::common<const uint8_t> data) {
    Parent::write_buffer(device_, cunit_id_, type, data);
  }

private:
  DeviceImpl device_;
  uint8_t cunit_id_;
};

} // namespace pnm::sls

#define _SLS_AXDIMM_CUNIT_H_
#endif
