//=- core/device/imdb/base.h - IMDB device common ops -*- C++-*-----=//

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

#ifndef _IMDB_BASE_DEVICE_H_
#define _IMDB_BASE_DEVICE_H_

#include "control.h"
#include "reg_mem.h"

#include "common/compiler_internal.h"
#include "common/file_descriptor_holder.h"
#include "common/mapped_file.h"

#include "pnmlib/imdb/libimdb.h"

#include "pnmlib/core/device.h"

#include <linux/imdb_resources.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace pnm::imdb::device {

class BaseDevice : public pnm::Device {
public:
  static std::unique_ptr<BaseDevice> make_device();

  volatile ThreadCSR *get_csr(uint8_t thread_id);
  const volatile ThreadCSR *get_csr(uint8_t thread_id) const;
  void dump_thread_csr(uint8_t thread_id) const;

  RegisterPointers register_pointers() const { return regs_; }

  Type get_device_type() const final { return Type::IMDB_CXL; }

  int get_devmem_fd() const final { return *devmem_; }
  int get_resource_fd() const final { return *resmanager_; }

  uint8_t lock_thread();

  void release_thread(uint8_t thread);

  auto alignment() const { return control_.alignment(); }

  auto memory_size() const { return control_.memory_size(); }

  auto free_size() const { return control_.free_size(); }

  bool is_thread_busy(uint8_t thread) const {
    return control_.thread_state(thread) == IMDB_THREAD_BUSY;
  }

protected:
  BaseDevice(const std::filesystem::path &devcontrol_path,
             const std::filesystem::path &devmem_path);

  template <typename T> T *get_regs_section(size_t offset) {
    auto *bytes = static_cast<uint8_t *>(devcontrol_.data());
    return reinterpret_cast<T *>(bytes + offset);
  }

  void reset_impl(ResetOptions options) final;
  virtual uint8_t lock_thread_impl();
  virtual void release_thread_impl(uint8_t thread_id);

  bool get_resource_cleanup_impl() const override {
    return control_.get_resource_cleanup();
  }

  uint64_t get_leaked_impl() const override { return control_.get_leaked(); }

  void set_resource_cleanup_impl(bool state) const override {
    control_.set_resource_cleanup(state);
  }

  void hardware_reset() const;
  void software_reset();

  RegisterPointers regs_{};
  // Device control registers
  pnm::utils::MappedFile devcontrol_;
  // Device working memory
  pnm::utils::FileDescriptorHolder devmem_;
  pnm::utils::FileDescriptorHolder resmanager_;
  std::array<pnm::utils::tsan_mutex, IMDB_THREAD_NUM> tsan_mutexes_;
  Control control_;
};

} // namespace pnm::imdb::device

#endif /* _IMDB_BASE_DEVICE_H_ */
