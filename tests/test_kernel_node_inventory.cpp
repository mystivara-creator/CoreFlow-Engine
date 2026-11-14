#include "coreflow/kernel_node_inventory.hpp"
#include "coreflow/baseline_store.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

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
    const fs::path state = root / "state";

    assert(::setenv("COREFLOW_TEST_STATE_DIR", state.c_str(), 1) == 0);
    auto& baseline = BaselineStore::instance();
    assert(baseline.recover_pending());

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

    const auto physical_queue = sys / "class/block/mmcblk0/queue";
    put(physical_queue / "scheduler", "none [mq-deadline] kyber\n");
    put(physical_queue / "read_ahead_kb", "128\n");
    put(physical_queue / "nr_requests", "128\n");
    put(physical_queue / "rq_affinity", "1\n");
    put(physical_queue / "max_segment_size", "65536\n");
    put(physical_queue / "virt_boundary_mask", "4095\n");
    put(physical_queue / "zone_append_max_bytes", "0\n");
    put(physical_queue / "stable_writes", "0\n");
    put(physical_queue / "write_cache", "write back\n");

    // Virtual block surfaces remain in the inventory for fingerprinting, but
    // can never be promoted to mutation authority by writability/baseline alone.
    const auto loop_queue = sys / "class/block/loop0/queue";
    put(loop_queue / "scheduler", "none [mq-deadline]\n");
    put(loop_queue / "read_ahead_kb", "128\n");
    const auto dm_queue = sys / "class/block/dm-0/queue";
    put(dm_queue / "scheduler", "none [mq-deadline]\n");
    put(dm_queue / "read_ahead_kb", "128\n");

    for (const auto& path : {
            physical_queue / "scheduler", physical_queue / "read_ahead_kb",
            physical_queue / "nr_requests", physical_queue / "rq_affinity"}) {
        assert(baseline.capture(path.string()));
    }

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
    assert(queue_child->classification == KernelNodeClassification::ObservationOnly);
    assert(!CandidateSelector::is_mutation_candidate(*queue_child));
    const auto* scheduler_parent = find(nodes, queue / "scheduler");
    assert(scheduler_parent != nullptr);
    assert(scheduler_parent->semantic_id == "block.queue.scheduler_policy");
    assert(scheduler_parent->accepted_values.find("mq-deadline") != std::string::npos);
    assert(scheduler_parent->classification == KernelNodeClassification::ObservationOnly);

    const auto* physical_scheduler = find(nodes, physical_queue / "scheduler");
    const auto* physical_read_ahead = find(nodes, physical_queue / "read_ahead_kb");
    const auto* physical_requests = find(nodes, physical_queue / "nr_requests");
    const auto* physical_affinity = find(nodes, physical_queue / "rq_affinity");
    assert(physical_scheduler && physical_read_ahead && physical_requests && physical_affinity);
    assert(physical_scheduler->classification == KernelNodeClassification::RuntimeTunable);
    assert(physical_read_ahead->classification == KernelNodeClassification::RuntimeTunable);
    assert(physical_requests->classification == KernelNodeClassification::RuntimeTunable);
    assert(physical_affinity->classification == KernelNodeClassification::RuntimeTunable);
    assert(CandidateSelector::is_mutation_candidate(*physical_scheduler));
    assert(CandidateSelector::is_mutation_candidate(*physical_read_ahead));
    assert(CandidateSelector::has_runtime_path(nodes, physical_requests->path));

    const auto* max_segment = find(nodes, physical_queue / "max_segment_size");
    const auto* boundary_mask = find(nodes, physical_queue / "virt_boundary_mask");
    const auto* zone_append = find(nodes, physical_queue / "zone_append_max_bytes");
    const auto* stable_writes = find(nodes, physical_queue / "stable_writes");
    const auto* write_cache = find(nodes, physical_queue / "write_cache");
    assert(max_segment && max_segment->classification == KernelNodeClassification::ObservationOnly);
    assert(boundary_mask && boundary_mask->classification == KernelNodeClassification::ObservationOnly);
    assert(zone_append && zone_append->classification == KernelNodeClassification::ObservationOnly);
    assert(stable_writes && stable_writes->classification == KernelNodeClassification::TestOnly);
    assert(write_cache && write_cache->classification == KernelNodeClassification::Protected);
    assert(!CandidateSelector::is_mutation_candidate(*max_segment));
    assert(!CandidateSelector::is_mutation_candidate(*stable_writes));
    assert(!CandidateSelector::is_mutation_candidate(*write_cache));

    for (const auto& virtual_device : {loop_queue, dm_queue}) {
        const auto* virtual_read_ahead = find(nodes, virtual_device / "read_ahead_kb");
        const auto* virtual_scheduler = find(nodes, virtual_device / "scheduler");
        assert(virtual_read_ahead && virtual_scheduler);
        assert(virtual_read_ahead->classification == KernelNodeClassification::ObservationOnly);
        assert(virtual_scheduler->classification == KernelNodeClassification::ObservationOnly);
        assert(!CandidateSelector::is_mutation_candidate(*virtual_read_ahead));
        assert(!CandidateSelector::is_mutation_candidate(*virtual_scheduler));
    }

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
    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    std::cout << "KernelNodeInventory dynamic parent/child scan PASS\n";
    return 0;
}
