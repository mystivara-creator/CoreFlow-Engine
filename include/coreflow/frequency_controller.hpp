#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace coreflow {
// Only kernel-exposed frequency tables are accepted; never invent an operating point.
class FrequencyController {
public:
    explicit FrequencyController(std::string sys_root = "/sys");
    void discover(bool cpu, bool gpu);
    bool apply(double capacity_scale);
    [[nodiscard]] size_t cpu_count() const noexcept;
    [[nodiscard]] size_t gpu_count() const noexcept;
private:
    struct Domain {
        std::string maximum, minimum;
        uint64_t baseline, last;
        std::vector<uint64_t> frequencies;
        bool gpu, retired{false};
    };
    std::string root_;
    std::vector<Domain> domains_;
    void add(const std::string& dir, bool gpu);
};
}
