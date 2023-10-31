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

#ifndef SLS_SECURE_PREPROCESSOR_H
#define SLS_SECURE_PREPROCESSOR_H

#include "sls_postrocessor.h"

#include <cstdint>
#include <memory>

namespace pnm::sls::secure {

/*! \brief Interface for tables preprocessor. Class that should handle base info
 * about tables structure and perform encryption of embedding tables
 * @tparam T -- type of table's entry
 * */
template <typename T> class IPreprocessor {
public:
  virtual ~IPreprocessor() = default;

  auto load(void *out, bool include_tag) { return load_impl(out, include_tag); }

  auto encrypted_buffer_size(bool include_tag) const {
    return encrypted_buffer_size_impl(include_tag);
  }

private:
  virtual std::unique_ptr<IPostprocessor<T>> load_impl(void *out,
                                                       bool include_tag) = 0;

  virtual uint64_t encrypted_buffer_size_impl(bool include_tag) const = 0;
};
} // namespace pnm::sls::secure

#endif // SLS_SECURE_PREPROCESSOR_H
