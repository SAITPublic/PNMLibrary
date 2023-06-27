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

#ifndef _AES_CRYPTO_H_
#define _AES_CRYPTO_H_

#include "common.h"

#include "common/sls_types.h"

#include <cstdint>
#include <iterator>

namespace sls::secure {

/*! \brief Common iterface for data encryption with AES.
 * The encryption are implemented in EncryptionBackend that
 * should be constructible from KeySize and secret key and
 * should has an encrypt method.
 */
template <AES_KEY_SIZE KeySize, typename EncryptionBackend> class AES_Engine {
public:
  // Size of encryption key in bits and bytes
  constexpr static unsigned aes_key_bits = static_cast<unsigned>(KeySize);
  constexpr static unsigned aes_key_bytes = aes_key_bits / __CHAR_BIT__;

  using AES_Key_type = FixedVector<uint8_t, aes_key_bytes>;

  explicit AES_Engine(const AES_Key_type &key)
      : aes_key_{key}, backend_{KeySize, aes_key_.data()} {}

  void encrypt(const uint8_t *begin, const uint8_t *end, uint8_t *out) const {
    auto size = std::distance(begin, end);
    backend_.encrypt(begin, size,
                     out); // [TODO: @y-lavrinenko] Add return code check when
                           // error catch strategy introduced
  }

private:
  AES_Key_type aes_key_;
  EncryptionBackend backend_;
};
} // namespace sls::secure
#endif
