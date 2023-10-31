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

#ifndef _SLS_IO_TRAITS_H_
#define _SLS_IO_TRAITS_H_

#include "pnmlib/common/128bit_math.h"

#include <cstdint>

namespace pnm::sls::secure {
// Here we use traits structure to define device-specific method for io.
// The traits are used because we try to keep original writers/readers unchanged
// and keep interface of io-system is similar to conventional streams
template <typename T, typename TT> struct io_traits_base {
  // Size of the regular data that are written/read by T writer/reader
  template <typename TData> static constexpr auto data_size = sizeof(TData);

  // Size of an extra data (like verification tag, or other) for each row in
  // embedding table.
  static constexpr auto meta_size = sizeof(pnm::types::uint128_t);

  // Write extra data for row in embedding tables.
  static void write_meta(T &out, const char *data, uint64_t size) {
    out.write(data, size);
  }

  // Memory size that is required by T to properly store/read count sequential
  // elements with or without meta
  template <typename TData>
  static constexpr auto memory_size(uint64_t count, bool with_meta) {
    return count * TT::template data_size<TData> +
           (with_meta ? TT::meta_size : 0);
  }
};

template <typename T>
struct io_traits : public io_traits_base<T, io_traits<T>> {};
} // namespace pnm::sls::secure

#endif //_SLS_IO_TRAITS_H_
