#pragma once

#include <cstdint>

#include "coreflow/types.hpp"

namespace coreflow {

class RuntimeObserver {
public:
    RuntimeObserver() = default;
    RuntimeSample sample(const DeviceProfile& profile) const;

private:
    void readMemory(RuntimeSample&) const;
    void readLoad(RuntimeSample&) const;
    void readCpuUtilization(RuntimeSample&) const;
    void readThermal(RuntimeSample&, const DeviceProfile&) const;
    void readCharging(RuntimeSample&) const;

    mutable bool have_cpu_baseline_{false};
    mutable std::uint64_t previous_cpu_total_{0};
    mutable std::uint64_t previous_cpu_idle_{0};
};

} // namespace coreflow
