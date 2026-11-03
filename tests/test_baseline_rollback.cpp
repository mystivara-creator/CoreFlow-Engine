/**
 * @file test_baseline_rollback.cpp
 * @brief Unit Test: Transactional Baseline Rollback & Immediate Fail-Safe Verification
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
    std::cout << "========================================================\n"
              << "[Test] Running CoreFlow BaselineStore Rollback & Fail-Safe Test\n"
              << "========================================================" << std::endl;

    std::string test_node_a = "/tmp/mock_uclamp_node_a";
    std::string test_node_b = "/tmp/mock_uclamp_node_b";

    // Setup mock sysfs nodes in /tmp
    {
        std::ofstream fa(test_node_a, std::ios::trunc);
        fa << "0"; // OEM initial value
        std::ofstream fb(test_node_b, std::ios::trunc);
        fb << "100"; // OEM initial value
    }

    auto& store = BaselineStore::instance();

    // ------------------------------------------------------------------------
    // Part 1: Normal Lifecycle Verification (Capture -> Mutation -> Rollback)
    // ------------------------------------------------------------------------
    std::cout << "[Test 1/2] Verifying standard capture, mutation, and rollback..." << std::endl;
    assert(store.capture(test_node_a));
    assert(store.apply_mutation(test_node_a, "512"));
    assert(store.has_active_mutations());

    // Verify node now reads modified value
    {
        std::ifstream f(test_node_a);
        std::string val;
        f >> val;
        std::cout << "  -> Mutated value: '" << val << "' (expected '512')" << std::endl;
        assert(val == "512");
    }

    // Trigger Emergency Rollback
    size_t restored = store.rollback_all();
    assert(restored == 1);
    assert(!store.has_active_mutations());

    // Verify node restored to exact OEM value
    {
        std::ifstream f(test_node_a);
        std::string val;
        f >> val;
        std::cout << "  -> Restored value: '" << val << "' (expected '0')" << std::endl;
        assert(val == "0");
    }

    // ------------------------------------------------------------------------
    // Part 2: Immediate Fail-Safe Fallback Verification
    // If a node write fails, all active mutations must immediately revert to OEM.
    // ------------------------------------------------------------------------
    std::cout << "[Test 2/2] Verifying immediate fail-safe fallback on write error..." << std::endl;
    assert(store.apply_mutation(test_node_a, "700"));
    assert(store.has_active_mutations());

    // Attempt mutation on an invalid unwriteable path
    std::string invalid_node = "/dev/null/impossible_sysfs_path";
    bool failed_res = store.apply_mutation(invalid_node, "999");
    assert(!failed_res); // Write MUST fail

    // Because mutation failed, fail-safe MUST have rolled back test_node_a to OEM!
    assert(!store.has_active_mutations());
    {
        std::ifstream f(test_node_a);
        std::string val;
        f >> val;
        std::cout << "  -> Fail-safe fallback restored value: '" << val << "' (expected '0')" << std::endl;
        assert(val == "0");
    }

    // Cleanup mock nodes
    std::filesystem::remove(test_node_a);
    std::filesystem::remove(test_node_b);

    std::cout << "[Test] PASS: All baseline rollback & fail-safe invariants verified successfully!" << std::endl;
    return 0;
}
