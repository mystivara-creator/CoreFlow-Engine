#include "coreflow/config.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

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
    assert(c.vulkan_layer_mode == "disabled");
    assert(c.cpu_pressure_boost_pct == 50.0f);
    assert(c.memory_pressure_guard_pct == 0.5f);
    assert(c.io_pressure_guard_pct == 5.0f);
    assert(c.thermal_headroom_celsius == 10.0f);
    assert(c.thermal_emergency_margin_celsius == 2.0f);

    // 2. Test section-aware parsing: misplaced keys must be ignored, while [stability] legacy alias works.
    {
        std::ofstream f(path);
        f << "[general]\n"
             "thermal_trip_celsius = 55.0\n"
             "top_app_interactive_min = 600\n"
             "[stability]\n"
             "cpu_pressure_boost_pct = 12.5\n"
             "[vulkan_engine]\n"
             "vulkan_layer_mode = full_capability_override\n";
    }
    auto c2 = coreflow::DaemonConfig::load_from_file(path.string());
    assert(c2.thermal_trip_celsius == 44.0f); // ignored under [general]
    assert(c2.top_app_interactive_min == 320u); // ignored under [general]
    assert(c2.cpu_pressure_boost_pct == 12.5f); // accepted under [stability] alias
    assert(c2.vulkan_layer_mode == "safe_hardware"); // full_capability_override clamped to safe_hardware

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
            {"vulkan_layer_mode", "safe_hardware"},
        });

    assert(merged.find("# Header comment") != std::string::npos);
    assert(merged.find("custom_operator_flag = keep_me") != std::string::npos);
    assert(merged.find("[custom_vendor_section]") != std::string::npos);
    assert(merged.find("oem_hint = 42") != std::string::npos);
    assert(merged.find("top_app_interactive_min = 400") != std::string::npos);
    assert(merged.find("ui_adaptive_bias = battery") != std::string::npos);
    assert(merged.find("[vulkan_engine]") != std::string::npos);
    assert(merged.find("vulkan_layer_mode = safe_hardware") != std::string::npos);

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

    std::filesystem::remove(path);
    std::cout << "Configuration bounds, section validation, and surgical merge PASS\n";
}
