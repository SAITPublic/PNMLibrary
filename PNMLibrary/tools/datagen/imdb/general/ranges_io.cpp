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

#include "ranges_io.h"

#include "ranges_info.h"

#include "common/mapped_file.h"

#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/common/misc_utils.h"

#include <fmt/core.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

void RangesIO::store_to_file(const Ranges &ranges,
                             const std::filesystem::path &root,
                             const std::string &filename_prefix,
                             const RangesInfo &scan_info) {
  const auto data_path = root / (filename_prefix + scan_range_suffix_bin);
  const auto info_path = root / (filename_prefix + scan_range_suffix_info);
  scan_info.store_to_file(info_path);

  std::ofstream out(data_path, std::ios::out | std::ios::binary);
  out.exceptions(std::ofstream::badbit);
  if (out) {
    out.write(reinterpret_cast<const char *>(ranges.data()),
              pnm::utils::byte_size_of_container(ranges));
  } else {
    throw std::runtime_error(fmt::format(
        "Unable scan range file in write mode. Reason: {}", strerror(errno)));
  }
}

Ranges RangesIO::load_from_file(const RangesMmap &mmapped_ranges) {
  const auto view = mmapped_ranges.mmapped_file.get_view<RangeOperation>();
  const auto expected_count = mmapped_ranges.info.num_requests();
  const decltype(expected_count) actual_count = view.size();
  if (expected_count != actual_count) {
    throw std::runtime_error(fmt::format(
        "Expected scan ranges count ({}) doesn't match actual count ({})",
        expected_count, actual_count));
  }
  return std::vector(view.begin(), view.end());
}

} // namespace tools::gen::imdb
