/**
 * @file kernel_node_inventory.hpp
 * @brief Bounded dynamic inventory of kernel parent policies and child tunables.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/device_profile.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

class KernelNodeInventory final {
public:
    explicit KernelNodeInventory(std::string sys_root = "/sys",
                                 std::string proc_root = "/proc");

    // One bounded scan per profile generation. Returned nodes are evidence
    // records only; unknown ABI contracts remain observation-only.
    [[nodiscard]] std::vector<NodeCapability> scan(std::size_t maximum_nodes = 512U) const;

private:
    std::string sys_root_;
    std::string proc_root_;
};

// Shared semantic pre-filter used by the tuner and by tests.  It deliberately
// does not infer authority from writability: the capability must already carry
// a recognized runtime contract, verified constraints, and a verified OEM
// baseline (NodeCapabilityState::MutationEligible).
class CandidateSelector final {
public:
    [[nodiscard]] static bool is_mutation_candidate(const NodeCapability& node) noexcept {
        return node.classification == KernelNodeClassification::RuntimeTunable &&
               node.mutation_eligible();
    }

    [[nodiscard]] static const NodeCapability* find_runtime_node(
        const std::vector<NodeCapability>& nodes, std::string_view path) noexcept {
        const auto found = std::find_if(nodes.begin(), nodes.end(), [&](const NodeCapability& node) {
            return node.path == path && is_mutation_candidate(node);
        });
        return found == nodes.end() ? nullptr : &*found;
    }

    [[nodiscard]] static bool has_runtime_path(
        const std::vector<NodeCapability>& nodes, std::string_view path) noexcept {
        return find_runtime_node(nodes, path) != nullptr;
    }

    [[nodiscard]] static bool is_physical_block_device(std::string_view name) noexcept {
        const bool candidate = name.rfind("mmcblk", 0) == 0 ||
                               name.rfind("sd", 0) == 0 ||
                               name.rfind("nvme", 0) == 0;
        return candidate && name.rfind("md", 0) != 0 && name.rfind("loop", 0) != 0 &&
               name.rfind("ram", 0) != 0 && name.rfind("zram", 0) != 0 &&
               name.rfind("dm-", 0) != 0 && name.rfind("nbd", 0) != 0 &&
               name.rfind("ublkb", 0) != 0 && name.rfind("zd", 0) != 0;
    }
};

} // namespace coreflow
