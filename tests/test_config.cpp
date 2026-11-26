#include "coreflow/config.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

int main() {
    const auto path = std::filesystem::temp_directory_path() / "coreflow-v2-test-config.ini";
    {
        std::ofstream f(path);
        f << "[thermal_guardian]\n"
             "thermal_trip_celsius=garbage\n"
             "thermal_recover_celsius=nan\n"
             "temp_monitor_interval_ms=-1\n"
             "[uclamp_tuning]\n"
             "top_app_interactive_min=99999999\n"
             "top_app_interactive_max=1\n"
             "ui_adaptive_bias=responsiveness\n"
             "ui_balanced_uclamp_min=480\n"
             "[stability_policy]\n"
             "cpu_pressure_boost_pct=999\n"
             "memory_pressure_guard_pct=0\n"
             "io_pressure_guard_pct=nan\n"
             "thermal_headroom_celsius=99\n"
             "thermal_emergency_margin_celsius=1\n";
    }
    auto c = coreflow::DaemonConfig::load_from_file(path.string());
    assert(c.thermal_trip_celsius == 44.0f);
    assert(c.thermal_recover_celsius == 40.0f);
    assert(c.temp_monitor_interval_ms == 1000u);
    assert(c.top_app_interactive_min == 1024u);
    assert(c.top_app_interactive_max == 1024u);
    assert(c.ui_adaptive_bias == "responsiveness");
    assert(c.ui_balanced_uclamp_min == 480u);
    assert(c.cpu_pressure_boost_pct == 50.0f);
    assert(c.memory_pressure_guard_pct == 0.5f);
    assert(c.io_pressure_guard_pct == 5.0f);
    assert(c.thermal_headroom_celsius == 10.0f);
    assert(c.thermal_emergency_margin_celsius == 2.0f);

    // 2. Test section-aware parsing: misplaced keys must be ignored, while [stability] legacy alias works.
    {
        std::ofstream f(path);
        f << "[general]\n"
             "enable_tuner = false\n"
             "thermal_trip_celsius = 55.0\n"
             "top_app_interactive_min = 600\n"
             "[stability]\n"
             "cpu_pressure_boost_pct = 12.5\n";
    }
    auto c2 = coreflow::DaemonConfig::load_from_file(path.string());
    assert(!c2.enable_tuner); // explicit tuner opt-out is honored in [general]
    assert(coreflow::DaemonConfig::canonical_section_for_key("enable_tuner") == "general");
    assert(c2.thermal_trip_celsius == 44.0f); // ignored under [general]
    assert(c2.top_app_interactive_min == 320u); // ignored under [general]
    assert(c2.cpu_pressure_boost_pct == 12.5f); // accepted under [stability] alias

    // All audited uclamp keys load from their canonical section and clamp to [0, 1024].
    {
        std::ofstream f(path);
        f << "[uclamp_tuning]\n"
             "foreground_min=2048\n"
             "background_max=0\n"
             "top_app_game_min=444\n"
             "top_app_game_max=2048\n"
             "sched_util_clamp_min_rt_default=1025\n";
    }
    auto uclamp = coreflow::DaemonConfig::load_from_file(path.string());
    assert(uclamp.foreground_min == 1024u);
    assert(uclamp.background_max == 0u);
    assert(uclamp.top_app_game_min == 444u);
    assert(uclamp.top_app_game_max == 1024u);
    assert(uclamp.sched_util_clamp_min_rt_default == 1024u);
    assert(coreflow::DaemonConfig::canonical_section_for_key("foreground_min") == "uclamp_tuning");
    assert(coreflow::DaemonConfig::canonical_section_for_key("background_max") == "uclamp_tuning");
    assert(coreflow::DaemonConfig::canonical_section_for_key("top_app_game_min") == "uclamp_tuning");
    assert(coreflow::DaemonConfig::canonical_section_for_key("top_app_game_max") == "uclamp_tuning");
    assert(coreflow::DaemonConfig::canonical_section_for_key("sched_util_clamp_min_rt_default") == "uclamp_tuning");


    // 3. Test surgical read-modify-write config merging (preserves comments & unknown keys)
    const std::string original =
        "# Header comment\n"
        "[general]\n"
        "log_level = debug\n"
        "custom_operator_flag = keep_me\n"
        "\n"
        "[uclamp_tuning]\n"
        "# Comment inside uclamp_tuning\n"
        "top_app_interactive_min = 320\n"
        "\n"
        "[custom_vendor_section]\n"
        "oem_hint = 42\n";

    const std::string merged = coreflow::DaemonConfig::merge_config_text(
        original,
        {
            {"top_app_interactive_min", "400"},
            {"ui_adaptive_bias", "battery"},
            {"direct_adaptive_policy", "auto"},
        });

    assert(merged.find("# Header comment") != std::string::npos);
    assert(merged.find("custom_operator_flag = keep_me") != std::string::npos);
    assert(merged.find("[custom_vendor_section]") != std::string::npos);
    assert(merged.find("oem_hint = 42") != std::string::npos);
    assert(merged.find("top_app_interactive_min = 400") != std::string::npos);
    assert(merged.find("ui_adaptive_bias = battery") != std::string::npos);
    assert(merged.find("[direct_adaptive]") != std::string::npos);
    assert(merged.find("direct_adaptive_policy = auto") != std::string::npos);

    {
        std::ofstream f(path);
        f << original;
    }
    assert(coreflow::DaemonConfig::update_config_file(
        path.string(),
        {{"top_app_interactive_min", "512"}, {"ui_balanced_uclamp_min", "512"}}));
    auto c3 = coreflow::DaemonConfig::load_from_file(path.string());
    assert(c3.top_app_interactive_min == 512u);
    assert(c3.ui_balanced_uclamp_min == 512u);

    std::ifstream verify_in(path);
    std::stringstream verify_buf;
    verify_buf << verify_in.rdbuf();
    assert(verify_buf.str().find("custom_operator_flag = keep_me") != std::string::npos);
    assert(verify_buf.str().find("oem_hint = 42") != std::string::npos);

    // Every audited uclamp key participates in merge and update/load round-trips.
    const std::vector<std::pair<std::string, std::string>> uclamp_updates = {
        {"foreground_min", "72"},
        {"background_max", "180"},
        {"top_app_game_min", "520"},
        {"top_app_game_max", "960"},
        {"sched_util_clamp_min_rt_default", "144"},
    };
    const std::string uclamp_merged =
        coreflow::DaemonConfig::merge_config_text(original, uclamp_updates);
    for (const auto& [key, value] : uclamp_updates) {
        assert(uclamp_merged.find(key + " = " + value) != std::string::npos);
    }
    {
        std::ofstream f(path);
        f << original;
    }
    assert(coreflow::DaemonConfig::update_config_file(path.string(), uclamp_updates));
    const auto updated_uclamp = coreflow::DaemonConfig::load_from_file(path.string());
    assert(updated_uclamp.foreground_min == 72u);
    assert(updated_uclamp.background_max == 180u);
    assert(updated_uclamp.top_app_game_min == 520u);
    assert(updated_uclamp.top_app_game_max == 960u);
    assert(updated_uclamp.sched_util_clamp_min_rt_default == 144u);

    // top_app_game_max clamps >= top_app_game_min.
    {
        std::ofstream f(path);
        f << "[uclamp_tuning]\n"
             "top_app_game_min=700\n"
             "top_app_game_max=300\n";
    }
    const auto inverted_game_uclamp = coreflow::DaemonConfig::load_from_file(path.string());
    assert(inverted_game_uclamp.top_app_game_min == 700u);
    assert(inverted_game_uclamp.top_app_game_max == 700u);

    // Render pipeline bounds clamping and injection-resistant update_config_file checks.
    {
        std::ofstream f(path);
        f << "[render_pipeline]\n"
             "jank_detection_threshold_ratio=99.0\n"
             "burst_boost_duration_ms=999999\n"
             "touch_boost_uclamp_min=4096\n";
    }
    const auto clamped_render = coreflow::DaemonConfig::load_from_file(path.string());
    assert(clamped_render.jank_detection_threshold_ratio == 5.0f);
    assert(clamped_render.burst_boost_duration_ms == 1000u);
    assert(clamped_render.touch_boost_uclamp_min == 1024u);

    assert(!coreflow::DaemonConfig::update_config_file(
        path.string(), {{"unknown_key_xyz", "1"}}));
    assert(!coreflow::DaemonConfig::update_config_file(
        path.string(), {{"log_level", "info\n[general]\nenable_uclamp=false"}}));

    // v5 profile, feature toggles, and per-app list validation/clearing
    assert(coreflow::DaemonConfig::update_config_file(
        path.string(),
        {
            {"profile", "smoothness"},
            {"enable_frame_smoothness", "true"},
            {"enable_storage_burst", "false"},
            {"enable_memory_guard", "true"},
            {"enable_thermal_pacing", "true"},
            {"gaming_apps", "com.miHoYo.GenshinImpact,com.tencent.ig"},
            {"excluded_apps", "com.android.camera"},
        }));
    const auto v5_cfg = coreflow::DaemonConfig::load_from_file(path.string());
    // v6.1.1 Zero-Profile Autonomous: legacy profile keys parse safely and normalize to autonomous.
    assert(v5_cfg.profile == "autonomous");
    assert(v5_cfg.enable_frame_smoothness);
    assert(!v5_cfg.enable_storage_burst);
    assert(v5_cfg.gaming_apps == "com.miHoYo.GenshinImpact,com.tencent.ig");
    assert(v5_cfg.excluded_apps == "com.android.camera");
    // Clearing per-app list to empty string must succeed, while shell/INI metacharacters must be rejected.
    assert(coreflow::DaemonConfig::update_config_file(path.string(), {{"excluded_apps", ""}}));
    assert(coreflow::DaemonConfig::load_from_file(path.string()).excluded_apps.empty());
    assert(!coreflow::DaemonConfig::update_config_file(path.string(), {{"gaming_apps", "com.app;rm -rf /"}}));

    // CoreFlowError integrates with std::error_code without throwing exceptions.
    const std::error_code ok_ec = coreflow::CoreFlowError::Success;
    const std::error_code thermal_ec = coreflow::CoreFlowError::ThermalEmergency;
    assert(!ok_ec);
    assert(static_cast<bool>(thermal_ec));
    assert(std::string(thermal_ec.category().name()) == "coreflow");
    assert(!thermal_ec.message().empty());

    std::filesystem::remove(path);
    std::cout << "Configuration bounds, section validation, and surgical merge PASS\n";
}
