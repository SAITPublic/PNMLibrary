/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form
 * by any means, electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

#include "trace_generator_core.h"

#include "execution_generator.h"
#include "ir.h"

#include "common/log.h"
#include "common/topology_constants.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

namespace pnm::sls {

/*
 * Here we implement algorithm to store 2D array in 1D in column-wise order.
 * We treat indices array as 2D array with variable X dimension. In row, we
 * store indices for single lookup. For the single table in pack we have:
 * -- lookup_0: i_00, i_01, i_02, ..., i_0L[0],
 * -- lookup_1: i_10, i_11, i_12, ..., i_1L[1],
 * -- ........................................,
 * -- lookup_N: i_N0, i_N1, i_N2, ..., i_NL[N],
 * where N - table_batches_chunk_ for current table, L[] - array with lookup's
 * lengths. In general L[0] != L[1] != ... != L[N].
 *
 * To store array in column-wise order we iterate over columns for each row with
 * single index (the so-called lookup_offset). If the lookup_offset for the row
 * less than row's size then we write trace for this element, otherwise we just
 * ignore it. We repeat loop over all rows until we will sure that we write
 * complete trace for all rows.
 *
 * Ex:
 * -- 0, 1, 2, 3,
 * -- 4, 5, 6,
 * -- 7, 8, 9, 10, 11
 * 1-st iteration:
 * -- 0, | 1, 2, 3,
 * -- 4, | 5, 6,
 * -- 7, | 8, 9, 10, 11
 * >> 0, 4, 7,
 * 2-nd iteration:
 * -- 1, | 2, 3,
 * -- 5, | 6,
 * -- 8, | 9, 10, 11
 * >> 0, 4, 7, 1, 5, 8,
 * 3-rd iteration:
 * -- 2, | 3,
 * -- 6, |
 * -- 9, | 10, 11
 * >> 0, 4, 7, 1, 5, 8, 2, 6, 9,
 * 4-th iteration:
 * -- 3, |
 * -- 10, | 11,
 * >> 0, 4, 7, 1, 5, 8, 2, 6, 9, 3, 10,
 * 5-th iteration:
 * -- 11, |
 * >> 0, 4, 7, 1, 5, 8, 2, 6, 9, 3, 10, 11
 * lookup_offset greater than all row's sizes
 * */
uint64_t TraceGeneratorCore::write_object_trace(
    uint64_t cunit_offset, const uint32_t *length, uint64_t batches_count,
    const uint32_t *index, uint64_t psum_id_base, uint32_t instruction_type,
    std::vector<InstructionIR> &trace) const {
  uint64_t lookup_offset = 0;
  bool has_available_lookup = true;
  const uint32_t *batch_index_base = nullptr; // start of indices for batch
  while (has_available_lookup) {
    has_available_lookup = false;

    batch_index_base = index; // init indices for the 0 batch
    for (auto batch_id = 0UL; batch_id < batches_count; ++batch_id) {
      if (lookup_offset < length[batch_id]) {
        write_ir(instruction_type, psum_id_base + batch_id,
                 batch_index_base[lookup_offset], cunit_offset, trace);
        has_available_lookup = true;
      }
      batch_index_base += length[batch_id]; // move to index next range
    }
    ++lookup_offset; // move to the next column
  }
  return batch_index_base - index; // return size of processed indices
}

void TraceGeneratorCore::generate_instructions(
    const std::vector<Execution::TableData> &tables_data,
    const std::vector<const uint32_t *> &lengths,
    const std::vector<const uint32_t *> &indices, uint32_t type,
    uint8_t compute_unit, const mem_objects_vector &objects,
    std::vector<InstructionIR> &trace) const {
  uint32_t batch_offset = 0;
  for (uint64_t table_idx = 0; table_idx < tables_data.size(); ++table_idx) {
    const auto &current_table_data = tables_data[table_idx];
    const auto object = std::find_if(
        objects.cbegin(), objects.cend(), [&current_table_data](auto &object) {
          return object.id() == current_table_data.table_id_;
        });
    assert(object != objects.cend());
    write_object_trace(object->offset(compute_unit), lengths[table_idx],
                       current_table_data.table_batches_chunk_,
                       indices[table_idx], batch_offset, type, trace);
    batch_offset += current_table_data.table_batches_chunk_;
  }
}

void TraceGeneratorCore::generate_instructions(
    const Execution &exec, const std::vector<const uint32_t *> &lengths,
    const std::vector<const uint32_t *> &indices, uint32_t type,
    uint8_t compute_unit, const mem_objects_vector &objects,
    std::vector<InstructionIR> &trace) const {
  generate_instructions(exec.get_tables_data(), lengths, indices, type,
                        compute_unit, objects, trace);
}

void TraceGeneratorCore::init(uint32_t sparse_feature_size,
                              uint32_t max_tables_rows_num, bool is_tagged) {
  const auto extended_spase_feature_size =
      sparse_feature_size +
      (is_tagged ? pnm::sls::device::topo().AlignedTagSize : 0);
  init_mapping(extended_spase_feature_size, max_tables_rows_num);

  pnm::log::debug("SLS instruction generator is initialized.");
}

void TraceGeneratorCore::write_ir(uint32_t opcode, uint32_t output_idx,
                                  uint32_t table_row_idx, uint64_t offset,
                                  std::vector<InstructionIR> &trace) const {
  const uint64_t addr = get_phys_address(table_row_idx, offset);

  trace.emplace_back(encode_ir(opcode, output_idx, addr));
}

uint64_t TraceGeneratorCore::get_phys_address(uint32_t table_row_idx,
                                              uint64_t offset) const {
  assert(table_row_idx < max_tables_rows_num_);
  return offset + table_row_idx * bytes_per_row_full_;
}

void TraceGeneratorCore::init_mapping(uint32_t sparse_feature_size_full,
                                      uint32_t max_tables_rows_num) {
  bytes_per_row_full_ = sparse_feature_size_full;
  max_tables_rows_num_ = max_tables_rows_num;
};

} // namespace pnm::sls
