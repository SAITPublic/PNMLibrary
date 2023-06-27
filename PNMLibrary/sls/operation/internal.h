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

#ifndef _SLS_OPERATION_INTERNAL_H_
#define _SLS_OPERATION_INTERNAL_H_

#include "execution_generator.h"
#include "inst_generator.h"
#include "packs_data.h"
#include "reduction_operation.h"
#include "trace_generator.h"

#include "core/device/sls/base.h"
#include "core/device/sls/constants.h"
#include "core/device/sls/rank_memory.h"

#include "common/make_error.h"
#include "common/timer.h"
#include "common/topology_constants.h"

#include "pnmlib/sls/embedded_tables.h"
#include "pnmlib/sls/operation.h"
#include "pnmlib/sls/type.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/views.h"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace pnm::sls {

template <typename InstructionGenerator>
class SLSOperation : public ReductionOperation {
public:
  SLSOperation(PNMSLSOperation &sls_op, const device::BaseDevice &device)
      : sls_op_(sls_op), packs_data_(sls_op) {
    init_exec_gen(device);
    init_trace_gen();
    packs_data_.init(exec_gen_);
  }

  uint32_t get_num_exec_iterations(uint8_t pack) const override {
    return exec_gen_.get_pack_execs(pack).size();
  }

  void write_result(uint8_t output_no, uint8_t pack, uint32_t exec_no,
                    uint8_t *src) override {
    const auto &exec = exec_gen_.get_pack_execs(pack)[exec_no];
    switch (output_no) {
    case 0:
      write_result(exec, packs_data_.psum(pack, exec_no), src,
                   [size = get_sparse_ft_data_nb()] { return size; });
      break;
    case 1:
      write_result(exec, packs_data_.tags(pack, exec_no), src,
                   [size = get_sparse_ft_tag_nb()]() { return size; });
      break;
    default:
      throw pnm::error::make_not_sup("Output buffer number {}.", output_no);
    }
  }

  pnm::common_view<uint8_t> get_tmp_buffer(uint8_t pack,
                                           uint32_t exec_no) const override {
    const size_t read_size =
        exec_gen_.get_pack_execs(pack)[exec_no].get_psum_read_size();
    psum_intermediate_buffer().resize(read_size);
    return pnm::make_view(psum_intermediate_buffer());
  }

  uint8_t get_num_outputs() const override { return is_tagged() ? 2 : 1; }

  const std::vector<uint64_t> &
  generate_instructions(uint8_t pack, uint8_t compute_unit,
                        uint32_t exec_no) override {
    MEASURE_TIME();
    const auto &execs = exec_gen_.get_pack_execs(pack);
    assert(exec_no < execs.size());
    const auto &exec = execs[exec_no];
    return trace_gen_[pack].generate_instructions(
        exec, packs_data_.lengths(pack, exec_no),
        packs_data_.indices(pack, exec_no),
        static_cast<uint32_t>(sls_op_.op_type()), compute_unit,
        sls_op_.tables()->layout()[pack], is_tagged());
  }

  ~SLSOperation() override = default;

private:
  bool is_tagged() const { return sls_op_.op_type() == SLSType::Uint32Tagged; }

  uint32_t get_sparse_ft_tag_nb() const {
    return is_tagged() ? sizeof(pnm::uint128_t) : 0;
  }

  uint32_t get_sparse_ft_data_nb() const {
    return sls_op_.sparse_feature_size() * pnm::device::topo().DataSize;
  }

  void write_result(const Execution &exec, std::vector<uint8_t *> &dst,
                    uint8_t *src, const std::function<uint32_t()> &size_func) {
    assert(src);
    uint64_t buff_table_offset = 0;
    const auto &tables_data = exec.get_tables_data();
    for (uint64_t table_idx = 0ULL; table_idx < tables_data.size();
         ++table_idx) {
      const uint64_t current_table_data_size =
          tables_data[table_idx].table_batches_chunk_ * size_func();
      auto *start = src + buff_table_offset;
      std::copy_n(start, current_table_data_size, dst[table_idx]);
      buff_table_offset += current_table_data_size;
    }
  }

