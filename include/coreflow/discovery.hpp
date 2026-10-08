#pragma once

#include "coreflow/types.hpp"

namespace coreflow {

class EnvironmentDiscovery {
public:
    DeviceProfile discover() const;

private:
    void discoverEnvironment(DeviceProfile&) const;
    void discoverCpuPolicies(DeviceProfile&) const;
    void discoverThermalZones(DeviceProfile&) const;
    void discoverCharging(DeviceProfile&) const;
    void discoverIo(DeviceProfile&) const;
    void discoverSystemControls(DeviceProfile&) const;
    void discoverGpu(DeviceProfile&) const;
    void finalizeCapabilities(DeviceProfile&) const;
};

} // namespace coreflow
