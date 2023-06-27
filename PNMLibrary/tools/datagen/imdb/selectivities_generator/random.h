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

#ifndef RANDOM_SELECTIVITIES_GEN_H
#define RANDOM_SELECTIVITIES_GEN_H

#include "base.h"

#include "common/log.h"

#include <fmt/core.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

namespace tools::gen::imdb {

class RandomSelectivitiesGen : public ISelectivitiesGenerator {
public:
  explicit RandomSelectivitiesGen(size_t seed = std::random_device{}())
      : seed_(seed) {
    pnm::log::info("RandomSelectivityGen created with seed {}", seed_);
  }

private:
  std::vector<double>
  create_selectivities_impl(size_t count, double min_selectivity,
                            double max_selectivity) override {
    std::mt19937_64 engine(seed_);
    std::vector<double> selectivities;
    selectivities.resize(count);
    std::uniform_real_distribution<double> distrib(min_selectivity,
                                                   max_selectivity);
    const auto gen = [&distrib, &engine]() { return distrib(engine); };
    std::generate(selectivities.begin(), selectivities.end(), gen);
    return selectivities;
  }

  size_t seed_;
};

} // namespace tools::gen::imdb

#endif // RANDOM_SELECTIVITIES_GEN_H
