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

#ifndef IMDB_ISELECTIVITIES_GENERATOR_H
#define IMDB_ISELECTIVITIES_GENERATOR_H

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace tools::gen::imdb {

/**
  @brief Interface for selectivity generators of scan ranges and predicate
  vectors generators
*/
class ISelectivitiesGenerator {
public:
  auto create(size_t count, double min_selectivity = 0.,
              double max_selectivity = 1.) {
    validate_selectivities(min_selectivity, max_selectivity);
    return create_selectivities_impl(count, min_selectivity, max_selectivity);
  }

  virtual ~ISelectivitiesGenerator() = default;

protected:
  static void validate_selectivities(double min_selectivity,
                                     double max_selectivity) {
    if (min_selectivity < 0. || min_selectivity > 1. || max_selectivity < 0. ||
        max_selectivity > 1.) {
      throw std::runtime_error("Selectivities must be in range [0, 1]");
    }

    if (min_selectivity > max_selectivity) {
      throw std::runtime_error(
          "Minimal selectivity must be less or equal to maximal selectivity");
    }
  }

private:
  virtual std::vector<double>
  create_selectivities_impl(size_t count, double min_selectivity,
                            double max_selectivity) = 0;
};

} // namespace tools::gen::imdb

#endif // IMDB_ISELECTIVITIES_GENERATOR_H
