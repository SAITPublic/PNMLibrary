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

#ifndef _NOSIMD_BASELINE_H_
#define _NOSIMD_BASELINE_H_

#include "imdb/software_scan/nosimd/cpu_scan.h"

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>

class NoSIMDBaseline {
public:
  size_t scan(const pnm::imdb::compressed_vector &column,
              pnm::imdb::RangeOperation range,
              pnm::imdb::index_vector &result) const {
    pnm::imdb::internal::cpu::in_range_to_iv(column, range, result);
    return result.size();
  }

  size_t scan(const pnm::imdb::compressed_vector &column,
              pnm::imdb::RangeOperation range,
              pnm::imdb::bit_vector &result) const {
    pnm::imdb::internal::cpu::in_range_to_bv(column, range, result);
    return result.size();
  }

  size_t scan(const pnm::imdb::compressed_vector &column,
              const pnm::imdb::predictor_input_vector &predicate,
              pnm::imdb::index_vector &result) const {
    pnm::imdb::internal::cpu::in_list_to_iv(column, predicate, result);
    return result.size();
  }

  size_t scan(const pnm::imdb::compressed_vector &column,
              const pnm::imdb::predictor_input_vector &predicate,
              pnm::imdb::bit_vector &result) const {
    pnm::imdb::internal::cpu::in_list_to_bv(column, predicate, result);
    return result.size();
  }
};

#endif //_NOSIMD_BASELINE_H_
