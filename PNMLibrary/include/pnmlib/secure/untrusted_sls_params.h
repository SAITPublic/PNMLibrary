#ifndef UNTRUSTED_SLS_PARAMS_H
#define UNTRUSTED_SLS_PARAMS_H

#include "pnmlib/common/views.h"

#include <linux/sls_resources.h>

#include <cstdint>

namespace sls::secure {

struct UntrustedDeviceParams {
  pnm::common_view<const uint32_t> rows;
  // [TODO: @e-kutovoi] Make this a uint32_t or convert all other usages to
  // uint64_t
  uint64_t sparse_feature_size;
  bool with_tag{};
  sls_user_preferences preference = SLS_ALLOC_AUTO;
};

} // namespace sls::secure

#endif // UNTRUSTED_SLS_PARAMS_H
