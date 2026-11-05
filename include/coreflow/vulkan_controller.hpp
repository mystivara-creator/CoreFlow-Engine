/**
 * @file vulkan_controller.hpp
 * @brief Universal Multi-Vendor Vulkan 1.4 Capability Inspector, Implicit Layer
 *        Coordinator, and Baseline-Guarded GPU Surface Optimizer.
 * @author mystivara-creator
 * @license MIT
 */

#pragma once

#include "types.hpp"
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

enum class VulkanLayerMode : uint8_t {
    Disabled = 0,               ///< Passive native inspection only; zero layer override
    SafeHardware = 1,           ///< Opsi A: Hardware-safe Vulkan 1.4 & driver-backed extension bridge + Adaptive VRS
    FullCapabilityOverride = 2  ///< Opsi B: Full VkLayer_coreflow capability completion + SPIR-V creation guard
};

enum class GpuVendor : uint8_t {
    Unknown = 0,
    QualcommAdreno = 1,     ///< Vendor ID 0x5143 (KGSL /sys/class/kgsl/kgsl-3d0)
    ArmMali = 2,            ///< Vendor ID 0x13B5 (Mali / Immortalis devfreq)
    SamsungXclipse = 3,     ///< Vendor ID 0x1002 / 0x144D (AMD RDNA Xclipse)
    ImaginationPowerVR = 4, ///< Vendor ID 0x1010 (PowerVR rogue/devfreq)
    GenericSysfs = 5        ///< Generic Linux / AOSP devfreq GPU domain
};

struct VulkanConfig {
    VulkanLayerMode mode{VulkanLayerMode::Disabled};
    bool enable_skiavk_threaded{false};
    bool enable_adaptive_vrs{false};
    bool enable_pipeline_cache_prewarm{false};
};

struct GpuKernelSurface {
    bool available{false};
    GpuVendor vendor{GpuVendor::Unknown};
    std::string vendor_label{"Universal AOSP GPU"};
    std::string sysfs_root;
    std::string active_governor{"default"};
    std::string baseline_governor{"default"};
    uint32_t current_freq_mhz{0};
    uint32_t max_freq_mhz{0};
    uint32_t min_freq_mhz{0};
    uint32_t adrenoboost_level{0};
    bool bus_split_enabled{true};
    bool throttling_enabled{true};
};

struct VulkanCapabilitiesSnapshot {
    bool vulkan_available{false};
    bool layer_installed{false};
    bool layer_active{false};
    VulkanLayerMode active_mode{VulkanLayerMode::Disabled};
    GpuVendor vendor{GpuVendor::Unknown};
    uint32_t vendor_id{0};
    uint32_t device_id{0};
    std::string device_name{"Detecting Vulkan Physical Device..."};
    std::string api_version{"0.0.0"};
    std::string effective_api_version{"0.0.0"};
    std::string driver_version_str{"0.0.0"};
    uint32_t device_extensions_count{0};
    uint32_t effective_device_extensions_count{0};
    uint32_t layer_implemented_extensions_count{0};
    uint32_t application_enabled_extensions_count{0};
    uint32_t instance_extensions_count{0};

    // RFC-4122 8-4-4-4-12 UUIDs read dynamically from VkPhysicalDeviceIDProperties / VkPhysicalDeviceProperties
    std::string device_uuid{"00000000-0000-0000-0000-000000000000"};
    std::string driver_uuid{"00000000-0000-0000-0000-000000000000"};
    std::string pipeline_cache_uuid{"00000000-0000-0000-0000-000000000000"};

