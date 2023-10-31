#include "imdb/operation/scan.h"

#include "core/device/imdb/base.h"
#include "core/device/imdb/reg_mem.h"
#include "core/device/imdb/simulator_core.h"

#include "common/log.h"
#include "common/mapped_file.h"
#include "common/topology_constants.h"

#include "tools/datagen/imdb/column_generator/factory.h"
#include "tools/datagen/imdb/general/columns_info.h"
#include "tools/datagen/imdb/general/ranges_info.h"
#include "tools/datagen/imdb/golden_vec_generator/gen.h"
#include "tools/datagen/imdb/range_generator/factory.h"
#include "tools/datagen/imdb/selectivities_generator/factory.h"
#include "tools/datagen/imdb/utils.h"

#include "pnmlib/imdb/libimdb.h"
#include "pnmlib/imdb/scan.h"
#include "pnmlib/imdb/scan_types.h"

#include "pnmlib/core/buffer.h"
#include "pnmlib/core/context.h"
#include "pnmlib/core/device.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/views.h"

#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-type-util.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using pnm::imdb::OperationType;
using pnm::imdb::OutputType;
using pnm::imdb::device::RegisterPointers;
using pnm::imdb::device::SimulatorCore;
using pnm::imdb::device::to_volatile_ptrs;
using pnm::operations::Scan;
using pnm::utils::as_vatomic;
using tools::gen::imdb::GeneratedData;
using ImdbBaseDev = pnm::imdb::device::BaseDevice;
using pnm::imdb::device::topo;

// Simulated register storage in ordinary memory.
// Since we use this storage for IMDB simulator core tests, we don't need to
// lock IMDB threads in the system resource manager.
struct DummyRegisterStorage {
  volatile CommonCSR common{};
  volatile StatusCSR status{};
  std::unique_ptr<volatile ThreadCSR[]> thread{};
  RegisterPointers reg_ptrs_{};

  DummyRegisterStorage() {
    thread = std::unique_ptr<ThreadCSR[]>(new ThreadCSR[topo().NumOfThreads]());
    reg_ptrs_ = {.common = &common,
                 .status = &status,
                 .thread = to_volatile_ptrs(thread, topo().NumOfThreads)};
  }

  RegisterPointers &get_pointers() { return reg_ptrs_; }
};

// Base test fixture.
class SimulatorTests : public ::testing::Test {
public:
  static constexpr auto IMDB_SHMEM_PATH = "/dev/shm/imdb-data";

  SimulatorTests()
      : memory_(IMDB_SHMEM_PATH,
                ctx_->device()->as<ImdbBaseDev>()->memory_size()) {}

  struct ScanOperationProxy {
  public:
    template <typename R> static auto make_result_view(R &result) {
      if constexpr (std::is_same_v<R, pnm::imdb::index_vector>) {
        return pnm::views::make_view(result);
      } else {
        return pnm::views::make_view(result.container());
      }
    }

    template <typename R>
    ScanOperationProxy(const pnm::imdb::compressed_vector &db,
                       pnm::imdb::RangeOperation predicate, R &result,
                       OutputType output_type)
        : db_{pnm::views::make_view(db.container()), ctx_},
          result_{make_result_view(result), ctx_}, predicate_{predicate},
          compression_{db.value_bits()}, size_{db.size()},
          output_type_{output_type} {}

    template <typename R>
    ScanOperationProxy(const pnm::imdb::compressed_vector &db,
                       const pnm::imdb::bit_vector &predicate, R &result,
                       OutputType output_type)
        : db_{pnm::views::make_view(db.container()), ctx_},
          result_{make_result_view(result), ctx_},
          predicate_{pnm::memory::Buffer<uint32_t>{
              pnm::views::make_view(predicate.container()), ctx_}},
          in_list_max_value_{predicate.size()}, compression_{db.value_bits()},
          size_{db.size()}, output_type_{output_type} {}

    Scan operation() const {
      auto visitor = [this](auto &&p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, pnm::imdb::RangeOperation>) {
          return Scan(Scan::Column{compression_, size_, &db_}, p, &result_,
                      output_type_);
        } else {
          return Scan(Scan::Column{compression_, size_, &db_},
                      Scan::InListPredicate{in_list_max_value_, &p}, &result_,
                      output_type_);
        }
      };

