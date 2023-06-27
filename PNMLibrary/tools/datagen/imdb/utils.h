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

#ifndef IMDB_TEST_UTILS_H
#define IMDB_TEST_UTILS_H

#include "imdb/utils/runner/avx2_scan.h"

#include "common/log.h"

#include "tools/datagen/imdb/column_generator/base.h"
#include "tools/datagen/imdb/column_generator/factory.h"
#include "tools/datagen/imdb/general/bit_vectors_io.h"
#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/columns_io.h"
#include "tools/datagen/imdb/general/index_vectors_io.h"
#include "tools/datagen/imdb/general/predicate_vecs_info.h"
#include "tools/datagen/imdb/general/predicate_vecs_io.h"
#include "tools/datagen/imdb/general/ranges_info.h"
#include "tools/datagen/imdb/general/ranges_io.h"
#include "tools/datagen/imdb/general/vector_of_vectors_info.h"
#include "tools/datagen/imdb/golden_vec_generator/gen.h"
#include "tools/datagen/imdb/max_values_generator/base.h"
#include "tools/datagen/imdb/max_values_generator/factory.h"
#include "tools/datagen/imdb/predicate_vec_generator/base.h"
#include "tools/datagen/imdb/predicate_vec_generator/factory.h"
#include "tools/datagen/imdb/range_generator/base.h"
#include "tools/datagen/imdb/range_generator/factory.h"
#include "tools/datagen/imdb/selectivities_generator/base.h"
#include "tools/datagen/imdb/selectivities_generator/factory.h"

#include "pnmlib/imdb/scan_types.h"

#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace tools::gen::imdb {

using namespace pnm::imdb;

/** @brief Common parameters among column, ranges and predictors generators*/
struct CommonGeneratorParams {
  // directory where certain data ( column, ranges or predictors) is
  // stored.
  std::filesystem::path root_;
  // In root_ directory can be contained several different sets of
  // ranges/predictors. Prefix is needed to distinguish them.
  std::optional<std::string> filename_prefix_;
  // Name of generator of  column/ranges/predictors. Needed for factories.
  std::string generator_name_;
};

/** @brief parameters for  column generator.
 *  Some of the parameters are optional - if not set, generator uses default
 * values.
 */
struct GeneratorColumnParams {
  CommonGeneratorParams common_params_;
  // size in bits of value in compressed column
  uint64_t entry_bit_size_;
  // count of elements in generated  column
  index_type entry_count_;
  // seed for random generator. If not set, seed std::random_device{}() is used
  std::optional<size_t> seed_;
  // step for positioned generator. Values are generated with this step. For
  // example, for step 3 values will be something like {2, 5, 8, ...} for step
  // -4 - {40, 36, 32, ...}
  std::optional<int32_t> step_;
  // Minimal value of entry in column. Value is used only if seed is set for
  // random generator or step is set for positioned generator. If min value is
  // not set, default value is used (0). If value exceedes 2^entry_bit_size -
  // exception is thrown.
  std::optional<compressed_element_type> min_value_;
  // Maximal value of  column entry.
  // Value is used only if min_value_ is set (and either step_ or seed_ is set
  // also). By default is used (2^entry_bit_size_ - 1). If value exceedes
  // (2^entry_bit_size_ - 1) or below min_value_ - exception is thrown.
  std::optional<compressed_element_type> max_value_;

  /** @brief constructs string based on values in fields of this struct.
   * This string is argument when generator is created by factory
   */
  std::string get_factory_str_arg() const {
    std::string gen_args = std::to_string(entry_bit_size_);
    const auto &gen_name = common_params_.generator_name_;
    if (gen_name == literals::column_random_generator) {
      if (seed_.has_value()) {
        gen_args += fmt::format(" {}", seed_.value());
      }
    } else if (gen_name == literals::column_positioned_generator) {
      if (step_.has_value()) {
        gen_args += fmt::format(" {}", step_.value());
      }
    }

    if (min_value_.has_value()) {
      gen_args += fmt::format(" {}", min_value_.value());
      if (max_value_.has_value()) {
        gen_args += fmt::format(" {}", max_value_.value());
      }
    }

    return gen_args;
  }
};

