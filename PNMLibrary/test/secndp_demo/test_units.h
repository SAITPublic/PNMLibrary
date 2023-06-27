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

#ifndef _TEST_UNITS_H
#define _TEST_UNITS_H

#include "secndp_demo_common.h"

#include "tools/test_utils/device_params_gen.h"

#include "pnmlib/common/views.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <sched.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <string>
#include <vector>

/*! Run SecNDP in sequential mode.
 * */
template <typename T, template <typename DataType = T> typename Runner,
          typename SLSDevice>
class SecNDPSlsSequential {
public:
  using runner_type = Runner<T>;

  explicit SecNDPSlsSequential([[maybe_unused]] unsigned num_process,
                               bool with_tag = false)
      : with_tag_{with_tag} {}

  void run(const std::filesystem::path &root,
           const std::string &indices_set_name) const {
    auto dataset = TestLoader<T>(root, indices_set_name);
    Runner<T> runner;
    auto params =
        sls::tests::SimpleDeviceParamsGenerator<SLSDevice>::generate_params(
            pnm::make_view(dataset.tables_meta().rows()),
            dataset.tables_meta().cols(), with_tag_);
    runner.init(&params);
    run_impl(runner, dataset);
  }

  virtual const std::string &get_name() const {
    static const std::string name{"SecNDPSlsSequential"};
    return name;
  }

  void print() const {
    fmt::print("\n{}. MAC verification is {}\n", get_name(),
               (this->with_tag_ ? "ON" : "OFF"));
  }

  virtual ~SecNDPSlsSequential() = default;

protected:
  void run_impl(Runner<T> &runner, TestLoader<T> &dataset) const;
  bool with_tag_;
};

template <typename T, template <typename DataType = T> typename Runner,
          typename SLSDevice>
void SecNDPSlsSequential<T, Runner, SLSDevice>::run_impl(
    Runner<T> &runner, TestLoader<T> &dataset) const {
  TimeStatistics t("MAIN TEST RUN");

  fmt::print("Encrypt and offload embedded tables to device...\n");
  t.start_section("- Tables encryption/offload");
  runner.load_tables(dataset.tables().data(),
                     pnm::make_view(dataset.tables_meta().rows()),
                     dataset.tables_meta().cols(), with_tag_);
  t.end_section("- Tables encryption/offload");
  fmt::print("Done!\n");

  t.start_section("- Prepare SLS requests");
  auto requests_count = dataset.tables_meta().num_tables() *
                        dataset.indices_meta().minibatch_size();
  std::vector<T> psum(dataset.tables_meta().cols() * requests_count);
  std::vector<uint8_t> row_checks(requests_count, false); // use only with_tag
  t.end_section("- Prepare SLS requests");

  static constexpr auto NUM_RUNS = 10;
  auto NUM_RUNS_STR = std::to_string(NUM_RUNS);
  auto sls_label_name = "- Make SLS (x" + NUM_RUNS_STR + ")";
  t.start_section(sls_label_name.c_str());
  bool result = true;
  for (auto i = 0; i < NUM_RUNS; ++i) {
    result &= runner.run(dataset.indices_meta().minibatch_size(),
                         pnm::make_view(dataset.indices_meta().lengths()),
                         pnm::make_view(dataset.indices()),
                         pnm::view_cast<uint8_t>(pnm::make_view(psum)),
                         pnm::make_view(row_checks));
  }
  t.end_section(sls_label_name.c_str());

  EXPECT_TRUE(result) << "Overall SLS verification result is failed";
  t.start_section("- Check SLS result");
  Utils::compare_with_golden(dataset.golden(), psum,
                             dataset.tables_meta().cols(), row_checks,
                             with_tag_);
  t.end_section("- Check SLS result");
  t.summarize();
}

/*! Run SecNDP in multithread mode
 * */
template <typename T, template <typename DataType = T> typename Runner,
          typename SLSDevice>
