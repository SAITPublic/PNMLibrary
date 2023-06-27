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

#ifndef IMDB_TABLE_COLUMNS_IO_H
#define IMDB_TABLE_COLUMNS_IO_H

#include "columns_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/scan_types.h"

#include <filesystem>

namespace tools::gen::imdb {

using namespace pnm::imdb;

inline constexpr auto column_default_name = "table_column.bin";
inline constexpr auto column_info_default_name = "table_column.info";

/*! \brief Class that allows to read columns of databases via mmap */
class ColumnsMmap {
public:
  ColumnsMmap() = default;

  ColumnsMmap(const std::filesystem::path &data_path,
              const std::filesystem::path &info_path)
      : mmapped_file(data_path), info(info_path) {}

  pnm::utils::MappedFile mmapped_file;
  ColumnsInfo info;
};

/*! \brief Class for file read/write operations for predicate vectors. */
class ColumnsIO {
public:
  static void store_to_file(const compressed_vector &column,
                            const std::filesystem::path &root,
                            const ColumnsInfo &info);

  static compressed_vector load_from_file(const ColumnsMmap &mmapped_column);
};
} // namespace tools::gen::imdb

#endif // IMDB_TABLE_COLUMNS_IO_H
