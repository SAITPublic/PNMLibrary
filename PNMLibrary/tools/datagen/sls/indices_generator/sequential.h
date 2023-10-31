
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

#ifndef SLS_SEQUENTIAL_GENERATOR_H
#define SLS_SEQUENTIAL_GENERATOR_H

#include "base.h"

#include "tools/datagen/sls/general/indices_info.h"
#include "tools/datagen/sls/general/tables_info.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace tools::gen::sls {

/*! \brief Generate indices for each batch as sequence of number from 0 to
 * table_size * max_index_fraction_ */
class SequentialIndicesGen : public IIndicesGenerator {
private:
  std::vector<uint32_t>
  create_table_impl(const IndicesInfo &indices_info,
                    const TablesInfo &tables_info) override {
    std::vector<uint32_t> indices;
    auto lengths_iterator = indices_info.lengths().begin();
    for (auto table_size : tables_info.rows()) {
      for (auto i = 0U; i < indices_info.minibatch_size(); ++i) {
        if (*lengths_iterator >= table_size) {
          throw std::logic_error("Index should be lower than table size.");
        }
        auto k = 0ULL;
        std::generate_n(std::back_inserter(indices), *lengths_iterator,
                        [&k]() { return k++; });
        ++lengths_iterator;
      }
    }
    return indices;
  }
};

} // namespace tools::gen::sls

#endif // SLS_SEQUENTIAL_GENERATOR_H
