#include "coreflow/direct_adaptive_runtime.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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
    state.metrics.push_back({"display.refresh_hz", refresh / 1000.0, 1.0, now, refresh > 0.0});
    state.metrics.push_back({"cpu.util", cpu, 1.0, now, true});
    state.metrics.push_back({"screen.off", screen_off ? 1.0 : 0.0, 1.0, now, true});
    state.metrics.push_back({"device.ram_ge_6gb", 1.0, 1.0, now, true});
    state.metrics.push_back({"device.zram_active", 1.0, 1.0, now, true});
    if (thermal_valid) state.metrics.push_back({"thermal.pressure", thermal, 1.0, now, true});
    return state;
}

PolicyIntent run_policy(DeviceManifest& manifest, OemBaseline& baseline,
                           NormalizedState& state,
                           OperatingMode previous_mode = OperatingMode::Unknown) {
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
    const PolicyInput input{manifest, baseline, state, reference, controls, algorithm, previous_mode};
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

    // High-refresh panels retain their measured frame period instead of
    // saturating at the old 240 Hz normalization ceiling.
    {
        DeviceManifest manifest;
        manifest.nodes.push_back(numeric_node("up-rate-480", "cpu.up_rate_limit_us",
            "cpu.big", 1000, {0.0L, 1000000.0L, 1.0L}));
        manifest.nodes.push_back(numeric_node("down-rate-480", "cpu.down_rate_limit_us",
            "cpu.big", 1000, {0.0L, 1000000.0L, 1.0L}));
        OemBaseline baseline;
        for (const auto& node : manifest.nodes) {
            baseline.entries.push_back({node.id, node.observed_value, node.identity, Clock::now()});
        }
        auto state = state_with(480.0, 0.10);
        const auto intent = run_policy(manifest, baseline, state);
        const auto* up = find_action(intent, "up-rate-480");
        const auto* down = find_action(intent, "down-rate-480");
        assert(up && std::get<std::int64_t>(up->target_value) == 125);
        assert(down && std::get<std::int64_t>(down->target_value) == 10000);
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

    // KGSL indices are inverse to devfreq. Both bounds are required:
    // max_pwrlevel is the faster ceiling (lower index), min_pwrlevel the
    // slower floor (higher index). A single unpaired bound is never mutated.
    {
        DeviceManifest manifest;
        const std::vector<NodeValue> levels{
            std::int64_t{0}, std::int64_t{1}, std::int64_t{2}, std::int64_t{3},
            std::int64_t{4}, std::int64_t{5}, std::int64_t{6}, std::int64_t{7}};
        manifest.nodes.push_back(numeric_node("kgsl-max", "gpu.kgsl.max_pwrlevel",
            "gpu.kgsl:Adreno", 0, {0.0L, 6.0L, 1.0L}, levels));
        manifest.nodes.back().domain = NodeDomain::Gpu;
        manifest.nodes.push_back(numeric_node("kgsl-min", "gpu.kgsl.min_pwrlevel",
            "gpu.kgsl:Adreno", 6, {0.0L, 6.0L, 1.0L}, levels));
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
        const auto* kgsl_max = find_action(intent, "kgsl-max");
        const auto* kgsl_min = find_action(intent, "kgsl-min");
        const auto* mali = find_action(intent, "mali-max");
        assert(kgsl_max && std::get<std::int64_t>(kgsl_max->target_value) == 2);
        assert(kgsl_min && std::get<std::int64_t>(kgsl_min->target_value) == 5);
        assert(mali && std::get<std::int64_t>(mali->target_value) >= 800000);
        std::size_t max_position = intent.actions.size();
        std::size_t min_position = intent.actions.size();
        for (std::size_t i = 0; i < intent.actions.size(); ++i) {
            if (intent.actions[i].node_id == "kgsl-max") max_position = i;
            if (intent.actions[i].node_id == "kgsl-min") min_position = i;
        }
        assert(max_position < min_position); // Existing min bound already permits the ceiling.

        // If the target ceiling is outside the current lower bound, widen the
        // lower bound first so the KGSL max setter cannot clamp its target.
        DeviceManifest expanding;
        expanding.nodes.push_back(numeric_node("expand-max", "gpu.kgsl.max_pwrlevel",
            "gpu.kgsl:Adreno", 0, {0.0L, 6.0L, 1.0L}, levels));
        expanding.nodes.back().domain = NodeDomain::Gpu;
        expanding.nodes.push_back(numeric_node("expand-min", "gpu.kgsl.min_pwrlevel",
            "gpu.kgsl:Adreno", 1, {0.0L, 6.0L, 1.0L}, levels));
        expanding.nodes.back().domain = NodeDomain::Gpu;
        OemBaseline expanding_baseline;
        for (const auto& node : expanding.nodes) {
            expanding_baseline.entries.push_back({node.id, node.observed_value, node.identity, Clock::now()});
        }
        const auto expansion = run_policy(expanding, expanding_baseline, state);
        std::size_t expand_max_position = expansion.actions.size();
        std::size_t expand_min_position = expansion.actions.size();
        for (std::size_t i = 0; i < expansion.actions.size(); ++i) {
            if (expansion.actions[i].node_id == "expand-max") expand_max_position = i;
            if (expansion.actions[i].node_id == "expand-min") expand_min_position = i;
        }
        assert(expand_min_position < expand_max_position);

        DeviceManifest unpaired;
        unpaired.nodes.push_back(numeric_node("kgsl-only-min", "gpu.kgsl.min_pwrlevel",
            "gpu.kgsl:Adreno", 6, {0.0L, 6.0L, 1.0L}, levels));
        unpaired.nodes.back().domain = NodeDomain::Gpu;
        OemBaseline unpaired_baseline;
        unpaired_baseline.entries.push_back({"kgsl-only-min", std::int64_t{6},
            unpaired.nodes.front().identity, Clock::now()});
        const auto unpaired_intent = run_policy(unpaired, unpaired_baseline, state);
        assert(find_action(unpaired_intent, "kgsl-only-min") == nullptr);
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

    // Mode hysteresis prevents repeated transitions near CPU and thermal
    // thresholds. ThermalGuard remains active if its telemetry disappears.
    {
        DeviceManifest manifest;
        OemBaseline baseline;

        auto thermal_entry = state_with(120.0, 0.0, false, 0.86, true);
        assert(run_policy(manifest, baseline, thermal_entry, OperatingMode::Balanced).mode ==
               OperatingMode::ThermalGuard);

        auto fused_oem_guard = state_with(120.0, 0.0, false, 0.50, true);
        fused_oem_guard.metrics.push_back({"thermal.guard", 1.0, 1.0, Clock::now(), true});
        assert(run_policy(manifest, baseline, fused_oem_guard, OperatingMode::Balanced).mode ==
               OperatingMode::ThermalGuard);

        auto thermal_hold = state_with(120.0, 0.0, false, 0.82, true);
        assert(run_policy(manifest, baseline, thermal_hold, OperatingMode::ThermalGuard).mode ==
               OperatingMode::ThermalGuard);

        auto thermal_exit = state_with(120.0, 0.0, false, 0.75, true);
        assert(run_policy(manifest, baseline, thermal_exit, OperatingMode::ThermalGuard).mode ==
               OperatingMode::Balanced);

        auto load_entry = state_with(120.0, 0.72);
        assert(run_policy(manifest, baseline, load_entry, OperatingMode::Balanced).mode ==
               OperatingMode::HighLoad);

        auto load_hold = state_with(120.0, 0.67);
        assert(run_policy(manifest, baseline, load_hold, OperatingMode::HighLoad).mode ==
               OperatingMode::HighLoad);

        auto load_exit = state_with(120.0, 0.60);
        assert(run_policy(manifest, baseline, load_exit, OperatingMode::HighLoad).mode ==
               OperatingMode::Balanced);

        auto thermal_evidence_lost = state_with(120.0, 0.0);
        assert(run_policy(manifest, baseline, thermal_evidence_lost, OperatingMode::ThermalGuard).mode ==
               OperatingMode::ThermalGuard);
    }

    // The interactive governor is admitted only when its actual ABI tunables
    // are present under the discovered cpufreq policy.
    {
        namespace fs = std::filesystem;
        char temp_path[] = "/tmp/coreflow-da-policy-XXXXXX";
        const char* created = ::mkdtemp(temp_path);
        assert(created != nullptr);
        const fs::path root(created);
        const fs::path policy = root / "sys/devices/system/cpu/cpufreq/policy0";
        fs::create_directories(policy / "interactive");
        fs::create_directories(root / "proc");

        const auto write_file = [](const fs::path& file, const std::string& value) {
            std::ofstream stream(file);
            assert(stream.good());
            stream << value;
            stream.close();
            assert(stream.good());
        };
        write_file(policy / "scaling_available_frequencies", "300000 600000 900000 1200000\n");
        write_file(policy / "scaling_governor", "interactive\n");
        write_file(policy / "interactive/hispeed_freq", "600000\n");
        write_file(policy / "interactive/hispeed_load", "85\n");

        const fs::path kgsl = root / "sys/class/kgsl/kgsl-3d0";
        fs::create_directories(kgsl);
        write_file(kgsl / "gpu_available_frequencies", "710000000 600000000 500000000\n");
        write_file(kgsl / "num_pwrlevels", "3\n");
        write_file(kgsl / "gpu_model", "Adreno\n");
        write_file(kgsl / "default_pwrlevel", "1\n");
        write_file(kgsl / "max_pwrlevel", "0\n");
        write_file(kgsl / "min_pwrlevel", "1\n");

        const fs::path unpaired_kgsl = root / "sys/class/kgsl/kgsl-unpaired";
        fs::create_directories(unpaired_kgsl);
        write_file(unpaired_kgsl / "gpu_available_frequencies", "710000000 600000000 500000000\n");
        write_file(unpaired_kgsl / "num_pwrlevels", "3\n");
        write_file(unpaired_kgsl / "min_pwrlevel", "1\n");

        TopologyScanner scanner;
        const auto discovery = scanner.discover({root / "sys", root / "proc"});
        assert(discovery.enumeration_completed);
        std::size_t hispeed_freq_nodes = 0U;
        std::size_t hispeed_load_nodes = 0U;
        std::size_t kgsl_max_nodes = 0U;
        std::size_t kgsl_min_nodes = 0U;
        std::size_t kgsl_default_nodes = 0U;
        for (const auto& node : discovery.manifest.nodes) {
            if (node.semantic == "cpu.hispeed_freq") ++hispeed_freq_nodes;
            if (node.semantic == "cpu.hispeed_load") ++hispeed_load_nodes;
            if (node.semantic == "gpu.kgsl.max_pwrlevel") ++kgsl_max_nodes;
            if (node.semantic == "gpu.kgsl.min_pwrlevel") ++kgsl_min_nodes;
            if (node.semantic == "gpu.kgsl.default_pwrlevel") ++kgsl_default_nodes;
        }
        assert(hispeed_freq_nodes == 1U);
        assert(hispeed_load_nodes == 1U);
        assert(kgsl_max_nodes == 1U);
        assert(kgsl_min_nodes == 1U);
        assert(kgsl_default_nodes == 0U);
        fs::remove_all(root);
    }

    // Heterogeneous CPU topology uses kernel capacity evidence for two- and
    // three-cluster layouts. Ambiguous frequency-only layouts are not guessed.
    {
        namespace fs = std::filesystem;
        const auto write_file = [](const fs::path& file, const std::string& value) {
            fs::create_directories(file.parent_path());
            std::ofstream stream(file);
            assert(stream.good());
            stream << value;
            stream.close();
            assert(stream.good());
        };
        const auto discover_domains = [&](const fs::path& root, bool with_capacity) {
            const std::vector<std::string> frequencies{
                "300000 600000 900000 1500000\n",
                "400000 800000 1200000 2200000\n",
                "500000 1000000 1800000 3000000\n"};
            const std::vector<std::string> capacities{"300\n", "600\n", "900\n"};
            for (unsigned index = 0U; index < 3U; ++index) {
                const fs::path policy = root / "sys/devices/system/cpu/cpufreq" /
                    ("policy" + std::to_string(index));
                write_file(policy / "scaling_available_frequencies", frequencies[index]);
                write_file(policy / "scaling_governor", "interactive\n");
                write_file(policy / "related_cpus", std::to_string(index) + "\n");
                write_file(policy / "interactive/hispeed_load", "85\n");
                if (with_capacity) {
                    write_file(root / "sys/devices/system/cpu" /
                        ("cpu" + std::to_string(index)) / "cpu_capacity", capacities[index]);
                }
            }
            fs::create_directories(root / "proc");
            TopologyScanner scanner;
            const auto result = scanner.discover({root / "sys", root / "proc"});
            assert(result.enumeration_completed);
            std::vector<std::string> domains;
            for (const auto& node : result.manifest.nodes) {
                if (node.semantic == "cpu.hispeed_load") domains.push_back(node.domain_id);
            }
            std::sort(domains.begin(), domains.end());
            return domains;
        };

        char capacity_path[] = "/tmp/coreflow-da-capacity-XXXXXX";
        const char* capacity_created = ::mkdtemp(capacity_path);
        assert(capacity_created != nullptr);
        const fs::path capacity_root(capacity_created);
        const auto capacity_domains = discover_domains(capacity_root, true);
        assert(capacity_domains.size() == 3U);
        assert(capacity_domains[0] == "cpu.big");
        assert(capacity_domains[1] == "cpu.little");
        assert(capacity_domains[2] == "cpu.mid");
        fs::remove_all(capacity_root);

        char ambiguous_path[] = "/tmp/coreflow-da-ambiguous-XXXXXX";
        const char* ambiguous_created = ::mkdtemp(ambiguous_path);
        assert(ambiguous_created != nullptr);
        const fs::path ambiguous_root(ambiguous_created);
        const auto ambiguous_domains = discover_domains(ambiguous_root, false);
        assert(ambiguous_domains.size() == 3U);
        for (const auto& domain : ambiguous_domains) assert(domain == "cpu.unknown");
        fs::remove_all(ambiguous_root);
    }

    std::cout << "Direct Adaptive Sweet-Spot policy tests passed\n";
    return 0;
}
