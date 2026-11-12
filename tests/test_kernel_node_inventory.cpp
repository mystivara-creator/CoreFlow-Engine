#include "coreflow/kernel_node_inventory.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace coreflow;

static void put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << value;
    assert(stream.good());
}

static const NodeCapability* find(const std::vector<NodeCapability>& nodes,
                                  const fs::path& path) {
    const auto it = std::find_if(nodes.begin(), nodes.end(), [&](const auto& node) {
        return node.path == path.string();
    });
    return it == nodes.end() ? nullptr : &*it;
}

int main() {
    const auto root = fs::temp_directory_path() / "coreflow-kernel-node-inventory-test";
    fs::remove_all(root);
    const fs::path sys = root / "sys";
    const fs::path proc = root / "proc";

    const auto governor = sys / "devices/system/cpu/cpufreq/policy0/schedutil";
    put(sys / "devices/system/cpu/cpufreq/policy0/scaling_governor", "schedutil\n");
    put(sys / "devices/system/cpu/cpufreq/policy0/scaling_available_governors",
        "schedutil walt performance powersave\n");
    put(governor / "up_rate_limit_us", "500\n");
    put(governor / "down_rate_limit_us", "10000\n");
    put(governor / "vendor_adaptive_level", "2\n");

    const auto queue = sys / "class/block/virtual0/queue";
    put(queue / "scheduler", "none [mq-deadline] kyber\n");
    put(queue / "iostats", "1\n");
    put(queue / "read_ahead_kb", "128\n");
    put(queue / "nr_requests", "128\n");
    put(queue / "iosched/read_expire", "500\n");
    put(queue / "iosched/writes_starved", "2\n");

    put(sys / "block/zram0/disksize", "1073741824\n");
    put(sys / "block/zram0/comp_algorithm", "[lz4] lzo lzo-rle zstd\n");
    put(proc / "sys/vm/swappiness", "100\n");

    KernelNodeInventory inventory(sys.string(), proc.string());
    const auto nodes = inventory.scan(64U);
    assert(!nodes.empty());

    const auto* up = find(nodes, governor / "up_rate_limit_us");
    assert(up != nullptr);
    assert(up->parent_policy_path == (sys / "devices/system/cpu/cpufreq/policy0/scaling_governor").string());
    assert(up->parent_policy_value == "schedutil");
    assert(up->rediscover_after_parent_change);
    assert(up->state == NodeCapabilityState::Readable);
    assert(!up->mutation_eligible());

    const auto* queue_child = find(nodes, queue / "read_ahead_kb");
    assert(queue_child != nullptr);
    assert(queue_child->parent_policy_path == (queue / "scheduler").string());
    assert(queue_child->parent_policy_value == "mq-deadline");
    assert(queue_child->dependency_group.find("block:virtual0") == 0U);
    const auto* scheduler_parent = find(nodes, queue / "scheduler");
    assert(scheduler_parent != nullptr);
    assert(scheduler_parent->semantic_id == "block.queue.scheduler_policy");
    assert(scheduler_parent->accepted_values.find("mq-deadline") != std::string::npos);

    const auto* governor_parent = find(nodes, sys / "devices/system/cpu/cpufreq/policy0/scaling_governor");
    assert(governor_parent != nullptr);
    assert(governor_parent->accepted_values.find("walt") != std::string::npos);

    const auto* iosched_child = find(nodes, queue / "iosched/read_expire");
    assert(iosched_child != nullptr);
    assert(iosched_child->rediscover_after_parent_change);

    const auto* zram = find(nodes, sys / "block/zram0/comp_algorithm");
    assert(zram != nullptr);
    const auto* vm = find(nodes, proc / "sys/vm/swappiness");
    assert(vm != nullptr && vm->source == ProfileSource::Procfs);

    // Parent policy changes invalidate children. The next bounded scan observes
    // the newly active governor and scheduler child ABI without stale paths.
    const auto alternate_governor = sys / "devices/system/cpu/cpufreq/policy0/vendor_governor";
    put(sys / "devices/system/cpu/cpufreq/policy0/scaling_governor", "vendor_governor\n");
    put(alternate_governor / "vendor_adaptive_level", "3\n");
    put(queue / "scheduler", "none [kyber]\n");
    fs::remove(queue / "iosched/read_expire");
    fs::remove(queue / "iosched/writes_starved");
    put(queue / "iosched/read_lat_nsec", "1000000\n");
    const auto refreshed = inventory.scan(64U);
    assert(find(refreshed, alternate_governor / "vendor_adaptive_level") != nullptr);
    assert(find(refreshed, queue / "iosched/read_lat_nsec") != nullptr);
    assert(find(refreshed, queue / "iosched/read_expire") == nullptr);
    assert(find(refreshed, queue / "scheduler")->current_value == "kyber");

    const auto bounded = inventory.scan(2U);
    assert(bounded.size() <= 2U);

    fs::remove_all(root);
    std::cout << "KernelNodeInventory dynamic parent/child scan PASS\n";
    return 0;
}
