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
#ifndef IMDB_SCAN_RANGES_INFO_H
#define IMDB_SCAN_RANGES_INFO_H

#include "tools/datagen/line_parser.h"

#include <cstddef>
#include <filesystem>
#include <utility>
#include <vector>

namespace tools::gen::imdb {

/*! \brief Basic information about scan ranges structure*/
class RangesInfo {
public:
  RangesInfo() = default;

  RangesInfo(std::vector<double> selectivities)
      : selectivities_{std::move(selectivities)},
        num_requests_{selectivities_.size()} {}

  explicit RangesInfo(const std::filesystem::path &path);

  const auto &selectivities() const { return selectivities_; }

  auto num_requests() const { return num_requests_; }

  void store_to_file(const std::filesystem::path &path) const;

private:
  LineParser create_parser();

  std::vector<double> selectivities_;

  size_t num_requests_{0};
};

} // namespace tools::gen::imdb

#endif // IMDB_SCAN_RANGES_INFO_H