      return std::visit(visitor, predicate_);
    }

    void sync_result() { result_.copy_from_device(); }

  private:
    pnm::ContextHandler ctx_ = pnm::make_context(pnm::Device::Type::IMDB);
    pnm::memory::Buffer<uint32_t> db_;
    pnm::memory::Buffer<uint32_t> result_;

    std::variant<pnm::memory::Buffer<uint32_t>, pnm::imdb::RangeOperation>
        predicate_;
    uint64_t in_list_max_value_ = 0;

    uint64_t compression_, size_;
    OutputType output_type_;
  };

protected:
  // Return pointers to IMDB registers used for testing.
  // `SimulatorDeviceTests` returns registers from system register file mapping.
  // `SimCoreTests` returns registers from dummy storage.
  virtual RegisterPointers &imdb_regs() = 0;

  // Setup IMDB registers.
  void setup_regs(size_t thread_id, const pnm::imdb::ScanOperation &scan_op) {
    auto *thread_regs = imdb_regs().thread[thread_id];
    scan_op.write_registers(thread_regs);

    // This should not be done on real hardware (because this register is
    // read-only), but needs to be done on a simulator to distinguish between
    // the situations when the scan operation has not yet started and when it
    // has finished. The delay between writing to the control register and a
    // simulated engine actually starting the operation (and updating the status
    // register) is inevitable and unpredictable on a simulator, because the OS
    // scheduler gives no guarantees how much time will pass until the engine
    // thread updates the status register before scanning.
    thread_regs->THREAD_STATUS = 0;
  }

  // Write to registers to start a scan operation.
  void start_scan(size_t thread_id, OutputType op_type) {
    uint32_t stored_value = 1;
    switch (op_type) {
    case OutputType::BitVector:
      break;
    case OutputType::IndexVector:
      stored_value |= 0b10;
      break;
    }

    auto &thread_regs = *imdb_regs().thread[thread_id];
    as_vatomic<uint32_t>(&thread_regs.THREAD_STATUS)->store(THREAD_STATUS_BUSY);
    as_vatomic<uint32_t>(&thread_regs.THREAD_CTRL)->store(stored_value);
  }

  // Read the THREAD_STATUS thread register to check if the engine has finished
  // execution or not. Returns true if the engine has finished an operation,
  // false otherwise.
  bool poll_status(size_t thread_id) {
    auto &thread_regs = *imdb_regs().thread[thread_id];
    auto status = as_vatomic<uint32_t>(&thread_regs.THREAD_STATUS)->load();
    if (status != THREAD_STATUS_BUSY) {
      return true;
    }
    // Yield is not useful for the hardware device, but speeds up the simulator
    // a little bit.
    std::this_thread::yield();
    return false;
  }

  // Read the IMDB_COM_STATUS register to check the status of all threads
  // at once atomically. For each thread, returns true if the respective thread
  // is idle, false if it is busy.
  std::vector<bool> poll_all() {
    const uint32_t status =
        as_vatomic<uint32_t>(&imdb_regs().status->IMDB_COM_STATUS)->load();
    std::vector<bool> result(topo().NumOfThreads);
    for (uint32_t i = 0; i < topo().NumOfThreads; ++i) {
      result[i] = (status & (1U << i)) != 0;
    }
    return result;
  }

  template <OutputType output_type>
  auto create_output_buf(pnm::imdb::compressed_vector_view column) {
    if constexpr (output_type == OutputType::IndexVector) {
      // Index vector should have some spare space to allow 32-byte AVX2 writes.
      return pnm::imdb::index_vector(column.size() + 16);
    } else {
      return pnm::imdb::bit_vector(column.size());
    }
  }

  // Don't require specific types for the result and output buffer (even though
  // we can name them) to simplify the code.
  template <OutputType output_type, typename Result>
  static void check_regs(const volatile ThreadCSR &thread_regs,
                         const Result &golden, const Result &output_buf) {
    EXPECT_EQ(thread_regs.THREAD_ERROR, 0);
    if constexpr (output_type == OutputType::IndexVector) {
      EXPECT_LE(thread_regs.THREAD_RES_SIZE_SB, output_buf.size());
      EXPECT_TRUE(std::equal(golden.begin(), golden.end(), output_buf.begin()));
    } else {
      EXPECT_EQ(thread_regs.THREAD_RES_SIZE_SB, output_buf.size());
      EXPECT_TRUE(std::equal(output_buf.begin(), output_buf.end(),
                             golden.begin(), golden.end()));
    }
  }

  // Run a full scan operation test cycle on provided data. The caller is
  // responsible for any necessary thread locking or enabling/disabling.
  template <OperationType operation_type, OutputType output_type>
  void test_on_data_unlocked(
      size_t thread_id,
      const GeneratedData<operation_type, output_type> &data) {
    auto output_buf = create_output_buf<output_type>(data.column_.view());

    for (size_t i = 0; i < data.input_.size(); ++i) {
      ScanOperationProxy proxy(data.column_, data.input_[i], output_buf,
                               output_type);
      auto user_op = proxy.operation();
      auto scan_op = pnm::imdb::ScanOperation(user_op);

      setup_regs(thread_id, scan_op);

      start_scan(thread_id, output_type);
      while (!poll_status(thread_id)) {
        // Only relevant for simulator, not for hardware device.
        std::this_thread::yield();
      }

      proxy.sync_result();

      check_regs<output_type>(*imdb_regs().thread[thread_id], data.result_[i],
                              output_buf);
    }
  }

  pnm::imdb::device::BaseDevice &device() {
    // This cast is safe because `ctx_` was created for the IMDB device.
    return *ctx_->device()->as<pnm::imdb::device::BaseDevice>();
  }

  pnm::ContextHandler ctx_ = pnm::make_context(pnm::Device::Type::IMDB);
  pnm::utils::MappedFile memory_;
};

