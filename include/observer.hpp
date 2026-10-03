#pragma once
#include "coreflow/types.hpp"

namespace coreflow {

class RuntimeObserver {
public:
    RuntimeSample sample() const;

private:
    void readMemory(RuntimeSample&) const;
    void readLoad(RuntimeSample&) const;
    void readThermal(RuntimeSample&) const;
};

} // namespace coreflow