/** @brief parameters for generator of scan ranges.
 *  Some of the parameters are optional - if not set, generator uses default
 *  values.
 */
struct GeneratorRangesParams {
  CommonGeneratorParams common_params_;
  // Generator generate or at least try to generate scan ranges with given
  // selectivities. Selectivity is defined as results_count/entry_count, where
  // results_count - count of results of scan operation (number of elements in
  // index vector or bitvector), entry_count - count of elements in compressed
  //  column
  std::vector<double> selectivities_;
  // seed for random generator of scan ranges - if not set,
  // std::random_device{}() is used by default in generator
  std::optional<size_t> seed_;
  // selectivity_epsilon - needed for precise selectivity generator (the one
  // that actually tries to generate ranges with given selectivity with
  // epsilon). If not set, default value is used (see
  // PreciseRandomRangesGen). Used if seed_ is set (due to logic of
  // parsing arguments from string in factory of generators).
  std::optional<double> selectivity_epsilon_;
  // whether to generate golden vectors at the same time as ranges or not
  bool generate_golden_ = true;

  /** @brief constructs string based on values in fields of this struct.
   * This string is argument when generator is created by factory
   */
  std::string get_factory_str_arg() const {
    std::string gen_args;
    const auto &gen_name = common_params_.generator_name_;

    if (gen_name == literals::scan_ranges_generator_random ||
        gen_name == literals::scan_ranges_generator_random_precise) {

      if (!seed_.has_value()) {
        return gen_args;
      }
      gen_args = std::to_string(seed_.value());

      if (!selectivity_epsilon_.has_value()) {
        return gen_args;
      }

      gen_args += std::to_string(selectivity_epsilon_.value());
    }

    return gen_args;
  }
};

/** @brief parameters for generator of predicate vectors
 *  Some of the parameters are optional - if not set, generator uses default
 *  values.
 */
struct GeneratorPredicateVectorsParams {
  CommonGeneratorParams common_params_;
  // Generator generate or at least try to generate predictors with given
  // selectivities. Selectivity is defined as results_count/entry_count, where
  // results_count - count of results of scan operation (number of elements in
  // index vector or bitvector), entry_count - count of elements in compressed
  //  column
  std::vector<double> selectivities_;
  std::vector<compressed_element_type> predicate_max_values_;
  // seed for random generator of predicate vectors - if not set,
  // std::random_device{}() is used by default in generator
  std::optional<size_t> seed_;
  // selectivity_epsilon - needed for precise selectivity generator (the one
  // that actually tries to generate predictors with given selectivity with
  // epsilon). If not set, default value is used (see
  // PreciseRandomPredicateVecsGen). Used if seed_ is set (due to logic of
  // parsing arguments from string in factory of generators).
  std::optional<double> selectivity_epsilon_;
  // whether to generate golden vectors at the same time as predicate vectors or
  // not
  bool generate_golden_ = true;

  /** @brief constructs string based on values in fields of this struct.
   * This string is argument when generator is created by factory
   */
  std::string get_factory_str_arg() const {
    std::string gen_args;
    const auto &gen_name = common_params_.generator_name_;

    if (gen_name == literals::scan_ranges_generator_random ||
        gen_name == literals::scan_ranges_generator_random_precise) {

      if (!seed_.has_value()) {
        return gen_args;
      }
      gen_args = std::to_string(seed_.value());

      if (!selectivity_epsilon_.has_value()) {
        return gen_args;
      }

      gen_args += std::to_string(selectivity_epsilon_.value());
    }

    return gen_args;
  }
};

template <OperationType scan_operation>
using InputDataType =
    std::conditional_t<scan_operation == OperationType::InRange, Ranges,
                       Predictors>;
