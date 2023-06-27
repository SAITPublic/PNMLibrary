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

#ifndef SLS_RANDOM_LEN_GENERATOR_H
#define SLS_RANDOM_LEN_GENERATOR_H

#include "base.h"

#include "common/topology_constants.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <random>
#include <vector>

class RandomLengthsGen : public ILengthsGenerator {
public:
  explicit RandomLengthsGen(int type_of_tables_with_overflow)
      : type_of_tables_with_overflow_(type_of_tables_with_overflow) {}

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

    std::uniform_int_distribution<uint32_t> overflowed_table_id_distrib(
        0, num_tables - 1);

    const uint32_t overflowed_table_id = overflowed_table_id_distrib(engine);

    using pnm::device::topo;

    const uint32_t single_overflowed_table_batch_lookups =
        (topo().InstBufSize / topo().NumOfInstBuf / topo().InstructionSize) - 1;

    const uint32_t single_batch_lookups_min =
        (topo().InstBufSize / topo().InstructionSize / minibatch_size) + 1;

    for (size_t table_idx = 0; table_idx < num_tables; ++table_idx) {
      if (!type_of_tables_with_overflow_) {
        // No tables with overflow
        std::generate_n(
            std::back_inserter(lengths), minibatch_size,
            [&rows_distr, &engine]() { return rows_distr(engine); });
      } else if (type_of_tables_with_overflow_ == -1) {
        // Instruction overflow in one random table
        std::generate_n(std::back_inserter(lengths), minibatch_size,
                        [&rows_distr, &engine, &table_idx, overflowed_table_id,
                         single_overflowed_table_batch_lookups]() {
                          if (overflowed_table_id == table_idx) {
                            return single_overflowed_table_batch_lookups;
                          }
                          return rows_distr(engine);
                        });
      } else if (type_of_tables_with_overflow_ > 0) {
        // Instruction overflow in each 'type_of_tables_with_overflow_'rd table
        std::generate_n(
            std::back_inserter(lengths), minibatch_size,
            [&rows_distr, &engine, &table_idx, single_batch_lookups_min,
             this]() {
              if ((table_idx + 1) % this->type_of_tables_with_overflow_ == 0) {
                // Set lookups for table batches for guaranteed
                // instruction buffer overflow
                return single_batch_lookups_min;
              }
              return rows_distr(engine);
            });
      }
    }

    return lengths;
  }
  // a number that characterizes tables with overflow
  const int type_of_tables_with_overflow_;
};

#endif // SLS_RANDOM_LEN_GENERATOR_H
