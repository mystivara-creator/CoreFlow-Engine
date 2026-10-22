#pragma once
#include "coreflow/model/types.hpp"
#include <vector>
namespace coreflow {
class RuntimeObserver {
public:
    ObservationFrame observe(const std::vector<Capability>& capabilities) const;
private:
    SystemObservation observeSystem() const;
};
}
