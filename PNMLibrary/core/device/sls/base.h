//=- core/device/sls/base.h - sls device base ops -*- C++-*---=//

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

#ifndef _SLS_BASE_DEVICE_H_
#define _SLS_BASE_DEVICE_H_

#include "constants.h"
#include "control.h"
#include "memblockhandler/base.h"
#include "rank_memory.h"

#include "common/compiler_internal.h"
#include "common/file_descriptor_holder.h"
#include "common/timer.h"
#include "common/topology_constants.h"

#include "pnmlib/core/device.h"
#include "pnmlib/core/sls_cunit_info.h"

#include <linux/sls_resources.h>

#include <atomic>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

#define SINGLE_DOUBLE                                                          \
  1 /* Temporary SBDB RTL image is used ([TODO:] it should be erased later) */

namespace pnm::sls::device {

class BaseDevice : public Device {
public:
  static constexpr auto bit_mask_size = 64ULL;
  using bit_mask = std::bitset<bit_mask_size>;

  // Set of predefined values to control SLS device
  // For detail see confluence page:
  // https://confluence.samsungds.net/display/NP/SLS+operation+on+CXL+device
  static constexpr uint32_t SLS_ENABLE_VALUE = 0xCAFE;
  static constexpr uint32_t SLS_DISABLE_VALUE = 0;
  static constexpr uint32_t SLS_CONTROL_SWITCHED_VALUE = 0x10001;

  BaseDevice(Device::Type dev_type, const std::filesystem::path &data_path);

  ~BaseDevice() override = default;
  BaseDevice(const BaseDevice &) = delete;
  BaseDevice &operator=(const BaseDevice &) = delete;
  static std::unique_ptr<BaseDevice> make_device(Device::Type dev_type);

  Type get_device_type() const final { return dev_type_; }

  const auto &mem_block_handler() const { return mem_block_handler_; }

  volatile std::atomic<uint32_t> *control_switch_register(uint8_t compute_unit);
  volatile std::atomic<uint32_t> *exec_sls_register(uint8_t compute_unit);
  volatile std::atomic<uint32_t> *
  psum_poll_register(uint8_t compute_unit) const;

  void write_inst_block(uint8_t compute_unit, const uint8_t *buf_in,
                        size_t write_size) {
    MEASURE_TIME();
    mem_block_handler_->write_block(SLS_BLOCK_INST, compute_unit, 0, buf_in,
                                    write_size);
  }
  void write_cfgr_block(uint8_t compute_unit, uint32_t offset,
                        const uint8_t *buf_in, size_t write_size) {
    mem_block_handler_->write_block(SLS_BLOCK_CFGR, compute_unit, offset,
                                    buf_in, write_size);
  }
  void read_cfgr_block(uint8_t compute_unit, uint32_t offset, uint8_t *buf_out,
                       size_t read_size) {
    mem_block_handler_->read_block(SLS_BLOCK_CFGR, compute_unit, offset,
                                   buf_out, read_size);
  }

  // after receiving PSUM buffer is reset by H/W
  void read_and_reset_psum(uint8_t compute_unit, uint8_t *psum_out,
                           size_t read_size) {
    MEASURE_TIME();
    mem_block_handler_->read_block(SLS_BLOCK_PSUM, compute_unit, 0, psum_out,
                                   read_size);
  }
  // after receiving TAGS buffer is reset by H/W
  void read_and_reset_tags(uint8_t compute_unit, uint8_t *tags_out,
                           size_t read_size);

  size_t get_min_block_size(sls_mem_blocks_e block_type) const {
    return mem_block_handler_->get_min_block_size(block_type);
  }
  size_t get_base_memory_size() const {
    return mem_block_handler_->get_base_memory_size();
  }

  int get_devmem_fd() const final { return *data_fd_; }

  int get_resource_fd() const final { return *control_fd_; }

  auto get_acquisition_timeout() const {
    return control_.get_acquisition_timeout();
  }

  void set_acquisition_timeout(uint64_t value) {
    control_.set_acquisition_timeout(value);
  }

  auto get_compute_unit_info(uint8_t compute_unit,
                             SlsComputeUnitInfo key) const {
    return control_.get_compute_unit_info(compute_unit, key);
  }

  void read_poll(uint8_t compute_unit, uint32_t offset, uint8_t *buf_out,
                 size_t read_size) {
    read_cfgr_block(compute_unit, topo().RegPolling + offset, buf_out,
                    read_size);
  }
  void get_num_processed_inst(uint8_t compute_unit, uint8_t *buf_out) {
    read_poll(compute_unit, POLL_INST_NUM_OFFSET, buf_out, POLL_INST_NUM_SIZE);
  }

  bool acquire_cunit_for_write_from_range(uint8_t *compute_unit,
                                          bit_mask cunit_mask);
  bool acquire_cunit_for_read(uint8_t compute_unit) const;
  void release_cunit_for_write(uint8_t compute_unit) const;
  void release_cunit_for_read(uint8_t compute_unit) const;

private:
  using packs_to_objects = RankMemory::packs_to_objects;

  pnm::utils::FileDescriptorHolder control_fd_;
  pnm::utils::FileDescriptorHolder data_fd_;

  std::unique_ptr<ISLSMemBlockHandler> mem_block_handler_;

  uint8_t acquire_idle_cunit_from_range(bit_mask cunit_mask) const;

  void setup_sim();

  mutable std::vector<pnm::utils::tsan_mutex> tsan_mutexes_;

  // [TODO: MCS23-1393] Remove these functions
  std::function<void(uint8_t)> exec_trace_;
  std::function<void(uint8_t)> clear_buffer_;

protected:
  static constexpr size_t POLL_INST_NUM_SIZE = 4;
  Control control_;
  const Device::Type dev_type_;

  void reset_impl(ResetOptions options) final;

  void set_resource_cleanup_impl(bool state) const final {
    control_.set_resource_cleanup(state);
  }

  bool get_resource_cleanup_impl() const final {
    return control_.get_resource_cleanup();
  }

  uint64_t get_leaked_impl() const final { return control_.get_leaked(); }
};

} // namespace pnm::sls::device

#endif /* _SLS_BASE_DEVICE_H_ */
