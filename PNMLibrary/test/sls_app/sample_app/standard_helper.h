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

#ifndef _SLS_TEST_STANDARD_HELPER_H
#define _SLS_TEST_STANDARD_HELPER_H

#include "common.h"

#include "test/sls_app/api/golden.h"
#include "test/sls_app/api/helper.h"
#include "test/sls_app/api/structs.h"
#include "test/sls_app/api/utils.h"
#include "tools/datagen/sls/general/traits.h"
#include "tools/datagen/sls/utils.h"

#include <filesystem>
#include <string>
#include <vector>

namespace test_app {

template <typename T> class SLSTestStandardHelper : public SLSTestHelper<T> {
public:
  using SLSTestHelper<T>::SLSTestHelper;

  SLSTestStandardHelper(const StandardRunParams &run_params,
                        const SlsModelParams &model_params,
                        const SLSParamsGenerator &params_generator)
      : SLSTestHelper<T>(run_params.helper_params, model_params,
                         params_generator),
        testapp_run_params_(run_params.testapp_params) {}

  ~SLSTestStandardHelper() override = default;

  SlsOpParams
  generate_sls_run_params_impl(const std::filesystem::path &root) const {
    SlsOpParams op_params;

    const std::string indices_set_name =
        generate_indices_path(this->run_params_, testapp_run_params_);

    const std::string generator_lengths_name =
        testapp_run_params_.lengths_generation_type;

    const std::string generator_indices_name = "random";

    const std::string entry_type = DataTypeTraits<T>().name;

    const sls::tests::GeneratorWithOverflowParams params{
        .max_num_lookup = testapp_run_params_.max_num_lookup,
        .min_num_lookup = testapp_run_params_.min_num_lookup,
        .mini_batch_size = this->run_params_.mini_batch_size,
        .root = root,
        .prefix = indices_set_name,
        .generator_lengths_name = generator_lengths_name,
        .overflow_type =
            overflow_type_to_string(testapp_run_params_.overflow_type),
        .generator_indices_name = generator_indices_name,
        .entry_type = entry_type};

    sls::tests::get_or_create_test_indices(params, op_params.lS_lengths,
                                           op_params.lS_indices);

    return op_params;
  }

  double perform_golden_check(const std::vector<T> &psum_res,
                              const std::filesystem::path &root) const {
    const auto indices_set_name =
        generate_indices_path(this->run_params_, testapp_run_params_);
    const auto read_golden_vec = load_golden(root, indices_set_name);
    return test_app::perform_golden_check<T>(psum_res, read_golden_vec, 1);
  }

  std::vector<T> load_golden(const std::filesystem::path &root,
                             const std::string &indices_set_name) const {
    std::vector<T> golden;
    auto in = sls::tests::get_golden_vector(root, indices_set_name);
    auto size = stream_size(in);
    golden.resize(size / sizeof(T));
    in.read(reinterpret_cast<char *>(golden.data()), size);

    return golden;
  }

  void print_params() const override {
    SLSTestHelper<T>::print_params();
    testapp_run_params_.print();
  }

  TestAppRunParams testapp_run_params_;
};

} // namespace test_app

#endif // _SLS_TEST_STANDARD_HELPER_H