// Main test fixture for simulator core tests (isolated from SimulatorDevice).
class SimCoreTests : public SimulatorTests {
public:
  SimCoreTests()
      : sim_(imdb_regs_.get_pointers(),
             static_cast<uint8_t *>(memory_.data())) {}

  template <OperationType operation_type, OutputType output_type>
  void test_on_data(size_t thread_id,
                    const GeneratedData<operation_type, output_type> &data) {
    sim_.enable_thread(thread_id);
    test_on_data_unlocked(thread_id, data);
    sim_.disable_thread(thread_id);
  }

protected:
  DummyRegisterStorage imdb_regs_;
  SimulatorCore sim_;

  RegisterPointers &imdb_regs() override { return imdb_regs_.get_pointers(); }
};

TEST_F(SimCoreTests, EnableDisableThreadWorks) {
  for (size_t thread_id = 0; thread_id < topo().NumOfThreads; ++thread_id) {
    EXPECT_FALSE(sim_.is_thread_enabled(thread_id));
  }

  sim_.enable_thread(0);
  EXPECT_TRUE(sim_.is_thread_enabled(0));
  for (size_t thread_id = 1; thread_id < topo().NumOfThreads; ++thread_id) {
    EXPECT_FALSE(sim_.is_thread_enabled(thread_id));
  }

  sim_.disable_thread(0);
  for (size_t thread_id = 0; thread_id < topo().NumOfThreads; ++thread_id) {
    EXPECT_FALSE(sim_.is_thread_enabled(thread_id));
  }
}

TEST_F(SimCoreTests, DestructorDisablesThreads) {
  for (size_t thread_id = 0; thread_id < topo().NumOfThreads; ++thread_id) {
    sim_.enable_thread(thread_id);
  }
  // If the destructor does not work correctly, this test will either hang or
  // abort.
}

TEST_F(SimCoreTests, EnableDisableValidation) {
  // Incorrect thread state.
  EXPECT_THROW(sim_.disable_thread(0), pnm::error::InvalidArguments);
  sim_.enable_thread(0);
  EXPECT_THROW(sim_.enable_thread(0), pnm::error::InvalidArguments);

  // Invalid thread id.
  const size_t invalid_thread_id = topo().NumOfThreads;
  EXPECT_THROW(sim_.is_thread_enabled(invalid_thread_id),
               pnm::error::InvalidArguments);
  EXPECT_THROW(sim_.enable_thread(invalid_thread_id),
               pnm::error::InvalidArguments);
  EXPECT_THROW(sim_.disable_thread(invalid_thread_id),
               pnm::error::InvalidArguments);
}

