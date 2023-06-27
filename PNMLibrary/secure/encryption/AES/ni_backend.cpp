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

#include "ni_backend.h"

#include "key_exp128.h"
#include "key_exp192.h"
#include "key_exp256.h"

#include "secure/encryption/AES/common.h"

#include "common/compiler_internal.h"

#include <emmintrin.h>
#include <wmmintrin.h>

#include <algorithm>
#include <cstdint>
#include <memory>

namespace {
unsigned aes_rounds_count(sls::secure::AES_KEY_SIZE key) {
  switch (key) {
  case sls::secure::AES_KEY_SIZE::AES_128:
    return 10;
  case sls::secure::AES_KEY_SIZE::AES_192:
    return 12;
  case sls::secure::AES_KEY_SIZE::AES_256:
    return 14;
  };
  return 0;
}

// Size of plaintext in bytes. Defined by AES algorithm.
constexpr auto AES_BLOCK_SIZE = 16U;

/*! /brief Perform encription of plaintext via AES-NI
 *
 * @param in pointer to the PLAINTEXT
 * @param out pointer to the CIPHERTEXT buffer
 * @param length text length in bytes
 * @paramm key pointer to the expanded key schedule
 * @param number_of_rounds number of AES rounds 10, 12 or 14
 *
 * @return Number of encrypted bytes
 * */
unsigned long AES_ECB_encrypt(const uint8_t *in, uint8_t *out,
                              unsigned long length, const uint8_t *key,
                              unsigned int number_of_rounds) {
  const auto *casted_in = reinterpret_cast<const __m128i *>(in);
  auto *casted_out = reinterpret_cast<__m128i *>(out);
  const auto *casted_key = reinterpret_cast<const __m128i *>(key);

  static constexpr auto PIPELINE_SIZE = 4U;

  const auto cipher_blocks = length / AES_BLOCK_SIZE;
  const auto pipeline_main_blocks = cipher_blocks / PIPELINE_SIZE;

  // Encrypt main blocks with instruction pipeline
  for (auto i = 0U; i < pipeline_main_blocks; ++i) {
    __m128i tmp[PIPELINE_SIZE];

    for (auto k = 0U; k < PIPELINE_SIZE; ++k) {
      tmp[k] = _mm_loadu_si128(&casted_in[i * PIPELINE_SIZE + k]);
    }

    for (auto &v : tmp) {
      v = _mm_xor_si128(v, casted_key[0]);
    }

    for (auto j = 1U; j < number_of_rounds; j++) {
      for (auto &v : tmp) {
        v = _mm_aesenc_si128(v, casted_key[j]);
      }
    }

    for (auto &v : tmp) {
      v = _mm_aesenclast_si128(v, casted_key[number_of_rounds]);
    }

    for (auto k = 0U; k < PIPELINE_SIZE; ++k) {
      _mm_storeu_si128(&casted_out[i * PIPELINE_SIZE + k], tmp[k]);
    }
  }

  auto aes_simple_rounds = [&casted_key, number_of_rounds](auto data) {
    data = _mm_xor_si128(data, casted_key[0]);
    for (auto j = 1U; j < number_of_rounds; j++) {
      data = _mm_aesenc_si128(data, casted_key[j]);
    }
    data = _mm_aesenclast_si128(data, casted_key[number_of_rounds]);
    return data;
  };

  // Encrypt last blocks without pipeline
  for (auto i = pipeline_main_blocks * PIPELINE_SIZE; i < cipher_blocks; ++i) {
    auto tmp = _mm_loadu_si128(&casted_in[i]);
    _mm_storeu_si128(&casted_out[i], aes_simple_rounds(tmp));
  }

  if (UNLIKELY(length % AES_BLOCK_SIZE)) { // We have uncompleted
                                           // block of plain text
    uint8_t augmented_block[AES_BLOCK_SIZE] = {};
    std::copy(&in[cipher_blocks * AES_BLOCK_SIZE], &in[length],
              augmented_block);
    auto tmp =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(augmented_block));
    _mm_storeu_si128(&casted_out[cipher_blocks], aes_simple_rounds(tmp));

    // We encrypt extra block
    return (length / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
  }

  return length;
}
} // namespace

sls::secure::AES_NI_Backend::AES_NI_Backend(sls::secure::AES_KEY_SIZE key_size,
                                            const uint8_t *key)
    : encryption_rounds_(::aes_rounds_count(key_size)) {
  key_schedule_ =
      std::make_unique<uint8_t[]>((encryption_rounds_ + 1) * ::AES_BLOCK_SIZE);
  switch (key_size) {
  case AES_KEY_SIZE::AES_128:
    AES_128_Key_Expansion(key, key_schedule_.get());
    break;
  case AES_KEY_SIZE::AES_192:
    AES_192_Key_Expansion(key, key_schedule_.get());
    break;
  case AES_KEY_SIZE::AES_256:
    AES_256_Key_Expansion(key, key_schedule_.get());
    break;
  }
}

int sls::secure::AES_NI_Backend::encrypt(const uint8_t *data, int length,
                                         uint8_t *out) const {
  return ::AES_ECB_encrypt(data, out, length, key_schedule_.get(),
                           encryption_rounds_);
}
