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

#ifndef _SLS_SECURE_ENGINE_H_
#define _SLS_SECURE_ENGINE_H_

#include "AES/ni_backend.h"
#include "AES/sls_extra.h"

#include "secure/encryption/AES/common.h"

#include "common/compiler_internal.h"
#include "common/sls_types.h"

#include "pnmlib/common/error.h"
#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/views.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

namespace pnm::sls::secure {
/*! \brief The class instance perform encryption/decription process of
 * raw tables.
 */
template <typename EntryType = uint32_t, unsigned OTPBytes = 16,
          typename EncBackend = AES_NI_Backend,
          AES_KEY_SIZE KeySize = AES_KEY_SIZE::AES_128>
class EncryptionEngine : private CommonCryptoEngine<EncBackend, KeySize> {
  using CryptoEngine = CommonCryptoEngine<EncBackend, KeySize>;

public:
  /*
   * With paper: "SecNDP: Secure Near-Data Processing with Untrusted Memory
   * The OTPBytes stands for of bytes in W_c constant.
   * The sizeof(EntryType) stands for bytes in W_e constant.
   */
  using otp_type =
      pnm::types::FixedVector<EntryType, OTPBytes / sizeof(EntryType)>;
  using typename CryptoEngine::key_type;

  explicit EncryptionEngine(uint32_t version) : CryptoEngine(version) {}

  EncryptionEngine(uint32_t version, const typename CryptoEngine::key_type &key)
      : CryptoEngine(version, key) {
    static_assert(OTPBytes % 16 == 0, "OTP size in bytes should divide by 16");
    static_assert(sizeof(EntryType) <= OTPBytes &&
                      OTPBytes % sizeof(EntryType) == 0,
                  "Assuming integral number of entries in OTP");
  }

  /*! \brief Encrypt data range from begin to end.
   *
   * This method implement data encryption according to Algorithm 1 from article
   * [SecNDP: Secure Near-Data Processing with Untrusted Memory]. The OTP is
   * generated for each data block with length OTPBytes. After that OTP value is
   * subtracted from data slice and result is stored into range specified by
   * enc_begin.
   *
   * The addresses in encryption process calculated from 'offset' value with
   * fixed step by equation: address = offset + OTPBytes * n, where n is a
   * number of data block with size OTPBytes.
   *
   * @tparam InIt1, InIt2 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam OutIt iterator type. Must meet the requirements of
   * LegacyOutputIterator.
   *
   * @param[in] begin,end the range of data for encryption
   * @param[in] offset the offset for data region
   * @param[out] enc_begin the beginning of the destination for encrypted data
   *
   * \return pair of output iterator to the element in the destination range,
   * one past the last element and size of encrypted data.
   */
  template <typename InIt1, typename InIt2, typename OutIt>
  std::pair<OutIt, size_t> encrypt(InIt1 begin, InIt2 end, OutIt enc_begin,
                                   uintptr_t offset) const {
    if (std::distance(begin, end) % otp_type{}.size() != 0) {
      throw pnm::error::InvalidArguments(
          "The range should contain integral number of OTP.");
    }

    otp_type OTP, data_slice;

    auto col_offset = offset;
    for (auto it = begin; it != end; std::advance(it, data_slice.size())) {
      std::copy_n(it, data_slice.size(), data_slice.begin());
      generate_otp(0x0, version_, col_offset, aes_engine_, OTP);
      data_slice -= OTP;
      enc_begin = std::copy(data_slice.begin(), data_slice.end(), enc_begin);
      col_offset += OTPBytes;
    }

    return {enc_begin, col_offset - offset};
  }

  /*! \brief Decrypt PSUM
   *
   * This method perform decryption of PSUM. Method requires lines offsets that
   * used for PSUM calculation. The offset is a unique value for a data block.
   * The offset is used for address calculation in encryption and decryption
   * process. In common case the offset value represent start address of values
   * range that used for PSUM. The offset values should be the same as in the
   * encryption process.
   *
   * For tables that presented as an array of rows the offsets can be evaluated
   * as table_base + index * sparse_feature_size * sizeof(T) where table_base --
   * address of the table start, index -- value from indices array, T -- entry
   * type.
   *
   * @tparam InIt1, InIt2 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam InIt3, InIt4 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam OutIt iterator type. Must meet the requirements of
   * LegacyOutputIterator.
   * @tparam BinaryOperation
   *
   * @param[in] begin,end the range of data for decryption
   * @param[in] offsets_b,offsets_e the range of offsets of lines that used in
   * PSUM calculation
   * @param[out] output the beginning of the destination for decrypted PSUM
   * @param op 	binary operation function object that will be applied. The
   * signature of the function should be equivalent to the following:
   * \code{cpp}
   * Ret fun(const Type1 &a, const Type1 &b);
   * \endcode
   * The Type1 is should be the otp_type or can be implicitly converted to
   * otp_type.
   *
   * \return output iterator to the element in the destination range,
   * one past the last element and size of encrypted data.
   */
  template <typename InIt1, typename InIt2, typename InIt3, typename InIt4,
            typename OutIt, typename BinaryOperation>
  OutIt decrypt_psum(InIt1 begin, InIt2 end, InIt3 offsets_b, InIt4 offsets_e,
                     OutIt output, BinaryOperation op) const {
    auto size = std::distance(begin, end);
    if (size % otp_type{}.size() != 0) {
      throw pnm::error::InvalidArguments(
          "The range should contain integral number of OTP.");
    }

    auto op_result = offset_transform_reduce_vec(
        offsets_b, offsets_e, std::vector<EntryType>(size, EntryType{}), op);

    return decrypt_psum(begin, end, op_result.begin(), output);
  }

