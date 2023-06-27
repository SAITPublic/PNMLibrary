#include "pnmlib/secure/sls_runner_factory.h"

#include "secure/plain/sls.h"

#include "common/make_error.h"

#include "pnmlib/secure/base_runner.h"

#include "pnmlib/common/compiler.h"

#include <cstdint>
#include <memory>
#include <type_traits>

namespace sls::secure {

PNM_API std::unique_ptr<IRunner> make_runner(RunnerType type) {

  switch (type) {
  case RunnerType::INT:
    return std::make_unique<ProdConsSLSRunner<int32_t>>();
  case RunnerType::FLOAT:
    return std::make_unique<ProdConsSLSRunner<uint32_t>>();
  default:
    throw pnm::error::make_not_sup("Secure runner type({}).",
                                   std::underlying_type_t<RunnerType>(type));
  }
}

} // namespace sls::secure
