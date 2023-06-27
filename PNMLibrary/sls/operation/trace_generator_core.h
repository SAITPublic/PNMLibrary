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

#ifndef _SLS_TRACE_GENERATOR_H_
#define _SLS_TRACE_GENERATOR_H_

#include "execution_generator.h"
#include "ir.h"

#include "core/device/sls/rank_memory.h"

#include "common/compiler_internal.h"

#include <cstdint>
#include <vector>

namespace pnm::sls {

class CACHE_ALIGNED TraceGeneratorCore {
public:
  using mem_objects_vector = device::RankMemory::mem_objects_vector;

  explicit TraceGeneratorCore()
      : bytes_per_row_full_(0), max_tables_rows_num_(0) {}

  void init(uint32_t sparse_feature_size, uint32_t max_tables_rows_num,
            bool is_tagged);

  void generate_instructions(const Execution &exec,
                             const std::vector<const uint32_t *> &lengths,
                             const std::vector<const uint32_t *> &indices,
                             uint32_t type, uint8_t compute_unit,
                             const mem_objects_vector &objects,
                             std::vector<InstructionIR> &trace) const;

private:
  void init_mapping(uint32_t sparse_feature_size_full,
                    uint32_t max_tables_rows_num);

  void write_ir(uint32_t opcode, uint32_t output_idx, uint32_t table_row_idx,
                uint64_t offset, std::vector<InstructionIR> &trace) const;

  uint64_t get_phys_address(uint32_t table_row_idx, uint64_t offset) const;

  void
  generate_instructions(const std::vector<Execution::TableData> &tables_data,
                        const std::vector<const uint32_t *> &lengths,
                        const std::vector<const uint32_t *> &indices,
                        uint32_t type, uint8_t compute_unit,
                        const mem_objects_vector &objects,
                        std::vector<InstructionIR> &trace) const;

  uint64_t write_object_trace(uint64_t cunit_offset, const uint32_t *length,
                              uint64_t batches_count, const uint32_t *index,
                              uint64_t psum_id_base, uint32_t instruction_type,
                              std::vector<InstructionIR> &trace) const;

  uint32_t bytes_per_row_full_;
  uint32_t max_tables_rows_num_;
};
} // namespace pnm::sls

#endif
