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

#include "scan.h"

#include "common/memory/atomic.h"

#include "pnmlib/imdb/libimdb.h"
#include "pnmlib/imdb/scan.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"

#include <type_traits>
#include <variant>

namespace pnm::imdb {

namespace {
template <typename T> auto address(const pnm::memory::Buffer<T> *buf) {
  return std::get<pnm::memory::SequentialRegion>(buf->device_region()).start;
}
} // namespace

void ScanOperation::set_predicate_info(volatile ThreadCSR *ptr) const {
  std::visit(
      [&](const auto &predicate) {
        using T = std::decay_t<decltype(predicate)>;
        if constexpr (std::is_same_v<T, RangeOperation>) {
          ptr->THREAD_COMP_MODE |= THREAD_COMP_MODE_IN_RANGE;

          ptr->THREAD_MIN_VALUE = predicate.start;
          ptr->THREAD_MAX_VALUE = predicate.end;
          ptr->THREAD_MIN_OPCODE = THREAD_MIN_OPCODE_VALUE;
          ptr->THREAD_MAX_OPCODE = THREAD_MAX_OPCODE_VALUE;

          ptr->THREAD_INLIST_ADDR = 0;
          ptr->THREAD_INLIST_SIZE = 0;
        } else if constexpr (std::is_same_v<
                                 T, pnm::operations::Scan::InListPredicate>) {
          ptr->THREAD_COMP_MODE |= THREAD_COMP_MODE_IN_LIST;

          ptr->THREAD_MIN_VALUE = 0;
          ptr->THREAD_MAX_VALUE = 0;
          ptr->THREAD_MIN_OPCODE = THREAD_MIN_OPCODE_VALUE;
          ptr->THREAD_MAX_OPCODE = THREAD_MAX_OPCODE_VALUE;

          ptr->THREAD_INLIST_ADDR = address(predicate.data);
          ptr->THREAD_INLIST_SIZE = predicate.max_value;
        } else {
          throw pnm::error::InvalidArguments("Predicate type.");
        }
      },
      frontend_.predicate());
}

void ScanOperation::set_output_type(volatile ThreadCSR *ptr) const {
  auto output_flag = frontend_.output_type() == OutputType::BitVector
                         ? THREAD_CTRL_BV
                         : THREAD_CTRL_IV;
  pnm::memory::hw_atomic_store(&ptr->THREAD_CTRL, output_flag);

  ptr->THREAD_RES_ADDR = address(frontend_.result());
  ptr->THREAD_RSLT_LMT = frontend_.output_type() == OutputType::BitVector
                             ? 0
                             : frontend_.result()->size();
}

void ScanOperation::write_registers(volatile ThreadCSR *ptr) const {
  ptr->THREAD_COMP_MODE =
      frontend_.bit_compression() & THREAD_COMP_MODE_ENTRY_BIT_SIZE_MASK;
  ptr->THREAD_START_ADDR = address(frontend_.column());
  ptr->THREAD_SIZE_SB = frontend_.elements_count();
  ptr->THREAD_RES_INDEX_OFFSET = 0;
  ptr->THREAD_COLUMN_OFFSET = 0;

  set_predicate_info(ptr);
  set_output_type(ptr);
}

void ScanOperation::read_registers(const volatile ThreadCSR *ptr) {
  result_size_ = ptr->THREAD_RES_SIZE_SB;
  execution_time_ = ptr->THREAD_DONE_TIME - ptr->THREAD_START_TIME;
}

} // namespace pnm::imdb
