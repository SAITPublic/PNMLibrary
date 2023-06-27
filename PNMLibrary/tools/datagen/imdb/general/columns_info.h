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

#ifndef IMDB_TABLE_COLUMNS_INFO_H
#define IMDB_TABLE_COLUMNS_INFO_H

#include "tools/datagen/line_parser.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstdint>
#include <filesystem>

namespace tools::gen::imdb {

using namespace pnm::imdb;

/*! \brief Class with basic information about columns in databases */
class ColumnsInfo {
public:
  ColumnsInfo() = default;

  ColumnsInfo(uint64_t entry_bit_size, index_type entry_count)
      : entry_bit_size_{entry_bit_size}, entry_count_{entry_count} {}

  explicit ColumnsInfo(const std::filesystem::path &path) {
    load_from_file(path);
  }

  void load_from_file(const std::filesystem::path &path);

  auto entry_bit_size() const { return entry_bit_size_; }

  auto entry_count() const { return entry_count_; }

  void store_to_file(const std::filesystem::path &path) const;

private:
  LineParser create_parser();

  // size of value in bits
  uint64_t entry_bit_size_;
  // column size
  index_type entry_count_;
};

} // namespace tools::gen::imdb

#endif // IMDB_TABLE_COLUMNS_INFO_H
