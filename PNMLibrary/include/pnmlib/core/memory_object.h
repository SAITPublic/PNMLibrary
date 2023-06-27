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

#ifndef _PNM_MEMORY_OBJECT_H_
#define _PNM_MEMORY_OBJECT_H_

#include <linux/pnm_sls_mem_topology.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <utility>

namespace pnm::memory {

/** @brief Represents table object for sls operation */
class MemoryObject {
public:
  MemoryObject(uint64_t object_id, uint64_t size_bytes)
      : object_id_(object_id), size_bytes_(size_bytes) {}

  [[nodiscard]] auto id() const noexcept { return object_id_; }
  [[nodiscard]] auto bytes() const noexcept { return size_bytes_; }
  [[nodiscard]] auto offset(uint8_t rank_id) const {
    auto [is_valid, rank_offset] = offsets_[rank_id];
    assert(is_valid);
    return rank_offset;
  }

  void set_offset(uint8_t rank_id, uint64_t offset) {
    assert(rank_id < offsets_.size());
    offsets_[rank_id] = {true, offset};
  }

private:
  //! User id of object
  uint64_t object_id_;
  //! Size of object in bytes
  uint64_t size_bytes_;
  //! Offsets of object in each rank
  std::array<std::pair<bool, uint64_t>, NUM_OF_RANK> offsets_ = {};
};

} // namespace pnm::memory

#endif
