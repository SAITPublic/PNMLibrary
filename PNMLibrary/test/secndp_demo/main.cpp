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

#include "test_units.h"

#include "secure/plain/device/trivial_cpu.h"
#include "secure/plain/device/untrusted_sls.h"
#include "secure/plain/sls.h"

#include "common/topology_constants.h"

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <gtest/internal/gtest-type-util.h>

#include "CLI/App.hpp"
#include "CLI/CLI.hpp" // NOLINT(misc-include-cleaner)
#include "CLI/Error.hpp"
#include "CLI/Validators.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

/*! Global Environment class for all tests
 * */
class TestEnvironment : public ::testing::Environment {
public:
  static auto &root() {
    static std::filesystem::path root_v{};
    return root_v;
  }
  static auto &indices_set_name() {
    static std::string indices_set_name_v{};
    return indices_set_name_v;
  }
  static auto &num_instances() {
    static unsigned num_instances_v{};
    return num_instances_v;
  }
  static auto &with_tag() {
    static bool with_tag_v{};
    return with_tag_v;
  }

  TestEnvironment(int argc, char **argv) {
    CLI::App app{"Demo application for SecNDP API"};

    app.add_option("num_instances", num_instances(),
                   "Number of engines to start")
        ->required();
    app.add_option("tables_root", root(), "Path to folder containing test data")
        ->required()
        ->check(CLI::ExistingDirectory);
    app.add_option("indices_set_name", indices_set_name(),
                   "Name of indices data to use. This should be contained in "
                   "tables_root folder")
        ->required();
    app.add_flag("-t,--tag", with_tag(),
                 "Enable/disable tags during encryption")
        ->default_val(false);

    try {
      app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
      std::exit(app.exit(e));
    }
  }
};

// Fixture for typed tests
template <typename T> struct SecNDPTestFixture : public ::testing::Test {
  using test_object_type = T;
};

using pnm::sls::secure::ProdConsCpuRunner;
using pnm::sls::secure::ProdConsSlsRunner;
using pnm::sls::secure::SyncCpuRunner;
using pnm::sls::secure::SyncSlsRunner;

using CPU = pnm::sls::secure::TrivialCPU<uint32_t>;
using SLS_ACC = pnm::sls::secure::UntrustedDevice<uint32_t>;

using SecNDPTestObjects = ::testing::Types<
    SecNDPSlsSequential<uint32_t, SyncCpuRunner, CPU>,
    SecNDPSlsSequential<uint32_t, SyncSlsRunner, SLS_ACC>,

    SecNDPSlsSequential<uint32_t, ProdConsCpuRunner, CPU>,
    SecNDPSlsSequential<uint32_t, ProdConsSlsRunner, SLS_ACC>,

    SecNDPSlsMultiThreads<uint32_t, SyncCpuRunner, CPU>,
    SecNDPSlsMultiThreads<uint32_t, SyncSlsRunner, SLS_ACC>,

    SecNDPSlsMultiThreads<uint32_t, ProdConsCpuRunner, CPU>,
    SecNDPSlsMultiThreads<uint32_t, ProdConsSlsRunner, SLS_ACC>,

    SecNDPSlsMultiProcesses<uint32_t, SyncCpuRunner, CPU>,
    SecNDPSlsMultiProcesses<uint32_t, SyncSlsRunner, SLS_ACC>,

    SecNDPSlsMultiProcesses<uint32_t, ProdConsCpuRunner, CPU>,
    SecNDPSlsMultiProcesses<uint32_t, ProdConsSlsRunner, SLS_ACC>>;

TYPED_TEST_SUITE(SecNDPTestFixture, SecNDPTestObjects);

TYPED_TEST(SecNDPTestFixture, SecNDPRun) {
  if (TestEnvironment::with_tag() &&
      pnm::sls::device::topo().Bus == pnm::sls::device::BusType::CXL) {
    GTEST_SKIP() << "SLS-CXL does not support tags\n";
  }

  typename TestFixture ::test_object_type const test_object{
      TestEnvironment::num_instances(), TestEnvironment::with_tag()};
  test_object.print();
  test_object.run(TestEnvironment::root(), TestEnvironment::indices_set_name());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  for (const std::string &arg : testing::internal::GetArgvs()) {
    if (arg == "--gtest_list_tests") {
      return RUN_ALL_TESTS();
    }
  }

  ::testing::AddGlobalTestEnvironment(new TestEnvironment{argc, argv});
  return RUN_ALL_TESTS();
}
