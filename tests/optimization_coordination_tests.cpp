#include "coreflow/discovery/strategy_catalog.hpp"
#include "coreflow/optimization/optimization.hpp"
#include "coreflow/optimization/workload.hpp"
#include "coreflow/outcome/outcome.hpp"

#include <cassert>
#include <string>
#include <vector>

#ifndef COREFLOW_SOURCE_DIR
#define COREFLOW_SOURCE_DIR "."
#endif

using namespace coreflow;

namespace {

Capability numeric(const std::string& path, const std::string& domain,
                   const std::string& attribute, double value, double lo, double hi) {
    Capability c;
    c.id = path;
    c.path = path;
    c.raw_value = std::to_string(static_cast<long long>(value));
    c.numeric_value = value;
    c.numeric = true;
    c.writable = true;
    c.range = {lo, hi, true};
    c.optimization_supported = true;
    c.domain = domain;
    c.attribute = attribute;
    return c;
}

Capability selector(const std::string& path, const std::string& domain,
                    const std::string& attribute, const std::string& active,
                    std::vector<std::string> choices) {
    Capability c;
    c.id = path;
    c.path = path;
    c.raw_value = active;
    c.writable = true;
    c.optimization_supported = true;
    c.domain = domain;
    c.attribute = attribute;
    c.choices = std::move(choices);
    return c;
}

ObservationFrame ioFrame() {
    ObservationFrame f;
    f.system.io_pressure_valid = true;
    f.system.io_pressure_some_avg10 = 5.0;
    return f;
}

ObservationFrame cpuFrame() {
    ObservationFrame f;
    f.system.cpu_pressure_valid = true;
    f.system.cpu_pressure_some_avg10 = 5.0;
    return f;
}

} // namespace

