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

#ifndef SLS_DUPLICATE_RANDOM_LEN_GENERATOR_H
#define SLS_DUPLICATE_RANDOM_LEN_GENERATOR_H

#include "base.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

class DuplicateRandomLengthsGen : public ILengthsGenerator {
private:
  std::vector<uint32_t>
  create_lengths_impl(size_t num_tables, std::optional<size_t> min_num_lookup,
                      size_t max_num_lookup, size_t minibatch_size) override {

    std::vector<uint32_t> lengths;

    const size_t num_requests = num_tables * minibatch_size;

    lengths.reserve(num_requests);

    std::mt19937_64 engine{std::random_device{}()};

    auto real_min_num_lookup =
        min_num_lookup ? *min_num_lookup : max_num_lookup;

    std::uniform_int_distribution<uint32_t> rows_distr(real_min_num_lookup,
                                                       max_num_lookup);

    std::vector<size_t> lookups_per_table(num_tables);

    std::generate(lookups_per_table.begin(), lookups_per_table.end(),
                  [&engine, &rows_distr]() { return rows_distr(engine); });

    for (const auto &lookups : lookups_per_table) {
      lengths.insert(lengths.end(), minibatch_size, lookups);
    }

    return lengths;
  }
};

#endif // SLS_DUPLICATE_RANDOM_LEN_GENERATOR_H
