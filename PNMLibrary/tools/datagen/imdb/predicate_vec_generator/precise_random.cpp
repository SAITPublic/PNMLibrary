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

#include "precise_random.h"

#include "random.h"

#include "common/log.h"

#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/predicate_vecs_info.h"

#include "pnmlib/imdb/bit_containers.h"
#include "pnmlib/imdb/scan_types.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tools::gen::imdb {

PreciseRandomPredicateVecsGen::PreciseRandomPredicateVecsGen(
    std::shared_ptr<const compressed_vector> column, size_t seed,
    double selectivity_epsilon)
    : column_{std::move(column)}, seed_{seed},
      selectivity_epsilon_{std::fabs(selectivity_epsilon)} {
  pnm::log::info("PreciseRandomPredicateVecsGen created with seed {}", seed_);
}

Predictors PreciseRandomPredicateVecsGen::create_predicate_vecs_impl(
    const PredicateVectorsInfo &predicate_info,
    const ColumnsInfo &column_info) {

  // Needed in order not to recalculate count for the same max values
  // several times. Key - max value of predicate vector, value - how many
  // elements of column are in range [0, max_value]
  std::map<compressed_element_type, size_t> max_value_to_results_count;
  column_entry_count_.clear();

  auto generator = RandomPredicateVecsGen(seed_);
  Predictors predicate_vectors = generator.create(predicate_info, column_info);
  assert(predicate_vectors.size() == predicate_info.selectivities().size());
  assert(predicate_vectors.size() ==
         predicate_info.predicate_max_values().size());

  const compressed_element_type max_entry_value =
      (1U << column_info.entry_bit_size()) - 1;
  column_entry_count_ = calc_entry_to_count_map(max_entry_value + 1);

  for (size_t i = 0; i < predicate_info.num_requests(); ++i) {
    const auto selectivity = predicate_info.selectivities()[i];
    const auto max_value = predicate_info.predicate_max_values()[i];
    const auto expected_results_count =
        static_cast<size_t>(std::round(selectivity * column_->size()));
    const size_t max_results_count = calc_max_results_count(
        max_value, max_entry_value, max_value_to_results_count);

    auto &predictor = predicate_vectors[i];

    if (max_results_count < expected_results_count) {
      for (compressed_element_type i = 0U; i <= max_value; ++i) {
        predictor.insert(i);
      }
      continue;
    }

    adjust_predictor(expected_results_count, predictor);
  }

  column_entry_count_.clear();
  return predicate_vectors;
}

std::unordered_map<compressed_element_type, size_t>
PreciseRandomPredicateVecsGen::calc_entry_to_count_map(
    size_t expected_size) const {
  std::unordered_map<compressed_element_type, size_t> entry_to_count;
  entry_to_count.reserve(expected_size);
  for (auto value : *column_) {
    ++entry_to_count[value];
  }
  return entry_to_count;
}

size_t PreciseRandomPredicateVecsGen::calc_max_results_count(
    compressed_element_type max_value, compressed_element_type max_entry_value,
    std::map<compressed_element_type, size_t> &max_value_to_results_count)
    const {
  if (max_entry_value == max_value) {
    return column_->size();
  }

  if (auto it = max_value_to_results_count.find(max_value);
      it != max_value_to_results_count.end()) {
    return it->second;
  }

  size_t max_results_count = 0;
  for (compressed_element_type value = 0; value <= max_value; ++value) {
    if (auto it = column_entry_count_.find(value);
        it != column_entry_count_.end()) {
      max_results_count += it->second;
    }
  }

  max_value_to_results_count[max_value] = max_results_count;
  return max_results_count;
}

size_t PreciseRandomPredicateVecsGen::calc_results_count(
    const predictor_input_vector &predictor) const {
  size_t count = 0;
  for (const auto &value : predictor) {
    if (auto it = column_entry_count_.find(value);
        it != column_entry_count_.end()) {
      count += it->second;
    }
  }

  return count;
}

double PreciseRandomPredicateVecsGen::calc_actual_epsilon(
    int64_t count_difference) const {
  return std::fabs(count_difference / static_cast<double>(column_->size()));
}

void PreciseRandomPredicateVecsGen::adjust_predictor(
    size_t expected_results_count, predictor_input_vector &predictor) const {
  const auto results_count = calc_results_count(predictor);
  int64_t count_diff =
      results_count > expected_results_count
          ? static_cast<int64_t>(results_count - expected_results_count)
          : -static_cast<int64_t>(expected_results_count - results_count);

  if (calc_actual_epsilon(count_diff) < selectivity_epsilon_) {
    return;
  }

  for (auto [column_entry, entry_count] : column_entry_count_) {
    if (column_entry >= predictor.size()) {
      continue;
    }

    int64_t next_count_diff;
    if (count_diff < 0) {
      if (predictor.contains(column_entry)) {
        continue;
      }
      next_count_diff = count_diff + static_cast<int64_t>(entry_count);
    } else {
      if (!predictor.contains(column_entry)) {
        continue;
      }
      next_count_diff = count_diff - static_cast<int64_t>(entry_count);
    }

    if (std::abs(next_count_diff) < std::abs(count_diff)) {
      count_diff < 0 ? predictor.insert(column_entry)
                     : predictor.erase(column_entry);
      count_diff = next_count_diff;
    }

    if (calc_actual_epsilon(count_diff) < selectivity_epsilon_) {
      break;
    }
  }
}
} // namespace tools::gen::imdb
