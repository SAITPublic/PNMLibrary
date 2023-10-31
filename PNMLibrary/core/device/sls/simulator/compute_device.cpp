#include "core/device/sls/simulator/compute_device.h"

#include "core/device/sls/base.h"
#include "core/device/sls/compute_device.h"
#include "core/device/sls/simulator/sim.h"
#include "core/device/sls/utils/constants.h"

#include "pnmlib/common/misc_utils.h"

#include <cstdint>

namespace pnm::sls::device {

SimulatorComputeDevice::SimulatorComputeDevice(BaseDevice *device)
    : ComputeDevice(device) {}

bool SimulatorComputeDevice::acquire_cunit_from_range(uint8_t *compute_unit,
                                                      cunit cunit_mask) {
  const auto acquired =
      device_->acquire_cunit_from_range(compute_unit, cunit_mask);
  if (acquired) {
    // Make sure that on simulator thread start registers are in reset state
    reset_psum_poll_register(*compute_unit);
    ComputeDevice::write_exec_sls_register(*compute_unit, SLS_DISABLE_VALUE);
    // Since the SimulatorDevice is in the constructor arguments, we can be sure
    // that the static_cast is correct
    static_cast<SimulatorDevice *>(device_)->get_sim().enable_thread(
        *compute_unit);
  }
  return acquired;
}

void SimulatorComputeDevice::write_exec_sls_register(uint8_t compute_unit,
                                                     uint32_t val) {
  // Thread that's going to wait on psum poll register
  // must reset that register to 0, so polling will exit only
  // after simulator async worker will write FINISH value to it
  // signaling that SLS computation is over
  if (val == SLS_ENABLE_VALUE) {
    reset_psum_poll_register(compute_unit);
  }
  ComputeDevice::write_exec_sls_register(compute_unit, val);
}

void SimulatorComputeDevice::release_cunit(uint8_t compute_unit) const {
  // Here order is important, thread must be disabled before relasing unit
  // since it potentially still can use unit for write despite it being released
  // Also, since the SimulatorDevice is in the constructor arguments, we can be
  // sure that the static_cast is correct
  static_cast<SimulatorDevice *>(device_)->get_sim().disable_thread(
      compute_unit);
  ComputeDevice::release_cunit(compute_unit);
}

void SimulatorComputeDevice::reset_psum_poll_register(uint8_t compute_unit) {
  pnm::utils::as_vatomic<uint32_t>(device_->psum_poll_register(compute_unit))
      ->store(0);
}

} // namespace pnm::sls::device
