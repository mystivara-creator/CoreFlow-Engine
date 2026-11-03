/**
 * @file uclamp.hpp
 * @brief Capability-driven Android/Linux utilization clamp controller
 * @author mystivara-creator
 * @license MIT
 */
#pragma once
#include "types.hpp"
#include <string>
#include <string_view>

namespace coreflow {

class UclampGovernor {
public:
    // Empty root means runtime discovery from mounted cgroup filesystems.
    // An explicit root exists only for deterministic tests or controlled bring-up.
    explicit UclampGovernor(std::string_view explicit_root = {});
    ~UclampGovernor() = default;

    [[nodiscard]] bool is_supported() const noexcept;
    [[nodiscard]] std::string_view target_path() const noexcept { return target_path_; }
    [[nodiscard]] std::string_view encoding_name() const noexcept;

    bool apply_state(WorkloadState state,
                     uint32_t balanced_min,
                     uint32_t balanced_max);
    bool apply_budget(WorkloadState state, uint32_t balanced_min,
                      uint32_t balanced_max, uint32_t capacity);


    [[nodiscard]] uint32_t get_top_app_min() const;
    [[nodiscard]] uint32_t get_top_app_max() const;
    [[nodiscard]] bool owned_values_intact();

private:
    enum class Encoding : uint8_t { Unknown, Percentage, Capacity1024 };
    enum class MountKind : uint8_t { Auto, CgroupV1, CgroupV2 };

    std::string target_path_;
    Encoding encoding_{Encoding::Unknown};
    bool supported_{false};
    bool has_written_{false};
    uint32_t oem_max_{1024};
    uint32_t last_min_{0};
    uint32_t last_max_{1024};

    void discover(std::string_view explicit_root);
    bool validate_target(const std::string& directory, MountKind kind, Encoding& encoding) const;
    bool write_top_app(uint32_t min_val, uint32_t max_val);
    [[nodiscard]] std::string encode(uint32_t capacity, bool maximum) const;
    [[nodiscard]] uint32_t read_uclamp_node(const std::string& path) const;
};

} // namespace coreflow
