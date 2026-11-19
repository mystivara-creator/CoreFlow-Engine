#include "coreflow/runtime_architecture.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace coreflow::runtime;

namespace {

NodeDescriptor numeric_node(std::string id, std::string semantic,
                            std::string domain, std::int64_t current,
                            NumericRange range,
                            std::vector<NodeValue> opps = {}) {
    NodeDescriptor node;
    node.id = std::move(id);
    node.semantic = std::move(semantic);
    node.domain_id = std::move(domain);
    node.domain = NodeDomain::Cpu;
    node.value_kind = ValueKind::SignedInteger;
    node.access = AccessState::Writable;
    node.read_probe = ProbeState::Available;
    node.write_probe = ProbeState::Available;
    node.identity = {1U, static_cast<std::uint64_t>(node.id.size() + 10U)};
    node.observed_value = current;
    node.numeric_range = range;
    node.advertised_values = std::move(opps);
    return node;
}

NormalizedState state_with(double refresh, double cpu, bool screen_off = false,
                           double thermal = 0.0, bool thermal_valid = false) {
    NormalizedState state;
    const auto now = Clock::now();
    state.metrics.push_back({"display.refresh_hz", refresh / 240.0, 1.0, now, refresh > 0.0});
    state.metrics.push_back({"cpu.util", cpu, 1.0, now, true});
    state.metrics.push_back({"screen.off", screen_off ? 1.0 : 0.0, 1.0, now, true});
    state.metrics.push_back({"device.ram_ge_6gb", 1.0, 1.0, now, true});
    state.metrics.push_back({"device.zram_active", 1.0, 1.0, now, true});
    if (thermal_valid) state.metrics.push_back({"thermal.pressure", thermal, 1.0, now, true});
    return state;
}

PolicyIntent run_policy(DeviceManifest& manifest, OemBaseline& baseline, NormalizedState& state) {
    std::vector<ControlCapability> controls;
    for (const auto& node : manifest.nodes) {
        controls.push_back({
            node.id, node.value_kind, *node.numeric_range, node.observed_value,
            node.observed_value, node.advertised_values, true
        });
    }
    const AlgorithmConstants algorithm{0.08, 0.20, 32U, 4096U};
    AdaptiveReference reference;
    reference.generated_at = Clock::now();
    const PolicyInput input{manifest, baseline, state, reference, controls, algorithm};
    return calculate_policy_intent(input, Clock::now());
}

const PolicyAction* find_action(const PolicyIntent& intent, const std::string& id) {
    for (const auto& action : intent.actions) if (action.node_id == id) return &action;
    return nullptr;
}

} // namespace

