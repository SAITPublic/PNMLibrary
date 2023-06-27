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

#ifndef IMDB_ITABLE_COLUMN_GENERATOR_H
#define IMDB_ITABLE_COLUMN_GENERATOR_H

#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/columns_io.h"

#include "pnmlib/imdb/scan_types.h"

#include <filesystem>

namespace tools::gen::imdb {

using namespace pnm::imdb;

/** @brief Interface for compressed column generator. */
class IColumnGenerator {
public:
  auto create(const ColumnsInfo &info) { return create_column_impl(info); }

  void create_and_store(const std::filesystem::path &root,
                        const ColumnsInfo &info) {
    const auto data = create(info);
    ColumnsIO::store_to_file(data, root, info);
  }

  virtual ~IColumnGenerator() = default;

private:
  virtual compressed_vector create_column_impl(const ColumnsInfo &info) = 0;
};

} // namespace tools::gen::imdb

#endif // IMDB_ITABLE_COLUMN_GENERATOR_H
