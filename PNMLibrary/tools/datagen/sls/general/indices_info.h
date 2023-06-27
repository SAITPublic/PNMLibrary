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
#ifndef SLS_INDICES_INFO_H
#define SLS_INDICES_INFO_H

#include "tools/datagen/line_parser.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

/*! \brief Basic information about indices structure. Assume that tables has
 * minibatch_size[i] lookups per table. Each lookup set contains num_lookup
 * indices.
 */
class IndicesInfo {
public:
  IndicesInfo() = default;

  IndicesInfo(size_t minibatch_size, std::vector<uint32_t> &lengths)
      : minibatch_sizes_{minibatch_size}, lengths_{std::move(lengths)},
        num_requests_{lengths_.size()} {}

  IndicesInfo(const std::vector<size_t> &num_lookup, size_t minibatch_sizes)
      : minibatch_sizes_{minibatch_sizes} {
    num_requests_ = num_lookup.size() * minibatch_sizes_;
    lengths_.reserve(num_requests_);

    for (const auto &lookups : num_lookup) {
      lengths_.insert(lengths_.end(), minibatch_sizes_, lookups);
    }
  }

  explicit IndicesInfo(const std::filesystem::path &path);

  auto minibatch_size() const { return minibatch_sizes_; }

  const auto &lengths() const { return lengths_; }

  auto num_requests() const { return num_requests_; }

  void store_to_file(const std::filesystem::path &path) const;

private:
  LineParser create_parser();

  size_t minibatch_sizes_{};

  // the number of indices in single batch
  std::vector<uint32_t> lengths_{};

  size_t num_requests_;
};

namespace sls::tests {
/*! \brief Simple struct that store istream to indices file */
struct Indices {
  std::ifstream in;
  IndicesInfo info;
};
} // namespace sls::tests

#endif // SLS_INDICES_INFO_H