int main() {
    // CPU targets must be selected from advertised OPPs, never an invented Hz value.
    {
        DeviceManifest manifest;
        manifest.nodes.push_back(numeric_node("little-hispeed", "cpu.hispeed_freq",
            "cpu.little", 998400, {300000.0L, 1800000.0L, std::nullopt},
            {std::int64_t{300000}, std::int64_t{600000}, std::int64_t{900000},
             std::int64_t{1200000}, std::int64_t{1500000}, std::int64_t{1800000}}));
        manifest.nodes.push_back(numeric_node("big-load", "cpu.hispeed_load",
            "cpu.big", 85, {0.0L, 100.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("little-load", "cpu.hispeed_load",
            "cpu.little", 85, {0.0L, 100.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("up-rate", "cpu.up_rate_limit_us",
            "cpu.big", 1000, {0.0L, 1000000.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("down-rate", "cpu.down_rate_limit_us",
            "cpu.big", 1000, {0.0L, 1000000.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("boost", "cpu.boost",
            "cpu.big", -6, {-128.0L, 128.0L, 1.0L}));
        OemBaseline baseline;
        for (const auto& node : manifest.nodes) baseline.entries.push_back({node.id, node.observed_value, node.identity, Clock::now()});
        auto state = state_with(120.0, 0.20);
        const auto intent = run_policy(manifest, baseline, state);
        assert(intent.mode == OperatingMode::Balanced);
        const auto* big = find_action(intent, "big-load");
        const auto* little = find_action(intent, "little-load");
        const auto* up = find_action(intent, "up-rate");
        const auto* down = find_action(intent, "down-rate");
        const auto* boost = find_action(intent, "boost");
        assert(big && std::get<std::int64_t>(big->target_value) == 82);
        assert(little && std::get<std::int64_t>(little->target_value) == 90);
        assert(up && std::get<std::int64_t>(up->target_value) == 500);
        assert(down && std::get<std::int64_t>(down->target_value) == 10000);
        assert(boost && std::get<std::int64_t>(boost->target_value) == 0);
        const auto* opp = find_action(intent, "little-hispeed");
        assert(opp);
        const auto selected = std::get<std::int64_t>(opp->target_value);
        assert(selected == 300000 || selected == 600000 || selected == 900000 ||
               selected == 1200000 || selected == 1500000 || selected == 1800000);
    }

    // Screen-off policy uses T_frame * window and does not require PSI.
    {
        DeviceManifest manifest;
        manifest.nodes.push_back(numeric_node("gpu-idle", "gpu.idle_timer",
            "gpu.kgsl:test", 0, {0.0L, 1000000.0L, 1.0L}));
        OemBaseline baseline{{}, {}, {}};
        baseline.entries.push_back({"gpu-idle", std::int64_t{0}, manifest.nodes.front().identity, Clock::now()});
        auto state = state_with(120.0, 0.0, true);
        const auto intent = run_policy(manifest, baseline, state);
        assert(intent.mode == OperatingMode::Idle);
        const auto* action = find_action(intent, "gpu-idle");
        assert(action);
        assert(std::get<std::int64_t>(action->target_value) == 67);
    }

    // KGSL indices are inverse to devfreq: index zero is the highest OPP.
    {
        DeviceManifest manifest;
        manifest.nodes.push_back(numeric_node("kgsl-default", "gpu.kgsl.default_pwrlevel",
            "gpu.kgsl:Adreno", 7, {0.0L, 7.0L, 1.0L},
            {std::int64_t{0}, std::int64_t{1}, std::int64_t{2}, std::int64_t{3},
             std::int64_t{4}, std::int64_t{5}, std::int64_t{6}, std::int64_t{7}}));
        manifest.nodes.back().domain = NodeDomain::Gpu;
        manifest.nodes.push_back(numeric_node("mali-max", "gpu.devfreq.max_freq",
            "gpu.devfreq:Mali", 400000, {200000.0L, 1000000.0L, std::nullopt},
            {std::int64_t{200000}, std::int64_t{400000}, std::int64_t{600000},
             std::int64_t{800000}, std::int64_t{1000000}}));
        manifest.nodes.back().domain = NodeDomain::Gpu;
        OemBaseline baseline;
        for (const auto& node : manifest.nodes) baseline.entries.push_back({node.id, node.observed_value, node.identity, Clock::now()});
        auto state = state_with(120.0, 0.20);
        const auto intent = run_policy(manifest, baseline, state);
        const auto* kgsl = find_action(intent, "kgsl-default");
        const auto* mali = find_action(intent, "mali-max");
        assert(kgsl && std::get<std::int64_t>(kgsl->target_value) == 2);
        assert(mali && std::get<std::int64_t>(mali->target_value) >= 800000);
    }

    // Thermal safety must outrank screen-off idle, and recovery must return
    // to the idle envelope rather than retaining the thermal target.
    {
        DeviceManifest manifest;
        manifest.nodes.push_back(numeric_node("cpu-hispeed", "cpu.hispeed_freq",
            "cpu.little", 300000, {300000.0L, 1800000.0L, std::nullopt},
            {std::int64_t{300000}, std::int64_t{600000}, std::int64_t{900000},
             std::int64_t{1200000}, std::int64_t{1500000}, std::int64_t{1800000}}));
        OemBaseline baseline;
        baseline.entries.push_back({"cpu-hispeed", std::int64_t{300000},
            manifest.nodes.front().identity, Clock::now()});

        auto hot_screen_off = state_with(120.0, 0.0, true, 0.90, true);
        const auto thermal_intent = run_policy(manifest, baseline, hot_screen_off);
        assert(thermal_intent.mode == OperatingMode::ThermalGuard);
        const auto* thermal_target = find_action(thermal_intent, "cpu-hispeed");
        assert(thermal_target);
        assert(std::get<std::int64_t>(thermal_target->target_value) == 1200000);

        auto recovered_screen_off = state_with(120.0, 0.0, true);
        const auto idle_intent = run_policy(manifest, baseline, recovered_screen_off);
        assert(idle_intent.mode == OperatingMode::Idle);
        const auto* idle_target = find_action(idle_intent, "cpu-hispeed");
        assert(idle_target);
        assert(std::get<std::int64_t>(idle_target->target_value) == 900000);
    }

    // Storage values are scoped to discovered media semantics, not device names.
    {
        DeviceManifest manifest;
        manifest.nodes.push_back(numeric_node("ufs-ra", "storage.read_ahead_kb",
            "storage.ufs:disk", 512, {0.0L, 65536.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("emmc-ra", "storage.read_ahead_kb",
            "storage.emmc:disk", 512, {0.0L, 65536.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("iostats", "storage.iostats",
            "storage.ufs:disk", 1, {0.0L, 1.0L, 1.0L}));
        OemBaseline baseline;
        for (const auto& node : manifest.nodes) baseline.entries.push_back({node.id, node.observed_value, node.identity, Clock::now()});
        auto state = state_with(120.0, 0.10);
        const auto intent = run_policy(manifest, baseline, state);
        const auto* ufs = find_action(intent, "ufs-ra");
        const auto* emmc = find_action(intent, "emmc-ra");
        const auto* stats = find_action(intent, "iostats");
        assert(ufs && std::get<std::int64_t>(ufs->target_value) == 128);
        assert(emmc && std::get<std::int64_t>(emmc->target_value) == 256);
        assert(stats && std::get<std::int64_t>(stats->target_value) == 0);
    }

    // VM targets are capability-bound and adapt to RAM/ZRAM; thermal pressure
    // has priority over load and does not depend on an optional PSI metric.
    {
        DeviceManifest manifest;
        manifest.nodes.push_back(numeric_node("swappiness", "vm.swappiness",
            "vm", 100, {0.0L, 200.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("page-cluster", "vm.page_cluster",
            "vm", 3, {0.0L, 8.0L, 1.0L}));
        OemBaseline baseline;
        for (const auto& node : manifest.nodes) baseline.entries.push_back({node.id, node.observed_value, node.identity, Clock::now()});
        auto state = state_with(120.0, 0.95, false, 0.90, true);
        const auto intent = run_policy(manifest, baseline, state);
        assert(intent.mode == OperatingMode::ThermalGuard);
        const auto* swappiness = find_action(intent, "swappiness");
        const auto* cluster = find_action(intent, "page-cluster");
        assert(swappiness && std::get<std::int64_t>(swappiness->target_value) == 65);
        assert(cluster && std::get<std::int64_t>(cluster->target_value) == 0);
    }

    std::cout << "Direct Adaptive Sweet-Spot policy tests passed\n";
    return 0;
}
