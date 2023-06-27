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

#ifndef TABLE_COLUMN_ENTRY_GENS_H
#define TABLE_COLUMN_ENTRY_GENS_H

#include "common/log.h"

#include "pnmlib/imdb/scan_types.h"

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <random>
#include <stdexcept>

namespace tools::gen::imdb {

using namespace pnm::imdb;

namespace utils {

inline compressed_element_type calc_entry_max_value(uint64_t entry_bit_size) {
  return (1U << entry_bit_size) - 1;
}

inline void validate_entry_gen_args(uint64_t entry_bit_size,
                                    compressed_element_type min_value,
                                    compressed_element_type max_value) {
  const auto entry_max_value = calc_entry_max_value(entry_bit_size);
  if (min_value > entry_max_value || max_value > entry_max_value) {
    throw std::invalid_argument(
        "Minimal and max values must be less than 2^entry_bit_size");
  }
  if (min_value > max_value) {
    throw std::invalid_argument(
        "Minimal value must be less than or equal to max value");
  }
}
} // namespace utils

/** @brief Generate random column entry */
struct RandomEntry {

  explicit RandomEntry(
      uint64_t entry_bit_size, size_t seed = std::random_device{}(),
      std::optional<compressed_element_type> min_value = std::nullopt,
      std::optional<compressed_element_type> max_value = std::nullopt)
      : distr_{std::uniform_int_distribution<compressed_element_type>(
            min_value.value_or(0),
            max_value.value_or(utils::calc_entry_max_value(entry_bit_size)))},
        engine_{seed} {
    utils::validate_entry_gen_args(entry_bit_size, distr_.min(), distr_.max());
    pnm::log::info("RandomEntry created with seed {}", seed);
  }

  compressed_element_type operator()([[maybe_unused]] index_type index) {
    return distr_(engine_);
  }

private:
  std::uniform_int_distribution<compressed_element_type> distr_;
  std::mt19937_64 engine_;
};

/** @brief Generate position dependent entry.
 *   Values will be generated in range [min_value_, max_value] with step.
 *   For example,
 *   for range [1,10] with step 4 result will be {1, 5, 9, 4, 8, 3, 7 ... }
 *   for range [1,10] with step -1 result will be {10, 9, 8, 7, 6 ... }.
 *   entry_bit_size is used for validation or filling default max_value.
 */
struct PositionedEntry {
  explicit PositionedEntry(
      uint64_t entry_bit_size, int32_t step = 1,
      std::optional<compressed_element_type> min_value = std::nullopt,
      std::optional<compressed_element_type> max_value = std::nullopt)
      : min_value_{min_value.value_or(0U)},
        max_value_{
            max_value.value_or(utils::calc_entry_max_value(entry_bit_size))},
        length_{max_value_ > min_value_ ? max_value_ - min_value_ + 1 : 0},
        step_(length_ != 0 ? step % length_ : 0) {
    utils::validate_entry_gen_args(entry_bit_size, min_value_, max_value_);
  }

  compressed_element_type min_value_;
  compressed_element_type max_value_;
  int64_t length_;
  int64_t step_;

  compressed_element_type operator()(index_type index) const {
    if (min_value_ == max_value_) {
      return min_value_;
    }

    // overflow safe expression { start_value + (step * index) % length }
    const int64_t mult = static_cast<int64_t>(index) * step_;
    const uint32_t mod_value = std::abs(mult) % length_;
    return step_ >= 0 ? min_value_ + mod_value : max_value_ - mod_value;
  }
};
} // namespace tools::gen::imdb

#endif // TABLE_COLUMN_ENTRY_GENS_H
