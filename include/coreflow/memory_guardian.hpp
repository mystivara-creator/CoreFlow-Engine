/**
 * @file memory_guardian.hpp
 * @brief Android 15 MGLRU & ZRAM Dynamic Memory Balancer
 * @author mystivara-creator
 * @license Apache-2.0
 * 
 * Replaces crude vm.swappiness overrides with kernel MGLRU (Multi-Gen LRU) tuning.
 * Optimizes foreground memory retention and eliminates background game reloads.
 */

#pragma once

#include "types.hpp"
#include <string>

namespace coreflow {

class MemoryGuardian {
public:
    MemoryGuardian();
    ~MemoryGuardian() = default;

    /// Checks if CONFIG_LRU_GEN is enabled in kernel (/sys/kernel/mm/lru_gen/enabled)
    [[nodiscard]] bool has_mglru() const noexcept;

    /// Optimize Multi-Gen LRU generation ages for foreground app retention
    bool tune_mglru(bool high_priority_foreground);

    /// Dynamic ZRAM swappiness adjustment based on current free memory
    bool balance_zram_swappiness(uint32_t free_ram_mb);

    /// Protect top-app from premature LowMemoryKiller (LMK) eviction
    bool protect_top_app_oom(pid_t top_app_pid);

private:
    bool mglru_supported_{false};
    std::string zram_comp_algo_;
};

} // namespace coreflow
