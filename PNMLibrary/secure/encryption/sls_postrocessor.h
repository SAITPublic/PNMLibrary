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

#ifndef SLS_SECURE_POSTROCESSOR_H
#define SLS_SECURE_POSTROCESSOR_H

#include "pnmlib/common/rowwise_view.h"
#include "pnmlib/common/variable_row_view.h"
#include "pnmlib/common/views.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace sls::secure {

using requests_view =
    std::pair<const uint64_t,
              pnm::variable_row_view<const uint32_t, const uint32_t>>;
using indices_t = std::vector<requests_view>;

template <typename T> using psums_view = pnm::rowwise_view<T>;
template <typename T> using tags_view = pnm::rowwise_view<T>;

/*! \brief Interface for tables handler. Class that should handle base info
 * about tables structure.
 *
 * @tparam T is a type of data stored in table
 * */
template <typename T> class IPostprocessor {
public:
  virtual ~IPostprocessor() = default;

  void perform_sls(const indices_t &indices, psums_view<T> psum) const {
    perform_sls_impl(make_view(indices), psum, tags_view<T>{});
  }

  void perform_sls(pnm::common_view<const requests_view> indices,
                   psums_view<T> psum) const {
    perform_sls_impl(indices, psum, tags_view<T>{});
  }

  bool decrypt_psum(psums_view<T> psums, psums_view<T> epsums) const {
    return decrypt_psum_impl(psums, epsums, tags_view<T>{}, tags_view<T>{}, {});
  }

  void perform_sls(const indices_t &indices, psums_view<T> psum,
                   tags_view<T> tags) const {
    perform_sls_impl(make_view(indices), psum, tags);
  }

  void perform_sls(pnm::common_view<const requests_view> indices,
                   psums_view<T> psum, tags_view<T> tags) const {
    perform_sls_impl(indices, psum, tags);
  }

  bool decrypt_psum(psums_view<T> psums, psums_view<T> epsums,
                    tags_view<T> ctags, tags_view<T> etags,
                    pnm::common_view<uint8_t> check_result) const {
    return decrypt_psum_impl(psums, epsums, ctags, etags, check_result);
  }

private:
  virtual void perform_sls_impl(pnm::common_view<const requests_view> indices,
                                psums_view<T> psum,
                                tags_view<T> tags) const = 0;

  virtual bool
  decrypt_psum_impl(psums_view<T> psums, psums_view<T> epsums,
                    tags_view<T> ctags, tags_view<T> etags,
                    pnm::common_view<uint8_t> check_result) const = 0;
};
} // namespace sls::secure

#endif // SLS_SECURE_POSTROCESSOR_H