class SecNDPSlsMultiThreads : public SecNDPSlsSequential<T, Runner, SLSDevice> {
public:
  SecNDPSlsMultiThreads(unsigned num_threads, bool with_tag)
      : SecNDPSlsSequential<T, Runner, SLSDevice>(num_threads, with_tag),
        nthreads_{num_threads} {}

  const std::string &get_name() const override {
    static const std::string name{"SecNDPSlsMultiThreads"};
    return name;
  }

  void run(const std::filesystem::path &root,
           const std::string &indices_set_name) const {
    auto dataset = TestLoader<T>(root, indices_set_name);

    std::vector<std::future<void>> workers;
    const auto params =
        sls::tests::GroupedParameterGenerator<SLSDevice>::generate_params(
            pnm::make_view(dataset.tables_meta().rows()),
            dataset.tables_meta().cols(), nthreads_, this->with_tag_);
    for (auto i = 0U; i < nthreads_; ++i) {
      auto core = [this, i, &dataset, &params]() {
        Runner<T> runner;
        runner.init(&params[i]);
        this->run_impl(runner, dataset);
      };
      workers.emplace_back(std::async(std::launch::async, core));
    }
    for (auto &w : workers) {
      ASSERT_NO_THROW(w.get());
    }
  }

private:
  unsigned nthreads_;
};

/*! Run SecNDP in multithread mode
 * */
template <typename T, template <typename DataType = T> typename Runner,
          typename SLSDevice>
class SecNDPSlsMultiProcesses
    : public SecNDPSlsSequential<T, Runner, SLSDevice> {
public:
  SecNDPSlsMultiProcesses(unsigned num_process, bool with_tag)
      : SecNDPSlsSequential<T, Runner, SLSDevice>(num_process, with_tag),
        nproc_{num_process} {}

  const std::string &get_name() const override {
    static const std::string name{"SecNDPSlsMultiProcesses"};
    return name;
  }

  void run(const std::filesystem::path &root,
           const std::string &indices_set_name) const {
    auto dataset = TestLoader<T>(root, indices_set_name);

    auto *semaphore = static_cast<sem_t *>(
        mmap(nullptr, sizeof(sem_t), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    static constexpr auto SHARE_PROCESS = 1;
    ASSERT_EQ(sem_init(semaphore, SHARE_PROCESS, 0), 0) << "errno: " << errno;

    std::vector<pid_t> child;
    size_t child_id;
    for (child_id = 0U; child_id < nproc_; ++child_id) {
      auto pid = fork();
      ASSERT_NE(pid, -1);
      if (pid == 0) {
        break;
      }
      child.emplace_back(pid);
    }

    // If we are here and didn't crush, child vec. contains nproc_ elements for
    // parent and less for child
    if (child.size() == nproc_) { // Parent process
      // Do some awesome parent stuff here

      // Notify children
      std::for_each(child.begin(), child.end(), [semaphore](auto &) {
        ASSERT_EQ(sem_post(semaphore), 0) << "errno: " << errno;
      });

      for (auto pid : child) {
        int status;
        ASSERT_EQ(waitpid(pid, &status, 0), pid)
            << "errno: " << errno << " Status:" << status;
      }

      ASSERT_EQ(sem_destroy(semaphore), 0);
      munmap(semaphore, sizeof(*semaphore));
    } else { // Child code
      ASSERT_EQ(sem_wait(semaphore), 0) << "errno: " << errno;
      const auto params =
          sls::tests::GroupedParameterGenerator<SLSDevice>::generate_params(
              pnm::make_view(dataset.tables_meta().rows()),
              dataset.tables_meta().cols(), nproc_, this->with_tag_);
      {
        Runner<T> runner;
        runner.init(&params[child_id]);
        EXPECT_NO_THROW(this->run_impl(runner, dataset));
      }
      std::exit(::testing::Test::HasFailure());
    }
  }

private:
  unsigned nproc_;
};

#endif //_TEST_UNITS_H
