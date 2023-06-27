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

#include "imdb/operation/scan.h"

#include "tools/datagen/imdb/utils.h"

#include "pnmlib/imdb/bit_containers.h"
#include "pnmlib/imdb/libimdb.h"
#include "pnmlib/imdb/scan.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"
#include "pnmlib/core/memory.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-type-util.h>

#include <cstdint>
#include <stdexcept>

using namespace pnm::imdb;
using namespace tools::gen::imdb;
using pnm::operations::Scan;

auto &context() {
  static auto ctx = pnm::make_context(pnm::Device::Type::IMDB_CXL);
  return ctx;
}

bool get_ctrl_start_status(const ThreadCSR &registers) {
  return (registers.THREAD_CTRL & THREAD_CTRL_STATUS_MASK) == THREAD_CTRL_START;
}

OperationType get_op_type(const ThreadCSR &registers) {
  return (registers.THREAD_COMP_MODE & THREAD_COMP_MODE_OPERATION_TYPE_MASK) ==
                 THREAD_COMP_MODE_IN_RANGE
             ? OperationType::InRange
             : OperationType::InList;
}

uint64_t get_entry_size(const ThreadCSR &registers) {
  return registers.THREAD_COMP_MODE & THREAD_COMP_MODE_ENTRY_BIT_SIZE_MASK;
}

template <typename T> struct PredicateHelper {};

template <> struct PredicateHelper<RangeOperation> {
  static RangeOperation generate() {
    return details::generate_input<OperationType::InRange>().front();
  }

  static RangeOperation get(const ThreadCSR &registers) {
    if (get_op_type(registers) != OperationType::InRange) {
      throw std::runtime_error("Expected InRange operation!");
    }

    return {registers.THREAD_MIN_VALUE, registers.THREAD_MAX_VALUE};
  }

  static bool all_zeros(const ThreadCSR &registers) {
    return registers.THREAD_MIN_VALUE == 0 &&
           registers.THREAD_MIN_OPCODE == THREAD_MIN_OPCODE_VALUE &&
           registers.THREAD_MAX_VALUE == 0 &&
           registers.THREAD_MAX_OPCODE == THREAD_MAX_OPCODE_VALUE;
  }

  static bool equal(RangeOperation lhs, RangeOperation rhs) {
    return lhs.start == rhs.start && lhs.end == rhs.end;
  }
};

template <> struct PredicateHelper<predictor_input_vector> {
  static pnm::memory::Buffer<uint32_t> generate() {
    predictor_input_vector pv =
        details::generate_input<OperationType::InList>().front();
    pnm::memory::Buffer<uint32_t> buf(pnm::make_view(pv.container()),
                                      context());
    return buf;
  }

  static bool all_zeros(const ThreadCSR &registers) {
    return registers.THREAD_INLIST_ADDR == 0 &&
           registers.THREAD_INLIST_SIZE == 0;
  }
};

template <typename T> struct ResultHelper {};

template <> struct ResultHelper<bit_vector> {
  static pnm::memory::Buffer<uint32_t>
  make(const pnm::memory::Buffer<uint32_t> &column) {
    return pnm::memory::Buffer<uint32_t>(column.size(), context());
  }

  static bool all_zeros([[maybe_unused]] const ThreadCSR &registers) {
    return true;
  }
};

template <> struct ResultHelper<index_vector> {
  static pnm::memory::Buffer<uint32_t>
  make([[maybe_unused]] const pnm::memory::Buffer<uint32_t> &column) {
    return pnm::memory::Buffer<uint32_t>(column.size(), context());
  }

  static bool all_zeros(const ThreadCSR &registers) {
    return registers.THREAD_RSLT_LMT == 0;
  }
};

template <typename T> struct IMDBScanOperationTest : public ::testing::Test {
  using predicate_type = typename T::first;
  using result_type = typename T::second;

  using predicate_helper = PredicateHelper<predicate_type>;
  using result_helper = ResultHelper<result_type>;

  static constexpr auto result_type_v =
      std::is_same_v<result_type, index_vector> ? OutputType::IndexVector
                                                : OutputType::BitVector;

  using other_predicate_helper =
      PredicateHelper<pnm::utils::OtherType<predicate_type, RangeOperation,
                                            predictor_input_vector>>;
  using other_result_helper = ResultHelper<
      pnm::utils::OtherType<result_type, bit_vector, index_vector>>;

  void basic() {
    const auto column = details::generate_column();
    const pnm::memory::Buffer<compressed_element_type> column_buffer(
        pnm::make_view(column.container()), context());

    const auto predicate = predicate_helper::generate();
    auto result = result_helper::make(column_buffer);

    auto predicate_ptr = [](auto &predicate) {
      if constexpr (std::is_same_v<predicate_type, RangeOperation>) {
        return predicate;
      } else {
        return Scan::InListPredicate{predicate.size(), &predicate};
      }
    };

    Scan user_op(
        Scan::Column{column.value_bits(), column.size(), &column_buffer},
        predicate_ptr(predicate), &result, result_type_v);
    const ScanOperation op(user_op);

    auto registers = op.encode_to_registers();

    auto address = [](auto &b) {
      return std::get<pnm::memory::SequentialRegion>(b.device_region()).start;
    };

    ASSERT_EQ(get_entry_size(registers), column.value_bits());

    ASSERT_EQ(registers.THREAD_START_ADDR, address(column_buffer));
    ASSERT_EQ(registers.THREAD_SIZE_SB, column.size());
    ASSERT_EQ(registers.THREAD_COLUMN_OFFSET, 0);

    ASSERT_EQ(registers.THREAD_RES_ADDR, address(result));
    if constexpr (result_type_v == OutputType::IndexVector) {
      ASSERT_EQ(registers.THREAD_RSLT_LMT, result.size());
    }

    ASSERT_FALSE(get_ctrl_start_status(registers));

    ASSERT_TRUE(other_predicate_helper::all_zeros(registers));
    ASSERT_TRUE(other_result_helper::all_zeros(registers));
  }
};

using types = ::testing::Types<
    pnm::utils::TypePair<RangeOperation, bit_vector>,
    pnm::utils::TypePair<predictor_input_vector, bit_vector>,
    pnm::utils::TypePair<RangeOperation, index_vector>,
    pnm::utils::TypePair<predictor_input_vector, index_vector>>;

TYPED_TEST_SUITE(IMDBScanOperationTest, types);

TYPED_TEST(IMDBScanOperationTest, Basic) { this->basic(); }

TEST(IMDBScanOperationExtraTest, TooManyBits) {
  constexpr uint64_t wrong_bit_compression = 28;
  constexpr index_type column_size = 100;

  const pnm::memory::Buffer<uint32_t> db(column_size, context());
  const pnm::memory::Buffer<uint32_t> predicate(column_size, context());
  const pnm::memory::Buffer<uint32_t> result(column_size, context());

  ASSERT_THROW(Scan(Scan::Column{wrong_bit_compression, column_size, &db},
                    Scan::InListPredicate{column_size, &predicate}, &result,
                    OutputType::IndexVector),
               pnm::error::InvalidArguments);
}
