#include "secure/common/sls_io.h"
#include "secure/encryption/io_traits.h"

#include "common/topology_constants.h"

#include "pnmlib/common/128bit_math.h"

#include <gtest/gtest.h>

struct io_traits_fixture : ::testing::Test {
  static constexpr auto probe_cols = 4;
};

TEST_F(io_traits_fixture, TrivialMemoryWriter) {
  using namespace sls::secure;

  EXPECT_EQ(io_traits<TrivialMemoryWriter>::data_size<int>, sizeof(int));
  EXPECT_EQ(io_traits<TrivialMemoryWriter>::data_size<double>, sizeof(double));

  EXPECT_EQ(io_traits<TrivialMemoryWriter>::meta_size, sizeof(pnm::uint128_t));

  EXPECT_EQ(io_traits<TrivialMemoryWriter>::memory_size<int>(probe_cols, false),
            probe_cols * sizeof(int));
  EXPECT_EQ(io_traits<TrivialMemoryWriter>::memory_size<int>(probe_cols, true),
            probe_cols * sizeof(int) + sizeof(pnm::uint128_t));
}

TEST_F(io_traits_fixture, MemoryWriter) {
  using namespace sls::secure;

  EXPECT_EQ(io_traits<MemoryWriter>::data_size<int>, sizeof(int));
  EXPECT_EQ(io_traits<MemoryWriter>::data_size<double>, sizeof(double));

  EXPECT_EQ(io_traits<MemoryWriter>::meta_size,
            pnm::device::topo().AlignedTagSize);

  EXPECT_EQ(io_traits<MemoryWriter>::memory_size<int>(probe_cols, false),
            probe_cols * sizeof(int));
  EXPECT_EQ(io_traits<MemoryWriter>::memory_size<int>(probe_cols, true),
            probe_cols * sizeof(int) + pnm::device::topo().AlignedTagSize);
}

struct UserWriter;
inline constexpr auto user_writer_data_size = 64;

namespace sls::secure {
template <>
struct io_traits<UserWriter>
    : io_traits_base<UserWriter, io_traits<UserWriter>> {
  template <typename> static constexpr auto data_size = ::user_writer_data_size;
};
} // namespace sls::secure

TEST_F(io_traits_fixture, UserWriter) {
  using namespace sls::secure;

  EXPECT_EQ(io_traits<UserWriter>::data_size<int>, ::user_writer_data_size);
  EXPECT_EQ(io_traits<UserWriter>::data_size<double>, ::user_writer_data_size);

  EXPECT_EQ(io_traits<UserWriter>::meta_size, sizeof(pnm::uint128_t));

  EXPECT_EQ(io_traits<UserWriter>::memory_size<int>(probe_cols, false),
            probe_cols * ::user_writer_data_size);
  EXPECT_EQ(io_traits<UserWriter>::memory_size<int>(probe_cols, true),
            probe_cols * ::user_writer_data_size + sizeof(pnm::uint128_t));
}
