#pragma once

#include "coreflow/types.hpp"

namespace coreflow {

class RuntimeObserver {
public:
    RuntimeSample sample(const DeviceProfile& profile) const;

private:
    void readMemory(RuntimeSample&) const;
    void readLoad(RuntimeSample&) const;
    void readThermal(RuntimeSample&, const DeviceProfile&) const;
    void readCharging(RuntimeSample&) const;
};

} // namespace coreflow
