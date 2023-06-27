
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

#ifndef SLS_RANDOM_GENERATOR_H
#define SLS_RANDOM_GENERATOR_H

#include "base.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <random>
#include <stdexcept>
#include <vector>

/*! \brief Generate indices for each batch randomly in range [0, table_size) */
class RandomIndicesGen : public IIndicesGenerator {
private:
  std::vector<uint32_t>
  create_table_impl(const IndicesInfo &indices_info,
                    const TablesInfo &tables_info) override {
    std::vector<uint32_t> indices;
    std::mt19937_64 engine{std::random_device{}()};
    auto lengths_iterator = indices_info.lengths().begin();
    for (auto table_size : tables_info.rows()) {
      std::uniform_int_distribution<uint32_t> index_gen{
          0, static_cast<unsigned int>(table_size - 1)};

      for (auto i = 0U; i < indices_info.minibatch_size(); ++i) {
        if (*lengths_iterator >= table_size) {
          throw std::logic_error("Index should be lower than table size.");
        }
        std::generate_n(std::back_inserter(indices), *lengths_iterator,
                        [&engine, &index_gen]() { return index_gen(engine); });
        ++lengths_iterator;
      }
    }

    return indices;
  }
};

#endif // SLS_RANDOM_GENERATOR_H
