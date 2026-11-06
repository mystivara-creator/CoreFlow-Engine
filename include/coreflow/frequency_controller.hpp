#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {
enum class MutationDomain : uint8_t;
// Only kernel-exposed frequency tables are accepted; never invent an operating point.
class FrequencyController {
public:
    explicit FrequencyController(std::string sys_root = "/sys");
    ~FrequencyController();
    void discover(bool cpu, bool gpu);
    bool apply(double capacity_scale);
    [[nodiscard]] size_t cpu_count() const noexcept;
    [[nodiscard]] size_t gpu_count() const noexcept;
private:
    struct Domain {
        std::string maximum, minimum;
        uint64_t baseline, last;
        std::vector<uint64_t> frequencies;
        bool gpu, retired{false}, mutation_active{false};
    };
    std::string root_;
    mutable std::mutex mutex_;
    std::vector<Domain> domains_;
    std::string cpu_transaction_id_;
    std::string gpu_transaction_id_;
    uint64_t lease_generation_{0};
    bool acquire_domain_lease(bool gpu);
    bool rollback_domain(bool gpu);
    bool handle_preemption(MutationDomain domain, std::string_view transaction_id) noexcept;
    void release_domain_lease(bool gpu) noexcept;
    void add(const std::string& dir, bool gpu);
};
}
