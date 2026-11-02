/**
 * @file test_baseline_rollback.cpp
 * @brief Unit Test: Transactional Baseline Rollback Verification
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/baseline_store.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace coreflow;

int main() {
    std::cout << "[Test] Running CoreFlow BaselineStore Rollback Test..." << std::endl;

    // Setup mock sysfs node in /tmp
    std::string test_node = "/tmp/mock_uclamp_min";
    {
        std::ofstream f(test_node, std::ios::trunc);
        f << "0"; // OEM initial value
    }

    auto& store = BaselineStore::instance();

    // 1. Capture OEM Baseline
    assert(store.capture(test_node));

    // 2. Apply Performance Mutation
    assert(store.apply_mutation(test_node, "512"));
    assert(store.has_active_mutations());

    // Verify node now reads modified value
    {
        std::ifstream f(test_node);
        std::string val;
        f >> val;
        std::cout << "[Test] Node mutated value: '" << val << "' (expected '512')" << std::endl;
        assert(val == "512");
    }

    // 3. Trigger Emergency Rollback
    size_t restored = store.rollback_all();
    assert(restored == 1);
    assert(!store.has_active_mutations());

    // Verify node restored to exact OEM value
    {
        std::ifstream f(test_node);
        std::string val;
        f >> val;
        std::cout << "[Test] Restored node value: '" << val << "' (expected '0')" << std::endl;
        assert(val == "0");
    }

    // Cleanup mock node
    std::filesystem::remove(test_node);

    std::cout << "[Test] PASS: All baseline rollback invariants verified!" << std::endl;
    return 0;
}
