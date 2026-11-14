#include "coreflow/kernel_node_inventory.hpp"
#include "coreflow/baseline_store.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <filesystem>
#include <fcntl.h>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;
namespace {
constexpr std::size_t kMaxEntriesPerDirectory = 192U;
constexpr std::size_t kMaxValueBytes = 4096U;

std::string read_value(const fs::path& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    std::string value;
    std::array<char, 512> buffer{};
    for (;;) {
        const ssize_t count = ::read(fd, buffer.data(), buffer.size());
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        const auto amount = static_cast<std::size_t>(count);
        if (value.size() > kMaxValueBytes || amount > kMaxValueBytes - value.size()) {
            value.clear();
            break;
        }
        value.append(buffer.data(), amount);
    }
    (void)::close(fd);
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

std::vector<fs::path> regular_children(const fs::path& directory,
                                       std::size_t maximum = kMaxEntriesPerDirectory) {
    std::vector<fs::path> result;
    std::error_code ec;
    fs::directory_iterator it(directory, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && result.size() < maximum; it.increment(ec)) {
        std::error_code status_error;
        const auto status = it->symlink_status(status_error);
        if (status_error || fs::is_symlink(status) || !fs::is_regular_file(status)) continue;
        result.push_back(it->path());
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<fs::path> directories(const fs::path& directory,
                                  std::string_view prefix = {}) {
    std::vector<fs::path> result;
    std::error_code ec;
    fs::directory_iterator it(directory, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && result.size() < kMaxEntriesPerDirectory; it.increment(ec)) {
        const auto name = it->path().filename().string();
        if (!prefix.empty() && name.rfind(prefix, 0) != 0) continue;
        std::error_code type_error;
        if (fs::is_directory(it->path(), type_error) && !type_error) result.push_back(it->path());
    }
    std::sort(result.begin(), result.end());
    return result;
}

NodeCapability observe(const fs::path& path, HardwareDomain domain,
                       ProfileSource source, std::string semantic,
                       std::string dependency, std::string parent_path,
                       std::string parent_value, bool rediscover) {
    NodeCapability node{};
    node.path = path.string();
    node.domain = domain;
    node.source = source;
    node.semantic_id = std::move(semantic);
    node.dependency_group = std::move(dependency);
    node.parent_policy_path = std::move(parent_path);
    node.parent_policy_value = std::move(parent_value);
    node.rediscover_after_parent_change = rediscover;
    node.unit.clear();
    node.state = NodeCapabilityState::Discovered;
    node.rejection_code = NodeRejectionReason::SemanticsUnverified;
    node.rejection_reason = "Dynamically enumerated ABI node; constraints and write contract not verified";
    node.evidence = "Bounded directory traversal; parent policy relationship observed, no mutation attempted";
    node.current_value = read_value(path);
    if (!node.current_value.empty()) node.state = NodeCapabilityState::Readable;
    return node;
}

void append_unique(std::vector<NodeCapability>& nodes, NodeCapability node,
                   std::size_t maximum) {
    if (nodes.size() >= maximum || node.path.empty()) return;
    const auto found = std::find_if(nodes.begin(), nodes.end(), [&](const auto& existing) {
        return existing.path == node.path;
    });
    if (found == nodes.end()) nodes.push_back(std::move(node));
}

std::string active_token(std::string value) {
    const auto open = value.find('[');
    const auto close = open == std::string::npos ? std::string::npos : value.find(']', open + 1U);
    if (open != std::string::npos && close != std::string::npos) return value.substr(open + 1U, close - open - 1U);
    return value;
}

bool parse_u64(std::string_view text, uint64_t& value) noexcept {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0)
        text.remove_prefix(1U);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0)
        text.remove_suffix(1U);
    if (text.empty()) return false;
    uint64_t parsed = 0U;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed, 10);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    value = parsed;
    return true;
}

bool writable_attribute(const fs::path& path) noexcept {
    struct stat st{};
    if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))
        return false;
    // access(W_OK) alone is insufficient on root test runners because root can
    // bypass ordinary DAC mode bits. Never infer a writable ABI from that.
    return (st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0 &&
           ::access(path.c_str(), W_OK) == 0;
}

KernelNodeClassification classification_for_leaf(std::string_view leaf) noexcept {
    if (leaf == "stable_writes") return KernelNodeClassification::TestOnly;
    if (leaf == "write_cache") return KernelNodeClassification::Protected;
    return KernelNodeClassification::ObservationOnly;
}

bool baseline_verified(const std::string& path, const std::string& value) {
    if (path.empty() || value.empty()) return false;
    auto& baseline = BaselineStore::instance();
    const auto audit = baseline.get_audit_trail();
    const auto found = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == path && !node.is_modified && node.factory_value == value;
    });
    return found != audit.end() || baseline.owns_retained_performance_value(path, value);
}

