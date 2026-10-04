#pragma once
#include "coreflow/types.hpp"

namespace coreflow {

class EnvironmentDiscovery {
public:
    DeviceProfile discover() const;

private:
    void discoverCpuPolicies(DeviceProfile&) const;
    void discoverThermalZones(DeviceProfile&) const;
    void discoverCharging(DeviceProfile&) const;
};

} // namespace coreflow
