#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "coreflow/types.hpp"

namespace coreflow {

class RuntimeObserver {
public:
    RuntimeObserver() = default;
    RuntimeSample sample(const DeviceProfile& profile) const;

private:
    void readMemory(RuntimeSample&) const;
    void readLoad(RuntimeSample&) const;
    void readCpuUtilization(RuntimeSample&) const;
    void readThermal(RuntimeSample&, const DeviceProfile&) const;
    void readCharging(RuntimeSample&) const;
    void readIoActivity(RuntimeSample&) const;
    void readProcessProfile(RuntimeSample&) const;

    mutable bool have_cpu_baseline_{false};
    mutable std::uint64_t previous_cpu_total_{0};
    mutable std::uint64_t previous_cpu_idle_{0};
    mutable bool have_io_baseline_{false};
    mutable std::uint64_t previous_io_read_sectors_{0};
    mutable std::uint64_t previous_io_write_sectors_{0};
    mutable std::uint64_t sample_counter_{0};
    mutable double last_interval_seconds_{5.0};
    mutable std::chrono::steady_clock::time_point previous_sample_time_{};
    mutable std::chrono::steady_clock::time_point previous_process_profile_time_{};
    mutable bool have_sample_time_{false};
    mutable bool have_process_profile_time_{false};
    mutable std::unordered_map<int, std::uint64_t> previous_process_ticks_;
};

} // namespace coreflow
