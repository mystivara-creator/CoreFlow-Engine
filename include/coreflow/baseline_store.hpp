/**
 * @file baseline_store.hpp
 * @brief Transactional Baseline Store and Automatic Rollback Engine
 * @author mystivara-creator
 * @license Apache-2.0
 * 
 * Solves the critical issue where third-party tweaks permanently pollute factory
 * settings. CoreFlow captures OEM values on startup and guarantees safe restoration
 * upon shutdown, crash, or thermal emergency.
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

    /// Atomic rollback of all modified nodes to factory baseline
    size_t rollback_all();

    /// Query if any system node currently differs from factory baseline
    [[nodiscard]] bool has_active_mutations() const;

    /// Retrieve audit list of all tracked nodes
    [[nodiscard]] std::vector<NodeSnapshot> get_audit_trail() const;

private:
    BaselineStore() = default;
    ~BaselineStore() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, NodeSnapshot> store_;

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
