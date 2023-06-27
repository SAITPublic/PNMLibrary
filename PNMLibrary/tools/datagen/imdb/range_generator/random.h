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

#ifndef IMDB_RANDOM_SCAN_RANGES_GEN_H
#define IMDB_RANDOM_SCAN_RANGES_GEN_H

#include "base.h"

#include "common/log.h"

#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/generators_utils.h"
#include "tools/datagen/imdb/general/ranges_info.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>
#include <cstdint>
#include <random>

namespace tools::gen::imdb {

/** @brief Class for relatively fast generating scan ranges */
class RandomRangesGen : public IRangesGenerator {

public:
  RandomRangesGen(size_t seed = std::random_device{}()) : seed_(seed) {
    pnm::log::info("RandomRangesGen created with seed {}", seed_);
  }

private:
  Ranges create_scan_ranges_impl(const RangesInfo &scan_info,
                                 const ColumnsInfo &column_info) override {
    Ranges scan_ranges;
    scan_ranges.reserve(scan_info.num_requests());
    std::mt19937_64 engine{seed_};

    const auto entry_max_value = (1ULL << column_info.entry_bit_size()) - 1;

    for (auto selectivity : scan_info.selectivities()) {
      const auto request_range_size = value_per_request_count(
          entry_max_value + 1, column_info.entry_count(), selectivity);

      std::uniform_int_distribution<uint32_t> uni(0, entry_max_value -
                                                         request_range_size);

      const auto range_start = uni(engine);
      scan_ranges.push_back(
          RangeOperation{range_start, range_start + request_range_size});
    }

    return scan_ranges;
  }

  size_t seed_;
};

} // namespace tools::gen::imdb

#endif // IMDB_RANDOM_SCAN_RANGES_GEN_H