void promote_runtime_tunable(NodeCapability& node, std::string unit,
                             std::string constraints, uint64_t minimum,
                             uint64_t maximum) {
    node.unit = std::move(unit);
    node.constraints = std::move(constraints);
    node.accepted_values = node.constraints;
    uint64_t value = 0U;
    if (!parse_u64(node.current_value, value)) {
        node.rejection_code = NodeRejectionReason::FormatMismatch;
        node.rejection_reason = "Known scalar ABI has a non-numeric current value";
        return;
    }
    if (value < minimum || value > maximum) {
        node.rejection_code = NodeRejectionReason::ConstraintsUnverified;
        node.rejection_reason = "Current scalar lies outside the documented runtime contract";
        return;
    }
    if (!writable_attribute(node.path)) {
        node.rejection_code = NodeRejectionReason::PermissionDenied;
        node.rejection_reason = "Known scalar ABI is readable but write access is not established";
        return;
    }
    if (!baseline_verified(node.path, node.current_value)) {
        node.rejection_code = NodeRejectionReason::BaselineNotCaptured;
        node.rejection_reason = "Known scalar ABI has no verified immutable OEM baseline";
        return;
    }
    node.classification = KernelNodeClassification::RuntimeTunable;
    node.state = NodeCapabilityState::MutationEligible;
    node.rejection_code = NodeRejectionReason::None;
    node.rejection_reason.clear();
}

std::string observation_semantic_id(std::string_view leaf) {
    if (leaf == "max_segment_size") return "block.queue.hardware.max_segment_size";
    if (leaf == "virt_boundary_mask") return "block.queue.hardware.virt_boundary_mask";
    if (leaf == "zone_append_max_bytes") return "block.queue.hardware.zone_append_max_bytes";
    if (leaf == "stable_writes") return "block.queue.test.stable_writes";
    if (leaf == "write_cache") return "block.queue.protected.write_cache";
    return "block.queue.attribute.observation_only";
}

bool is_virtual_block_device(std::string_view name) noexcept {
    return name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
           name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0 ||
           name.rfind("md", 0) == 0 || name.rfind("nbd", 0) == 0 ||
           name.rfind("ublkb", 0) == 0 || name.rfind("zd", 0) == 0;
}
} // namespace

KernelNodeInventory::KernelNodeInventory(std::string sys_root, std::string proc_root)
    : sys_root_(std::move(sys_root)), proc_root_(std::move(proc_root)) {
    if (sys_root_.empty()) sys_root_ = "/sys";
    if (proc_root_.empty()) proc_root_ = "/proc";
}

