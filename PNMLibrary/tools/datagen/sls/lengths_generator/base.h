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

#ifndef SLS_ILENGTHS_GENERATOR_H
#define SLS_ILENGTHS_GENERATOR_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

/*! \brief Interface for indices generators */
class ILengthsGenerator {
public:
  auto create(size_t num_tables, std::optional<size_t> min_num_lookup,
              size_t max_num_lookup, size_t minibatch_size) {
    return create_lengths_impl(num_tables, min_num_lookup, max_num_lookup,
                               minibatch_size);
  }

  virtual ~ILengthsGenerator() = default;

private:
  virtual std::vector<uint32_t>
  create_lengths_impl(size_t num_tables, std::optional<size_t> min_num_lookup,
                      size_t max_num_lookup, size_t minibatch_size) = 0;
};

#endif // SLS_ILENGTHS_GENERATOR_H
