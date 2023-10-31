#include "secure/encryption/AES/common.h"
#include "secure/encryption/AES/crypto.h"
#include "secure/encryption/AES/ni_backend.h"

#include <gtest/gtest.h>

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <unordered_map>

namespace {
template <typename FwIter1, typename FwIter2>
void check_range(FwIter1 begin, FwIter1 end, FwIter2 ebegin, FwIter2 eend) {
  EXPECT_EQ(std::distance(begin, end), std::distance(ebegin, eend));
  auto it = begin;
  auto eit = ebegin;
  for (; it != end; ++it, ++eit) {
    EXPECT_EQ(*it, *eit);
  }
}

template <typename FwIter> void dump_range(FwIter begin, FwIter end) {
  for (auto it = begin; it != end; ++it) {
    fmt::print("{:02x}", static_cast<int>(*it));
  }
}

constexpr auto BLOCK_SIZE = 16;
constexpr unsigned char plain_text[] =
    "The quick brown fox jumps over the lazy dog!!!!!";
constexpr auto plain_size = sizeof(plain_text) - 1;

//+BLOCK_SIZE required to allocate space for encrypted data from OpenSSL
std::array<unsigned char, sizeof(plain_text) + BLOCK_SIZE> output_buffer() {
  std::array<unsigned char, sizeof(plain_text) + BLOCK_SIZE> buffer{};
  buffer.fill(0);
  return buffer;
}

std::unordered_map<unsigned, const char *> KEY_SET{
    {128, "1234567890123456"},
    {192, "123456789012345678901234"},
    {256, "12345678901234567890123456789012"}};

std::unordered_map<unsigned, std::array<unsigned char, plain_size>> goldens{
    {128,
     {0xd7, 0x0f, 0x1a, 0x15, 0x85, 0x53, 0xb7, 0x21, 0xc5, 0xdb, 0x92, 0x20,
      0xa6, 0x27, 0xe7, 0xb1, 0x7b, 0xff, 0x78, 0xff, 0x7a, 0x72, 0xc4, 0xab,
      0x56, 0x5a, 0xc9, 0x03, 0xa9, 0x4d, 0xf7, 0x6d, 0xd1, 0x5f, 0x27, 0x89,
      0x3a, 0x23, 0x57, 0x52, 0xf1, 0x5a, 0x27, 0x38, 0x93, 0xe8, 0xcd, 0x7d}},
    {192,
     {0x64, 0xe2, 0xa2, 0xb6, 0x81, 0x52, 0x7a, 0xdd, 0x50, 0xb5, 0x83, 0xff,
      0xf6, 0x76, 0x23, 0xd3, 0x7e, 0x09, 0x2e, 0xb2, 0xcb, 0x3a, 0x33, 0x55,
      0x39, 0xe8, 0x6a, 0x29, 0x21, 0x33, 0x4f, 0x08, 0xc0, 0xde, 0x2f, 0x93,
      0xac, 0xa7, 0xd6, 0x07, 0xd4, 0xc9, 0xa2, 0xd4, 0xea, 0xdd, 0xfb, 0x94}},
    {256,
     {0xae, 0x3c, 0xa0, 0x13, 0x44, 0xe3, 0x22, 0xa8, 0x90, 0xe0, 0x44, 0xe7,
      0xa6, 0xcc, 0xe8, 0x89, 0xf6, 0x79, 0x56, 0xb0, 0x07, 0x9d, 0x8a, 0x79,
      0xae, 0xad, 0x2e, 0x2b, 0xe2, 0xad, 0x1d, 0x11, 0x09, 0x51, 0x0d, 0x80,
      0x3a, 0xa3, 0xe4, 0x1d, 0xc6, 0x30, 0x96, 0x67, 0x25, 0xa7, 0xfb, 0x92}}};
} // namespace

template <typename EncryptionBackend, pnm::sls::secure::AES_KEY_SIZE KeySize>
void aes_encryption_test() {
  constexpr auto KEY_BITS = static_cast<unsigned>(KeySize);
  fmt::print("Start with AES-{}\n", KEY_BITS);
  constexpr auto KEY_SIZE = KEY_BITS / 8;
  typename pnm::sls::secure::AES_Engine<KeySize,
                                        EncryptionBackend>::AES_Key_type key;

  std::copy(KEY_SET[KEY_BITS], KEY_SET[KEY_BITS] + KEY_SIZE, key.data());

  const pnm::sls::secure::AES_Engine<KeySize, EncryptionBackend> engine{key};

  auto output = output_buffer();
  engine.encrypt(plain_text, plain_text + plain_size, output.data());

  dump_range(goldens[KEY_BITS].begin(), goldens[KEY_BITS].end());
  fmt::print("\n");
  dump_range(output.data(), output.data() + plain_size);
  fmt::print("\n");
  check_range(output.data(), output.data() + plain_size,
              goldens[KEY_BITS].begin(), goldens[KEY_BITS].end());

  fmt::print("====AES-{} OK====\n", KEY_BITS);
}

using namespace pnm::sls::secure;

TEST(AES_NI, AES_128) {
  aes_encryption_test<AES_NI_Backend, AES_KEY_SIZE::AES_128>();
}

TEST(AES_NI, AES_192) {
  aes_encryption_test<AES_NI_Backend, AES_KEY_SIZE::AES_192>();
}

TEST(AES_NI, AES_256) {
  aes_encryption_test<AES_NI_Backend, AES_KEY_SIZE::AES_256>();
}

TEST(AES_NI, AES_128_Plaintext_Underflow) {
  // Here we check only valgrind/asan errors
  static constexpr auto size = 20U;
  const char plain[size] = "The quick brown fox";

  static constexpr auto enc_size = size + !!(size % BLOCK_SIZE) * BLOCK_SIZE;
  uint8_t encrypted[enc_size] = {};

  const AES_NI_Backend enc_engine(
      AES_KEY_SIZE::AES_128, reinterpret_cast<const uint8_t *>(KEY_SET[128]));
  enc_engine.encrypt(reinterpret_cast<const uint8_t *>(plain), size, encrypted);

  const uint8_t golden[enc_size] = {
      0xd7, 0x0f, 0x1a, 0x15, 0x85, 0x53, 0xb7, 0x21, 0xc5, 0xdb, 0x92,
      0x20, 0xa6, 0x27, 0xe7, 0xb1, 0x13, 0xb0, 0x5a, 0x7d, 0x26, 0x7c,
      0x34, 0xa2, 0xa5, 0xb3, 0x48, 0x66, 0xc2, 0xc2, 0xd0, 0x00};
  for (auto i = 0U; i < enc_size; ++i) {
    ASSERT_EQ(golden[i], encrypted[i]) << "Mismatch at position " << i;
  }
}
