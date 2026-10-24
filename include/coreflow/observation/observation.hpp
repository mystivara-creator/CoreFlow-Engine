#pragma once
#include "coreflow/model/types.hpp"
#include "coreflow/observation/noise_floor.hpp"
#include "coreflow/observation/surface_flinger.hpp"
#include <mutex>
#include <vector>
namespace coreflow {
class RuntimeObserver {
public:
    ObservationFrame observe(const std::vector<Capability>& capabilities) const;
private:
    SystemObservation observeSystem() const;
    // Serialize complete observations: CPU counter deltas, frame observer state,
    // and the shared noise-floor history must advance in the same order.
    mutable std::mutex observe_mutex_;
    mutable std::mutex cpu_sample_mutex_;
    mutable unsigned long long previous_cpu_total_{};
    mutable unsigned long long previous_cpu_idle_{};
    mutable NoiseFloorTracker noise_;
    SurfaceFlingerObserver surface_flinger_;
};
}
