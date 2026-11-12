/**
 * @file mount_topology.hpp
 * @brief Runtime mount and cgroup ABI discovery shared by kernel consumers.
 * @license Apache-2.0
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

enum class CgroupFilesystem : uint8_t { V1, V2 };
enum class CgroupMembershipInterface : uint8_t { None, Threads, Tasks, Processes };
enum class UclampEncoding : uint8_t { Unknown, Percent, Capacity1024 };
enum class UclampResolutionStatus : uint8_t {
    NotEvaluated = 0,
    NoCpuCgroupCandidate,
    AmbiguousCpuCandidate,
    UclampAbiUnverified,
    BaselineCaptureFailed,
    WriteOpenDenied,
    SelinuxAccessDenied,
    AccessVerified,
    MutationEligible,
};

struct CgroupMount final {
    uint64_t mount_id{0};
    uint64_t parent_id{0};
    CgroupFilesystem filesystem{CgroupFilesystem::V1};
    std::string root{ "/" };
    std::string mount_point;
    std::vector<std::string> controllers;
    bool read_only{false};
};

struct CgroupGroupCapability final {
    CgroupMount mount;
    std::string relative_path;
    std::string absolute_path;
    std::string uclamp_min_path;
    std::string uclamp_max_path;
    std::string uclamp_latency_sensitive_path;
    std::string io_weight_path;
    bool has_uclamp_min{false};
    bool has_uclamp_max{false};
    bool has_uclamp_latency_sensitive{false};
    bool has_io_weight{false};
    CgroupMembershipInterface membership{CgroupMembershipInterface::None};
    bool has_process_membership{false};
    bool has_process_list{false};
    uint32_t verified_user_processes{0};
    bool uclamp_write_open_verified{false};
    int uclamp_write_open_errno{0};
};

struct ProcessCgroupMembership final {
    CgroupMount mount;
    std::string cgroup_path;
    std::string relative_path;
    std::string absolute_path;
};

struct ParsedUclampValue final {
    uint32_t capacity{0};
    UclampEncoding encoding{UclampEncoding::Unknown};
    bool is_max{false};
};

class MountTopologyResolver final {
public:
    explicit MountTopologyResolver(std::string proc_root = "/proc")
        : proc_root_(std::move(proc_root)) {}

    [[nodiscard]] const std::string& proc_root() const noexcept { return proc_root_; }
    [[nodiscard]] std::vector<CgroupMount> discover_cgroup_mounts() const;
    [[nodiscard]] std::vector<CgroupGroupCapability> discover_cgroup_groups(
        std::size_t max_directories = 2048U, unsigned max_depth = 8U,
        bool cpu_only = false) const;
    [[nodiscard]] std::vector<ProcessCgroupMembership> process_memberships(
        int pid, std::size_t max_bytes = 16384U) const;
    [[nodiscard]] std::optional<ProcessCgroupMembership> resolve_process_membership(
        int pid, const CgroupMount& mount) const;
    [[nodiscard]] std::optional<CgroupGroupCapability> resolve_foreground_group() const;
    // CPU-only process targeting. cpuset and unrelated controller mounts are
    // excluded before candidate ranking, and OOM score is not a membership gate.
    [[nodiscard]] std::optional<CgroupGroupCapability> resolve_foreground_cpu_group() const;
    [[nodiscard]] std::optional<CgroupGroupCapability> resolve_foreground_uclamp_group(
        UclampResolutionStatus* status = nullptr) const;
    [[nodiscard]] static const char* uclamp_resolution_status_name(
        UclampResolutionStatus status) noexcept;
    [[nodiscard]] std::optional<CgroupGroupCapability> resolve_foreground_io_group() const;

    [[nodiscard]] static std::optional<ParsedUclampValue> parse_uclamp_value(
        std::string_view raw) noexcept;
    [[nodiscard]] static std::string encode_uclamp_value(
        uint32_t capacity, UclampEncoding encoding, bool maximum);
    [[nodiscard]] static const char* membership_name(CgroupMembershipInterface value) noexcept;

private:
    std::string proc_root_;
};

} // namespace coreflow
