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

#ifndef _SLS_CUNIT_H_

#include "core/device/sls/base.h"

#include "common/make_error.h"

#include "pnmlib/core/device.h"

#include "pnmlib/common/views.h"

#include <cstdint>
#include <type_traits>

namespace pnm::sls {

enum class BlockType {
  INST,
  TAGS,
  PSUM,
};

enum class PollType {
  ENABLE,
  FINISH,
  CONTROL_SWITCH,
};

template <typename Impl> class ComputeUnit {
public:
  uint8_t id() { return impl()->id(); }

  void enable() { impl()->enable(); }

  void disable() { impl()->disable(); }

  void run() { impl()->run(); }

  void run_async() { impl()->run_async(); }

  void wait() { impl()->wait(); }

  void poll(PollType type) { impl()->poll(type); }

  void read_buffer(BlockType type, pnm::common_view<uint8_t> buf) {
    impl()->read_buffer(type, buf);
  }

  void write_buffer(BlockType type, pnm::common_view<const uint8_t> buf) {
    impl()->write_buffer(type, buf);
  }

protected:
  void read_buffer(device::BaseDevice *device, uint8_t cunit_id, BlockType type,
                   pnm::common_view<uint8_t> data) {
    switch (type) {
    case BlockType::PSUM:
      device->read_and_reset_psum(cunit_id, data.begin(), data.size());
      break;
    case BlockType::TAGS:
      device->read_and_reset_tags(cunit_id, data.begin(), data.size());
      break;
    default:
      throw pnm::error::make_not_sup(
          "Read operation for block type: {}, device {}",
          std::underlying_type_t<BlockType>(type),
          std::underlying_type_t<Device::Type>(device->get_device_type()));
    }
  }

  void write_buffer(device::BaseDevice *device, uint8_t cunit_id,
                    BlockType type, pnm::common_view<const uint8_t> data) {
    switch (type) {
    case BlockType::INST:
      device->write_inst_block(cunit_id, data.data(), data.size());
      break;
    default:
      throw pnm::error::make_not_sup(
          "Write operation for block type: {}, device {}",
          std::underlying_type_t<BlockType>(type),
          std::underlying_type_t<Device::Type>(device->get_device_type()));
    }
  }

private:
  inline Impl *impl() { return static_cast<Impl *>(this); }

  inline const Impl *impl() const { return static_cast<const Impl *>(this); }
};

} // namespace pnm::sls

#define _SLS_CUNIT_H_
#endif
