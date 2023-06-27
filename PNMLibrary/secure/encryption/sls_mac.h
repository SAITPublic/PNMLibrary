/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef _SLS_SECURE_MAC_H_
#define _SLS_SECURE_MAC_H_

#include "AES/ni_backend.h"
#include "AES/sls_extra.h"

#include "secure/encryption/AES/common.h"

#include "common/compiler_internal.h"
#include "common/sls_types.h"

#include "pnmlib/common/128bit_math.h"
#include "pnmlib/common/views.h"

#include <cstdint>
#include <iterator>
#include <numeric>
#include <vector>

namespace sls::secure {

/*! \brief Class performs verification tag encryption and
 * validation of SLS result.
 */
template <AES_KEY_SIZE KeySize = AES_KEY_SIZE::AES_128,
          typename EncryptionBackend = AES_NI_Backend>
class VerificationEngine
    : private CommonCryptoEngine<EncryptionBackend, KeySize> {
  using CryptoEngine = CommonCryptoEngine<EncryptionBackend, KeySize>;

public:
  using typename CryptoEngine::key_type;
  using tag_type = pnm::uint128_t;
  constexpr static auto TAG_BYTES = sizeof(tag_type);

  constexpr static auto Q = pnm::longint_traits<tag_type>::BIG_VALUE;

  VerificationEngine(uintptr_t table_address, uint32_t version)
      : CryptoEngine(version), base_address_{table_address} {}

  VerificationEngine(uintptr_t table_address, uint32_t version,
                     const typename CryptoEngine::key_type &key)
      : CryptoEngine(version, key), base_address_{table_address} {}

  /*! \brief Create encrypted validation tag
   *
   * This method creates validation tag for data range and encrypts it.
   *
   * @tparam InIt1, InIt2 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   *
   * @param[in] begin,end the range of data for tag generation
   * @param[in] address the offset for data region
   *
   * \return one part of validation tag
   */
  template <typename InIt1, typename InIt2>
  tag_type
  generate_mac(InIt1 begin, InIt2 end,
               uintptr_t address) const NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
    auto T = linear_checksum(begin, end); // T_i from article
    alignas(16) FixedVector<uint8_t, TAG_BYTES> mac_otp{};
    generate_otp(0x10, version_, address, aes_engine_, mac_otp);
    auto E = *reinterpret_cast<tag_type *>(mac_otp.data());
    return T - E;
  }

  /*! \brief Validation of PSUM
   *
   * This method performs validation of PSUM. The offsets are transformed to
   * encrypted tag. The left fold operation op applied to transformed offsets.
   *
   * The offsets should be the subset of offsets that used in encryption/tag
   * generation process.
   *
   * @tparam InIt1, InIt2 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam InIt3, InIt4 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam BinaryFunction
   *
   * @param[in] begin,end the range of SLS result for validation
   * @param[in] offsets_b,offsets_e the range of offsets of lines that used in
   * PSUM
   * @param[in] C the computed tag
   * @param[in] op the binary function that represent operation under the table
   * lines (ex. SLS). The table lines is represented by encrypted tag. The
   * signature of the function should be equivalent to the following:
   * \code{cpp}
   * Ret fun(const Type1 &a, const Type1 &b);
   * \endcode
   * The Type1 is should be the __m128i or can be implicitly converted to
   * __m128i.
   *
   * \return true if PSUM is valid and false otherwise
   */
  template <typename InIt1, typename InIt2, typename InIt3, typename InIt4,
            typename BinaryFunction>
  bool validate_psum(InIt1 begin, InIt2 end, InIt3 offsets_b, InIt4 offsets_e,
                     tag_type C, BinaryFunction op) const {
    auto E = offset_transform_reduce_vec(offsets_b, offsets_e, tag_type{}, op);
    return validate_psum(begin, end, C, E);
  }

  /*! \brief Validation of PSUM
   *
   * This method performs validation of PSUM using two part secret sharing.
   *
   * @tparam InIt1, InIt2 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   *
   * @param[in] begin,end the range of SLS result for validation
   * @param[in] computed_tag_sum the computed tag from AxDIMM
   * @param[in] generated_tag_sum the computed tag by offset transform-reduce op
   *
   * \return true if PSUM is valid and false otherwise
   */
  template <typename InIt1, typename InIt2>
  bool validate_psum(InIt1 begin, InIt2 end, tag_type computed_tag_sum,
                     tag_type generated_tag_sum) const
      NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
    auto T = linear_checksum(begin, end);
    auto vT = computed_tag_sum + generated_tag_sum;

    return T == vT;
  }

  /*! \brief Perform linear arithmetic operation over tags.
   *
   * This method performs transformation of offsets into set of tags and
   * apply linear operation op to all tags.
   *
   * @tparam InIt1, InIt2 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam BinaryOperation
   *
   * @param[in] begin,end the range of offsets
   * @param[in] init initial value
   * @param[in] op binary operation function object that will be applied.
   * The signature of the function should be equivalent to the following:
   * \code{cpp}
   * Ret fun(const Type1 &a, const Type1 &b);
   * \endcode
   * The Type1 is should be the tag_type or can be implicitly converted to
   * tag_type.
   *
   * \return The result of left fold of the given range over op
   */
  template <typename InIt1, typename InIt2, typename BinaryFunction>
  [[deprecated(
      "Use offset_transform_reduce_vec for the best performance")]] tag_type
  offset_transform_reduce(InIt1 first, InIt2 last, tag_type init,
                          BinaryFunction op) const {
    auto reduce_func = [this, op](const auto &sum,
                                  [[maybe_unused]] auto address) {
      alignas(16) FixedVector<uint8_t, TAG_BYTES> otp{};
      generate_otp(0x10, version_, address, aes_engine_, otp);
      auto Ei = *reinterpret_cast<tag_type *>(otp.data());
      return op(sum, Ei);
    };

    return std::accumulate(first, last, init, reduce_func);
  }

  /*! \brief Vectorized version of offset_transform_reduce
   * */
  template <typename InIt1, typename InIt2, typename BinaryFunction>
  tag_type offset_transform_reduce_vec(InIt1 first, InIt2 last, tag_type init,
                                       BinaryFunction op) const {
    // Preallocated buffer for OTPs
    std::vector<tag_type> buffer(std::distance(first, last), tag_type{});

    generate_otp_vec(0x10, version_, pnm::make_view(first, last), 1, 1,
                     aes_engine_, pnm::make_view(buffer));

    return std::accumulate(buffer.begin(), buffer.end(), init, op);
  }

private:
  using CryptoEngine::aes_engine_;
  using CryptoEngine::version_;

  template <typename InIt1, typename InIt2>
  tag_type linear_checksum(InIt1 begin, InIt2 end) const {
    alignas(16) FixedVector<uint8_t, TAG_BYTES> otp{};
    generate_otp(0x01, version_, base_address_, aes_engine_, otp);
    auto s = *reinterpret_cast<tag_type *>(otp.data());

    // Here we implement operation \sum_j^{N} P_{i,j} * s^{j + 1}.
    // If we  want direct representation of eq. from Algorithm 2 we should
    // use reverse iterator for begin and end
    return std::accumulate(begin, end, tag_type{},
                           [&s](auto sum, auto e)
                               NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW {
                                 sum = sum + s * e;
                                 s = (s * s) % Q;
                                 return sum;
                               });
  }

  uintptr_t base_address_;
};
} // namespace sls::secure

#endif