    // Native hardware readouts (truthful physical ICD query)
    bool hw_ray_tracing{false};
    bool hw_mesh_shaders{false};
    bool hw_variable_rate_shading{false};
    bool hw_cooperative_matrix{false};
    bool hw_rt_pipeline{false};
    bool hw_ray_query{false};
    bool hw_acceleration_structure{false};
    bool hw_portability_subset{false};
    bool hw_shader_float64{false};
    bool hw_index_type_uint8{false};
    bool hw_line_rasterization{false};
    bool hw_push_descriptors{false};
    bool hw_global_priority_query{false};
    bool hw_shader_float_controls2{false};
    bool hw_maintenance5{false};
    bool hw_maintenance6{false};
    bool hw_maintenance7{false};
    bool hw_descriptor_buffer{false};
    bool hw_graphics_pipeline_library{false};
    uint32_t hw_max_push_descriptors{0};

    // Effective capabilities exposed to applications (strictly hardware-truthful pass-through)
    // Section 1: Capabilities
    bool ray_tracing{false};
    bool mesh_shaders{false};
    bool variable_rate_shading{false};
    bool cooperative_matrix{false};
    bool rt_pipeline{false};
    bool ray_query{false};
    bool acceleration_structure{false};
    bool portability_subset{false};

    // Section 2: Core Features
    bool robust_buffer_access{false};
    bool sampler_anisotropy{false};
    bool shader_int16{false};
    bool shader_float64{false};

    // Section 3: Vulkan 1.1
    bool protected_memory{false};

    // Section 4: Vulkan 1.2
    bool timeline_semaphore{false};
    bool descriptor_indexing{false};
    bool buffer_device_address{false};

    // Section 5: Vulkan 1.3
    bool dynamic_rendering{false};
    bool synchronization2{false};

    // Section 6: Vulkan 1.4
    bool index_type_uint8{false};
    bool line_rasterization{false};
    bool push_descriptors{false};
    bool global_priority_query{false};
    bool shader_float_controls2{false};
    uint32_t max_push_descriptors{0};

    // Section 7: Highlights
    bool maintenance5{false};
    bool maintenance6{false};
    bool maintenance7{false};
    bool descriptor_buffer{false};
    bool graphics_pipeline_library{false};

    // Runtime Pipeline Acceleration Flags
    bool skiavk_active{false};
    bool adaptive_vrs_active{false};
    bool pipeline_cache_prewarm_active{false};

    GpuKernelSurface gpu_surface{};
};

class VulkanController {
public:
    static VulkanController& instance();

    /// Discover native Vulkan ICD via dlopen("libvulkan.so") and probe GPU sysfs surfaces
    void initialize(const VulkanConfig& config = {},
                    const std::string& sysfs_root_override = "",
                    const std::string& state_dir_override = "/data/adb/coreflow");

    /// Re-evaluate GPU kernel pacing & Adaptive VRS according to workload and thermal state
    bool apply_workload_policy(WorkloadState state, double capacity_scale, bool thermal_guard) noexcept;

    /// Update configuration and write atomic state file for libVkLayer_coreflow.so
    void configure(const VulkanConfig& config) noexcept;

    /// Return thread-safe snapshot of hardware + effective Vulkan capabilities
    [[nodiscard]] VulkanCapabilitiesSnapshot snapshot() const;

    static const char* mode_name(VulkanLayerMode mode) noexcept;
    static VulkanLayerMode parse_mode(std::string_view text) noexcept;
    static const char* vendor_name(GpuVendor vendor) noexcept;

private:
    VulkanController() = default;

    void probe_native_vulkan_icd() noexcept;
    void discover_gpu_kernel_surface(const std::string& root_override) noexcept;
    void recompute_effective_capabilities() noexcept;
    void sync_layer_state_file() const noexcept;

    mutable std::mutex mutex_;
    VulkanConfig config_{};
    VulkanCapabilitiesSnapshot snap_{};
    std::string state_dir_{"/data/adb/coreflow"};
    std::string adrenoboost_node_;
    std::string bus_split_node_;
    std::string throttling_node_;
    std::string mali_policy_node_;
    uint32_t missing_safe_exts_{9};
    uint32_t missing_override_exts_{7};
    uint32_t raw_api_version_{0};
    bool hw_probed_{false};
};

} // namespace coreflow
