/**
 * @file baseline_store.hpp
 * @brief Transactional Baseline Store and Automatic Rollback Engine
 * @author mystivara-creator
 * @license MIT
 * 
 * Records pre-mutation values and attempts verified per-node restoration.
 * Same-boot process crashes are recovered on next start using a durable journal;
 * recovery is best-effort and not an atomic whole-kernel transaction.
 */

#pragma once

#include "types.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace coreflow {

struct NodeSnapshot {
    std::string path;
    std::string factory_value;
    std::string active_value;
    bool is_modified{false};
    std::string prior_value;
    uint64_t dev{0};
    uint64_t ino{0};
};

struct RetainedPerformanceBaseline final {
    std::string transaction_id;
    std::string endpoint;
    std::string verified_value;
};

class BaselineStore {
public:
    static BaselineStore& instance();

    /// Snapshot a sysfs / proc node before modifying it
    bool capture(const std::string& path);

    /// Record a planned mutation; returns false if node wasn't snapshotted or write failed
    bool apply_mutation(const std::string& path, const std::string& new_value);

    /// Persist a verified performance operating point independently of OEM recovery data.
    /// Idempotent for an identical transaction ID and payload.
    [[nodiscard]] bool retain_performance_baseline(
        const std::string& transaction_id, const std::string& endpoint,
        const std::string& verified_value);
    [[nodiscard]] std::vector<RetainedPerformanceBaseline> performance_baselines() const;
    /// True only when a persisted retained-performance record and the live inode/value
    /// prove that the current non-OEM value is still owned by CoreFlow.
    [[nodiscard]] bool owns_retained_performance_value(
        const std::string& endpoint, const std::string& verified_value) const;

    /// Roll back a specific node to factory value
    bool rollback(const std::string& path);

    /// Recover unfinished mutations from the previous daemon process (same boot).
    [[nodiscard]] bool recover_pending();

    /// Best-effort verified rollback of all modified nodes; never claim atomicity.
    size_t rollback_all();

    /// Query if any system node currently differs from factory baseline
    [[nodiscard]] bool has_active_mutations() const;

    /// Verify no node remains modified and atomically persist an empty journal.
    /// Used before recovering detached controller leases.
    [[nodiscard]] bool verify_and_persist_clear_state();

    /// Check whether a captured node still matches its expected value and inode/device identity
    [[nodiscard]] bool owns_expected_state(const std::string& path, const std::string& expected_value) const;
    [[nodiscard]] bool reassert_mutation_from_oem_baseline(
        const std::string& path, const std::string& expected_value);

    /// Returns true if neither kernel boot_id nor per-boot boot_epoch could be validated
    [[nodiscard]] bool identity_unavailable() const noexcept;

    /// Returns the active validated boot epoch/ID (empty if unavailable)
    [[nodiscard]] std::string boot_id() const;

    /// Re-initialize paths and boot identity (used by deterministic test harnesses)
    void reinitialize();

    /// Retrieve audit list of all tracked nodes
    [[nodiscard]] std::vector<NodeSnapshot> get_audit_trail() const;

private:
    BaselineStore();
    ~BaselineStore() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, NodeSnapshot> store_;
    std::unordered_map<std::string, RetainedPerformanceBaseline> performance_store_;
    std::string state_dir_;
    std::string journal_path_;
    std::string boot_id_;
    bool identity_unavailable_{false};
    bool recovery_checked_{false};

    void init_paths_and_boot_id_locked();
    bool persist_locked();
    bool persist_performance_locked();
    void load_performance_locked();
    // Transaction/runtime rollback for one path; rollback_locked remains reserved
    // for crash recovery and an explicit global safety sweep.
    bool rollback_path_locked(const std::string& path);
    size_t rollback_locked();

    std::string read_raw_node(const std::string& path, uint64_t* out_dev = nullptr, uint64_t* out_ino = nullptr) const;
    bool write_raw_node(const std::string& path, const std::string& value, uint64_t expected_dev = 0, uint64_t expected_ino = 0);
};

} // namespace coreflow
