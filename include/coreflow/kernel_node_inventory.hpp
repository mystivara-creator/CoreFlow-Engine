/**
 * @file kernel_node_inventory.hpp
 * @brief Bounded dynamic inventory of kernel parent policies and child tunables.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/device_profile.hpp"
#include <cstddef>
#include <string>
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

} // namespace coreflow