int main() {
    // 1) Learning uses the direction that was actually written. A probe forced
    //    downward at the upper bound that fails must make the next upward probe
    //    in the same context go up, not repeat the downward move.
    {
        const auto frame = ioFrame();
        auto c = numeric("/virtual/read_ahead", "sysfs.block.queue", "read_ahead_kb", 10, 0, 10);
        OptimizationEngine engine;
        const auto first = engine.propose({c}, frame);
        assert(first && std::stod(first->target_value) < 10.0);  // upward is out of bounds
        engine.recordOutcome(*first, false, false, frame);
        c.raw_value = "5";
        c.numeric_value = 5.0;
        const auto second = engine.propose({c}, frame);
        assert(second && std::stod(second->target_value) > 5.0);
    }

    // 2) reset() restores the untrained proposal exactly.
    {
        const auto frame = ioFrame();
        auto c = numeric("/virtual/reset_probe", "sysfs.block.queue", "nr_requests", 10, 0, 10);
        OptimizationEngine trained;
        const auto fresh_target = OptimizationEngine{}.propose({c}, frame)->target_value;
        const auto p = trained.propose({c}, frame);
        trained.recordOutcome(*p, false, false, frame);
        trained.reset();
        assert(trained.propose({c}, frame)->target_value == fresh_target);
    }

    // 3) Elevator change: retained scheduler -> rediscovery -> child tunables of the
    //    new elevator are proposed; a same-named tunable on another queue is not.
    {
        const auto frame = ioFrame();
        const std::string queue = "/sys/block/sda/queue";
        auto sched = selector(queue + "/scheduler", "sysfs.block.queue", "scheduler",
                              "mq-deadline", {"none", "mq-deadline", "kyber", "bfq"});
        OptimizationEngine engine;
        const auto change = engine.propose({sched}, frame);
        assert(change && change->capability_id == queue + "/scheduler");
        engine.recordOutcome(*change, true, true, frame);

        // The scheduler change is retained, so the runtime must rediscover before any
        // follow-up is possible. The request is delivered exactly once.
        assert(engine.consumeRediscoveryRequest());
        assert(!engine.consumeRediscoveryRequest());

        // Inventory after rediscovery: the new elevator's directory is visible now.
        auto sched_after = sched;
        sched_after.raw_value = change->target_value;
        const auto child = numeric(queue + "/iosched/read_expire", "sysfs.block.iosched",
                                   "read_expire", 500, 1, 10000);
        const auto other_queue = numeric("/sys/block/sdb/queue/iosched/read_expire",
                                         "sysfs.block.iosched", "read_expire", 500, 1, 10000);
        const auto follow = engine.propose({sched_after, child, other_queue}, frame);
        assert(follow);
        assert(follow->capability_id == queue + "/iosched/read_expire");
    }

    // 4) Governor change: follow-ups are the tunables of the newly selected governor
    //    directory, not the previous governor's directory.
    {
        const auto frame = cpuFrame();
        const std::string policy = "/sys/devices/system/cpu/cpufreq/policy0";
        auto gov = selector(policy + "/scaling_governor", "sysfs.cpu.cpufreq", "scaling_governor",
                            "schedutil", {"schedutil", "ondemand", "performance"});
        OptimizationEngine engine;
        const auto change = engine.propose({gov}, frame);
        assert(change);
        // Force the evaluated target to the ondemand governor for a deterministic test.
        ChangeProposal retained = *change;
        retained.capability_id = policy + "/scaling_governor";
        retained.path = policy + "/scaling_governor";
        retained.target_value = "ondemand";
        retained.allowed_values = {"schedutil", "ondemand", "performance"};
        engine.recordOutcome(retained, true, true, frame);
        assert(engine.consumeRediscoveryRequest());

        auto new_gov = gov;
        new_gov.raw_value = "ondemand";
        const auto new_child = numeric(policy + "/ondemand/sampling_rate", "sysfs.cpu.governor",
                                       "sampling_rate", 10000, 1000, 1000000);
        const auto old_child = numeric(policy + "/schedutil/rate_limit_us", "sysfs.cpu.governor",
                                       "rate_limit_us", 2000, 100, 50000);
        const auto follow = engine.propose({new_gov, new_child, old_child}, frame);
        assert(follow);
        assert(follow->capability_id == policy + "/ondemand/sampling_rate");
    }

    // 5) A non-retained selector change requests no rediscovery.
    {
        const auto frame = ioFrame();
        auto sched = selector("/sys/block/sda/queue/scheduler", "sysfs.block.queue", "scheduler",
                              "mq-deadline", {"none", "mq-deadline", "kyber"});
        OptimizationEngine engine;
        const auto change = engine.propose({sched}, frame);
        assert(change);
        engine.recordOutcome(*change, false, false, frame);
        assert(!engine.consumeRediscoveryRequest());
    }

    // 6) Outcome verification judges an elevator tunable on I/O pressure, and the
    //    vendor passive-trip headroom vetoes retention even when I/O improved.
    {
        ChangeProposal p;
        p.capability_id = "/sys/block/sda/queue/iosched/read_expire";
        p.path = p.capability_id;
        p.previous_value = "500";
        p.target_value = "600";
        p.domain = "sysfs.block.iosched";
        p.attribute = "read_expire";

        SystemObservation before, after;
        before.io_pressure_valid = after.io_pressure_valid = true;
        before.io_pressure_some_avg10 = 10.0;
        after.io_pressure_some_avg10 = 2.0;
        OutcomeVerifier verifier;
        auto outcome = verifier.verify(before, after, p);
        assert(outcome.measurable && outcome.improved);

        before.thermal_headroom_valid = after.thermal_headroom_valid = true;
        before.thermal_headroom_c = 6.0;
        after.thermal_headroom_c = 2.0;
        outcome = verifier.verify(before, after, p);
        assert(outcome.measurable && !outcome.improved);
        assert(outcome.explanation.find("OUTCOME_REGRESSION") != std::string::npos);
    }

    // 7) Governor tunables are judged on CPU pressure, like their policy.
    {
        ChangeProposal p;
        p.capability_id = "/sys/devices/system/cpu/cpufreq/policy0/schedutil/rate_limit_us";
        p.path = p.capability_id;
        p.previous_value = "2000";
        p.target_value = "1000";
        p.domain = "sysfs.cpu.governor";
        p.attribute = "rate_limit_us";
        SystemObservation before, after;
        before.cpu_pressure_valid = after.cpu_pressure_valid = true;
        before.cpu_pressure_some_avg10 = 8.0;
        after.cpu_pressure_some_avg10 = 1.0;
        const auto outcome = OutcomeVerifier{}.verify(before, after, p);
        assert(outcome.measurable && outcome.improved);
    }

    // 8) Workload scoring routes the new domains to the matching pressure signal.
    {
        ObservationFrame f = ioFrame();
        assert(workloadScoreForDomain("sysfs.block.iosched", f.system) > 0.0);
        assert(workloadScoreForDomain("sysfs.cpu.governor", f.system) == 0.0);
        f = cpuFrame();
        assert(workloadScoreForDomain("sysfs.cpu.governor", f.system) > 0.0);
        assert(workloadScoreForDomain("sysfs.block.iosched", f.system) == 0.0);
    }

    // 9) The shipped catalogue loads and covers the new semantic domains.
    {
        StrategyCatalog catalog;
        std::string error;
        assert(catalog.load(std::string(COREFLOW_SOURCE_DIR) + "/module/strategies.tsv", error));
        assert(error.empty());
        assert(catalog.find("sysfs.block.iosched", "read_expire") != nullptr);
        assert(catalog.find("sysfs.cpu.governor", "rate_limit_us") != nullptr);
        assert(catalog.find("sysfs.cpu.governor", "sampling_rate") != nullptr);
        assert(catalog.find("sysfs.block.iosched", "unknown") == nullptr);
    }
    return 0;
}
