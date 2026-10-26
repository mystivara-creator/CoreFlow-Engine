#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "coreflow/launch_detector.hpp"

namespace coreflow::observe {

// Counts process ids in the body of a cgroup.procs file (one decimal id per
// line). Malformed lines are skipped. Counting stops at kMaxPids, which bounds
// the work done on an untrusted file.
inline constexpr std::size_t kMaxCgroupPids = 65536U;
[[nodiscard]] std::size_t countCgroupProcs(const std::string& text) noexcept;

// Reads the foreground group membership and the I/O pressure file, then builds
// one LaunchObservation. The caller supplies the monotonic timestamp. Returns
// false when the membership file cannot be read. An unreadable pressure file
// yields io_pressure_some = -1, which the detector treats as unavailable.
[[nodiscard]] bool readLaunchObservation(const std::string& foreground_procs_path,
                                         const std::string& io_pressure_path,
                                         std::uint64_t timestamp_monotonic_ns,
                                         LaunchObservation& out);

} // namespace coreflow::observe
