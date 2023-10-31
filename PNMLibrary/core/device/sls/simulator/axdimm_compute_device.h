#ifndef _SLS_AXDIMM_SIM_COMPUTE_DEVICE_H_
#define _SLS_AXDIMM_SIM_COMPUTE_DEVICE_H_

#include "core/device/sls/base.h"
#include "core/device/sls/simulator/axdimm_sim.h"
#include "core/device/sls/simulator/compute_device.h"

#include <cstdint>

namespace pnm::sls::device {

class AxdimmSimulatorComputeDevice : public SimulatorComputeDevice {
public:
  AxdimmSimulatorComputeDevice(BaseDevice *device);

  void write_exec_sls_register(uint8_t compute_unit, uint32_t val);
  void write_control_switch_register(uint8_t compute_unit, uint32_t val);

private:
  AxdimmSimulator *sim_;
};

} // namespace pnm::sls::device

#endif