template <OperationType scan_operation>
using InputInfoType =
    std::conditional_t<scan_operation == OperationType::InRange, RangesInfo,
                       PredicateVectorsInfo>;
template <OutputType scan_output_type>
using OutputDataType =
    std::conditional_t<scan_output_type == OutputType::IndexVector,
                       IndexVectors, BitVectors>;
;

/** @brief Struct for data which is created by default generator (function
 * generate_data). It takes two template arguments - enums InList/Inrange
 * and IndexVector/BitVector. This structure contains data depending on value in
 * these enums. For example, if template type is OperationType::InRange,
 * then Ranges are stored, and Predicate vectors in case of
 * OperationType::InList.
 */
template <OperationType scan_operation, OutputType scan_output_type>
struct GeneratedData {
  compressed_vector column_;
  InputDataType<scan_operation> input_;
  OutputDataType<scan_output_type> result_;
};

// Function to mmap  column binary file and get info about
//  column structure.
inline ColumnsMmap get_mmap_columns(const std::filesystem::path &root) {
  const auto data_path = root / column_default_name;
  const auto info_path = root / column_info_default_name;

  return ColumnsMmap{data_path, info_path};
}

// Function to mmap  column binary file and get info about
// scan ranges structure.
inline PredicateVectorsMmap
get_mmap_predicate_vectors(const std::filesystem::path &root,
                           const std::string &filename_prefix) {
  const auto predicate_vecs_bin =
      root / (filename_prefix + predicate_vec_suffix_bin);
  const auto predicate_vecs_info =
      root / (filename_prefix + predicate_vec_suffix_info);
  return PredicateVectorsMmap{predicate_vecs_bin, predicate_vecs_info};
}

// Function to mmap scan ranges binary file and get info about
// scan ranges structure.
inline RangesMmap get_mmap_scan_ranges(const std::filesystem::path &root,
                                       const std::string &filename_prefix) {
  const auto scan_ranges_bin = root / (filename_prefix + scan_range_suffix_bin);
  const auto scan_ranges_info =
      root / (filename_prefix + scan_range_suffix_info);
  return RangesMmap{scan_ranges_bin, scan_ranges_info};
}

// Function to mmap bit vectors binary file and get info about
// vit vectors structure.
inline VectorOfVectorsMmap
get_mmap_bit_vectors(const std::filesystem::path &root,
                     const std::string &filename_prefix) {
  const auto bit_vectors_bin =
      root / (filename_prefix + literals::bit_vec_suffix_bin);
  const auto bit_vectors_info =
      root / (filename_prefix + literals::bit_vec_suffix_info);
  return VectorOfVectorsMmap{bit_vectors_bin, bit_vectors_info};
}

// Function to mmap index vectors binary file and get info about
// index vectors structure.
inline VectorOfVectorsMmap
get_mmap_index_vectors(const std::filesystem::path &root,
                       const std::string &filename_prefix) {
  const auto index_vec_bin =
      root / (filename_prefix + literals::index_vec_suffix_bin);
  const auto index_vec_info =
      root / (filename_prefix + literals::index_vec_suffix_info);
  return VectorOfVectorsMmap{index_vec_bin, index_vec_info};
}

/** @brief This function either finds  column on given root_path and loads
 * from file or generates it, stores to file and returns it
 */
inline compressed_vector
get_or_create_test_columns(const GeneratorColumnParams &params) {
  const auto &gen_params = params.common_params_;
  const auto &gen_name = gen_params.generator_name_;
  const auto &gen_args = params.get_factory_str_arg();
  const auto &root = gen_params.root_;
  if (std::filesystem::exists(root / column_default_name)) {
    pnm::log::info("Table columns exist, skipping creation");
  } else {
    pnm::log::info("Table columns not found, creating");
    if (!std::filesystem::exists(root) &&
        !std::filesystem::create_directory(root)) {
      PNM_LOG_ERROR("Can't create directory to store columns, path = {}.",
                    root.string());
    }

    const auto &factory = ColumnGeneratorFactory::default_factory();
    auto generator = factory.create(gen_name, gen_args);

    const ColumnsInfo info(params.entry_bit_size_, params.entry_count_);
    generator->create_and_store(root, info);
  }
  const auto columns_map = get_mmap_columns(root);
  return ColumnsIO::load_from_file(columns_map);
}

