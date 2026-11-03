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
};

class BaselineStore {
public:
    static BaselineStore& instance();

    /// Snapshot a sysfs / proc node before modifying it
    bool capture(const std::string& path);

    /// Record a planned mutation; returns false if node wasn't snapshotted
    bool apply_mutation(const std::string& path, const std::string& new_value);

    /// Roll back a specific node to factory value
    bool rollback(const std::string& path);

    /// Recover unfinished mutations from the previous daemon process (same boot).
    [[nodiscard]] bool recover_pending();

    /// Best-effort verified rollback of all modified nodes; never claim atomicity.
    /// Best-effort rollback of tracked modifications to original observed values
    size_t rollback_all();

    /// Query if any system node currently differs from factory baseline
    [[nodiscard]] bool has_active_mutations() const;

    /// Retrieve audit list of all tracked nodes
    [[nodiscard]] std::vector<NodeSnapshot> get_audit_trail() const;

private:
    BaselineStore();
    ~BaselineStore() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, NodeSnapshot> store_;
    std::string journal_path_;
    std::string boot_id_;
    bool recovery_checked_{false};
    bool persist_locked();
    size_t rollback_locked();

    std::string read_raw_node(const std::string& path) const;
    bool write_raw_node(const std::string& path, const std::string& value);
};

// RAII Scope Guard for temporary performance boosts
class ScopedBaselineLock {
public:
    explicit ScopedBaselineLock(std::vector<std::pair<std::string, std::string>> mutations);
    ~ScopedBaselineLock();

    ScopedBaselineLock(const ScopedBaselineLock&) = delete;
    ScopedBaselineLock& operator=(const ScopedBaselineLock&) = delete;

private:
    std::vector<std::string> modified_paths_;
};

} // namespace coreflow
