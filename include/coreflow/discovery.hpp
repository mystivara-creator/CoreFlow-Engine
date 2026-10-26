#pragma once

#include "coreflow/types.hpp"

namespace coreflow {

// Discovery is organised around one principle: nothing is named in advance.
// Inventory probes describe the device's classes (thermal, power, cpufreq,
// devfreq, block queues). Tunables come from the kernel mount table plus the
// vendor's own configuration, via discoverTunables(). The matrix is the only
// product, and policy reads it rather than a hard-coded list.
class EnvironmentDiscovery {
public:
    DeviceProfile discover() const;

private:
    void discoverEnvironment(DeviceProfile&) const;
    void discoverCpuPolicies(DeviceProfile&) const;
    void discoverThermalZones(DeviceProfile&) const;
    void discoverCharging(DeviceProfile&) const;
    void discoverIo(DeviceProfile&) const;
    void discoverGpu(DeviceProfile&) const;
    void finalizeCapabilities(DeviceProfile&) const;
    void discoverTunables(DeviceProfile&) const;
    void deriveEnvironmentAvailability(DeviceProfile&) const;
};

} // namespace coreflow
