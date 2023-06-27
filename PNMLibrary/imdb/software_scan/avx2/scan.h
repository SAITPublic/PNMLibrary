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

#ifndef _IMDB_SCAN_AVX2_H_
#define _IMDB_SCAN_AVX2_H_

#include "pnmlib/imdb/scan_types.h"

#include <cstddef>

namespace pnm::imdb::internal::avx2 {

/** @brief InRange scan with output to index vector
 *
 * @param column column with compressed data
 * @param range in-range scan predicate
 * @param result the reference to output index array with enough space to
 * place the scan result
 * @return the number of values that satisfy in-range predicate
 */
size_t scan(compressed_vector_view column, RangeOperation range,
            index_vector_view result);

/** @brief InRange scan with output to bit vector
 *
 * @param column column with compressed data
 * @param range in-range scan predicate
 * @param result the reference to the output bitvector with a size equal to
 * the DB entry count
 * @return the bitvector size in bytes (not relevant)
 */
size_t scan(compressed_vector_view column, RangeOperation range,
            bit_vector_view result);

/** @brief InList scan with output to index vector
 *
 * @param column column with compressed data
 * @param predicate in-list scan predicate
 * @param result the reference to output index array with enough space to
 * place the scan result
 * @return the number of values that satisfy in-range predicate
 */
size_t scan(compressed_vector_view column,
            predictor_input_vector_view predicate, index_vector_view result);

/** @brief InList scan with output to bit vector
 *
 * @param column column with compressed data
 * @param predicate in-list scan predicate
 * @param result the reference to the output bitvector with a size equal to
 * the DB entry count
 * @return the bitvector size in bytes (not relevant)
 */
size_t scan(compressed_vector_view column,
            predictor_input_vector_view predicate, bit_vector_view result);

} // namespace pnm::imdb::internal::avx2

#endif //_IMDB_SCAN_AVX2_H_
