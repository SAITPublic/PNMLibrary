/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef SLS_DEFAULT_ENTRY_GENS_H
#define SLS_DEFAULT_ENTRY_GENS_H

#include "tools/datagen/sls/general/traits.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>

/*! \brief Generate random table entry */
class RandomEntry {
public:
  uint32_t operator()([[maybe_unused]] size_t tid, [[maybe_unused]] size_t rid,
                      [[maybe_unused]] size_t cid) {
    return distr_(engine_);
  }

private:
  std::mt19937_64 engine_{std::random_device{}()};
  std::uniform_int_distribution<uint32_t> distr_{
      0, std::numeric_limits<uint16_t>::max()}; // Reduce probability of
                                                // overflow at sls operation
};

/*! \brief Generate position dependent entry */
struct PositionedEntry {
  uint32_t operator()(size_t tid, size_t rid, size_t cid) const {
    return (tid << 16) + (rid << 8) + cid;
  }
};

/*! \brief Generate position dependent float entry. Try to reproduce test_app
 * entries.  */
template <typename T> struct NumericEntry {
public:
  explicit NumericEntry(uint32_t knum_idx_values, uint32_t sparse_feature_size)
      : knum_idx_values_{knum_idx_values},
        sparse_feature_size_{sparse_feature_size} {
    kseed_sparse_feature_val_ = DataTypeTraits<T>().seed_sparse_feature_val;
  }

  T operator()([[maybe_unused]] size_t tid, [[maybe_unused]] size_t rid,
               [[maybe_unused]] size_t cid) {
    if (counter_ % sparse_feature_size_ == 0) {
      sparse_feature_part_ =
          (feature_id_ % knum_idx_values_ + 1) * kseed_sparse_feature_val_;
      ++feature_id_;
    }
    ++counter_;
    return sparse_feature_part_;
  }

private:
  size_t feature_id_ = 0;
  size_t counter_ = 0;
  T kseed_sparse_feature_val_;
  T sparse_feature_part_ = 0;
  uint32_t knum_idx_values_;
  uint32_t sparse_feature_size_;
};

struct FixedEntry {
  uint32_t value;
  explicit FixedEntry(uint32_t v) : value{v} {}
  uint32_t operator()([[maybe_unused]] size_t tid, [[maybe_unused]] size_t rid,
                      [[maybe_unused]] size_t cid) const {
    return value;
  }
};

#endif // SLS_DEFAULT_ENTRY_GENS_H
