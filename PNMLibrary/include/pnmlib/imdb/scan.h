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

#ifndef IMDB_SCAN_H
#define IMDB_SCAN_H

#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/buffer.h"

#include "pnmlib/common/compiler.h"
#include "pnmlib/common/error.h"

#include <cstdint>
#include <unordered_set>
#include <variant>

namespace pnm::operations {

/** @brief This takes in multiple input and output buffers and encodes this
 * information to be forwarded to the device itself
 */
class PNM_API Scan {
public:
  struct Column {
    uint64_t bit_compression;
    uint64_t count;
    const memory::Buffer<pnm::imdb::compressed_element_type> *data;
  };

  struct InListPredicate {
    uint64_t max_value;
    const memory::Buffer<pnm::imdb::compressed_element_type> *data;
  };

  Scan(Column column, pnm::imdb::RangeOperation predicate,
       const memory::Buffer<pnm::imdb::index_type> *result,
       pnm::imdb::OutputType out_type)
      : column_(column), predicate_(predicate), result_(result),
        out_type_(out_type) {
    validate_compression_bits();
  }

  Scan(Column column, InListPredicate predicate,
       const memory::Buffer<pnm::imdb::index_type> *result,
       pnm::imdb::OutputType out_type)
      : column_(column), predicate_(predicate), result_(result),
        out_type_(out_type) {
    validate_compression_bits();
  }

  void set_predicate(const std::variant<pnm::imdb::RangeOperation,
                                        InListPredicate> &predicate) {
    predicate_ = predicate;
  }

  void set_result(const pnm::memory::Buffer<pnm::imdb::index_type> *result) {
    result_ = result;
  }

  void set_result_size(uint64_t size) { result_size_ = size; }
  void set_execution_time(uint64_t time) { execution_time_ = time; }

  auto result_size() const { return result_size_; }
  auto execution_time() const { return execution_time_; }

  const auto &column() const { return column_.data; }
  uint64_t bit_compression() const { return column_.bit_compression; }
  auto elements_count() const { return column_.count; }
  const auto &predicate() const { return predicate_; }
  const auto &result() const { return result_; }
  auto output_type() const { return out_type_; }

private:
  const std::unordered_set<uint64_t> valid_bit_counts = {
      1, 2, 3, 4, 5, 6, 7, 9, 10, 12, 16, 17, 18};

  bool validate_compression_bits() {
    if (valid_bit_counts.find(column_.bit_compression) ==
        valid_bit_counts.end()) {
      throw pnm::error::InvalidArguments("An invalid column's entry size.");
    }
    return true;
  }

  Column column_;

  std::variant<pnm::imdb::RangeOperation, InListPredicate> predicate_;

  const pnm::memory::Buffer<pnm::imdb::index_type> *result_;
  pnm::imdb::OutputType out_type_;

  uint64_t result_size_ = 0;
  uint64_t execution_time_ = 0;
};

} // namespace pnm::operations

#endif // IMDB_SCAN_H