std::vector<NodeCapability> KernelNodeInventory::scan(std::size_t maximum_nodes) const {
    std::vector<NodeCapability> nodes;
    if (maximum_nodes == 0U) return nodes;
    const fs::path sys(sys_root_);
    const fs::path proc(proc_root_);

    // CPUFreq: enumerate the active governor's child ABI dynamically. A parent
    // governor switch invalidates every child path and forces fresh discovery.
    const fs::path cpufreq = sys / "devices/system/cpu/cpufreq";
    for (const auto& policy : directories(cpufreq, "policy")) {
        if (nodes.size() >= maximum_nodes) break;
        const fs::path parent = policy / "scaling_governor";
        const std::string governor = read_value(parent);
        if (governor.empty()) continue;
        NodeCapability parent_node = observe(parent, HardwareDomain::Cpu, ProfileSource::Sysfs,
            "cpufreq.governor_policy", policy.filename().string() + ":governor",
            {}, {}, false);
        parent_node.accepted_values = read_value(policy / "scaling_available_governors");
        parent_node.state = NodeCapabilityState::SemanticsVerified;
        parent_node.unit = "governor_token";
        parent_node.rejection_reason = "Parent policy observed; child ABI must be rediscovered after a policy change";
        append_unique(nodes, std::move(parent_node), maximum_nodes);
        const fs::path governor_dir = policy / governor;
        for (const auto& path : regular_children(governor_dir)) {
            auto child = observe(path, HardwareDomain::Cpu, ProfileSource::Sysfs,
                "cpufreq.governor_tunable.unverified", policy.filename().string() + ":governor",
                parent.string(), governor, true);
            child.classification = classification_for_leaf(path.filename().string());
            append_unique(nodes, std::move(child), maximum_nodes);
        }
    }

    // Block queue and scheduler-specific child tunables are read from the
    // currently active scheduler directory; switching it requires re-discovery.
    const fs::path block_root = sys / "class/block";
    for (const auto& block : directories(block_root)) {
        if (nodes.size() >= maximum_nodes) break;
        const fs::path queue = block / "queue";
        const fs::path scheduler = queue / "scheduler";
        const std::string scheduler_raw = read_value(scheduler);
        const std::string active_scheduler = active_token(scheduler_raw);
        const std::string device_name = block.filename().string();
        const bool physical = CandidateSelector::is_physical_block_device(device_name) &&
                              !is_virtual_block_device(device_name);
        const std::string group = "block:" + device_name;
        if (!scheduler_raw.empty()) {
            NodeCapability parent_node = observe(scheduler, HardwareDomain::Storage, ProfileSource::Sysfs,
                "block.queue.scheduler_policy", group + ":scheduler", {}, {}, false);
            parent_node.current_value = active_scheduler;
            parent_node.accepted_values = scheduler_raw;
            parent_node.state = NodeCapabilityState::SemanticsVerified;
            parent_node.unit = "scheduler_token";
            parent_node.rejection_reason = "OEM scheduler policy observed; child ABI is tied to the active token";
            if (physical && !active_scheduler.empty() && writable_attribute(scheduler) &&
                baseline_verified(parent_node.path, active_scheduler)) {
                parent_node.constraints = "current token must be one of the kernel-advertised scheduler tokens";
                parent_node.classification = KernelNodeClassification::RuntimeTunable;
                parent_node.state = NodeCapabilityState::MutationEligible;
                parent_node.rejection_code = NodeRejectionReason::None;
                parent_node.rejection_reason.clear();
            }
            append_unique(nodes, std::move(parent_node), maximum_nodes);
        }
        for (const auto& path : regular_children(queue)) {
            if (path == scheduler) continue;
            const std::string leaf = path.filename().string();
            auto child = observe(path, HardwareDomain::Storage, ProfileSource::Sysfs,
                observation_semantic_id(leaf), group + ":queue", scheduler.string(),
                active_scheduler, false);
            child.classification = classification_for_leaf(leaf);
            if (physical && leaf == "read_ahead_kb") {
                promote_runtime_tunable(child, "KiB", "32<=value<=512", 32U, 512U);
            } else if (physical && leaf == "nr_requests") {
                promote_runtime_tunable(child, "requests", "32<=value<=512", 32U, 512U);
            } else if (physical && leaf == "rq_affinity") {
                promote_runtime_tunable(child, "mode", "0<=value<=2", 0U, 2U);
            } else if (!physical && child.classification == KernelNodeClassification::RuntimeTunable) {
                child.classification = KernelNodeClassification::ObservationOnly;
            }
            append_unique(nodes, std::move(child), maximum_nodes);
        }
        if (!active_scheduler.empty()) {
            const fs::path iosched = queue / "iosched";
            for (const auto& path : regular_children(iosched)) {
                auto child = observe(path, HardwareDomain::Storage, ProfileSource::Sysfs,
                    "block.iosched.tunable.unverified", group + ":iosched", scheduler.string(),
                    active_scheduler, true);
                child.classification = classification_for_leaf(path.filename().string());
                // Scheduler-specific child controls are deliberately kept out
                // of the runtime candidate set until their active-scheduler ABI
                // and range have an explicit contract in DeviceProfiler.
                append_unique(nodes, std::move(child), maximum_nodes);
            }
        }
    }

    // Qualcomm KGSL pwrlevels are a single paired ABI, not independent tunables.
    const fs::path kgsl_root = sys / "class/kgsl";
    for (const auto& kgsl : directories(kgsl_root)) {
        if (nodes.size() + 2U > maximum_nodes) break;
        const fs::path max_path = kgsl / "max_pwrlevel";
        const fs::path min_path = kgsl / "min_pwrlevel";
        const fs::path count_path = kgsl / "num_pwrlevels";
        const fs::path table_path = kgsl / "gpu_available_frequencies";
        const std::string max_text = read_value(max_path);
        const std::string min_text = read_value(min_path);
        const std::string count_text = read_value(count_path);
        const std::string table_text = read_value(table_path);
        uint64_t current_max = 0U, current_min = 0U, reported = 0U;
        const bool scalar_valid = parse_u64(max_text, current_max) &&
            parse_u64(min_text, current_min) && parse_u64(count_text, reported);
        std::istringstream table_stream(table_text);
        std::vector<uint64_t> levels;
        uint64_t frequency = 0U;
        while (table_stream >> frequency) {
            if (frequency == 0U || levels.size() >= 512U) { levels.clear(); break; }
            levels.push_back(frequency);
        }
        const uint64_t last = levels.empty() ? 0U : static_cast<uint64_t>(levels.size() - 1U);
        const bool count_valid = !levels.empty() && reported == levels.size();
        const bool increasing = levels.size() >= 2U &&
            std::adjacent_find(levels.begin(), levels.end(), [](uint64_t a, uint64_t b) { return b <= a; }) == levels.end();
        const bool decreasing = levels.size() >= 2U &&
            std::adjacent_find(levels.begin(), levels.end(), [](uint64_t a, uint64_t b) { return b >= a; }) == levels.end();
        const bool pair_valid = scalar_valid && count_valid && (increasing || decreasing) &&
            current_max <= current_min && current_min <= last &&
            writable_attribute(max_path) && writable_attribute(min_path);
        auto max_node = observe(max_path, HardwareDomain::Gpu, ProfileSource::Sysfs,
            "kgsl.max_pwrlevel", "kgsl:" + kgsl.filename().string() + ":pwrlevel-pair", {}, {}, false);
        auto min_node = observe(min_path, HardwareDomain::Gpu, ProfileSource::Sysfs,
            "kgsl.min_pwrlevel", "kgsl:" + kgsl.filename().string() + ":pwrlevel-pair", {}, {}, false);
        max_node.unit = "level_index"; min_node.unit = "level_index";
        const std::string constraint = "0<=max_pwrlevel<=min_pwrlevel<=" + std::to_string(last) +
            ";num_pwrlevels agrees with gpu_available_frequencies";
        max_node.constraints = constraint; min_node.constraints = constraint;
        max_node.accepted_values = constraint; min_node.accepted_values = constraint;
        max_node.dependency_group = "kgsl:" + kgsl.filename().string() + ":pwrlevel-pair";
        min_node.dependency_group = max_node.dependency_group;
        auto& baseline = BaselineStore::instance();
        const bool baseline_ok = pair_valid && baseline.capture(max_path.string()) &&
            baseline.capture(min_path.string()) && baseline_verified(max_path.string(), max_text) &&
            baseline_verified(min_path.string(), min_text);
        if (baseline_ok) {
            for (NodeCapability* node : {&max_node, &min_node}) {
                node->classification = KernelNodeClassification::RuntimeTunable;
                node->state = NodeCapabilityState::MutationEligible;
                node->rejection_code = NodeRejectionReason::None;
                node->rejection_reason.clear();
                node->evidence = "Paired KGSL ABI and frequency table validated; OEM baseline captured";
            }
        } else {
            for (NodeCapability* node : {&max_node, &min_node}) {
                node->classification = KernelNodeClassification::ObservationOnly;
                node->rejection_code = NodeRejectionReason::ConstraintsUnverified;
                node->rejection_reason = "KGSL pair ABI, table, writability, or OEM baseline did not validate";
            }
        }
        append_unique(nodes, std::move(max_node), maximum_nodes);
        append_unique(nodes, std::move(min_node), maximum_nodes);
    }

    // ZRAM exposed controls and VM tunables are inventory evidence only. Active
    // swap state is captured by DeviceProfiler; unsafe busy-state nodes are not
    // promoted by this generic scanner.
    const fs::path zram_root = sys / "block";
    for (const auto& zram : directories(zram_root, "zram")) {
        if (nodes.size() >= maximum_nodes) break;
        for (const auto& path : regular_children(zram)) {
            auto node = observe(path, HardwareDomain::Memory, ProfileSource::Sysfs,
                "zram.tunable.unverified", "zram:" + zram.filename().string(), {}, {}, false);
            node.classification = classification_for_leaf(path.filename().string());
            append_unique(nodes, std::move(node), maximum_nodes);
        }
    }
    const fs::path vm_root = proc / "sys/vm";
    for (const auto& path : regular_children(vm_root)) {
        const std::string leaf = path.filename().string();
        auto node = observe(path, HardwareDomain::Memory, ProfileSource::Procfs,
            "vm.tunable.unverified", "vm", {}, {}, false);
        if (leaf == "page-cluster") {
            node.semantic_id = "vm.page_cluster";
            promote_runtime_tunable(node, "pages", "0<=value<=8", 0U, 8U);
        } else if (leaf == "vfs_cache_pressure") {
            node.semantic_id = "vm.vfs_cache_pressure";
            promote_runtime_tunable(node, "ratio", "50<=value<=200", 50U, 200U);
        } else if (leaf == "dirty_background_ratio") {
            node.semantic_id = "vm.dirty_background_ratio";
            promote_runtime_tunable(node, "percent", "0<=value<=10", 0U, 10U);
        } else if (leaf == "dirty_ratio") {
            node.semantic_id = "vm.dirty_ratio";
            promote_runtime_tunable(node, "percent", "5<=value<=20", 5U, 20U);
        }
        append_unique(nodes, std::move(node), maximum_nodes);
    }
    return nodes;
}

} // namespace coreflow
