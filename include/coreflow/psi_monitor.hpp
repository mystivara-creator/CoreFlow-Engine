/**
 * @file psi_monitor.hpp
 * @brief Linux Pressure Stall Information (PSI) Trigger Engine
 * @author mystivara-creator
 * @license Apache-2.0
 * 
 * Replaces reactive lag detection with kernel PSI triggers.
 * Automatically wakes epoll when memory or CPU stall exceeds microsecond thresholds.
 */

#pragma once

#include "types.hpp"
#include "reactor.hpp"
#include <string_view>
#include <functional>

namespace coreflow {

enum class PsiResource {
    Cpu,
    Memory,
    Io
};

using PsiCallback = std::function<void(PsiResource res, double stall_pct)>;

class PsiMonitor {
public:
    explicit PsiMonitor(EpollReactor& reactor);
    ~PsiMonitor() = default;

    /// Register a kernel PSI stall trigger (e.g. "some 20000 1000000" = 20ms stall in 1s window)
    bool register_trigger(PsiResource res, uint32_t stall_us, uint32_t window_us, PsiCallback callback);

    /// Query instantaneous pressure metrics from /proc/pressure/
    [[nodiscard]] double get_current_pressure(PsiResource res) const;

private:
    EpollReactor& reactor_;
    std::unordered_map<int, PsiResource> fd_map_;
};

} // namespace coreflow
