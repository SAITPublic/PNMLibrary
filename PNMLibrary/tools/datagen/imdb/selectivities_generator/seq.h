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

#ifndef SEQ_SELECTIVITIES_GEN_H
#define SEQ_SELECTIVITIES_GEN_H

#include "base.h"

#include <cstddef>
#include <vector>

namespace tools::gen::imdb {

class SeqSelectivitiesGen : public ISelectivitiesGenerator {

private:
  std::vector<double>
  create_selectivities_impl(size_t count, double min_selectivity,
                            double max_selectivity) override {
    if (count == 0) {
      return {};
    }

    std::vector<double> selectivities;
    selectivities.resize(count);
    const double selectivity_step =
        (max_selectivity - min_selectivity) / static_cast<double>(count);
    for (size_t i = 0; i < count; ++i) {
      selectivities[i] = min_selectivity + selectivity_step * i;
    }
    return selectivities;
  }
};

} // namespace tools::gen::imdb

#endif // SEQ_SELECTIVITIES_GEN_H
