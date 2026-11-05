/**
 * @file test_vulkan_controller.cpp
 * @brief Deterministic unit tests for Universal Vulkan 1.4 Capability Controller
 *        and Multi-Vendor GPU Kernel Surface Rollback.
 */

#include "coreflow/vulkan_controller.hpp"
#include "coreflow/baseline_store.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace coreflow;

namespace {

void write_file(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    return line;
}

} // namespace

int main() {
    const fs::path tmp_root = fs::temp_directory_path() / ("coreflow_vk_test_" + std::to_string(::getpid()));
    const fs::path state_dir = tmp_root / "state";
    const fs::path adreno_root = tmp_root / "adreno_sysfs";
    const fs::path mali_root = tmp_root / "mali_sysfs";

    fs::remove_all(tmp_root);
    fs::create_directories(state_dir);

    BaselineStore::instance().initialize(state_dir.string());

    // 1. Setup synthetic Qualcomm Adreno KGSL sysfs surface
    const fs::path kgsl = adreno_root / "sys/class/kgsl/kgsl-3d0";
    write_file(kgsl / "devfreq/governor", "msm-adreno-tz\n");
    write_file(kgsl / "devfreq/adrenoboost", "0\n");
    write_file(kgsl / "gpuclk", "950000000\n");
    write_file(kgsl / "max_gpuclk", "1100000000\n");
    write_file(kgsl / "bus_split", "1\n");
    write_file(kgsl / "throttling", "1\n");

    auto& vk = VulkanController::instance();
    vk.initialize(
        VulkanConfig{
            .mode = VulkanLayerMode::SafeHardware,
            .enable_skiavk_threaded = true,
            .enable_adaptive_vrs = true,
            .enable_pipeline_cache_prewarm = true,
        },
        adreno_root.string(),
        state_dir.string());

    const auto snap_safe = vk.snapshot();
    assert(snap_safe.active_mode == VulkanLayerMode::SafeHardware);
    assert(snap_safe.layer_active);
    assert(snap_safe.effective_api_version == "1.4.303");
    // In Opsi A (SafeHardware), Vulkan 1.4 features & safe extensions are enabled,
    // while missing silicon ALUs (ray_tracing, shader_float64) remain false unless hardware supports them.
    assert(snap_safe.index_type_uint8);
    assert(snap_safe.line_rasterization);
    assert(snap_safe.push_descriptors);
    assert(snap_safe.global_priority_query);
    assert(snap_safe.shader_float_controls2);
    assert(snap_safe.maintenance5);
    assert(snap_safe.maintenance6);
    assert(snap_safe.maintenance7);
    assert(snap_safe.descriptor_buffer);
    assert(snap_safe.graphics_pipeline_library);
    assert(snap_safe.max_push_descriptors >= 32U);
    assert(snap_safe.gpu_surface.available);
    assert(snap_safe.gpu_surface.vendor == GpuVendor::QualcommAdreno);
    assert(snap_safe.gpu_surface.current_freq_mhz == 950U);
    assert(snap_safe.gpu_surface.max_freq_mhz == 1100U);

    // Verify workload policy boosts adrenoboost under SustainedLoad and rolls back cleanly
    assert(vk.apply_workload_policy(WorkloadState::SustainedLoad, 0.95, false));
    assert(read_file(kgsl / "devfreq/adrenoboost") == "2");
    assert(BaselineStore::instance().rollback_all());
    assert(read_file(kgsl / "devfreq/adrenoboost") == "0");

    // 2. Test Opsi B (FullCapabilityOverride)
    vk.configure(VulkanConfig{
        .mode = VulkanLayerMode::FullCapabilityOverride,
        .enable_skiavk_threaded = true,
        .enable_adaptive_vrs = true,
        .enable_pipeline_cache_prewarm = true,
    });
    const auto snap_full = vk.snapshot();
    assert(snap_full.active_mode == VulkanLayerMode::FullCapabilityOverride);
    assert(snap_full.ray_tracing);
    assert(snap_full.mesh_shaders);
    assert(snap_full.variable_rate_shading);
    assert(snap_full.cooperative_matrix);
    assert(snap_full.rt_pipeline);
    assert(snap_full.ray_query);
    assert(snap_full.acceleration_structure);
    assert(snap_full.shader_float64);
    assert(snap_full.max_push_descriptors >= 64U);

    // Check layer state file was written atomically
    assert(fs::exists(state_dir / "vulkan_layer.state"));

    // 3. Test Disabled (OEM Passthrough) mode
    vk.configure(VulkanConfig{
        .mode = VulkanLayerMode::Disabled,
        .enable_skiavk_threaded = false,
        .enable_adaptive_vrs = false,
        .enable_pipeline_cache_prewarm = false,
    });
    const auto snap_disabled = vk.snapshot();
    assert(snap_disabled.active_mode == VulkanLayerMode::Disabled);
    assert(!snap_disabled.layer_active);
    assert(snap_disabled.ray_tracing == snap_disabled.hw_ray_tracing);
    assert(snap_disabled.shader_float64 == snap_disabled.hw_shader_float64);

    // 4. Test Universal Multi-Vendor Discovery on ARM Mali / Immortalis sysfs
    const fs::path mali_dev = mali_root / "sys/class/devfreq/13000000.mali";
    write_file(mali_dev / "governor", "simple_ondemand\n");
    write_file(mali_dev / "cur_freq", "850000000\n");
    write_file(mali_dev / "max_freq", "1100000000\n");
    write_file(mali_dev / "min_freq", "300000000\n");
    write_file(mali_dev / "device/power_policy", "[coarse_demand] always_on\n");

    vk.initialize(
        VulkanConfig{
            .mode = VulkanLayerMode::SafeHardware,
            .enable_skiavk_threaded = true,
            .enable_adaptive_vrs = true,
            .enable_pipeline_cache_prewarm = true,
        },
        mali_root.string(),
        state_dir.string());

    const auto snap_mali = vk.snapshot();
    assert(snap_mali.gpu_surface.available);
    assert(snap_mali.gpu_surface.vendor == GpuVendor::ArmMali);
    assert(snap_mali.gpu_surface.active_governor == "simple_ondemand");
    assert(snap_mali.gpu_surface.current_freq_mhz == 850U);
    assert(snap_mali.gpu_surface.max_freq_mhz == 1100U);

    (void)BaselineStore::instance().rollback_all();
    fs::remove_all(tmp_root);
    std::cout << "[PASS] Universal Vulkan 1.4 Controller & Multi-Vendor GPU Surface tests succeeded.\n";
    return 0;
}
