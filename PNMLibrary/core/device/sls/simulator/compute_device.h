#ifndef _SLS_SIMULATOR_COMPUTE_DEVICE_H_
#define _SLS_SIMULATOR_COMPUTE_DEVICE_H_

#include "core/device/sls/base.h"
#include "core/device/sls/compute_device.h"
#include "core/device/sls/utils/constants.h"

#include <cstdint>

namespace pnm::sls::device {
/** @brief Core device for SLS simulator compute units. Implements specific
 * behaviour for SLS simulator.
 */
class SimulatorComputeDevice : public ComputeDevice {
public:
  SimulatorComputeDevice(BaseDevice *device);

  /** @brief Acquires compute unit from kernel and launches simulator thread,
   * representing such unit.
   */
  bool acquire_cunit_from_range(uint8_t *compute_unit, cunit cunit_mask);

  /** @brief Writes to exec sls register, implementing actions specific for SLS
   * simulator
   */
  void write_exec_sls_register(uint8_t compute_unit, uint32_t val);

  /** @brief Releases compute unit to kernel and disables simulator thread,
   * representing such unit.
   */
  void release_cunit(uint8_t compute_unit) const;

private:
  /** @brief Sets psum poll register to 0 for particular compute unit.
   */
  void reset_psum_poll_register(uint8_t compute_unit);
};
} // namespace pnm::sls::device
#endif
