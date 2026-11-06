/**
 * @file cgroup_v2.cpp
 * @brief cgroups v2 implementation with coordinator-owned CPU policy transactions.
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/cgroup_v2.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/device_profiler.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;

CgroupV2Controller::CgroupV2Controller(std::string_view root_path)
    : root_path_(root_path),
      controller_name_("CgroupV2Controller@" +
                       std::to_string(reinterpret_cast<uintptr_t>(this))) {
    const std::string controllers_file = root_path_ + "/cgroup.controllers";
    supported_ = fs::exists(controllers_file);
    (void)CentralMutationCoordinator::instance().register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
            return handle_preemption(domain, transaction_id);
        });

    if (supported_) {
        std::cout << "[CoreFlow][CgroupV2] Modern cgroups v2 verified at "
                  << root_path_ << std::endl;
    } else {
        std::cerr << "[CoreFlow][CgroupV2] Notice: cgroups v2 not mounted at "
                  << root_path_ << ", attempting fallback discovery..." << std::endl;
    }
}

CgroupV2Controller::~CgroupV2Controller() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!transaction_id_.empty()) (void)rollback_transaction_locked();
    }
    // unregister waits for any callback that was already dispatched.
    CentralMutationCoordinator::instance().unregister_preemption_handler(controller_name_);
}

bool CgroupV2Controller::is_supported() const noexcept {
    return supported_;
}

std::string CgroupV2Controller::get_cgroup_path(CgroupTarget target) const {
    switch (target) {
        case CgroupTarget::TopApp: return root_path_ + "/top-app";
        case CgroupTarget::Foreground: return root_path_ + "/foreground";
        case CgroupTarget::Background: return root_path_ + "/background";
        case CgroupTarget::System: return root_path_ + "/system";
    }
    return root_path_;
}

bool CgroupV2Controller::migrate_task(pid_t tid, CgroupTarget target) {
    // Task migration requires a verified original membership and lifecycle owner.
    // Do not silently migrate an entire process when cgroup.threads is absent.
    (void)tid;
    (void)target;
    return false;
}

bool CgroupV2Controller::set_cpuset(CgroupTarget target, std::string_view cpus) {
    if (!supported_) return false;
    return write_node(get_cgroup_path(target) + "/cpuset.cpus", cpus);
}

bool CgroupV2Controller::set_cpu_weight(CgroupTarget target, uint32_t weight) {
    if (!supported_) return false;
    weight = std::clamp(weight, 1u, 10000u);
    return write_node(get_cgroup_path(target) + "/cpu.weight", std::to_string(weight));
}

bool CgroupV2Controller::unlock_vendor_restrictions() {
    // Opening vendor cpusets would bypass OEM task-placement policy without
    // proving a benefit. No transparent vendor-override backend is installed.
    return false;
}

bool CgroupV2Controller::acquire_lease_locked() {
    if (!transaction_id_.empty()) {
        return CentralMutationCoordinator::instance().acquire_lease(
            MutationDomain::CpuPolicy, controller_name_, transaction_id_,
            ControllerPriority::WorkloadOptimization);
    }
    transaction_id_ = "cgroup-cpu-" + std::to_string(++transaction_generation_);
    if (!CentralMutationCoordinator::instance().acquire_lease(
            MutationDomain::CpuPolicy, controller_name_, transaction_id_,
            ControllerPriority::WorkloadOptimization)) {
        transaction_id_.clear();
        return false;
    }
    return true;
}

void CgroupV2Controller::release_lease_locked() noexcept {
    if (transaction_id_.empty()) return;
    CentralMutationCoordinator::instance().release_lease(
        MutationDomain::CpuPolicy, controller_name_, transaction_id_);
    transaction_id_.clear();
}

bool CgroupV2Controller::rollback_transaction_locked() noexcept {
    if (transaction_id_.empty()) return active_mutation_paths_.empty();
    auto& store = BaselineStore::instance();
    for (const auto& path : active_mutation_paths_) {
        if (!store.rollback(path)) return false;
        const auto audit = store.get_audit_trail();
        const auto it = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == path;
        });
        if (it == audit.end() || it->is_modified || read_node(path) != it->factory_value) {
            return false;
        }
    }
    active_mutation_paths_.clear();
    release_lease_locked();
    return true;
}

bool CgroupV2Controller::handle_preemption(
    MutationDomain domain, std::string_view transaction_id) noexcept {
    if (domain != MutationDomain::CpuPolicy) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (transaction_id_.empty() || transaction_id_ != transaction_id) return false;
    return rollback_transaction_locked();
}

bool CgroupV2Controller::write_node(const std::string& path, std::string_view value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& store = BaselineStore::instance();
    if (!store.capture(path)) return false;

    const auto audit_before = store.get_audit_trail();
    const auto* snapshot_before = [&]() -> const NodeSnapshot* {
        const auto it = std::find_if(audit_before.begin(), audit_before.end(),
            [&](const NodeSnapshot& node) { return node.path == path; });
        return it == audit_before.end() ? nullptr : &*it;
    }();
    if (!snapshot_before) return false;

    const bool path_already_owned = std::find(
        active_mutation_paths_.begin(), active_mutation_paths_.end(), path) !=
        active_mutation_paths_.end();
    if (snapshot_before->is_modified && !path_already_owned) {
        // BaselineStore has a live mutation on this path, but this controller
        // did not add it to its transaction. Never extend or overwrite a foreign
        // or legacy mutation merely because this controller already owns the domain.
        return false;
    }

    // Verify every node still belongs to this transaction before adding another
    // CPU-policy mutation. An OEM change causes a verified handoff, never reassertion.
    if (!active_mutation_paths_.empty()) {
        for (const auto& owned_path : active_mutation_paths_) {
            const auto it = std::find_if(audit_before.begin(), audit_before.end(),
                [&](const NodeSnapshot& node) { return node.path == owned_path; });
            if (it == audit_before.end() ||
                !store.owns_expected_state(owned_path, it->active_value)) {
                if (!rollback_transaction_locked()) return false;
                return false;
            }
        }
    } else if (snapshot_before->is_modified) {
        // A mutation without this controller's lease is not ours to extend.
        return false;
    }

    const std::string current = read_node(path);
    const std::string requested(value);
    if (current.empty()) return false;
    if (current == requested) return true;

    if (requested == snapshot_before->factory_value) {
        if (!store.rollback(path) || read_node(path) != snapshot_before->factory_value) return false;
        active_mutation_paths_.erase(
            std::remove(active_mutation_paths_.begin(), active_mutation_paths_.end(), path),
            active_mutation_paths_.end());
        if (active_mutation_paths_.empty()) release_lease_locked();
        return true;
    }

    if (!acquire_lease_locked()) return false;
    const bool applied = store.apply_mutation(path, requested);
    const std::string readback = read_node(path);
    if (readback != snapshot_before->factory_value &&
        std::find(active_mutation_paths_.begin(), active_mutation_paths_.end(), path) ==
            active_mutation_paths_.end()) {
        // A failed write can still have partially changed a control node. Track
        // every non-baseline readback before deciding whether the transaction failed.
        active_mutation_paths_.push_back(path);
    }

    if (!applied || readback != requested) {
        if (active_mutation_paths_.empty()) {
            // First write did not change the node: do not strand a lease that owns
            // no mutation. Existing controller-owned mutations, if any, are handled
            // by the transaction rollback below.
            release_lease_locked();
        } else {
            // Keep CgroupV2Controller atomic at transaction scope: if adding a node
            // fails, restore every node this controller changed before releasing.
            (void)rollback_transaction_locked();
        }
        return false;
    }
    return true;
}

std::string CgroupV2Controller::read_node(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::string value;
    file >> value;
    return value;
}

} // namespace coreflow
