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

#ifndef _SLS_SECURE_EXTRA_H_
#define _SLS_SECURE_EXTRA_H_

#include "secure/encryption/AES/common.h"
#include "secure/encryption/AES/crypto.h"

#include "common/sls_types.h"

#include "pnmlib/common/misc_utils.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/views.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <random>

namespace pnm::sls::secure {

/*! \brief Class with commont method for Ecryption and Verification engines */
template <typename EncBackend, AES_KEY_SIZE KeySize> class CommonCryptoEngine {
public:
  using key_type = typename AES_Engine<KeySize, EncBackend>::AES_Key_type;

protected:
  explicit CommonCryptoEngine(uint32_t version)
      : CommonCryptoEngine(version, create_aes_key()) {}

  CommonCryptoEngine(uint32_t version, const key_type &key)
      : version_{version}, aes_engine_(key) {}

  key_type create_aes_key() const {
    key_type key;
    std::mt19937_64 reng{std::random_device{}()};
    std::uniform_int_distribution<uint16_t> distr{
        0, std::numeric_limits<uint8_t>::max()};
    std::generate(key.data(), key.data() + key.size(),
                  [&reng, &distr]() { return distr(reng); });
    return key;
  }

  uint32_t version_;
  AES_Engine<KeySize, EncBackend> aes_engine_;
};

/*! Fill seed for OTP with prefix, address and version for further encryption
 * */
template <typename T>
inline void prepare_otp(T *begin, T *end, uint32_t prefix, uint32_t version,
                        uintptr_t address) {
  auto *it = reinterpret_cast<uint8_t *>(begin);
  it = std::copy_n(reinterpret_cast<uint8_t *>(&version), sizeof(version), it);
  it = std::copy_n(reinterpret_cast<uint8_t *>(&address), sizeof(address), it);
  it = std::copy_n(reinterpret_cast<uint8_t *>(&prefix), sizeof(prefix), it);
  std::fill(it, reinterpret_cast<uint8_t *>(end), 0);
}

/*! Generate OTP for specified prefix, version and address by AES encryption of
 * block (version||address||prefix). There are three prefixes in article:
 * - 0x00 for common OTP,
 * - 0x01 for linear checksum
 * - 0x10 for MAC
 */
template <typename T, unsigned N, typename Encoder>
inline void generate_otp(uint32_t prefix, uint32_t version, uintptr_t address,
                         Encoder &engine, pnm::types::FixedVector<T, N> &otp) {
  static_assert(
      sizeof(version) + sizeof(address) + sizeof(prefix) <= N * sizeof(T),
      "Size of OTP should be greater or equal than 2 * sizeof(uint32_t) +"
      "sizeof(uintptr_t)");
  prepare_otp(otp.begin(), otp.end(), prefix, version, address);
  engine.encrypt(reinterpret_cast<uint8_t *>(otp.data()),
                 reinterpret_cast<uint8_t *>(otp.data()) +
                     pnm::utils::byte_size_of_container(otp),
                 reinterpret_cast<uint8_t *>(otp.data()));
}

/*! Fill output range by OTPs.
 *
 * The output array has structure:
 *
 * |----OTP-----||----OTP-----||----OTP-----||----OTP-----||----OTP-----|
 * |            |
 * |<-otp_size->|
 *
 * The number of OTPs in array is defined by otps_count
 * */
template <typename T, typename Encoder>
inline void generate_otp_vec(uint32_t prefix, uint32_t version,
                             pnm::views::common<uintptr_t> addresses,
                             uint32_t otp_size, uint32_t otps_count,
                             Encoder &engine, pnm::views::common<T> otps) {
  auto otps_array_v = pnm::views::make_rowwise_view(otps.begin(), otps.end(),
                                                    otp_size * otps_count);
  auto *address = addresses.begin();
  for (auto otp_batch : otps_array_v) {
    uintptr_t otp_address = *address;
    for (auto otp : pnm::views::make_rowwise_view(otp_batch.begin(),
                                                  otp_batch.end(), otp_size)) {
      prepare_otp(otp.begin(), otp.end(), prefix, version, otp_address);
      otp_address += otp_size * sizeof(T);
    }
    std::advance(address, 1);
  }

  auto casted_array = pnm::views::view_cast<uint8_t>(otps);
  engine.encrypt(casted_array.begin(), casted_array.end(),
                 casted_array.begin());
}

} // namespace pnm::sls::secure

#endif
