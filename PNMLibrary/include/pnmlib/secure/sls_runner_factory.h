#include "pnmlib/secure/base_runner.h"

#include <memory>

namespace sls::secure {

enum class RunnerType {
  INT,
  FLOAT,
};

std::unique_ptr<IRunner> make_runner(RunnerType type);
} // namespace sls::secure