/** @brief Loading  column and info from file */
inline std::pair<std::shared_ptr<const compressed_vector>, ColumnsInfo>
get_column_with_info(const std::filesystem::path &root) {

  auto column_mmap = get_mmap_columns(root);
  auto column = std::make_shared<const compressed_vector>(
      ColumnsIO::load_from_file(column_mmap));
  return std::make_pair(std::move(column), column_mmap.info);
}

/** @brief Computation of IndexVectors and BitVectors and saving them to files*/
template <typename Predicate>
inline void compute_and_store_golden(const std::filesystem::path &root,
                                     const std::string &filename_prefix,
                                     const compressed_vector &column,
                                     const Predicate &predicates) {
  using Gen = ImdbGoldenVecGenerator;
  Gen::compute_and_store_golden<BitVectors, Predicate>(root, filename_prefix,
                                                       column, predicates);
  Gen::compute_and_store_golden<IndexVectors, Predicate>(root, filename_prefix,
                                                         column, predicates);
}

/** @brief This function either finds predicate vectors on given root_path and
 * loads from file or generates it, stores to file and returns it. Already
 * existing  column saved on root_path is required if function generates
 * predicate vectors.
 */
inline Predictors get_or_create_test_predicate_vectors(
    const GeneratorPredicateVectorsParams &params) {
  const auto &gen_params = params.common_params_;
  const auto &gen_name = gen_params.generator_name_;
  const auto &gen_args = params.get_factory_str_arg();
  const auto &root = gen_params.root_;
  const auto prefix = gen_params.filename_prefix_.value_or("");

  if (std::filesystem::exists(root / (prefix + predicate_vec_suffix_bin))) {
    pnm::log::info("Predicate vectors exist, skipping creation");
  } else {
    pnm::log::info("Predicate vectors not found, creating");

    const auto [column, column_info] = get_column_with_info(root);

    const auto &factory = PredicateVecsGeneratorFactory::default_factory();
    auto predicate_generator = factory.create(gen_name, gen_args, column);

    const PredicateVectorsInfo info(params.selectivities_,
                                    params.predicate_max_values_);
    const auto predictors = predicate_generator->create(info, column_info);
    PredicateVectorsIO::store_to_file(predictors, root, prefix, info);

    if (params.generate_golden_) {
      compute_and_store_golden(root, prefix, *column, predictors);
    }
  }
  const auto predictors_mmap = get_mmap_predicate_vectors(root, prefix);
  return PredicateVectorsIO::load_from_file(predictors_mmap);
}

/** @brief This function either finds scan ranges on given root_path and loads
 * from file or generates it, stores to file and returns it. Already existing
 *  column saved on root_path is required if function generates scan
 * ranges.
 */
inline Ranges
get_or_create_test_scan_ranges(const GeneratorRangesParams &params) {
  const auto &gen_params = params.common_params_;
  const auto &gen_name = gen_params.generator_name_;
  const auto &gen_args = params.get_factory_str_arg();
  const auto &root = gen_params.root_;
  const auto prefix = gen_params.filename_prefix_.value_or("");

  if (std::filesystem::exists(root / (prefix + scan_range_suffix_bin))) {
    pnm::log::info("Scan ranges exist, skipping creation");
  } else {
    pnm::log::info("Scan ranges not found, creating");

    const auto [column, column_info] = get_column_with_info(root);

    const auto &factory = RangesGeneratorFactory::default_factory();
    auto ranges_generator = factory.create(gen_name, gen_args, column);

    const RangesInfo info(params.selectivities_);
    const auto ranges = ranges_generator->create(info, column_info);
    RangesIO::store_to_file(ranges, root, prefix, info);

    if (params.generate_golden_) {
      compute_and_store_golden(root, prefix, *column, ranges);
    }
  }

  const auto ranges_mmap = get_mmap_scan_ranges(root, prefix);
  return RangesIO::load_from_file(ranges_mmap);
}

