/**
 * @file memory_guardian.cpp
 * @brief Memory Guardian Implementation
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/memory_guardian.hpp"
#include "coreflow/baseline_store.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace coreflow {

namespace fs = std::filesystem;

MemoryGuardian::MemoryGuardian() {
    mglru_supported_ = fs::exists("/sys/kernel/mm/lru_gen/enabled");
    if (mglru_supported_) {
        std::cout << "[CoreFlow][Memory] Linux Kernel MGLRU (Multi-Gen LRU) active." << std::endl;
    }
}

bool MemoryGuardian::has_mglru() const noexcept {
    return mglru_supported_;
}

bool MemoryGuardian::tune_mglru(bool high_priority_foreground) {
    if (!mglru_supported_) return false;

    // Bitmap in /sys/kernel/mm/lru_gen/enabled:
    // 7 = all generations active with proactive page demotion
    const char* node = "/sys/kernel/mm/lru_gen/enabled";
    if (BaselineStore::instance().capture(node)) {
        return BaselineStore::instance().apply_mutation(node, high_priority_foreground ? "7" : "5");
    }
    return false;
}

bool MemoryGuardian::balance_zram_swappiness(uint32_t free_ram_mb) {
    const char* swappiness_node = "/proc/sys/vm/swappiness";
    if (!BaselineStore::instance().capture(swappiness_node)) return false;

    // Adapt swappiness dynamically:
    // If RAM is plenty (> 1500MB), keep swappiness low (60) to save CPU compression cycles
    // If RAM is low (< 800MB on mid-range), raise to 100 to prevent LMK foreground kills
    std::string target_val = (free_ram_mb < 800) ? "100" : "60";
    return BaselineStore::instance().apply_mutation(swappiness_node, target_val);
}

bool MemoryGuardian::protect_top_app_oom(pid_t pid) {
    (void)pid;
    // Android's process OOM policy is owned by lmkd; never override it here.
    return false;
}
} // namespace coreflow