  void init_trace_gen() {
    for (const auto pack : device::RankMemory::pack_identifiers()) {
      const auto pack_objects_num = sls_op_.tables()->layout()[pack].size();
      if (pack_objects_num) {
        auto max_pack_objects_rows_num = *std::max_element(
            sls_op_.tables()->layout()[pack].begin(),
            sls_op_.tables()->layout()[pack].end(),
            [table_sizes = sls_op_.tables_sizes()](const auto &lhs,
                                                   const auto &rhs) {
              return table_sizes[lhs.id()] < table_sizes[rhs.id()];
            });
        trace_gen_[pack].init(
            get_sparse_ft_data_nb(),
            sls_op_.tables_sizes()[max_pack_objects_rows_num.id()],
            is_tagged());
      }
    }
  }

  // The constants here are derived based on the variation_exec_nums benchmark
  // Can read more in MCS23-399
  double choose_optimal_buf_divider() const {
    double buf_divider;
    const auto inst_count =
        std::accumulate(sls_op_.lengths().begin(), sls_op_.lengths().end(), 0);
    if (inst_count < 1500) {
      buf_divider = 65;
    } else if (inst_count < 5'000) {
      buf_divider = 38;
    } else if (inst_count < 10'000) {
      buf_divider = 32;
    } else if (inst_count < 50'000) {
      buf_divider = 16;
    } else if (inst_count < 140'000) {
      buf_divider = 8;
    } else if (inst_count < 600'000) {
      buf_divider = 4;
    } else {
      buf_divider = 2;
    }
    return buf_divider;
  }

  void set_instr_buffer_size(size_t &max_inst_in_buf) const {
    double buf_divider = choose_optimal_buf_divider();

    // inst buffer divider can set through an environment variable
    if (const char *env_p = std::getenv("SLS_BUF_DIVIDER")) {
      buf_divider = std::strtod(env_p, nullptr);

      if (buf_divider < 1) {
        throw std::logic_error(fmt::format(
            "Buffer divider cannot be less than 1, but it is equal to {}\n",
            buf_divider));
      }
    }

    max_inst_in_buf /= buf_divider;

    // buffer size cannot be less than max_num_lookup
    const size_t max_num_lookup =
        *std::max_element(sls_op_.lengths().begin(), sls_op_.lengths().end());
    max_inst_in_buf = std::max(max_num_lookup, max_inst_in_buf);
  }

  // [TODO: @a.korzun] Remove `device` parameter.
  void init_exec_gen([[maybe_unused]] const device::BaseDevice &device) {
    size_t max_inst_in_buf = device::inst_per_single_buf_no_header();
    const size_t max_features_in_buf =
        device::psum_single_buf_size() / get_sparse_ft_data_nb();

    set_instr_buffer_size(max_inst_in_buf);

    exec_gen_.init(device::RankMemory::packs_array(sls_op_.tables()->layout()),
                   max_inst_in_buf, max_features_in_buf,
                   get_sparse_ft_data_nb());

    exec_gen_.generate(sls_op_.minibatch_size(), sls_op_.tables_sizes().size(),
                       sls_op_.lengths().data());
  }

  /*! An intermediate buffer is utilized for reading the individual execution
   * psum result. Given that tables in the execution can be in arbitrary order,
   * directly reading the execution result into the user's psum buffer may
   * overwrite data from other tables. This is problematic as the user expects
   * the tables to be in sequential order. Additionally, it is necessary to
   * handle the resetting of the psum buffer by the hardware/simulator after the
   * read operation. The simplest and most convenient solution to this problem
   * involves using an intermediate buffer. Each worker reads the single
   * execution result into its own intermediate buffer and subsequently places
   * each table batch at the correct position in the user's output buffer.
   */
  static auto &psum_intermediate_buffer() {
    thread_local static std::vector<uint8_t> buffer;
    return buffer;
  }

  PNMSLSOperation &sls_op_;

  ExecutionGenerator exec_gen_;
  ExecutionPacksData packs_data_;
  device::RankMemory::packs_array<TraceGenerator<InstructionGenerator>>
      trace_gen_;
};

using SLSOperationRanked = SLSOperation<RankedInstGenerator>;
using SLSOperationChanneled = SLSOperation<ChannelInstGenerator>;

} // namespace pnm::sls

#endif