  /*! \brief Perform linear arithmetic operatio over OTPs.
   *
   * This method perform transfromation of offsets into set of OTPs and
   * apply linear operation op to all OTPs.
   *
   * The offsets are used in address calculation for OTP:
   *  address = offset + n * OTPBytes.
   *
   * The offset values should be the part of offsets that used in encryption
   * process.
   *
   * For tables that presented as an array of rows the offsets can be evaluated
   * as table_base + index * sparse_feature_size * sizeof(T) where table_base --
   * address of the table start, index -- value from indices array, T -- entry
   * type.
   *
   * @tparam InIt1, InIt2 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam T initial value type.
   * @tparam BinaryOperation
   *
   * @param[in] begin,end the range of offsets
   * @param[in] init initial value
   * @param op 	binary operation function object that will be applied. The
   * signature of the function should be equivalent to the following:
   * \code{cpp}
   * Ret fun(const Type1 &a, const Type1 &b);
   * \endcode
   * The Type1 is should be the otp_type or can be implicitly converted to
   * otp_type.
   *
   * \return The result of left fold of the given range over op
   */
  template <typename InIt1, typename InIt2, typename T, typename BinaryOp>
  [[deprecated("Use offset_transform_reduce_vec(...) instead for the best "
               "performance")]] T
  offset_transform_reduce(InIt1 begin, InIt2 end, T init, BinaryOp op) const {
    auto decrypt_block = [this, op](const otp_type &data, uintptr_t base,
                                    uintptr_t offset) {
      otp_type OTP;
      generate_otp(0x0, version_, base + offset, aes_engine_, OTP);
      return op(data, OTP);
    };
    auto output = init.begin();

    otp_type data_slice{};
    uintptr_t col_offset = 0;
    for (auto it = init.begin(); it != init.end();
         std::advance(it, data_slice.size())) {
      std::copy_n(it, data_slice.size(), data_slice.begin());
      data_slice = std::accumulate(
          begin, end, data_slice,
          [col_offset, decrypt_block](const auto &sum, auto offset) {
            return decrypt_block(sum, offset, col_offset);
          });
      output = std::transform(data_slice.begin(), data_slice.end(), output,
                              output, std::plus{});
      col_offset += OTPBytes;
    }
    return init;
  }

  /*! \brief Vectorized version of offset_transform_reduce
   * */
  template <typename InIt1, typename InIt2, typename T, typename BinaryOp>
  T offset_transform_reduce_vec(InIt1 begin, InIt2 end, T init,
                                BinaryOp op) const {
    // Preallocated buffer for OTPs
    std::vector<EntryType> buffer(std::distance(begin, end) * init.size());
    auto buffer_view = pnm::views::make_view(buffer);

    static constexpr auto OTP_SIZE = OTPBytes / sizeof(EntryType);
    generate_otp_vec(0x00, version_, pnm::views::make_view(begin, end),
                     OTP_SIZE, init.size() / OTP_SIZE, aes_engine_,
                     buffer_view);

    auto otps_rows = pnm::views::make_rowwise_view(
        buffer_view.begin(), buffer_view.end(), init.size());

    return std::accumulate(otps_rows.begin(), otps_rows.end(), init, op);
  }

  /*! \brief Decrypt PSUM from two part according to secret sharing algorithm
   *
   * @tparam InIt1,InIt2,InIt3 iterator types. Must meet the requirements of
   * LegacyInputIterator.
   * @tparam OutIt iterator type. Must meet the requirements of
   * LegacyOutputIterator.
   *
   * @param psum_first,psum_last the range with encrypted psum
   * @param otp_sum_first the beginning of the OTP psum
   * @param dst_begin the beginning of the destination range with decrypted psum
   */
  template <typename InIt1, typename InIt2, typename InIt3, typename OutIt>
  OutIt decrypt_psum(InIt1 psum_first, InIt2 psum_last, InIt3 otp_sum_first,
                     OutIt dst_begin) const {
    auto plus = [](const auto &x, const auto &y)
                    NO_SANITIZE_UNSIGNED_INTEGER_OVERFLOW { return x + y; };
    return std::transform(psum_first, psum_last, otp_sum_first, dst_begin,
                          plus);
  };

private:
  using CryptoEngine::aes_engine_;
  using CryptoEngine::version_;
};
} // namespace pnm::sls::secure

#endif
