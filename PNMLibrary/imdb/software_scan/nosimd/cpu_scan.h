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

#ifndef IMDB_CPU_SCAN_H
#define IMDB_CPU_SCAN_H

#include "pnmlib/imdb/scan_types.h"

namespace pnm::imdb::internal::cpu {

inline void in_range_to_bv(const compressed_vector &column,
                           RangeOperation range, bit_vector &result) {
  result = bit_vector(column.size());

  for (index_type i = 0; i < column.size(); ++i) {
    auto element = column[i];

    if (range.start <= element && element <= range.end) {
      result.insert(i);
    }
  }
}

inline void in_range_to_iv(const compressed_vector &column,
                           RangeOperation range, index_vector &result) {
  result.clear();

  for (index_type i = 0; i < column.size(); ++i) {
    auto element = column[i];

    if (range.start <= element && element <= range.end) {
      result.push_back(i);
    }
  }
}

inline void in_list_to_bv(const compressed_vector &column,
                          const predictor_input_vector &predictor,
                          bit_vector &result) {
  result = bit_vector(column.size());

  for (index_type i = 0; i < column.size(); ++i) {
    if (predictor.contains(column[i])) {
      result.insert(i);
    }
  }
}

inline void in_list_to_iv(const compressed_vector &column,
                          const predictor_input_vector &predictor,
                          index_vector &result) {
  result.clear();

  for (index_type i = 0; i < column.size(); ++i) {
    if (predictor.contains(column[i])) {
      result.push_back(i);
    }
  }
}

} // namespace pnm::imdb::internal::cpu

#endif // IMDB_CPU_SCAN_H
