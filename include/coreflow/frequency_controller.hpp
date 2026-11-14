#pragma once
#include "coreflow/device_profile.hpp"
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {
class CentralMutationCoordinator;
enum class MutationDomain : uint8_t;
// Only kernel-exposed frequency tables are accepted; never invent an operating point.
class FrequencyController {
public:
    explicit FrequencyController(CentralMutationCoordinator& coordinator,
                                 std::string sys_root = "/sys",
                                 DeviceProfileHandle profile = {});
    ~FrequencyController();
    void discover(bool cpu, bool gpu);
    bool apply(double capacity_scale, bool thermal_guard = false);
    [[nodiscard]] size_t cpu_count() const noexcept;
    [[nodiscard]] size_t gpu_count() const noexcept;
private:
    struct Domain {
        std::string maximum, minimum;
        uint64_t baseline, last;
        std::vector<uint64_t> frequencies;
        bool gpu, retired{false}, mutation_active{false};
    };
    CentralMutationCoordinator& coordinator_;
    std::string root_;
    DeviceProfileHandle profile_;
    std::string controller_name_;
    mutable std::mutex mutex_;
    std::vector<Domain> domains_;
    std::string cpu_transaction_id_;
    std::string gpu_transaction_id_;
    uint64_t lease_generation_{0};
    bool acquire_domain_lease(bool gpu, bool thermal_guard);
    bool rollback_domain(bool gpu);
    bool handle_preemption(MutationDomain domain, std::string_view transaction_id) noexcept;
    void release_domain_lease(bool gpu) noexcept;
    void add(const std::string& dir, bool gpu);
};
}
