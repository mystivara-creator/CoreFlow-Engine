/**
 * @file baseline_store.cpp
 * @brief Baseline Store Implementation
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/baseline_store.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

namespace coreflow {

BaselineStore& BaselineStore::instance() {
    static BaselineStore inst;
    return inst;
}

std::string BaselineStore::read_raw_node(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::string content;
    std::getline(file, content);
    return content;
}

bool BaselineStore::write_raw_node(const std::string& path, const std::string& value) {
    // Try O_TRUNC first so regular files and mock test nodes don't leave trailing bytes
    int fd = ::open(path.c_str(), O_WRONLY | O_TRUNC | O_CLOEXEC);
    if (fd < 0) {
        // Fallback for virtual sysfs/procfs nodes where kernel drivers may reject O_TRUNC
        fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
        if (fd < 0) return false;
    }
    ssize_t w = ::write(fd, value.c_str(), value.size());
    ::ftruncate(fd, value.size());
    ::close(fd);
    return w == static_cast<ssize_t>(value.size());
}

bool BaselineStore::capture(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Already tracked? Don't overwrite initial factory baseline
    if (store_.find(path) != store_.end()) {
        return true;
    }

    std::string factory_val = read_raw_node(path);
    if (factory_val.empty()) {
        return false;
    }

    store_[path] = NodeSnapshot{
        .path = path,
        .factory_value = factory_val,
        .active_value = factory_val,
        .is_modified = false
    };

    return true;
}

bool BaselineStore::apply_mutation(const std::string& path, const std::string& new_value) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = store_.find(path);
    if (it == store_.end()) {
        // Automatically capture factory baseline first
        std::string factory_val = read_raw_node(path);
        if (factory_val.empty()) return false;
        store_[path] = NodeSnapshot{
            .path = path,
            .factory_value = factory_val,
            .active_value = factory_val,
            .is_modified = false
        };
        it = store_.find(path);
    }

    if (write_raw_node(path, new_value)) {
        it->second.active_value = new_value;
        it->second.is_modified = (it->second.active_value != it->second.factory_value);
        return true;
    }

    return false;
}

bool BaselineStore::rollback(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = store_.find(path);
    if (it == store_.end() || !it->second.is_modified) {
        return true;
    }

    if (write_raw_node(path, it->second.factory_value)) {
        it->second.active_value = it->second.factory_value;
        it->second.is_modified = false;
        std::cout << "[CoreFlow][Baseline] Restored factory value on " << path << std::endl;
        return true;
    }

    return false;
}

size_t BaselineStore::rollback_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t restored_count = 0;

    for (auto& [path, snap] : store_) {
        if (snap.is_modified) {
            if (write_raw_node(path, snap.factory_value)) {
                snap.active_value = snap.factory_value;
                snap.is_modified = false;
                restored_count++;
            }
        }
    }

    std::cout << "[CoreFlow][Baseline] Complete Rollback: " << restored_count 
              << " nodes restored to OEM factory state." << std::endl;
    return restored_count;
}

bool BaselineStore::has_active_mutations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [path, snap] : store_) {
        if (snap.is_modified) return true;
    }
    return false;
}

std::vector<NodeSnapshot> BaselineStore::get_audit_trail() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NodeSnapshot> trail;
    trail.reserve(store_.size());
    for (const auto& [_, snap] : store_) {
        trail.push_back(snap);
    }
    return trail;
}

ScopedBaselineLock::ScopedBaselineLock(std::vector<std::pair<std::string, std::string>> mutations) {
    for (const auto& [path, val] : mutations) {
        if (BaselineStore::instance().apply_mutation(path, val)) {
            modified_paths_.push_back(path);
        }
    }
}

ScopedBaselineLock::~ScopedBaselineLock() {
    for (const auto& path : modified_paths_) {
        BaselineStore::instance().rollback(path);
    }
}

} // namespace coreflow
