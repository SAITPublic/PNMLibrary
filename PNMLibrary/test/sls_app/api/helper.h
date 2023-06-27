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

#ifndef _SLS_TEST_HELPER_H
#define _SLS_TEST_HELPER_H

#include "constants.h"
#include "structs.h"

#include "test/mocks/context/utils.h"

#include "pnmlib/sls/embedded_tables.h"
#include "pnmlib/sls/operation.h"
#include "pnmlib/sls/type.h"

#include "pnmlib/core/context.h"
#include "pnmlib/core/runner.h"

#include "pnmlib/common/views.h"

#include <cstdint>
#include <vector>

namespace test_app {

template <typename T> class SLSTestHelper {
public:
  SLSTestHelper(const HelperRunParams &run_params,
                const SlsModelParams &model_params,
                const SLSParamsGenerator &params_generator)
      : run_params_(run_params), model_params_(model_params),
        params_generator_(params_generator),
        ctx_(test::mock::make_context_mock(run_params.context_type)),
        runner_{ctx_} {}

  virtual ~SLSTestHelper() = default;

  SLSType generate_data_types() const;

  PNMSLSOperation
  create_sls_op(const pnm::memory::EmbeddedTables *tables) const {
    const SLSType data_type = generate_data_types();

    return {static_cast<uint32_t>(model_params_.sparse_feature_size),
            pnm::make_const_view(model_params_.tables_rows_num), tables,
            data_type};
  }

  std::vector<T> run_sls(PNMSLSOperation &sls_op,
                         const SlsOpParams &sls_op_params,
                         const pnm::Runner &runner) const {
    const auto &[lS_indices, lS_lengths] = sls_op_params;

    auto psum_res = params_generator_.generate_psum(run_params_, model_params_,
                                                    kpsum_init_val<T>);

    sls_op.set_run_params(run_params_.mini_batch_size,
                          pnm::make_view(lS_lengths),
                          pnm::make_view(lS_indices),
                          pnm::view_cast<uint8_t>(pnm::make_view(psum_res)));

    for (int i = 0; i < run_params_.num_requests; ++i) {
      runner.run(sls_op);
    }

    return psum_res;
  }

  virtual void print_params() const {
    model_params_.print();
    run_params_.print();
  }

  HelperRunParams run_params_;
  SlsModelParams model_params_;
  const SLSParamsGenerator &params_generator_;
  pnm::ContextHandler ctx_;
  pnm::Runner runner_;
};

template <> inline SLSType SLSTestHelper<float>::generate_data_types() const {
  return SLSType::Float;
}

template <>
inline SLSType SLSTestHelper<uint32_t>::generate_data_types() const {
  return SLSType::Uint32;
}

} // namespace test_app

#endif // _SLS_TEST_HELPER_H
