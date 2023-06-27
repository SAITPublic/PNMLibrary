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

#ifndef SLS_TABLES_INFO_H
#define SLS_TABLES_INFO_H

#include "tools/datagen/line_parser.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

/*! \brief Class with basic information about tables structure */
class TablesInfo {
public:
  TablesInfo() = default;

  TablesInfo(size_t num_tables, size_t cols,
             std::vector<uint32_t> rows_in_tables)
      : num_tables_{num_tables}, cols_{cols},
        table_rows_{std::move(rows_in_tables)} {}

  explicit TablesInfo(const std::filesystem::path &path) {
    load_from_file(path);
  }

  void load_from_file(const std::filesystem::path &path);

  auto num_tables() const { return num_tables_; }
  auto cols() const { return cols_; }
  const auto &rows() const { return table_rows_; }

  void store_to_file(const std::filesystem::path &path) const;

private:
  LineParser create_parser();

  size_t num_tables_{};
  size_t cols_{};
  std::vector<uint32_t> table_rows_{};
};

#endif // SLS_TABLES_INFO_H
