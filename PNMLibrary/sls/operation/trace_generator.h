/*
 *  Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 *  This software is proprietary of Samsung Electronics.
 *  No part of this software, either material or conceptual may be copied or
 *  distributed, transmitted, transcribed, stored in a retrieval system or
 *  translated into any human or computer language in any form by any means,
 *  electronic, mechanical, manual or otherwise, or disclosed to third parties
 *  without the express written permission of Samsung Electronics.
 */

#ifndef PNM_SLS_TRACE_GENERATOR_H
#define PNM_SLS_TRACE_GENERATOR_H

#include "execution_generator.h"
#include "ir.h"
#include "trace_generator_core.h"

#include "hw/pnm_hw_instr_constants.h"

#include <cstdint>
#include <iterator>
#include <vector>

namespace pnm::sls {

template <typename InstructionGenerator> class TraceGenerator {
public:
  void init(uint32_t sparse_feature_size, uint32_t max_tables_rows_num,
            bool is_tagged) {
    bytes_per_row_ = sparse_feature_size;
    core_.init(sparse_feature_size, max_tables_rows_num, is_tagged);
  }

  /**@brief Generates instructions for particular execution part
   *
   * @param exec Prepared execution
   * @param lengths Pointer to lookups of tables batches that fit into
   * execution. It must point to the data for the first table in the execution.
   * @param indices Pointer to indices of tables that fit into execution.
   * It must point to the data for the first table in the execution.
   * @param type SLS instruction opcode
   * @param type Mapping of table index to table position in compute unit
   * @return vector of generated instructions
   */
  const std::vector<uint64_t> &generate_instructions(
      const Execution &exec, const std::vector<const uint32_t *> &lengths,
      const std::vector<const uint32_t *> &indices, uint32_t type,
      uint8_t compute_unit,
      const TraceGeneratorCore::mem_objects_vector &objects, bool is_tagged) {

    ir_.clear();

    core_.generate_instructions(exec, lengths, indices, type, compute_unit,
                                objects, ir_);

    // Resize trace vector to store all ir instructions plus header
    trace_.resize(ir_.size() + 1);

    write_header(is_tagged);

    compile();

    set_trace_end();

    return get_traces();
  }

  const std::vector<uint64_t> &get_traces() const { return trace_; }

private:
  void write_header(bool is_tagged) {
    uint64_t header = 0;
    // 64B: VSIZE = 0, 128B: VSIZE = 1, 256B: VSIZE = 2
    header |= (bytes_per_row_ / 128) &
              hw::INSTR_HEADER_VSIZE_MASK; // [4:0] -- sparse feature size
    header |=
        is_tagged << hw::INSTR_HEADER_TAGBIT_POS; // [8] --  is for TAGGED SLS

    trace_.front() = header;
  }

  void set_trace_end() { trace_.back() |= 1UL << hw::INSTR_TRACE_END_OFFSET; }

  void compile() {
    auto compile_func = [this](InstructionIR instruction) {
      auto inst = encoder_.make_empty_instruction(instruction.opcode,
                                                  instruction.output_index);

      encoder_.decode_address_to_instruction(instruction.address, inst);

      return encoder_.encode_instruction(inst);
    };

    std::transform(ir_.begin(), ir_.end(), std::next(trace_.begin()),
                   compile_func);
  }

  uint32_t bytes_per_row_;
  TraceGeneratorCore core_;
  std::vector<uint64_t> trace_;
  std::vector<InstructionIR> ir_;
  InstructionGenerator encoder_;
};

} // namespace pnm::sls

#endif // PNM_SLS_TRACE_GENERATOR_H
