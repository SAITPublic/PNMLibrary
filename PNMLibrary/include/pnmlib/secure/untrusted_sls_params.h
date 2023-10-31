#ifndef UNTRUSTED_SLS_PARAMS_H
#define UNTRUSTED_SLS_PARAMS_H

#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <cstdint>

namespace pnm::sls::secure {

struct UntrustedDeviceParams {
  pnm::views::common<const uint32_t> rows;
  // [TODO: @e-kutovoi] Make this a uint32_t or convert all other usages to
  // uint64_t
  uint64_t sparse_feature_size;
  bool with_tag{};
  sls_user_preferences preference = SLS_ALLOC_AUTO;
};

} // namespace pnm::sls::secure

#endif // UNTRUSTED_SLS_PARAMS_H
