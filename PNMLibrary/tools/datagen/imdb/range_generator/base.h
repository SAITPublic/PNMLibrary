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

#ifndef IMDB_ISCAN_RANGES_GENERATOR_H
#define IMDB_ISCAN_RANGES_GENERATOR_H

#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/ranges_info.h"
#include "tools/datagen/imdb/general/ranges_io.h"

#include "pnmlib/imdb/scan_types.h"

#include <filesystem>
#include <string>

namespace tools::gen::imdb {

/** @brief Interface for scan ranges generators */
class IRangesGenerator {
public:
  auto create(const RangesInfo &scan_info, const ColumnsInfo &column_info) {
    return create_scan_ranges_impl(scan_info, column_info);
  }

  void create_and_store(const std::filesystem::path &root,
                        const std::string &prefix, const RangesInfo &scan_info,
                        const ColumnsInfo &column_info) {
    const auto data = create_scan_ranges_impl(scan_info, column_info);
    RangesIO::store_to_file(data, root, prefix, scan_info);
  }

  virtual ~IRangesGenerator() = default;

private:
  virtual Ranges create_scan_ranges_impl(const RangesInfo &scan_info,
                                         const ColumnsInfo &column_info) = 0;
};

} // namespace tools::gen::imdb

#endif // IMDB_ISCAN_RANGES_GENERATOR_H
