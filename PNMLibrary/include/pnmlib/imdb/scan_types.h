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

#ifndef IMDB_SCAN_TYPES_H
#define IMDB_SCAN_TYPES_H

#include "bit_containers.h"

#include "pnmlib/imdb/bit_views.h"

#include "pnmlib/common/views.h"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace pnm::imdb {

enum class OperationType : uint8_t { InRange, InList };

enum class OutputType : uint8_t { BitVector, IndexVector };

using compressed_element_type = uint32_t;
using index_type = uint32_t;

struct RangeOperation {
  compressed_element_type start;
  compressed_element_type end;
};

using compressed_vector =
    pnm::containers::compressed_vector<compressed_element_type>;
using compressed_vector_view =
    pnm::views::bit_compressed<const compressed_element_type>;

using predictor_input_vector =
    pnm::containers::bit_vector<compressed_element_type>;
using predictor_input_vector_view = pnm::views::bit_vector_view<
    pnm::views::common<const compressed_element_type>>;

using bit_vector = pnm::containers::bit_vector<index_type>;
using bit_vector_view =
    pnm::views::bit_vector_view<pnm::views::common<index_type>>;

using index_vector = std::vector<index_type>;
using index_vector_view = pnm::views::common<index_type>;

// NOTE: Batching is not actually supported by hardware, these types are here
//       just for convenience
using BitVectors = std::vector<bit_vector>;
using IndexVectors = std::vector<index_vector>;

using Ranges = std::vector<RangeOperation>;
using Predictors = std::vector<predictor_input_vector>;

template <OutputType type>
using OutputContainer =
    std::conditional_t<type == OutputType::BitVector, bit_vector, index_vector>;

} // namespace pnm::imdb

#endif // IMDB_SCAN_TYPES_H
