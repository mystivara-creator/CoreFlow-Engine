#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "coreflow/vendor_knowledge.hpp"

namespace coreflow {

enum class ResourceDomain {
    CpuFreq,
    UClamp,
    CpuSet,
    Scheduler,
    Memory,
    Io,
    Gpu,
    Thermal,
    Charging,
    Power,
    CGroup,
    AndroidRuntime,
    // Any other kernel tunable discovered from vendor configuration or the
    // generic walkers that does not belong to one of the domains above.
    Vendor
};

// Domains whose discovered nodes are autonomous mutation candidates. The
// excluded domains are inventory/telemetry (thermal zones, charging, power
// supply, cgroup structure, Android runtime markers); their write surfaces are
// fatal classes anyway (see fatal_surface.hpp).
inline bool isTunableResourceDomain(ResourceDomain domain) noexcept {
    switch (domain) {
        case ResourceDomain::CpuFreq:
        case ResourceDomain::UClamp:
        case ResourceDomain::CpuSet:
        case ResourceDomain::Scheduler:
        case ResourceDomain::Memory:
        case ResourceDomain::Io:
        case ResourceDomain::Gpu:
        case ResourceDomain::Vendor:
            return true;
        case ResourceDomain::Thermal:
        case ResourceDomain::Charging:
        case ResourceDomain::Power:
        case ResourceDomain::CGroup:
        case ResourceDomain::AndroidRuntime:
            return false;
    }
    return false;
}

struct ResourceCapability {
    ResourceDomain domain{ResourceDomain::AndroidRuntime};
    std::string name;
    std::string path;
    bool exists{false};
    bool readable{false};
    bool writable{false};
    // Effective OS-level write access is evidence, not an optimization policy.
    bool permission_granted{false};
    // Explicit engine policy approval for this exact resource class/path.
    // A writable or readable node never sets this field by itself.
    bool policy_authorized{false};
    // Runtime-read confirmation; this does not prove that a write is safe.
    bool runtime_verified{false};
    bool mutation_ready{false};

    // Capability evidence used to generate candidate values (no name tables):
    //   options        values the kernel itself advertises for this node, from a
    //                  "[a] b c" selector line or a sibling available_* file
    //   ordered_scale  options are an ordered hardware scale (frequencies/levels)
    //   vendor_values  values the vendor's own init/power-HAL config writes
    std::vector<std::string> options{};
    bool ordered_scale{false};
    std::vector<VendorValue> vendor_values{};
};

struct AndroidEnvironment {
    std::string release;
    std::string sdk_level;
    std::string manufacturer;
    std::string model;
    std::string device;
    std::string product;
    std::string board;
    std::string hardware;
    std::string soc_manufacturer;
    std::string soc_model;
    std::string abi;
    std::string kernel_release;
    std::string fingerprint;
    std::string security_patch;
    std::string incremental;
    std::string treble_enabled;
    std::string gsi_running;
    std::string hardware_sku;
    std::string bootloader;

    bool proc_available{false};
    bool sys_available{false};
    bool cgroup_v2{false};
    bool cpuset_available{false};
    bool uclamp_available{false};
    bool scheduler_controls_available{false};
    bool devfreq_available{false};
};

struct EnvironmentCapabilityMatrix {
    AndroidEnvironment environment;
    std::vector<ResourceCapability> resources;

    inline std::size_t count(ResourceDomain domain) const noexcept {
        std::size_t result = 0;
        for (const auto& resource : resources) {
            if (resource.domain == domain) ++result;
        }
        return result;
    }

    inline std::size_t mutationReadyCount() const noexcept {
        std::size_t result = 0;
        for (const auto& resource : resources) {
            if (resource.mutation_ready) ++result;
        }
        return result;
    }

    inline bool has(
        ResourceDomain domain, const std::string& name) const noexcept {
        for (const auto& resource : resources) {
            if (resource.domain == domain && resource.name == name) return true;
        }
        return false;
    }
};

inline const char* resourceDomainName(ResourceDomain domain) noexcept {
    switch (domain) {
        case ResourceDomain::CpuFreq: return "CPUFREQ";
        case ResourceDomain::UClamp: return "UCLAMP";
        case ResourceDomain::CpuSet: return "CPUSET";
        case ResourceDomain::Scheduler: return "SCHEDULER";
        case ResourceDomain::Memory: return "MEMORY";
        case ResourceDomain::Io: return "IO";
        case ResourceDomain::Gpu: return "GPU";
        case ResourceDomain::Thermal: return "THERMAL";
        case ResourceDomain::Charging: return "CHARGING";
        case ResourceDomain::Power: return "POWER";
        case ResourceDomain::CGroup: return "CGROUP";
        case ResourceDomain::AndroidRuntime: return "ANDROID_RUNTIME";
        case ResourceDomain::Vendor: return "VENDOR";
    }
    return "UNKNOWN";
}

} // namespace coreflow
