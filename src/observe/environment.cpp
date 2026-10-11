#include "coreflow/environment.hpp"

namespace coreflow {

std::size_t EnvironmentCapabilityMatrix::count(ResourceDomain domain) const noexcept {
    std::size_t result = 0;
    for (const auto& resource : resources) {
        if (resource.domain == domain) ++result;
    }
    return result;
}

std::size_t EnvironmentCapabilityMatrix::mutationReadyCount() const noexcept {
    std::size_t result = 0;
    for (const auto& resource : resources) {
        if (resource.mutation_ready) ++result;
    }
    return result;
}

bool EnvironmentCapabilityMatrix::has(
    ResourceDomain domain, const std::string& name) const noexcept {
    for (const auto& resource : resources) {
        if (resource.domain == domain && resource.name == name) return true;
    }
    return false;
}

const char* resourceDomainName(ResourceDomain domain) noexcept {
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
    }
    return "UNKNOWN";
}

} // namespace coreflow
