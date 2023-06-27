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

#ifndef IMDB_TRIVIAL_TABLE_COLUMN_GENERATOR_H
#define IMDB_TRIVIAL_TABLE_COLUMN_GENERATOR_H

#include "base.h"

#include "tools/datagen/imdb/general/columns_info.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <utility>

namespace tools::gen::imdb {

using namespace pnm::imdb;

/**
  @brief Template for trivial column generator with variable generation
  strategy
*/
template <typename EntryGen>
class TrivialColumnGenerator : public IColumnGenerator {
public:
  TrivialColumnGenerator() = default;

  explicit TrivialColumnGenerator(EntryGen gen)
      : entry_generator_{std::move(gen)} {}

private:
  compressed_vector create_column_impl(const ColumnsInfo &info) override {
    compressed_vector column(info.entry_bit_size(), info.entry_count());

    for (size_t index = 0; index < info.entry_count(); ++index) {
      column.set_value(index, entry_generator_(index));
    }

    return column;
  }

  EntryGen entry_generator_;
};

} // namespace tools::gen::imdb

#endif // IMDB_TRIVIAL_TABLE_COLUMN_GENERATOR_H
