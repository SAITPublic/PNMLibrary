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

#ifndef _SLS_RANK_MEMORY_H_
#define _SLS_RANK_MEMORY_H_

#include "common/topology_constants.h"

#include "pnmlib/core/memory_object.h"

#include "pnmlib/common/views.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <numeric>
#include <vector>

namespace pnm::sls::device {

struct RankMemory {
  /* Pack is an aggregation of objects and single values --- pack-id.
   *
   * Pack-ID is a NUM_OF_RANK-size bitmask that represent which ranks are be
   * able to store or process related objects. The active bit at position I mean
   * that all object in this pack will be stored in I rank and this rank can
   * perform SLS operation.
   *
   * Pack-ID "b0101" shows that all related objects can be stored at ranks 0
   * and 2.
   *
   * Currently, NUM_OF_RANK = 4, so the total number of rank bitmasks is
   * 1 << NUM_OF_RANK == 16 for 4 ranks, mask range: 0b0000 - 0b1111.
   */
  static auto num_of_pack() {
    static auto num_of_pack_v = 1ULL << pnm::device::topo().NumOfRanks;
    return num_of_pack_v;
  }

  static auto &pack_identifiers() {
    static std::vector<uint64_t> id = []() {
      std::vector<uint64_t> id(num_of_pack() - 1);
      std::iota(id.begin(), id.end(), 1);
      return id;
    }();

    return id;
  }

  template <typename T> class packs_array {
  public:
    packs_array() : data_(num_of_pack()) {}

    explicit packs_array(std::vector<T> data) : data_{std::move(data)} {
      assert(data_.size() == num_of_pack());
    }

    auto &operator[](size_t index) { return data_[index]; }

    const auto &operator[](size_t index) const { return data_[index]; }

    auto cbegin() const { return data_.cbegin(); }
    auto cend() const { return data_.cend(); }

    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }

  private:
    std::vector<T> data_;
  };

  using mem_objects_vector = std::vector<pnm::memory::MemoryObject>;
  using packs_to_objects = packs_array<mem_objects_vector>;
  using object_write_map =
      std::map<uint64_t /*rank offset*/,
               pnm::common_view<const uint8_t> /*user data range*/>;

  static uint64_t
  get_max_object_size_in_bytes(const mem_objects_vector &objects) {
    return objects.empty()
               ? 0LU
               : std::max_element(objects.cbegin(), objects.cend(),
                                  [](const auto &lhs, const auto &rhs) {
                                    return lhs.bytes() < rhs.bytes();
                                  })
                     ->bytes();
  }
  // across all objects in all ranks
  static uint64_t get_max_object_size_in_bytes(const packs_to_objects &packs) {
    uint64_t max_object_size = 0;
    std::for_each(
        packs.cbegin(), packs.cend(), [&max_object_size](const auto &elm) {
          max_object_size =
              std::max(max_object_size, get_max_object_size_in_bytes(elm));
        });
    return max_object_size;
  }
  static uint64_t get_objects_size_in_bytes(const mem_objects_vector &objects) {
    return std::accumulate(
        objects.cbegin(), objects.cend(), 0LU,
        [](auto sum, const auto &elm) { return sum + elm.bytes(); });
  }
  // sum size of all objects on all ranks
  static uint64_t get_objects_size_in_bytes(const packs_to_objects &packs) {
    return std::accumulate(packs.cbegin(), packs.cend(), 0LU,
                           [](auto sum, const auto &elm) {
                             return sum + get_objects_size_in_bytes(elm);
                           });
  }
};

} // namespace pnm::sls::device
#endif /* _SLS_USER_ADDRESS_H_ */
