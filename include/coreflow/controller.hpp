#pragma once

#include "coreflow/policy.hpp"
#include "coreflow/observer.hpp" // Untuk RuntimeSample

namespace coreflow {

class MutationController {
public:
    MutationController() = default;

    // Memverifikasi apakah environment siap menerima mutasi (baseline & writability test)
    bool isReady() const noexcept {
        // Contoh real-world: cek apakah path sysfs kernel bisa ditulisi
        // Jika return false, engine tidak akan memaksa mengeksekusi mutasi.
        return true; 
    }

    // Eksekutor mutasi berdasarkan keputusan policy
    void execute(Decision decision, const RuntimeSample& sample) {
        switch (decision) {
            case Decision::Aggressive:
                // Terapkan profile performa maksimal
                applyPerformanceProfile();
                break;
            case Decision::Conservative:
                // Terapkan profile hemat daya / Doze
                applyEfficiencyProfile();
                break;
            case Decision::NoAction:
            default:
                break;
        }
    }

private:
    void applyPerformanceProfile() {
        // TODO: Logika penulisan sysfs untuk governor/frekuensi CPU
    }

    void applyEfficiencyProfile() {
        // TODO: Logika penulisan sysfs untuk penghematan baterai
    }
};

} // namespace coreflow
