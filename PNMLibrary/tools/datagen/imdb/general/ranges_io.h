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

#ifndef IMDB_SCAN_RANGES_IO_H
#define IMDB_SCAN_RANGES_IO_H

#include "ranges_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/scan_types.h"

#include <filesystem>
#include <string>

namespace tools::gen::imdb {

using namespace pnm::imdb;

inline constexpr auto scan_range_suffix_bin = ".scan_range.bin";
inline constexpr auto scan_range_suffix_info = ".scan_range.info";

/*! \brief Class that allows to read scan ranges via mmap */
class RangesMmap {
public:
  RangesMmap() = default;

  RangesMmap(const std::filesystem::path &data_path,
             const std::filesystem::path &info_path)
      : mmapped_file(data_path), info(info_path) {}

  pnm::utils::MappedFile mmapped_file;
  RangesInfo info;
};

/*! \brief Class for file read/write operations for scan ranges. */
class RangesIO {
public:
  static void store_to_file(const Ranges &ranges,
                            const std::filesystem::path &root,
                            const std::string &filename_prefix,
                            const RangesInfo &scan_info);

  static Ranges load_from_file(const RangesMmap &mmapped_ranges);
};

} // namespace tools::gen::imdb

#endif // IMDB_SCAN_RANGES_IO_H
