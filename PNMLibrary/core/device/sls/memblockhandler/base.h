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

#ifndef _SLS_BASE_MEMBLOCKHANDLER_H_
#define _SLS_BASE_MEMBLOCKHANDLER_H_

#include "core/device/sls/control.h"
#include "core/device/sls/memory_map.h"

#include "common/make_error.h"

#include "pnmlib/core/device.h"

#include <linux/sls_resources.h>

#include <cstdint>
#include <cstdlib>
#include <memory>

namespace pnm::sls::device {

class ISLSMemBlockHandler {
public:
  //[TODO: s-motov] Move init function to ctor
  void init(const MemInfo &info, int device_fd, Device::Type devtype);

  void write_block(sls_mem_blocks_e type, uint8_t compute_unit, uint32_t offset,
                   const uint8_t *buf_in, size_t write_size);
  void read_block(sls_mem_blocks_e type, uint8_t compute_unit, uint32_t offset,
                  uint8_t *buf_out, size_t read_size);

  void *get_mem_block_ptr(sls_mem_blocks_e type, uint8_t compute_unit,
                          uint32_t offset);
  size_t get_min_block_size(sls_mem_blocks_e block_type) const;
  size_t get_base_memory_size() const;

  virtual ~ISLSMemBlockHandler() = default;

  static std::unique_ptr<ISLSMemBlockHandler> create(Device::Type devtype);

protected:
  virtual void write_block_impl(const memAddr &mem_addr, uint8_t compute_unit,
                                sls_mem_blocks_e block_type, uint32_t offset,
                                const uint8_t *buf_in, size_t write_size) = 0;
  virtual void read_block_impl(const memAddr &mem_addr, uint8_t compute_unit,
                               sls_mem_blocks_e block_type, uint32_t offset,
                               uint8_t *buf_out, size_t read_size) = 0;

  virtual uint8_t num_compute_units() const = 0;

  virtual void *get_block_ptr(sls_mem_blocks_e type, uint8_t compute_unit,
                              uint32_t offset) = 0;

  MemMap &mem_map() noexcept { return mem_map_; };

private:
  MemMap mem_map_;
};

template <typename SLSMemBlockHandler>
class SLSMemBlockHandlerImpl : public ISLSMemBlockHandler {
private:
  void write_block_impl(const memAddr &mem_addr, uint8_t compute_unit,
                        sls_mem_blocks_e block_type, uint32_t offset,
                        const uint8_t *buf_in, size_t write_size) final {
    auto *derived = this->derived();

    switch (block_type) {
    case SLS_BLOCK_BASE:
      // Base block does not needed, because copying logic was moved to transfer
      // manager
      break;
    case SLS_BLOCK_INST:
      derived->inst_writer()(mem_addr, compute_unit, block_type, offset, buf_in,
                             write_size);
      return;
    case SLS_BLOCK_CFGR:
      derived->cfgr_writer()(mem_addr, compute_unit, block_type, offset, buf_in,
                             write_size);
      return;
    case SLS_BLOCK_TAGS:
      derived->tags_writer()(mem_addr, compute_unit, block_type, offset, buf_in,
                             write_size);
      return;
    case SLS_BLOCK_PSUM:
      derived->psum_writer()(mem_addr, compute_unit, block_type, offset, buf_in,
                             write_size);
      return;
    case SLS_BLOCK_MAX:
      break;
    }

    throw pnm::error::make_inval("Block type");
  }

  void read_block_impl(const memAddr &mem_addr, uint8_t compute_unit,
                       sls_mem_blocks_e block_type, uint32_t offset,
                       uint8_t *buf_out, size_t read_size) final {
    auto *derived = this->derived();

    switch (block_type) {
    case SLS_BLOCK_BASE:
      // Base block does not needed, because copying logic was moved to transfer
      // manager
      break;
    case SLS_BLOCK_INST:
      // We don't have logic for reading instructions
      break;
    case SLS_BLOCK_CFGR:
      derived->cfgr_reader()(mem_addr, compute_unit, block_type, offset,
                             buf_out, read_size);
      return;
    case SLS_BLOCK_TAGS:
      derived->tags_reader()(mem_addr, compute_unit, block_type, offset,
                             buf_out, read_size);
      return;
    case SLS_BLOCK_PSUM:
      derived->psum_reader()(mem_addr, compute_unit, block_type, offset,
                             buf_out, read_size);
      return;
    case SLS_BLOCK_MAX:
      break;
    }

    throw pnm::error::make_inval("Block type");
  };

  uint8_t num_compute_units() const final {
    return this->derived()->num_compute_units_impl();
  }

  void *get_block_ptr(sls_mem_blocks_e type, uint8_t compute_unit,
                      uint32_t offset) final {
    return this->derived()->get_block_ptr_impl(type, compute_unit, offset);
  }

protected:
  auto *derived() { return static_cast<SLSMemBlockHandler *>(this); }

  const auto *derived() const {
    return static_cast<const SLSMemBlockHandler *>(this);
  }
};

} // namespace pnm::sls::device

#endif //_SLS_BASE_MEMBLOCKHANDLER_H_