// all functions below namespace details are used by function generate_data
namespace details {
template <typename T> struct always_false : std::false_type {};

inline ColumnsInfo create_default_column_info() {
  return ColumnsInfo(10, 10'000);
}

template <OperationType scan_operation>
inline InputInfoType<scan_operation> create_input_info() {
  const auto column_info = create_default_column_info();
  const auto entry_bit_size = column_info.entry_bit_size();
  const auto num_requests = 10UL;
  const auto &selectivity_factory =
      SelectivitiesGeneratorFactory::default_factory();
  auto selectivity_gen =
      selectivity_factory.create(literals::selectivities_random_gen);
  auto selectivities = selectivity_gen->create(num_requests, 0.01, 0.95);

  if constexpr (scan_operation == OperationType::InRange) {
    return RangesInfo(std::move(selectivities));
  } else if constexpr (scan_operation == OperationType::InList) {
    const auto &max_values_factory =
        MaxValuesGeneratorFactory::default_factory();
    auto values_gen = max_values_factory.create(
        literals::max_values_random_generator, std::to_string(entry_bit_size));
    auto max_values = values_gen->create(num_requests, entry_bit_size, 1,
                                         (1UL << entry_bit_size) - 1);
    return PredicateVectorsInfo(std::move(selectivities),
                                std::move(max_values));
  } else {
    static_assert(details::always_false<InputDataType<scan_operation>>::value,
                  "Scan operation type is not supported");
  }
}

inline compressed_vector generate_column() {
  const auto &factory = ColumnGeneratorFactory::default_factory();
  const auto info = create_default_column_info();
  auto generator = factory.create(literals::column_random_generator,
                                  std::to_string(info.entry_bit_size()));
  return generator->create(info);
}

template <OperationType scan_operation>
inline InputDataType<scan_operation> generate_input() {
  const auto column_info = create_default_column_info();
  const auto input_info = create_input_info<scan_operation>();
  if constexpr (scan_operation == OperationType::InRange) {
    const auto &factory = RangesGeneratorFactory::default_factory();
    auto generator = factory.create(literals::scan_ranges_generator_random);
    return generator->create(input_info, column_info);
  } else if constexpr (scan_operation == OperationType::InList) {
    const auto &factory = PredicateVecsGeneratorFactory::default_factory();
    auto generator = factory.create(literals::predictor_generator_random);
    return generator->create(input_info, column_info);
  } else {
    static_assert(details::always_false<InputDataType<scan_operation>>::value,
                  "Scan operation type is not supported");
  }
}

template <OperationType scan_operation, OutputType scan_output_type,
          typename Runner = AVX2Runner>
inline OutputDataType<scan_output_type>
calculate_output(const compressed_vector &column,
                 const InputDataType<scan_operation> &input) {
  static_assert(scan_operation == OperationType::InList ||
                    scan_operation == OperationType::InRange,
                "Scan operation type is not supported");
  static_assert(scan_output_type == OutputType::BitVector ||
                    scan_output_type == OutputType::IndexVector,
                "Scan output type is not supported");

  using Out = OutputDataType<scan_output_type>;
  using In = InputDataType<scan_operation>;
  return ImdbGoldenVecGenerator::scan<Out, In, Runner>(column, input);
}
} // namespace details

/** @brief Function for generating some data without setting parameters.
 * All parameters are hardcoded in functions in namespace details above.
 * Function takes 3 template parameters - type of operation, type of output and
 * type of runner
 */
template <OperationType scan_operation, OutputType scan_output_type,
          typename Runner = AVX2Runner>
inline GeneratedData<scan_operation, scan_output_type> generate_data() {
  using namespace details;
  GeneratedData<scan_operation, scan_output_type> data;
  data.column_ = generate_column();
  data.input_ = generate_input<scan_operation>();
  data.result_ = calculate_output<scan_operation, scan_output_type, Runner>(
      data.column_, data.input_);
  return data;
}

/** @brief Function for generating some data with parameters and storing them at
 * s_root. Function takes 3 template parameters - type of operation,
 * type of output and type of runner for calculating output
 */
template <OperationType scan_operation, OutputType scan_output_type,
          typename Runner = AVX2Runner>
inline GeneratedData<scan_operation, scan_output_type>
generate_and_store_data(const std::filesystem::path &columns_root,
                        std::string column_generator_type,
                        std::string pred_generator_type, uint64_t bit_size,
                        index_type column_size, uint64_t inputs_size,
                        double selectivity, bool generate_golden,
                        size_t seed = std::random_device{}()) {
  const auto *op_type_str =
      scan_operation == OperationType::InList ? "list" : "range";
  auto dir_name = fmt::format("imdb_{}_{}_{}_{}b_{}ts_{}is_{}sel", op_type_str,
                              column_generator_type, pred_generator_type,
                              bit_size, column_size, inputs_size, selectivity);
  auto gen_root = columns_root / dir_name;
  GeneratedData<scan_operation, scan_output_type> data;

  // generate tables
  GeneratorColumnParams column_params;
  column_params.common_params_.root_ = gen_root;
  column_params.common_params_.generator_name_ = column_generator_type;
  column_params.entry_bit_size_ = bit_size;
  column_params.entry_count_ = column_size;
  column_params.step_ = 7; // arbitrary constant (only for positioned generator)
  column_params.seed_ = seed; // only for random generator
  data.column_ = get_or_create_test_columns(column_params);

  const char *inputs_filename_prefix = "inputs";

  // generate inputs
  if constexpr (scan_operation == OperationType::InList) {
    GeneratorPredicateVectorsParams pred_params;
    pred_params.common_params_.root_ = gen_root;
    pred_params.common_params_.generator_name_ = pred_generator_type;
    pred_params.common_params_.filename_prefix_ = inputs_filename_prefix;
    pred_params.predicate_max_values_ = std::vector<compressed_element_type>(
        inputs_size, (1UL << bit_size) - 1);
    pred_params.selectivities_ = std::vector<double>(inputs_size, selectivity);
    pred_params.generate_golden_ = generate_golden;
    pred_params.seed_ = seed;

    data.input_ = get_or_create_test_predicate_vectors(pred_params);
  } else {
    GeneratorRangesParams pred_params;
    pred_params.common_params_.root_ = gen_root;
    pred_params.common_params_.generator_name_ = pred_generator_type;
    pred_params.common_params_.filename_prefix_ = inputs_filename_prefix;
    pred_params.selectivities_ = std::vector<double>(inputs_size, selectivity);
    pred_params.generate_golden_ = generate_golden;
    pred_params.seed_ = seed;

    data.input_ = get_or_create_test_scan_ranges(pred_params);
  }

  if (generate_golden) {
    // The data is created and stored inside of `get_or_create_` functions
    // called above
    if constexpr (scan_output_type == OutputType::BitVector) {
      const auto loaded_mmap =
          get_mmap_bit_vectors(gen_root, inputs_filename_prefix);
      data.result_ = BitVectorsIO::load_from_file(loaded_mmap);
    } else {
      const auto loaded_mmap =
          get_mmap_index_vectors(gen_root, inputs_filename_prefix);
      data.result_ = IndexVectorsIO::load_from_file(loaded_mmap);
    }
  }

  return data;
}

} // namespace tools::gen::imdb

#endif // IMDB_TEST_UTILS_H
