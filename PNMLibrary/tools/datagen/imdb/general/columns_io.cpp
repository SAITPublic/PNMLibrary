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

#include "columns_io.h"

#include "columns_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/scan_types.h"

#include <fmt/core.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ostream>
#include <stdexcept>

namespace tools::gen::imdb {

using namespace pnm::imdb;

void ColumnsIO::store_to_file(const compressed_vector &column,
                              const std::filesystem::path &root,
                              const ColumnsInfo &info) {
  const auto db_path = root / column_default_name;
  const auto info_path = root / column_info_default_name;
  info.store_to_file(info_path);

  std::ofstream out(db_path, std::ios::out | std::ios::binary);
  out.exceptions(std::ofstream::badbit | std::ostream::failbit);
  if (out) {
    out.write(reinterpret_cast<const char *>(column.container().data()),
              column.bytes());
  } else {
    throw std::runtime_error(fmt::format(
        "Unable open column file in write mode. Reason: {}", strerror(errno)));
  }
}

compressed_vector ColumnsIO::load_from_file(const ColumnsMmap &mmapped_column) {
  const auto &mmapped_file = mmapped_column.mmapped_file;
  compressed_vector column(mmapped_column.info.entry_bit_size(),
                           mmapped_column.info.entry_count());
  if (column.bytes() != mmapped_file.size()) {
    throw std::runtime_error(
        fmt::format("Expected column size ({} bytes) doesn't match file "
                    "size ({} bytes)",
                    column.bytes(), mmapped_file.size()));
  }
  const auto view = mmapped_file.get_view<compressed_element_type>();
  std::copy(view.begin(), view.end(), column.container().begin());
  return column;
}
} // namespace tools::gen::imdb