// Test proper thread enable/disable synchronization.
// Do the following `num_runs` times:
// - Select a random thread id
//   - If it should be enabled, disable it
//   - Otherwise, enable it
// - Track the golden enabled/disabled mask and compare the actual
//   enabled/disabled values to it after each iteration.
TEST_F(SimCoreTests, EnableDisableStress) {
  std::vector<bool> golden(topo().NumOfThreads);
  std::fill(golden.begin(), golden.end(), false);

  auto rng = std::mt19937_64(std::random_device{}());
  auto dist = std::uniform_int_distribution<size_t>(0, topo().NumOfThreads - 1);

  static constexpr size_t num_runs = 1000;

  for (size_t i = 0; i < num_runs; ++i) {
    const size_t chosen_thread_id = dist(rng);
    const bool should_be_enabled = golden[chosen_thread_id];
    if (should_be_enabled) {
      sim_.disable_thread(chosen_thread_id);
      golden[chosen_thread_id] = false;
    } else {
      sim_.enable_thread(chosen_thread_id);
      golden[chosen_thread_id] = true;
    }

    for (size_t thread_id = 0; thread_id < topo().NumOfThreads; ++thread_id) {
      EXPECT_EQ(golden[thread_id], sim_.is_thread_enabled(thread_id));
    }
  }
}

template <typename T> class TypedSimCoreTests : public SimCoreTests {};

// Helper that holds template parameters of `GeneratedData` and provides
// constexpr access to them.
template <OperationType operation_type_param, OutputType output_type_param>
struct GeneratedDataParams {
  static constexpr OperationType operation_type = operation_type_param;
  static constexpr OutputType output_type = output_type_param;
};

using TypedSimCoreTestsTypes = ::testing::Types<
    GeneratedDataParams<OperationType::InRange, OutputType::BitVector>,
    GeneratedDataParams<OperationType::InRange, OutputType::IndexVector>,
    GeneratedDataParams<OperationType::InList, OutputType::BitVector>,
    GeneratedDataParams<OperationType::InList, OutputType::IndexVector>>;
TYPED_TEST_SUITE(TypedSimCoreTests, TypedSimCoreTestsTypes);

// Tests a scan operation.
TYPED_TEST(TypedSimCoreTests, Scan) {
  static constexpr size_t thread_id = 0;

  const auto data = tools::gen::imdb::generate_data<TypeParam::operation_type,
                                                    TypeParam::output_type>();
  this->test_on_data(thread_id, data);
}

// Tests polling for thread status using IMDB_COM_STATUS.
TYPED_TEST(TypedSimCoreTests, PollAll) {
  static constexpr size_t thread_id = 0;

  const auto data = tools::gen::imdb::generate_data<TypeParam::operation_type,
                                                    TypeParam::output_type>();
  auto output_buf = SimCoreTests::create_output_buf<TypeParam::output_type>(
      data.column_.view());

  SimCoreTests::sim_.enable_thread(thread_id);
  for (size_t i = 0; i < data.input_.size(); ++i) {
    SimCoreTests::ScanOperationProxy proxy(data.column_, data.input_[i],
                                           output_buf, TypeParam::output_type);
    auto user_op = proxy.operation();
    auto scan_op = pnm::imdb::ScanOperation(user_op);

    this->setup_regs(thread_id, scan_op);

    this->start_scan(thread_id, TypeParam::output_type);
    while (true) {
      auto status = this->poll_all();
      for (size_t tid = 0; tid < status.size(); ++tid) {
        if (tid != thread_id) {
          ASSERT_TRUE(status[tid])
              << "Engine " << tid << " is active, but only " << thread_id
              << " should be";
        }
      }
      if (status[thread_id]) {
        // Current engine is not executing any operation. Either it has not
        // started execution, or it has already finished. We need to look at the
        // thread status register to tell these cases apart. This workaround is
        // only required on a simulator, because the OS scheduler cannot
        // guarantee real-time updates to status registers from engine threads.
        if (this->poll_status(thread_id)) {
          break;
        }
      }
    }

    proxy.sync_result();

    SimCoreTests::check_regs<TypeParam::output_type>(
        this->imdb_regs_.thread[thread_id], data.result_[i], output_buf);
  }
}

