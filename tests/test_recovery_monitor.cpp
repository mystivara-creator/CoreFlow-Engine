#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/hardware_longevity.hpp"
#include "coreflow/recovery_monitor.hpp"
#include "coreflow/reactor.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <array>
#include <barrier>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace coreflow;

namespace {
void put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::trunc);
    out << value;
    assert(out.good());
}

std::string get(const fs::path& path) {
    std::ifstream in(path);
    std::string value;
    std::getline(in, value);
    return value;
}
} // namespace

int main() {
    char state_template[] = "/tmp/coreflow-recovery-monitor-XXXXXX";
    const char* state_dir = ::mkdtemp(state_template);
    assert(state_dir != nullptr);
    assert(::setenv("COREFLOW_TEST_STATE_DIR", state_dir, 1) == 0);

    auto& baseline = BaselineStore::instance();
    assert(baseline.recover_pending());
    CentralMutationCoordinator coordinator;
    coordinator.reset();

    // Retry admission is scoped to the exact lease and hard-capped at three.
    const std::string retry_owner = "ContentionBudgetTest";
    assert(coordinator.acquire_lease(
        MutationDomain::CpuPolicy, retry_owner, "retry-tx",
        ControllerPriority::WorkloadOptimization));
    uint32_t retry_attempt = 0;
    for (uint32_t expected = 1; expected <= 3; ++expected) {
        assert(coordinator.reserve_contention_retry(
            MutationDomain::CpuPolicy, retry_owner, "retry-tx", 9, &retry_attempt));
        assert(retry_attempt == expected);
    }
    assert(!coordinator.reserve_contention_retry(
        MutationDomain::CpuPolicy, retry_owner, "retry-tx", 9, &retry_attempt));
    const auto retry_leases = coordinator.active_leases();
    assert(retry_leases.size() == 1 && retry_leases.front().contention_retries == 3);
    coordinator.release_lease(MutationDomain::CpuPolicy, retry_owner, "retry-tx");

    // A higher-priority request can arrive for a second domain while the same
    // controller callback is still resolving its first domain. The second lease
    // must remain pending and the timer-driven monitor must retry it later.
    const std::string pending_owner = "PendingMultiDomainController";
    std::mutex pending_mutex;
    std::condition_variable pending_cv;
    bool cpu_callback_entered = false;
    bool release_cpu_callback = false;
    uint32_t pending_callbacks = 0;
    assert(coordinator.register_preemption_handler(
        pending_owner,
        [&](MutationDomain domain, std::string_view, std::string_view) {
            if (domain == MutationDomain::CpuPolicy) {
                std::unique_lock<std::mutex> lock(pending_mutex);
                ++pending_callbacks;
                cpu_callback_entered = true;
                pending_cv.notify_all();
                pending_cv.wait(lock, [&] { return release_cpu_callback; });
                return true;
            }
            if (domain == MutationDomain::GpuDevfreq) {
                std::lock_guard<std::mutex> lock(pending_mutex);
                ++pending_callbacks;
                return true;
            }
            return false;
        }));
    assert(coordinator.acquire_lease(
        MutationDomain::CpuPolicy, pending_owner, "pending-cpu-tx",
        ControllerPriority::BackgroundEfficiency));
    assert(coordinator.acquire_lease(
        MutationDomain::GpuDevfreq, pending_owner, "pending-gpu-tx",
        ControllerPriority::BackgroundEfficiency));

    std::thread cpu_preempt_request([&] {
        assert(!coordinator.acquire_lease(
            MutationDomain::CpuPolicy, "CpuEmergencyRequester", "cpu-emergency-tx",
            ControllerPriority::EmergencySafety));
    });
    bool cpu_callback_started = false;
    {
        std::unique_lock<std::mutex> lock(pending_mutex);
        cpu_callback_started = pending_cv.wait_for(
            lock, std::chrono::seconds(3), [&] { return cpu_callback_entered; });
    }

    bool gpu_request_rejected = false;
    if (cpu_callback_started) {
        gpu_request_rejected = !coordinator.acquire_lease(
            MutationDomain::GpuDevfreq, "GpuEmergencyRequester", "gpu-emergency-tx",
            ControllerPriority::EmergencySafety);
    }
    {
        std::lock_guard<std::mutex> lock(pending_mutex);
        release_cpu_callback = true;
    }
    pending_cv.notify_all();
    cpu_preempt_request.join();
    assert(cpu_callback_started);
    assert(gpu_request_rejected);

    const auto queued_leases = coordinator.active_leases();
    assert(queued_leases.size() == 1);
    assert(queued_leases.front().domain == MutationDomain::GpuDevfreq);
    assert(coordinator.is_preemption_requested(
        MutationDomain::GpuDevfreq, pending_owner, "pending-gpu-tx"));

    MutationRecoveryMonitor pending_monitor(baseline, coordinator);
    const RecoveryResult pending_retry =
        pending_monitor.on_timer_tick("retry pending multi-domain preemption");
    assert(pending_retry.status == RecoveryStatus::NoRecoveryRequired);
    assert(pending_callbacks == 2);
    assert(coordinator.active_leases().empty());
    coordinator.unregister_preemption_handler(pending_owner);

    // Repeat real multi-controller teardown while emergency dispatchers race
    // with object destruction. A missing sysfs path forces each owner's own
    // rollback to fail, so only monitor-driven journal recovery may retire it.
    constexpr uint32_t kRounds = 8;
    constexpr uint32_t kDispatchers = 4;
    for (uint32_t round = 0; round < kRounds; ++round) {
        assert(coordinator.active_leases().empty());
        assert(!coordinator.is_emergency_recovery_active());
        assert(!baseline.has_active_mutations());

        const fs::path round_root = fs::path(state_dir) / ("round-" + std::to_string(round));
        const fs::path tuner_sys = round_root / "tuner-sys";
        const fs::path queue = tuner_sys / "class/block/sdb/queue";
        const fs::path longevity_sys = round_root / "longevity-sys";
        const fs::path battery_dir = longevity_sys / "class/power_supply/battery";
        const fs::path battery_node = battery_dir / "constant_charge_current_max";

        put(queue / "scheduler", "[none] mq-deadline kyber bfq");
        put(queue / "read_ahead_kb", "128");
        put(queue / "nr_requests", "64");
        put(queue / "rq_affinity", "1");
        put(queue / "iosched/read_expire", "500");
        put(queue / "iosched/writes_starved", "2");
        put(queue / "iosched/fifo_batch", "16");

        put(battery_dir / "type", "Battery");
        put(battery_node, "2000000");
        put(battery_dir / "temp", "420");
        put(battery_dir / "status", "Charging");

        auto tuner = std::make_unique<AutonomousTuner>(coordinator, tuner_sys.string(), (round_root / "proc").string());
        const auto tuner_state = tuner->step({
            .cpu_psi = 6.0,
            .memory_psi = 0.5,
            .io_psi = 12.0,
            .thermal_pressure = 48.0,
        });
        assert(tuner_state.stage == "OBSERVING");
        assert(tuner_state.active_candidate_id.rfind("io.", 0) == 0);

        LongevityConfig longevity_config{};
        auto longevity = std::make_unique<HardwareLongevityManager>(coordinator, longevity_sys.string());
        longevity->initialize(longevity_config);
        assert(longevity->battery_node_path() == battery_node.string());
        assert(longevity->monitor_battery_thermals(44.0f));
        assert(get(battery_node) == "1500000");

        const auto audit = baseline.get_audit_trail();
        const auto tuner_mutation = std::find_if(
            audit.begin(), audit.end(),
            [&](const NodeSnapshot& node) {
                return node.path.rfind(tuner_sys.string() + "/", 0) == 0 && node.is_modified;
            });
        const auto battery_mutation = std::find_if(
            audit.begin(), audit.end(),
            [&](const NodeSnapshot& node) { return node.path == battery_node.string() && node.is_modified; });
        assert(tuner_mutation != audit.end());
        assert(battery_mutation != audit.end());
        const std::string tuner_path = tuner_mutation->path;
        const std::string tuner_active = tuner_mutation->active_value;
        const std::string tuner_factory = tuner_mutation->factory_value;
        const std::string battery_active = battery_mutation->active_value;
        const std::string battery_factory = battery_mutation->factory_value;
        assert(!tuner_active.empty() && !tuner_factory.empty());
        assert(!battery_active.empty() && !battery_factory.empty());

        auto leases = coordinator.active_leases();
        assert(leases.size() == 2);
        std::set<MutationDomain> domains;
        for (const auto& lease : leases) domains.insert(lease.domain);
        assert(domains.size() == 2);
        assert(domains.count(MutationDomain::BlockIo) == 1);
        assert(domains.count(MutationDomain::PowerLongevity) == 1);

        // Rename preserves the original inode while making the owned sysfs-like
        // paths temporarily unavailable to both real controller destructors.
        const fs::path hidden_tuner = tuner_path + ".hidden";
        const fs::path hidden_battery = battery_node.string() + ".hidden";
        fs::rename(tuner_path, hidden_tuner);
        fs::rename(battery_node, hidden_battery);
        assert(!fs::exists(tuner_path) && fs::exists(hidden_tuner));
        assert(!fs::exists(battery_node) && fs::exists(hidden_battery));

        coordinator.begin_emergency_recovery();

        // Two real controller destructors and four competing emergency
        // dispatchers begin at one barrier. The coordinator must serialize each
        // controller callback, drain in-flight callbacks, and retain failed leases.
        std::barrier start_gate(static_cast<std::ptrdiff_t>(kDispatchers + 3));
        std::thread tuner_destroyer([&] {
            start_gate.arrive_and_wait();
            tuner.reset();
        });
        std::thread longevity_destroyer([&] {
            start_gate.arrive_and_wait();
            longevity.reset();
        });
        std::vector<std::thread> dispatchers;
        dispatchers.reserve(kDispatchers);
        for (uint32_t index = 0; index < kDispatchers; ++index) {
            dispatchers.emplace_back([&, index] {
                start_gate.arrive_and_wait();
                for (uint32_t pass = 0; pass < 12; ++pass) {
                    coordinator.emergency_preempt_all(
                        "combined-controller-teardown-" + std::to_string(round) +
                        "-dispatcher-" + std::to_string(index));
                }
            });
        }
        start_gate.arrive_and_wait();
        tuner_destroyer.join();
        longevity_destroyer.join();
        for (auto& dispatcher : dispatchers) dispatcher.join();

        leases = coordinator.active_leases();
        assert(leases.size() == 2);
        for (const auto& lease : leases) {
            assert(lease.active);
            assert(lease.owner_detached);
            assert(lease.preemption_requested);
        }
        assert(coordinator.is_emergency_recovery_active());
        assert(baseline.has_active_mutations());

        MutationRecoveryMonitor monitor(baseline, coordinator);
        EpollReactor reactor;
        std::mutex tick_mutex;
        std::condition_variable tick_cv;
        bool first_attempt_done = false;
        bool recovery_done = false;
        RecoveryStatus first_attempt_status = RecoveryStatus::NoRecoveryRequired;

        // Exercise the same Linux epoll/timerfd dispatch primitive as coreflowd.
        // The first periodic tick must remain fail-closed while both owned nodes
        // are unavailable; later ticks retry until the original inodes return.
        const int recovery_timer = reactor.create_timerfd(
            std::chrono::milliseconds(20),
            [&](uint32_t) {
                bool first_attempt = false;
                {
                    std::lock_guard<std::mutex> lock(tick_mutex);
                    first_attempt = !first_attempt_done;
                }
                if (first_attempt) {
                    const auto result = monitor.on_timer_tick("combined controller teardown test");
                    {
                        std::lock_guard<std::mutex> lock(tick_mutex);
                        first_attempt_status = result.status;
                        first_attempt_done = true;
                    }
                    tick_cv.notify_all();
                    return;
                }

                const auto result = monitor.on_timer_tick("combined controller teardown periodic retry");
                if (result.recovered()) {
                    {
                        std::lock_guard<std::mutex> lock(tick_mutex);
                        recovery_done = true;
                    }
                    tick_cv.notify_all();
                    reactor.stop();
                }
            });
        assert(recovery_timer >= 0);
        std::thread reactor_thread([&] { reactor.run(); });

        bool first_tick_seen = false;
        {
            std::unique_lock<std::mutex> lock(tick_mutex);
            first_tick_seen = tick_cv.wait_for(
                lock, std::chrono::seconds(3), [&] { return first_attempt_done; });
        }
        if (!first_tick_seen) {
            reactor.stop();
            reactor_thread.join();
            assert(first_tick_seen);
        }

        bool admission_stayed_closed = coordinator.is_emergency_recovery_active() &&
            coordinator.active_leases().size() == 2 &&
            baseline.has_active_mutations() &&
            !coordinator.acquire_lease(
                MutationDomain::CpuPolicy,
                "LateController@" + std::to_string(round),
                "must-remain-closed",
                ControllerPriority::EmergencySafety);

        // The same inodes return. The next real timerfd event must restore both
        // original factory values, persist the empty journal, retire both
        // detached leases, and reopen admission only after callback drain.
        fs::rename(hidden_tuner, tuner_path);
        fs::rename(hidden_battery, battery_node);
        assert(fs::exists(tuner_path));
        assert(fs::exists(battery_node));

        bool timerfd_recovered = false;
        {
            std::unique_lock<std::mutex> lock(tick_mutex);
            timerfd_recovered = tick_cv.wait_for(
                lock, std::chrono::seconds(5), [&] { return recovery_done; });
        }
        if (!timerfd_recovered) reactor.stop();
        reactor_thread.join();

        assert(first_attempt_status == RecoveryStatus::JournalRecoveryPending);
        assert(admission_stayed_closed);
        assert(timerfd_recovered);
        assert(get(tuner_path) == tuner_factory);
        assert(get(battery_node) == battery_factory);
        assert(!baseline.has_active_mutations());
        assert(baseline.verify_and_persist_clear_state());
        assert(coordinator.active_leases().empty());
        assert(!coordinator.is_emergency_recovery_active());
        assert(monitor.on_timer_tick().status == RecoveryStatus::NoRecoveryRequired);

        fs::remove_all(round_root);
    }

    assert(baseline.verify_and_persist_clear_state());
    assert(coordinator.active_leases().empty());
    fs::remove_all(state_dir);
    return 0;
}
