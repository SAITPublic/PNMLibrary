#include "pnmlib/secure/base_runner.h"

#include <cstdint>
#include <memory>

namespace pnm::sls::secure {

enum class RunnerType : uint8_t {
  INT,
  FLOAT,
};

std::unique_ptr<IRunner> make_runner(RunnerType type);
} // namespace pnm::sls::secure
