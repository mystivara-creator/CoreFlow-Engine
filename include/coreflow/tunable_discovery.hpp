#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "coreflow/environment.hpp"
#include "coreflow/vendor_knowledge.hpp"

namespace coreflow {

// Generic, capability-driven tunable discovery.
//
// Walks the kernel's tunable filesystems (procfs sysctls, cpufreq and governor
// tunables, core_ctl, block queues and I/O schedulers, devfreq, GPU drivers,
// cpu/uclamp cgroups, cpuset, schedtune) plus every path the vendor's own
// configuration writes. Each regular file that the kernel marks writable is
// recorded with:
//   - its domain (derived from where it lives, not from its name)
//   - kernel-advertised options (selector lines / sibling available_* lists)
//   - vendor-declared values (VendorKnowledge)
// Admissibility is decided by classifySurface(), whose only hard rule is the
// universal FatalSurfaceGuard. No parameter names are listed anywhere.
struct TunableDiscoveryStats {
    std::size_t walked{0};
    std::size_t added{0};
    std::size_t vendor_paths{0};
    std::size_t fatal_skipped{0};
};

class TunableDiscovery {
public:
    static constexpr std::size_t kMaxTunables = 4096;

    // root_prefix lets host tests mount a fixture tree (e.g. "/tmp/x"); on the
    // device it is empty.
    explicit TunableDiscovery(std::string root_prefix = {}) : prefix_(std::move(root_prefix)) {}

    TunableDiscoveryStats discover(EnvironmentCapabilityMatrix& matrix,
                                   const VendorKnowledge& vendor) const noexcept;

    // Attach kernel-advertised options and vendor values to every tunable
    // already present in the matrix (including ones added by other probes).
    void enrich(EnvironmentCapabilityMatrix& matrix,
                const VendorKnowledge& vendor) const noexcept;

    // Exposed for tests.
    ResourceDomain domainFor(std::string_view device_path) const noexcept;
    std::string nameFor(std::string_view device_path) const;
    static bool kernelWritable(const std::string& path) noexcept;

private:
    std::string_view strip(std::string_view path) const noexcept;
    bool addTunable(EnvironmentCapabilityMatrix& matrix, const std::string& path,
                    TunableDiscoveryStats& stats) const noexcept;
    void walk(EnvironmentCapabilityMatrix& matrix, const std::string& dir, std::size_t depth,
              TunableDiscoveryStats& stats, bool (*accept_dir)(std::string_view)) const noexcept;

    std::string prefix_;
};

} // namespace coreflow
