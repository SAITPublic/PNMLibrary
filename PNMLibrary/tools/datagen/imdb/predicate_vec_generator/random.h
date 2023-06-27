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

#ifndef IMDB_RANDOM_PREDICATE_VECS_GEN_H
#define IMDB_RANDOM_PREDICATE_VECS_GEN_H

#include "base.h"

#include "common/log.h"

#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/generators_utils.h"
#include "tools/datagen/imdb/general/predicate_vecs_info.h"

#include "pnmlib/imdb/scan_types.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace tools::gen::imdb {
/** @brief Class for relatively fast random predicate vectors generating */
class RandomPredicateVecsGen : public IPredicateVecsGenerator {
public:
  RandomPredicateVecsGen(size_t seed = std::random_device{}()) : seed_(seed) {
    pnm::log::info("RandomPredicateVecsGen created with seed {}", seed_);
  }

private:
  Predictors
  create_predicate_vecs_impl(const PredicateVectorsInfo &predicate_info,
                             const ColumnsInfo &column_info) override {
    Predictors predicate_vectors;
    predicate_vectors.reserve(predicate_info.num_requests());
    std::mt19937_64 engine{seed_};

    assert(predicate_info.predicate_max_values().size() ==
           predicate_info.selectivities().size());
    assert(predicate_info.predicate_max_values().size() ==
           predicate_info.num_requests());

    const auto column_range_size = (1ULL << column_info.entry_bit_size());
    for (size_t i = 0; i < predicate_info.num_requests(); ++i) {
      const auto selectivity = predicate_info.selectivities()[i];
      const auto max_value = predicate_info.predicate_max_values()[i];
      std::uniform_int_distribution<uint32_t> uni(0, max_value);

      const auto request_range_size = value_per_request_count(
          column_range_size, column_info.entry_count(), selectivity);
      const auto correction_coef = 1 + selectivity;
      // works better for some reason
      // can be improved more if distribution properties from mathematical
      // statistics are taken into account
      const auto corrected_range_size =
          static_cast<size_t>(correction_coef * request_range_size);

      predictor_input_vector predictor(max_value + 1);
      predictor.insert(max_value);
      for (auto i = 1UL; i < corrected_range_size; ++i) {
        predictor.insert(uni(engine));
      }
      predicate_vectors.push_back(std::move(predictor));
    }

    return predicate_vectors;
  }

  size_t seed_;
};

} // namespace tools::gen::imdb

#endif // IMDB_RANDOM_PREDICATE_VECS_GEN_H
