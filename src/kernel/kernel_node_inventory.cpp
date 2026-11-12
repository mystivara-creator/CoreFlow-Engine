#include "coreflow/kernel_node_inventory.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
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
            append_unique(nodes, observe(path, HardwareDomain::Cpu, ProfileSource::Sysfs,
                "cpufreq.governor_tunable.unverified", policy.filename().string() + ":governor",
                parent.string(), governor, true), maximum_nodes);
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
        const std::string group = "block:" + block.filename().string();
        if (!scheduler_raw.empty()) {
            NodeCapability parent_node = observe(scheduler, HardwareDomain::Storage, ProfileSource::Sysfs,
                "block.queue.scheduler_policy", group + ":scheduler", {}, {}, false);
            parent_node.current_value = active_scheduler;
            parent_node.accepted_values = scheduler_raw;
            parent_node.state = NodeCapabilityState::SemanticsVerified;
            parent_node.unit = "scheduler_token";
            parent_node.rejection_reason = "OEM scheduler policy observed; child ABI is tied to the active token";
            append_unique(nodes, std::move(parent_node), maximum_nodes);
        }
        for (const auto& path : regular_children(queue)) {
            if (path == scheduler) continue;
            append_unique(nodes, observe(path, HardwareDomain::Storage, ProfileSource::Sysfs,
                "block.queue.tunable.unverified", group + ":queue", scheduler.string(),
                active_scheduler, false), maximum_nodes);
        }
        if (!active_scheduler.empty()) {
            const fs::path iosched = queue / "iosched";
            for (const auto& path : regular_children(iosched)) {
                append_unique(nodes, observe(path, HardwareDomain::Storage, ProfileSource::Sysfs,
                    "block.iosched.tunable.unverified", group + ":iosched", scheduler.string(),
                    active_scheduler, true), maximum_nodes);
            }
        }
    }

    // ZRAM exposed controls and VM tunables are inventory evidence only. Active
    // swap state is captured by DeviceProfiler; unsafe busy-state nodes are not
    // promoted by this generic scanner.
    const fs::path zram_root = sys / "block";
    for (const auto& zram : directories(zram_root, "zram")) {
        if (nodes.size() >= maximum_nodes) break;
        for (const auto& path : regular_children(zram)) {
            append_unique(nodes, observe(path, HardwareDomain::Memory, ProfileSource::Sysfs,
                "zram.tunable.unverified", "zram:" + zram.filename().string(), {}, {}, false), maximum_nodes);
        }
    }
    const fs::path vm_root = proc / "sys/vm";
    for (const auto& path : regular_children(vm_root)) {
        append_unique(nodes, observe(path, HardwareDomain::Memory, ProfileSource::Procfs,
            "vm.tunable.unverified", "vm", {}, {}, false), maximum_nodes);
    }
    return nodes;
}

} // namespace coreflow
