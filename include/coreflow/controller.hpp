#pragma once

#include "coreflow/policy.hpp"
#include "coreflow/observer.hpp"

namespace coreflow {

class MutationController {
public:
    MutationController() = default;

    // Memverifikasi apakah environment siap menerima mutasi
    bool isReady() const noexcept {
        return true; 
    }

    // Eksekutor mutasi berdasarkan keputusan policy
    void execute(Decision decision, const RuntimeSample& sample) {
        // Karena opsi lengkap Decision ada di types.hpp,
        // kita gunakan pengecekan generik NoAction agar lolos kompilasi.
        if (decision != Decision::NoAction) {
            // TODO: Tambahkan switch-case mutasi di sini nanti
            // setelah kamu mengecek isi dari file types.hpp
        }
    }
};

} // namespace coreflow
