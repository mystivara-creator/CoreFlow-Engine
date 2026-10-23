#pragma once
#include "coreflow/model/types.hpp"
#include "coreflow/observation/surface_flinger.hpp"
#include <mutex>
#include <vector>
namespace coreflow {
class RuntimeObserver {
public:
    ObservationFrame observe(const std::vector<Capability>& capabilities) const;
private:
    SystemObservation observeSystem() const;
    mutable std::mutex cpu_sample_mutex_;
    mutable unsigned long long previous_cpu_total_{};
    mutable unsigned long long previous_cpu_idle_{};
    SurfaceFlingerObserver surface_flinger_;
};
}