// Simulator tests using a simple engine scheduler.
class ScheduledSimCoreTests : public SimCoreTests {
protected:
  // Wait until an engine is available and return the engine ID.
  size_t wait_for_engine() {
    while (true) {
      auto thread_id = wait_for_engine_weak();
      if (poll_status(thread_id)) {
        return thread_id;
      }
    }
  }

private:
  // Wait until an engine is available and return the engine ID.
  // May return spuriously on a simulator (but not on a hardware device) — check
  // the thread register to verify the engine is idle.
  size_t wait_for_engine_weak() {
    while (true) {
      auto status = poll_all();
      for (size_t i = 0; i < status.size(); ++i) {
        if (status[i]) {
          return i;
        }
      }
    }
  }
};

// Stress-test and test for interaction of multiple engines.
TEST_F(ScheduledSimCoreTests, StressMultiEngine) {
  static constexpr size_t num_runs = 1000;
  static constexpr uint64_t table_size = 100'000;
  static constexpr uint64_t bit_size = 7;

  // Create generators.
  auto selectivities_generator =
      tools::gen::imdb::SelectivitiesGeneratorFactory::default_factory().create(
          "random");
  auto column_generator =
      tools::gen::imdb::ColumnGeneratorFactory::default_factory().create(
          "random", std::to_string(bit_size));
  auto scan_ranges_generator =
      tools::gen::imdb::RangesGeneratorFactory::default_factory().create(
          "random");

  // Create table column.
  const auto columns_info = tools::gen::imdb::ColumnsInfo(bit_size, table_size);
  auto column = column_generator->create(columns_info);

  // Create predicates (ranges).
  auto selectivities = selectivities_generator->create(num_runs, 0.05, 0.15);
  const auto scan_ranges_info =
      tools::gen::imdb::RangesInfo(std::move(selectivities));
  const auto ranges =
      scan_ranges_generator->create(scan_ranges_info, columns_info);

  // Create golden vectors.
  const auto golden_vecs =
      tools::gen::imdb::ImdbGoldenVecGenerator::scan<pnm::imdb::IndexVectors>(
          column, ranges);

  // Start execution.
  // golden_results: golden vectors for running operations, if any.
  // actual_results: output buffers for running operations.
  std::vector<std::optional<pnm::imdb::index_vector>> golden_results(
      topo().NumOfThreads);
  std::vector<pnm::imdb::index_vector> actual_results(topo().NumOfThreads);

  // A small hack to make `wait_for_engine` work. Normally, we don't care about
  // the state of the `THREAD_STATUS` register if `IMDB_COM_STATUS` reports it
  // is idle.
  for (size_t thread_id = 0; thread_id < topo().NumOfThreads; ++thread_id) {
    imdb_regs_.thread[thread_id].THREAD_STATUS = 0;
  }

  // Number of operations executed on each engine.
  std::vector<uint32_t> engine_stats(topo().NumOfThreads);

  auto context = pnm::make_context(pnm::Device::Type::IMDB);
  const pnm::memory::Buffer column_buf(
      pnm::views::make_view(column.container()), context);

  std::vector<pnm::memory::Buffer<pnm::imdb::index_type>> result_buf(
      topo().NumOfThreads);
  for (auto &buf : result_buf) {
    buf = pnm::memory::Buffer<pnm::imdb::index_type>(table_size + 16, context);
  }

  // Enable all threads.
  for (size_t thread_id = 0; thread_id < topo().NumOfThreads; ++thread_id) {
    SimCoreTests::sim_.enable_thread(thread_id);
  }

  // Schedule `num_runs` operations for the threads.
  for (size_t i = 0; i < num_runs; ++i) {
    // Wait for an idle engine.
    auto thread_id = wait_for_engine();

    auto &actual = actual_results[thread_id];
    // If an earlier operation has completed, check its correctness.
    if (const auto &golden = golden_results[thread_id]; golden.has_value()) {
      const auto &golden_value = golden.value();
      auto result_size = imdb_regs_.thread[thread_id].THREAD_RES_SIZE_SB;

      result_buf[thread_id].bind_user_region(pnm::views::make_view(actual));
      result_buf[thread_id].copy_from_device();

      actual.resize(result_size);

      ASSERT_EQ(result_size, golden_value.size());
      ASSERT_EQ(actual.size(), golden_value.size());
      const auto actual_view =
          pnm::views::make_view<const pnm::imdb::index_type>(
              actual.data(), actual.data() + golden_value.size());
      const auto golden_view = pnm::views::make_view(golden_value);
      EXPECT_EQ(actual_view, golden_view);
    }

    // Resize the output buffer.
    actual.resize(table_size + 16);
    // Set up the scan operation.
    Scan user_op(Scan::Column{column.value_bits(), column.size(), &column_buf},
                 ranges[i], &result_buf[thread_id], OutputType::IndexVector);
    auto scan_op = pnm::imdb::ScanOperation(user_op);
    setup_regs(thread_id, scan_op);

    // Save the golden vector for this operation.
    golden_results[thread_id] = golden_vecs[i];

    // Start scanning.
    start_scan(thread_id, OutputType::IndexVector);
    ++engine_stats[thread_id];
  }

  // Wait until all engines are done.
  while (true) {
    auto status = poll_all();
    if (std::all_of(status.begin(), status.end(), [](bool x) { return x; })) {
      // Simulator workaround. On hardware, break unconditionally.
      bool all_finished = true;
      for (size_t thread_id = 0; thread_id < topo().NumOfThreads; ++thread_id) {
        if (!poll_status(thread_id)) {
          all_finished = false;
          break;
        }
      }

      if (all_finished) {
        break;
      }
    }
    // Yield is not useful for the hardware device, but speeds up the simulator
    // a little bit.
    std::this_thread::yield();
  }

  // Check all remaining operations for correctness.
  for (size_t thread_id = 0; thread_id < golden_results.size(); ++thread_id) {
    auto &actual = actual_results[thread_id];
    actual.resize(result_buf[thread_id].size());
    result_buf[thread_id].bind_user_region(pnm::views::make_view(actual));
    result_buf[thread_id].copy_from_device();
    if (const auto &golden = golden_results[thread_id]; golden.has_value()) {
      const auto &golden_value = golden.value();
      auto result_size = imdb_regs_.thread[thread_id].THREAD_RES_SIZE_SB;
      ASSERT_EQ(result_size, golden_value.size());
      ASSERT_GE(actual.size(), golden_value.size());
      const auto actual_view =
          pnm::views::make_view<const pnm::imdb::index_type>(
              actual.data(), actual.data() + golden_value.size());
      const auto golden_view = pnm::views::make_view(golden_value);
      EXPECT_EQ(actual_view, golden_view);
    }
  }

  pnm::log::info("Engine stats: {}\n", fmt::join(engine_stats, " "));
}

