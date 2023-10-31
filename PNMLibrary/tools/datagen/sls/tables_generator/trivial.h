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

#ifndef SLS_TRIVIAL_GENERATOR_H
#define SLS_TRIVIAL_GENERATOR_H

#include "base.h"

#include "tools/datagen/sls/general/tables_info.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace tools::gen::sls {

/*! \brief Template for trivial tables generator with variable generation
 * strategy */
template <typename T, typename EntryGen>
class TrivialTablesGenerator : public ITablesGenerator {
public:
  TrivialTablesGenerator() = default;

  template <typename... Args>
  explicit TrivialTablesGenerator(Args &&...args)
      : entry_generator_{std::forward<Args>(args)...} {}

private:
  std::vector<uint8_t> create_table_impl(const TablesInfo &info) override {
    std::vector<T> table;

    for (size_t tid = 0; tid < info.num_tables(); ++tid) {
      for (size_t rid = 0; rid < info.rows()[tid]; ++rid) {
        for (size_t cid = 0; cid < info.cols(); ++cid) {
          table.emplace_back(entry_generator_(tid, rid, cid));
        }
      }
    }
    return {reinterpret_cast<uint8_t *>(table.data()),
            reinterpret_cast<uint8_t *>(table.data() + table.size())};
  }

  EntryGen entry_generator_;
};

} // namespace tools::gen::sls

#endif // SLS_TRIVIAL_GENERATOR_H
