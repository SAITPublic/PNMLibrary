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

#ifndef IMDB_PRECISE_RANDOM_PREDICATE_VECS_GEN_H
#define IMDB_PRECISE_RANDOM_PREDICATE_VECS_GEN_H

#include "base.h"

#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/predicate_vecs_info.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <unordered_map>

namespace tools::gen::imdb {

/** @brief Class for generating predicate vectors with precise actual
 * selectivity. */
class PreciseRandomPredicateVecsGen : public IPredicateVecsGenerator {
public:
  PreciseRandomPredicateVecsGen(std::shared_ptr<const compressed_vector> column,
                                size_t seed = std::random_device{}(),
                                double selectivity_epsilon = 0.001);

private:
  Predictors
  create_predicate_vecs_impl(const PredicateVectorsInfo &predicate_info,
                             const ColumnsInfo &column_info) override;

  /** @brief Converts compressed column to map (entry,count of entry).
   *
   * @param expected_size expected size of map
   * @return map
   */
  std::unordered_map<compressed_element_type, size_t>
  calc_entry_to_count_map(size_t expected_size) const;

  /** @brief Calculates entries count after scan operation for range [0,
   * max_value]
   *
   * @param max_value maximal value in predicate vector
   * @param max_entry_value maximal possible value of entry in compressed
   * column (determined by entry_bit_size)
   * @param max_value_to_results_count some kind of cache. If finds in this map
   * - takes value from this map. If not found, then calculates and pushes in
   * this map.
   * @return entries count
   */
  size_t calc_max_results_count(compressed_element_type max_value,
                                compressed_element_type max_entry_value,
                                std::map<compressed_element_type, size_t>
                                    &max_value_to_results_count) const;

  /** @brief Calculates entries count after scan operation for predicate vector
   *
   * @param predictor predicate vector
   * @return entries count
   */
  size_t calc_results_count(const predictor_input_vector &predictor) const;

  /** @brief Calculates difference between actual selectivity and expected
   *
   * @param count_difference difference between entries count after scan
   * operation and expected entries count for given selectivity
   * @return actual selectivity epsilon
   */
  double calc_actual_epsilon(int64_t count_difference) const;

  /** @brief Function of inserting into or erasing values from predicate vector
   * so results count after scan operation would be equal to the
   * expected_results_count with some precision
   *
   * @param expected_results_count expected entries count after scan operation
   * @param predictor predicate vector
   */
  void adjust_predictor(size_t expected_results_count,
                        predictor_input_vector &predictor) const;

  std::shared_ptr<const compressed_vector> column_;
  size_t seed_;
  double selectivity_epsilon_;

  /** @brief key - entry in column, value - count of entry in this
   * column since maximal value of entry is 262143, this map won't be too big
   */
  std::unordered_map<compressed_element_type, size_t> column_entry_count_;
};

} // namespace tools::gen::imdb

#endif // IMDB_PRECISE_RANDOM_PREDICATE_VECS_GEN_H