// Tests for the integration of SimlatorDevice's thread locking/unlocking and
// SimulatorCore's thread enabling/disabling.
class SimulatorDeviceTests : public SimulatorTests {
public:
  void SetUp() override {
    // [TODO: @a.korzun] We cannot force creation of a PNM context for a
    // simulator device when `SIM == 0`, so we run these tests only in simulator
    // mode, as we need a context with a simulator device. This should be fixed.
    if (PNM_PLATFORM == HARDWARE) {
      GTEST_SKIP() << "SimulatorDevice tests are not supported in the hardware "
                      "build mode";
    }
  }

protected:
  RegisterPointers &imdb_regs() override {
    return device().register_pointers();
  }

  template <OperationType operation_type, OutputType output_type>
  void test_on_data(const GeneratedData<operation_type, output_type> &data) {
    auto &dev = device();
    const size_t thread_id = dev.lock_thread();
    test_on_data_unlocked(thread_id, data);
    dev.release_thread(thread_id);
  }
};

TEST_F(SimulatorDeviceTests, MultiThreadSynchronization) {
  static constexpr size_t num_threads = 8;
  const auto thread_fn = [&]() {
    const auto data =
        tools::gen::imdb::generate_data<pnm::imdb::OperationType::InList,
                                        pnm::imdb::OutputType::IndexVector>();
    test_on_data(data);
  };

  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (size_t i = 0; i < num_threads; ++i) {
    threads.emplace_back(thread_fn);
  }

  for (auto &thread : threads) {
    thread.join();
  }
}
