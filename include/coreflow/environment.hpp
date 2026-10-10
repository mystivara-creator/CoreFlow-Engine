#pragma once

#include <cstdint>
#include <string>
#include <vector>

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
    AndroidRuntime
};

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

    std::size_t count(ResourceDomain domain) const noexcept;
    std::size_t mutationReadyCount() const noexcept;
    bool has(ResourceDomain domain, const std::string& name) const noexcept;
};

const char* resourceDomainName(ResourceDomain domain) noexcept;

} // namespace coreflow
