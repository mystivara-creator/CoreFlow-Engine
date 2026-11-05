/**
 * @file vulkan_controller.cpp
 * @brief Universal Multi-Vendor Vulkan 1.4 Inspector, Implicit Layer Coordinator,
 *        and Baseline-Guarded GPU Kernel Surface Optimizer.
 * @author mystivara-creator
 * @license MIT
 */

#include "coreflow/vulkan_controller.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {

namespace fs = std::filesystem;

namespace {

// Minimal Khronos Vulkan 1.0–1.4 C ABI definitions for zero-header dlopen probing
using VkBool32 = uint32_t;
using VkFlags = uint32_t;
using VkResult = int32_t;
using VkInstance = void*;
using VkPhysicalDevice = void*;

constexpr VkResult kVkSuccess = 0;
constexpr VkResult kVkIncomplete = 5;
constexpr uint32_t kVkTrue = 1U;
constexpr size_t kVkMaxPhysicalDeviceNameSize = 256;
constexpr size_t kVkUuidSize = 16;
constexpr size_t kVkMaxExtensionNameSize = 256;

// VkStructureType constants (Khronos official specification)
constexpr int32_t kVkStructureTypeApplicationInfo = 0;
constexpr int32_t kVkStructureTypeInstanceCreateInfo = 1;
constexpr int32_t kVkStructureTypePhysicalDeviceFeatures2 = 1000059000;
constexpr int32_t kVkStructureTypePhysicalDeviceProperties2 = 1000059001;
constexpr int32_t kVkStructureTypePhysicalDeviceIdProperties = 1000071004;
constexpr int32_t kVkStructureTypePhysicalDeviceVulkan11Features = 49;
constexpr int32_t kVkStructureTypePhysicalDeviceVulkan12Features = 51;
constexpr int32_t kVkStructureTypePhysicalDeviceVulkan13Features = 53;
constexpr int32_t kVkStructureTypePhysicalDevicePushDescriptorPropertiesKhr = 1000080000;

struct VkApplicationInfo {
    int32_t sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
    int32_t sType;
    const void* pNext;
    VkFlags flags;
    const VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkExtensionProperties {
    char extensionName[kVkMaxExtensionNameSize];
    uint32_t specVersion;
};

struct alignas(8) VkPhysicalDeviceLimits {
    uint64_t opaque[63]; // Exact 504 bytes with mandatory 8-byte alignment on 64-bit ARM64/x86_64
};

struct VkPhysicalDeviceSparseProperties {
    VkBool32 residencyStandard2DBlockShape;
    VkBool32 residencyStandard2DMultisampleBlockShape;
    VkBool32 residencyStandard3DBlockShape;
    VkBool32 residencyAlignedMipSize;
    VkBool32 residencyNonResidentStrict;
};

struct alignas(8) VkPhysicalDeviceProperties {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t deviceType;
    char deviceName[kVkMaxPhysicalDeviceNameSize];
    uint8_t pipelineCacheUUID[kVkUuidSize];
    // 4-byte compiler padding here on 64-bit so limits starts at offset 296 (total 824 bytes)
    VkPhysicalDeviceLimits limits;
    VkPhysicalDeviceSparseProperties sparseProperties;
    uint64_t reserved_tail_pad[16];
};

struct alignas(8) VkPhysicalDeviceIDProperties {
    int32_t sType;
    void* pNext;
    uint8_t deviceUUID[kVkUuidSize];
    uint8_t driverUUID[kVkUuidSize];
    uint8_t deviceLUID[8];
    uint32_t deviceNodeMask;
    VkBool32 deviceLUIDValid;
    uint64_t reserved_tail_pad[8];
};

struct alignas(8) VkPhysicalDevicePushDescriptorPropertiesKHR {
    int32_t sType;
    void* pNext;
    uint32_t maxPushDescriptors;
    uint64_t reserved_tail_pad[8];
};

struct alignas(8) VkPhysicalDeviceProperties2 {
    int32_t sType;
    void* pNext;
    VkPhysicalDeviceProperties properties;
    uint64_t reserved_tail_pad[16];
};

struct VkPhysicalDeviceFeatures {
    VkBool32 robustBufferAccess;
    VkBool32 fullDrawIndexUint32;
    VkBool32 imageCubeArray;
    VkBool32 independentBlend;
    VkBool32 geometryShader;
    VkBool32 tessellationShader;
    VkBool32 sampleRateShading;
    VkBool32 dualSrcBlend;
    VkBool32 logicOp;
    VkBool32 multiDrawIndirect;
    VkBool32 drawIndirectFirstInstance;
    VkBool32 depthClamp;
    VkBool32 depthBiasClamp;
    VkBool32 fillModeNonSolid;
    VkBool32 depthBounds;
    VkBool32 wideLines;
    VkBool32 largePoints;
    VkBool32 alphaToOne;
    VkBool32 multiViewport;
    VkBool32 samplerAnisotropy;
    VkBool32 textureCompressionETC2;
    VkBool32 textureCompressionASTC_LDR;
    VkBool32 textureCompressionBC;
    VkBool32 occlusionQueryPrecise;
    VkBool32 pipelineStatisticsQuery;
    VkBool32 vertexPipelineStoresAndAtomics;
    VkBool32 fragmentStoresAndAtomics;
    VkBool32 shaderTessellationAndGeometryPointSize;
    VkBool32 shaderImageGatherExtended;
    VkBool32 shaderStorageImageExtendedFormats;
    VkBool32 shaderStorageImageMultisample;
    VkBool32 shaderStorageImageReadWithoutFormat;
    VkBool32 shaderStorageImageWriteWithoutFormat;
    VkBool32 shaderUniformBufferArrayDynamicIndexing;
    VkBool32 shaderSampledImageArrayDynamicIndexing;
    VkBool32 shaderStorageBufferArrayDynamicIndexing;
    VkBool32 shaderStorageImageArrayDynamicIndexing;
    VkBool32 shaderClipDistance;
    VkBool32 shaderCullDistance;
    VkBool32 shaderFloat64;
    VkBool32 shaderInt64;
    VkBool32 shaderInt16;
    VkBool32 shaderResourceResidency;
    VkBool32 shaderResourceMinLod;
    VkBool32 sparseBinding;
    VkBool32 sparseResidencyBuffer;
    VkBool32 sparseResidencyImage2D;
    VkBool32 sparseResidencyImage3D;
    VkBool32 sparseResidency2Samples;
    VkBool32 sparseResidency4Samples;
    VkBool32 sparseResidency8Samples;
    VkBool32 sparseResidency16Samples;
    VkBool32 sparseResidencyAliased;
    VkBool32 variableMultisampleRate;
    VkBool32 inheritedQueries;
};

struct alignas(8) VkPhysicalDeviceVulkan11Features {
    int32_t sType;
    void* pNext;
    VkBool32 storageBuffer16BitAccess;
    VkBool32 uniformAndStorageBuffer16BitAccess;
    VkBool32 storagePushConstant16;
    VkBool32 storageInputOutput16;
    VkBool32 multiview;
    VkBool32 multiviewGeometryShader;
    VkBool32 multiviewTessellationShader;
    VkBool32 variablePointersStorageBuffer;
    VkBool32 variablePointers;
    VkBool32 protectedMemory;
    VkBool32 samplerYcbcrConversion;
    VkBool32 shaderDrawParameters;
    uint64_t reserved_tail_pad[8];
};

struct alignas(8) VkPhysicalDeviceVulkan12Features {
    int32_t sType;
    void* pNext;
    VkBool32 samplerMirrorClampToEdge;
    VkBool32 drawIndirectCount;
    VkBool32 storageBuffer8BitAccess;
    VkBool32 uniformAndStorageBuffer8BitAccess;
    VkBool32 storagePushConstant8;
    VkBool32 shaderBufferInt64Atomics;
    VkBool32 shaderSharedInt64Atomics;
    VkBool32 shaderFloat16;
    VkBool32 shaderInt8;
    VkBool32 descriptorIndexing;
    VkBool32 shaderInputAttachmentArrayDynamicIndexing;
    VkBool32 shaderUniformTexelBufferArrayDynamicIndexing;
    VkBool32 shaderStorageTexelBufferArrayDynamicIndexing;
    VkBool32 shaderUniformBufferArrayNonUniformIndexing;
    VkBool32 shaderSampledImageArrayNonUniformIndexing;
    VkBool32 shaderStorageBufferArrayNonUniformIndexing;
    VkBool32 shaderStorageImageArrayNonUniformIndexing;
    VkBool32 shaderInputAttachmentArrayNonUniformIndexing;
    VkBool32 shaderUniformTexelBufferArrayNonUniformIndexing;
    VkBool32 shaderStorageTexelBufferArrayNonUniformIndexing;
    VkBool32 descriptorBindingUniformBufferUpdateAfterBind;
    VkBool32 descriptorBindingSampledImageUpdateAfterBind;
    VkBool32 descriptorBindingStorageImageUpdateAfterBind;
    VkBool32 descriptorBindingStorageBufferUpdateAfterBind;
    VkBool32 descriptorBindingUniformTexelBufferUpdateAfterBind;
    VkBool32 descriptorBindingStorageTexelBufferUpdateAfterBind;
    VkBool32 descriptorBindingUpdateUnusedWhilePending;
    VkBool32 descriptorBindingPartiallyBound;
    VkBool32 descriptorBindingVariableDescriptorCount;
    VkBool32 runtimeDescriptorArray;
    VkBool32 samplerFilterMinmax;
    VkBool32 scalarBlockLayout;
    VkBool32 imagelessFramebuffer;
    VkBool32 uniformBufferStandardLayout;
    VkBool32 shaderSubgroupExtendedTypes;
    VkBool32 separateDepthStencilLayouts;
    VkBool32 hostQueryReset;
    VkBool32 timelineSemaphore;
    VkBool32 bufferDeviceAddress;
    VkBool32 bufferDeviceAddressCaptureReplay;
    VkBool32 bufferDeviceAddressMultiDevice;
    VkBool32 vulkanMemoryModel;
    VkBool32 vulkanMemoryModelDeviceScope;
    VkBool32 vulkanMemoryModelAvailabilityVisibilityChains;
    VkBool32 shaderOutputViewportIndex;
    VkBool32 shaderOutputLayer;
    VkBool32 subgroupBroadcastDynamicId;
    uint64_t reserved_tail_pad[8];
};

struct alignas(8) VkPhysicalDeviceVulkan13Features {
    int32_t sType;
    void* pNext;
    VkBool32 robustImageAccess;
    VkBool32 inlineUniformBlock;
    VkBool32 descriptorBindingInlineUniformBlockUpdateAfterBind;
    VkBool32 pipelineCreationCacheControl;
    VkBool32 privateData;
    VkBool32 shaderDemoteToHelperInvocation;
    VkBool32 shaderTerminateInvocation;
    VkBool32 subgroupSizeControl;
    VkBool32 computeFullSubgroups;
    VkBool32 synchronization2;
    VkBool32 textureCompressionASTC_HDR;
    VkBool32 shaderZeroInitializeWorkgroupMemory;
    VkBool32 dynamicRendering;
    VkBool32 shaderIntegerDotProduct;
    VkBool32 maintenance4;
    uint64_t reserved_tail_pad[8];
};

struct alignas(8) VkPhysicalDeviceFeatures2 {
    int32_t sType;
    void* pNext;
    VkPhysicalDeviceFeatures features;
    uint64_t reserved_tail_pad[8];
};

using PFN_vkGetInstanceProcAddr = void* (*)(VkInstance, const char*);
using PFN_vkCreateInstance = VkResult (*)(const VkInstanceCreateInfo*, const void*, VkInstance*);
using PFN_vkDestroyInstance = void (*)(VkInstance, const void*);
using PFN_vkEnumerateInstanceExtensionProperties = VkResult (*)(const char*, uint32_t*, VkExtensionProperties*);
using PFN_vkEnumeratePhysicalDevices = VkResult (*)(VkInstance, uint32_t*, VkPhysicalDevice*);
using PFN_vkGetPhysicalDeviceProperties = void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
using PFN_vkGetPhysicalDeviceProperties2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties2*);
using PFN_vkGetPhysicalDeviceFeatures = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
using PFN_vkGetPhysicalDeviceFeatures2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures2*);
using PFN_vkEnumerateDeviceExtensionProperties = VkResult (*)(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*);

constexpr uint32_t make_vk_version(uint32_t major, uint32_t minor, uint32_t patch) noexcept {
    return (major << 22U) | (minor << 12U) | patch;
}

std::string format_vk_version(uint32_t ver) {
    const uint32_t major = (ver >> 22U) & 0x7FU;
    const uint32_t minor = (ver >> 12U) & 0x3FFU;
    const uint32_t patch = ver & 0xFFFU;
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::string format_uuid_rfc4122(const uint8_t uuid[kVkUuidSize]) {
    char buf[40];
    std::snprintf(
        buf,
        sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5],
        uuid[6], uuid[7],
        uuid[8], uuid[9],
        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    return std::string(buf);
}

bool path_exists(const fs::path& p) noexcept {
    std::error_code ec;
    return fs::exists(p, ec) && !ec;
}

std::string read_first_line(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return "";
    char buf[256];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf) - 1);
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return "";
    buf[n] = '\0';
    std::string s(buf);
    const size_t eol = s.find_first_of("\r\n");
    if (eol != std::string::npos) s.resize(eol);
    const size_t first = s.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    const size_t last = s.find_last_not_of(" \t");
    return s.substr(first, last - first + 1);
}

uint32_t read_uint32_node(const std::string& path, uint32_t fallback = 0) noexcept {
    const std::string line = read_first_line(path);
    if (line.empty()) return fallback;
    double val = 0.0;
    if (!parse_ascii_double(line, val) || val < 0.0) return fallback;
    return static_cast<uint32_t>(val);
}

GpuVendor classify_vendor_id(uint32_t vendor_id, std::string_view device_name) noexcept {
    if (vendor_id == 0x5143U || device_name.find("Adreno") != std::string_view::npos) {
        return GpuVendor::QualcommAdreno;
    }
    if (vendor_id == 0x13B5U || device_name.find("Mali") != std::string_view::npos ||
        device_name.find("Immortalis") != std::string_view::npos) {
        return GpuVendor::ArmMali;
    }
    if (vendor_id == 0x144DU || vendor_id == 0x1002U ||
        device_name.find("Xclipse") != std::string_view::npos) {
        return GpuVendor::SamsungXclipse;
    }
    if (vendor_id == 0x1010U || device_name.find("PowerVR") != std::string_view::npos) {
        return GpuVendor::ImaginationPowerVR;
    }
    return GpuVendor::GenericSysfs;
}

} // namespace

VulkanController& VulkanController::instance() {
    static VulkanController inst;
    return inst;
}

const char* VulkanController::mode_name(VulkanLayerMode mode) noexcept {
    switch (mode) {
        case VulkanLayerMode::Disabled: return "disabled";
        case VulkanLayerMode::SafeHardware: return "safe_hardware";
        case VulkanLayerMode::FullCapabilityOverride: return "full_capability_override";
    }
    return "safe_hardware";
}

VulkanLayerMode VulkanController::parse_mode(std::string_view text) noexcept {
    if (text == "disabled" || text == "off" || text == "0" || text == "oem") {
        return VulkanLayerMode::Disabled;
    }
    // Production safety containment (VULKAN-01): FullCapabilityOverride is disallowed in production
    // runtime and clamped to SafeHardware pass-through inspection.
    if (text == "safe_hardware" || text == "full_capability_override" || text == "full" ||
        text == "override" || text == "opsi_b" || text == "opsi_a") {
        return VulkanLayerMode::SafeHardware;
    }
    return VulkanLayerMode::Disabled;
}

const char* VulkanController::vendor_name(GpuVendor vendor) noexcept {
    switch (vendor) {
        case GpuVendor::QualcommAdreno: return "Qualcomm Adreno";
        case GpuVendor::ArmMali: return "ARM Mali / Immortalis";
        case GpuVendor::SamsungXclipse: return "Samsung Xclipse (RDNA)";
        case GpuVendor::ImaginationPowerVR: return "Imagination PowerVR";
        case GpuVendor::GenericSysfs: return "Universal AOSP GPU";
        case GpuVendor::Unknown: return "Universal AOSP GPU";
    }
    return "Universal AOSP GPU";
}

void VulkanController::initialize(
    const VulkanConfig& config,
    const std::string& sysfs_root_override,
    const std::string& state_dir_override) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    if (config_.mode == VulkanLayerMode::FullCapabilityOverride) {
        config_.mode = VulkanLayerMode::SafeHardware;
    }
    if (!state_dir_override.empty()) {
        state_dir_ = state_dir_override;
    }

    if (!hw_probed_ && sysfs_root_override.empty()) {
        snap_ = VulkanCapabilitiesSnapshot{};
        probe_native_vulkan_icd();
        hw_probed_ = true;
    } else if (!sysfs_root_override.empty()) {
        snap_ = VulkanCapabilitiesSnapshot{};
    }
    snap_.active_mode = config_.mode;

    discover_gpu_kernel_surface(sysfs_root_override);

    const bool active_system_lib =
        path_exists("/system/lib64/libVkLayer_coreflow.so") &&
        path_exists("/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json");
    const bool staged_module_lib =
        (path_exists("/data/adb/modules/cfae_framework/system/lib64/libVkLayer_coreflow.so") &&
         path_exists("/data/adb/modules/cfae_framework/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json")) ||
        (path_exists("/data/adb/modules/coreflow_engine/system/lib64/libVkLayer_coreflow.so") &&
         path_exists("/data/adb/modules/coreflow_engine/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json"));
    snap_.layer_installed = active_system_lib || staged_module_lib;

    recompute_effective_capabilities();
    sync_layer_state_file();
}

void VulkanController::configure(const VulkanConfig& config) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    if (config_.mode == VulkanLayerMode::FullCapabilityOverride) {
        config_.mode = VulkanLayerMode::SafeHardware;
    }
    snap_.active_mode = config_.mode;
    recompute_effective_capabilities();
    sync_layer_state_file();
}

VulkanCapabilitiesSnapshot VulkanController::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snap_;
}

void VulkanController::probe_native_vulkan_icd() noexcept {
    // Silence stdout (fd 1) while invoking vendor libvulkan.so / GPU ICDs so driver
    // printf diagnostics never interleave with CLI JSON output (--status / --capabilities).
    struct ScopedStdoutSilencer {
        int saved_stdout{-1};
        ScopedStdoutSilencer() noexcept {
            std::fflush(stdout);
            saved_stdout = ::dup(STDOUT_FILENO);
            const int null_fd = ::open("/dev/null", O_WRONLY | O_CLOEXEC);
            if (null_fd >= 0) {
                (void)::dup2(null_fd, STDOUT_FILENO);
                if (null_fd != STDOUT_FILENO) (void)::close(null_fd);
            }
        }
        ~ScopedStdoutSilencer() noexcept {
            std::fflush(stdout);
            if (saved_stdout >= 0) {
                (void)::dup2(saved_stdout, STDOUT_FILENO);
                (void)::close(saved_stdout);
            }
        }
    } stdout_guard;

    // Prevent recursive self-interception while probing raw physical capabilities
    (void)::setenv("DISABLE_COREFLOW_VULKAN_LAYER", "1", 1);

    int dl_flags = RTLD_NOW | RTLD_LOCAL;
#ifdef RTLD_NODELETE
    dl_flags |= RTLD_NODELETE;
#endif
    void* lib = ::dlopen("libvulkan.so", dl_flags);
    if (!lib) {
        lib = ::dlopen("libvulkan.so.1", dl_flags);
    }
    if (!lib) {
        (void)::unsetenv("DISABLE_COREFLOW_VULKAN_LAYER");
        return;
    }

    auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        ::dlsym(lib, "vkGetInstanceProcAddr"));
    if (!get_proc) {
        (void)::unsetenv("DISABLE_COREFLOW_VULKAN_LAYER");
        return;
    }

    auto enum_inst_ext = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
        get_proc(nullptr, "vkEnumerateInstanceExtensionProperties"));
    auto create_inst = reinterpret_cast<PFN_vkCreateInstance>(
        get_proc(nullptr, "vkCreateInstance"));

    if (enum_inst_ext) {
        uint32_t inst_count = 0;
        if (enum_inst_ext(nullptr, &inst_count, nullptr) == kVkSuccess && inst_count > 0) {
            snap_.instance_extensions_count = inst_count;
        }
    }

    if (!create_inst) {
        (void)::unsetenv("DISABLE_COREFLOW_VULKAN_LAYER");
        return;
    }

    struct ProbeArena {
        VkApplicationInfo app_info{};
        VkInstanceCreateInfo create_info{};
        VkPhysicalDevicePushDescriptorPropertiesKHR push_props{};
        VkPhysicalDeviceIDProperties id_props{};
        VkPhysicalDeviceProperties2 props2{};
        VkPhysicalDeviceProperties props{};
        VkPhysicalDeviceVulkan13Features v13{};
        VkPhysicalDeviceVulkan12Features v12{};
        VkPhysicalDeviceVulkan11Features v11{};
        VkPhysicalDeviceFeatures2 f2{};
        VkPhysicalDeviceFeatures f{};
    };
    auto arena = std::make_unique<ProbeArena>();

    arena->app_info.sType = kVkStructureTypeApplicationInfo;
    arena->app_info.pNext = nullptr;
    arena->app_info.pApplicationName = "CoreFlowVulkanProbe";
    arena->app_info.applicationVersion = make_vk_version(3, 1, 0);
    arena->app_info.pEngineName = "CoreFlowEngine";
    arena->app_info.engineVersion = make_vk_version(3, 1, 0);
    arena->app_info.apiVersion = make_vk_version(1, 3, 0);

    arena->create_info.sType = kVkStructureTypeInstanceCreateInfo;
    arena->create_info.pNext = nullptr;
    arena->create_info.flags = 0;
    arena->create_info.pApplicationInfo = &arena->app_info;
    arena->create_info.enabledLayerCount = 0;
    arena->create_info.ppEnabledLayerNames = nullptr;
    arena->create_info.enabledExtensionCount = 0;
    arena->create_info.ppEnabledExtensionNames = nullptr;

    VkInstance instance = nullptr;
    if (create_inst(&arena->create_info, nullptr, &instance) != kVkSuccess || !instance) {
        arena->app_info.apiVersion = make_vk_version(1, 1, 0);
        if (create_inst(&arena->create_info, nullptr, &instance) != kVkSuccess || !instance) {
            arena->app_info.apiVersion = make_vk_version(1, 0, 0);
            if (create_inst(&arena->create_info, nullptr, &instance) != kVkSuccess || !instance) {
                (void)::unsetenv("DISABLE_COREFLOW_VULKAN_LAYER");
                return;
            }
        }
    }

    auto destroy_inst = reinterpret_cast<PFN_vkDestroyInstance>(
        get_proc(instance, "vkDestroyInstance"));
    auto enum_devs = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        get_proc(instance, "vkEnumeratePhysicalDevices"));
    auto get_props = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        get_proc(instance, "vkGetPhysicalDeviceProperties"));
    auto get_props2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        get_proc(instance, "vkGetPhysicalDeviceProperties2"));
    auto get_feats = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(
        get_proc(instance, "vkGetPhysicalDeviceFeatures"));
    auto get_feats2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        get_proc(instance, "vkGetPhysicalDeviceFeatures2"));
    auto enum_dev_ext = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        get_proc(instance, "vkEnumerateDeviceExtensionProperties"));

    uint32_t dev_count = 0;
    if (enum_devs && enum_devs(instance, &dev_count, nullptr) == kVkSuccess && dev_count > 0) {
        std::vector<VkPhysicalDevice> devs(dev_count, nullptr);
        if (enum_devs(instance, &dev_count, devs.data()) == kVkSuccess && devs[0] != nullptr) {
            VkPhysicalDevice phys = devs[0];
            snap_.vulkan_available = true;

            if (get_props2) {
                arena->push_props.sType = kVkStructureTypePhysicalDevicePushDescriptorPropertiesKhr;
                arena->push_props.pNext = nullptr;
                arena->push_props.maxPushDescriptors = 0;

                arena->id_props.sType = kVkStructureTypePhysicalDeviceIdProperties;
                arena->id_props.pNext = &arena->push_props;

                arena->props2.sType = kVkStructureTypePhysicalDeviceProperties2;
                arena->props2.pNext = &arena->id_props;

                get_props2(phys, &arena->props2);

                raw_api_version_ = arena->props2.properties.apiVersion;
                snap_.api_version = format_vk_version(arena->props2.properties.apiVersion);
                snap_.vendor_id = arena->props2.properties.vendorID;
                snap_.device_id = arena->props2.properties.deviceID;
                snap_.device_name = arena->props2.properties.deviceName;
                snap_.driver_version_str = format_vk_version(arena->props2.properties.driverVersion);
                snap_.pipeline_cache_uuid = format_uuid_rfc4122(arena->props2.properties.pipelineCacheUUID);
                snap_.device_uuid = format_uuid_rfc4122(arena->id_props.deviceUUID);
                snap_.driver_uuid = format_uuid_rfc4122(arena->id_props.driverUUID);
                if (arena->push_props.maxPushDescriptors > 0 && arena->push_props.maxPushDescriptors <= 1024) {
                    snap_.hw_max_push_descriptors = arena->push_props.maxPushDescriptors;
                    snap_.max_push_descriptors = arena->push_props.maxPushDescriptors;
                }
            } else if (get_props) {
                get_props(phys, &arena->props);
                raw_api_version_ = arena->props.apiVersion;
                snap_.api_version = format_vk_version(arena->props.apiVersion);
                snap_.vendor_id = arena->props.vendorID;
                snap_.device_id = arena->props.deviceID;
                snap_.device_name = arena->props.deviceName;
                snap_.driver_version_str = format_vk_version(arena->props.driverVersion);
                snap_.pipeline_cache_uuid = format_uuid_rfc4122(arena->props.pipelineCacheUUID);
            }

            snap_.vendor = classify_vendor_id(snap_.vendor_id, snap_.device_name);

            if (get_feats2) {
                arena->v13.sType = kVkStructureTypePhysicalDeviceVulkan13Features;
                arena->v13.pNext = nullptr;

                arena->v12.sType = kVkStructureTypePhysicalDeviceVulkan12Features;
                arena->v12.pNext = &arena->v13;

                arena->v11.sType = kVkStructureTypePhysicalDeviceVulkan11Features;
                arena->v11.pNext = &arena->v12;

                arena->f2.sType = kVkStructureTypePhysicalDeviceFeatures2;
                arena->f2.pNext = &arena->v11;

                get_feats2(phys, &arena->f2);

                snap_.robust_buffer_access = (arena->f2.features.robustBufferAccess == kVkTrue);
                snap_.sampler_anisotropy = (arena->f2.features.samplerAnisotropy == kVkTrue);
                snap_.shader_int16 = (arena->f2.features.shaderInt16 == kVkTrue);
                snap_.hw_shader_float64 = (arena->f2.features.shaderFloat64 == kVkTrue);
                snap_.protected_memory = (arena->v11.protectedMemory == kVkTrue);
                snap_.timeline_semaphore = (arena->v12.timelineSemaphore == kVkTrue);
                snap_.descriptor_indexing = (arena->v12.descriptorIndexing == kVkTrue);
                snap_.buffer_device_address = (arena->v12.bufferDeviceAddress == kVkTrue);
                snap_.dynamic_rendering = (arena->v13.dynamicRendering == kVkTrue);
                snap_.synchronization2 = (arena->v13.synchronization2 == kVkTrue);
            } else if (get_feats) {
                get_feats(phys, &arena->f);
                snap_.robust_buffer_access = (arena->f.robustBufferAccess == kVkTrue);
                snap_.sampler_anisotropy = (arena->f.samplerAnisotropy == kVkTrue);
                snap_.shader_int16 = (arena->f.shaderInt16 == kVkTrue);
                snap_.hw_shader_float64 = (arena->f.shaderFloat64 == kVkTrue);
            }

            if (enum_dev_ext) {
                uint32_t ext_count = 0;
                const VkResult res = enum_dev_ext(phys, nullptr, &ext_count, nullptr);
                if ((res == kVkSuccess || res == kVkIncomplete) && ext_count > 0 && ext_count <= 2048) {
                    snap_.device_extensions_count = ext_count;
                    std::vector<VkExtensionProperties> exts(ext_count);
                    if (enum_dev_ext(phys, nullptr, &ext_count, exts.data()) == kVkSuccess) {
                        for (const auto& ext : exts) {
                            const std::string_view name(ext.extensionName);
                            if (name == "VK_KHR_ray_tracing_pipeline") {
                                snap_.hw_rt_pipeline = true;
                                snap_.hw_ray_tracing = true;
                            } else if (name == "VK_KHR_ray_query") {
                                snap_.hw_ray_query = true;
                                snap_.hw_ray_tracing = true;
                            } else if (name == "VK_KHR_acceleration_structure") {
                                snap_.hw_acceleration_structure = true;
                            } else if (name == "VK_EXT_mesh_shader") {
                                snap_.hw_mesh_shaders = true;
                            } else if (name == "VK_KHR_fragment_shading_rate") {
                                snap_.hw_variable_rate_shading = true;
                            } else if (name == "VK_KHR_cooperative_matrix") {
                                snap_.hw_cooperative_matrix = true;
                            } else if (name == "VK_KHR_portability_subset") {
                                snap_.hw_portability_subset = true;
                            } else if (name == "VK_EXT_index_type_uint8" || name == "VK_KHR_index_type_uint8") {
                                snap_.hw_index_type_uint8 = true;
                            } else if (name == "VK_EXT_line_rasterization" || name == "VK_KHR_line_rasterization") {
                                snap_.hw_line_rasterization = true;
                            } else if (name == "VK_KHR_global_priority" || name == "VK_EXT_global_priority_query") {
                                snap_.hw_global_priority_query = true;
                            } else if (name == "VK_KHR_shader_float_controls2") {
                                snap_.hw_shader_float_controls2 = true;
                            } else if (name == "VK_KHR_maintenance5") {
                                snap_.hw_maintenance5 = true;
                            } else if (name == "VK_KHR_maintenance6") {
                                snap_.hw_maintenance6 = true;
                            } else if (name == "VK_KHR_maintenance7") {
                                snap_.hw_maintenance7 = true;
                            } else if (name == "VK_EXT_descriptor_buffer") {
                                snap_.hw_descriptor_buffer = true;
                            } else if (name == "VK_EXT_graphics_pipeline_library") {
                                snap_.hw_graphics_pipeline_library = true;
                            } else if (name == "VK_KHR_push_descriptor") {
                                snap_.hw_push_descriptors = true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (destroy_inst) {
        destroy_inst(instance, nullptr);
    }
    (void)::unsetenv("DISABLE_COREFLOW_VULKAN_LAYER");
}

void VulkanController::discover_gpu_kernel_surface(const std::string& root_override) noexcept {
    adrenoboost_node_.clear();
    bus_split_node_.clear();
    throttling_node_.clear();
    mali_policy_node_.clear();

    auto& baseline = BaselineStore::instance();
    (void)baseline.recover_pending();

    GpuKernelSurface surf{};
    const std::string prefix = root_override.empty() ? "" : root_override;

    // 1. Probe Qualcomm Adreno KGSL (/sys/class/kgsl/kgsl-3d0)
    const std::string kgsl_dir = prefix + "/sys/class/kgsl/kgsl-3d0";
    if (path_exists(kgsl_dir)) {
        surf.available = true;
        surf.vendor = GpuVendor::QualcommAdreno;
        const std::string gpu_model = read_first_line(kgsl_dir + "/gpu_model");
        surf.vendor_label = gpu_model.empty()
            ? "Qualcomm Adreno (KGSL)"
            : "Qualcomm " + gpu_model + " (KGSL)";
        surf.sysfs_root = kgsl_dir;

        const std::string gov_path = kgsl_dir + "/devfreq/governor";
        if (path_exists(gov_path)) {
            surf.active_governor = read_first_line(gov_path);
            surf.baseline_governor = surf.active_governor;
        }
        const uint32_t clk_hz = read_uint32_node(kgsl_dir + "/gpuclk", 0);
        const uint32_t max_hz = read_uint32_node(kgsl_dir + "/max_gpuclk", 0);
        surf.current_freq_mhz = clk_hz > 1000000U ? clk_hz / 1000000U : clk_hz / 1000U;
        surf.max_freq_mhz = max_hz > 1000000U ? max_hz / 1000000U : max_hz / 1000U;

        if (path_exists(kgsl_dir + "/devfreq/adrenoboost")) {
            adrenoboost_node_ = kgsl_dir + "/devfreq/adrenoboost";
            (void)baseline.capture(adrenoboost_node_);
            surf.adrenoboost_level = read_uint32_node(adrenoboost_node_, 0);
        }
        if (path_exists(kgsl_dir + "/bus_split")) {
            bus_split_node_ = kgsl_dir + "/bus_split";
            (void)baseline.capture(bus_split_node_);
            surf.bus_split_enabled = (read_uint32_node(bus_split_node_, 1) != 0);
        }
        if (path_exists(kgsl_dir + "/throttling")) {
            throttling_node_ = kgsl_dir + "/throttling";
            (void)baseline.capture(throttling_node_);
            surf.throttling_enabled = (read_uint32_node(throttling_node_, 1) != 0);
        }

        if (snap_.vendor == GpuVendor::Unknown) {
            snap_.vendor = GpuVendor::QualcommAdreno;
        }
        if (!snap_.vulkan_available && !gpu_model.empty()) {
            snap_.device_name = gpu_model;
        }
        snap_.gpu_surface = surf;
        return;
    }

    // 2. Probe Universal Devfreq GPU domains (ARM Mali / Immortalis, Samsung Xclipse, PowerVR, Panfrost)
    const std::string devfreq_root = prefix + "/sys/class/devfreq";
    if (path_exists(devfreq_root)) {
        std::error_code ec;
        for (fs::directory_iterator it(devfreq_root, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec)) {
            const std::string name = it->path().filename().string();
            const bool is_mali = (name.find("mali") != std::string::npos);
            const bool is_xclipse = (name.find("sgpu") != std::string::npos || name.find("xclipse") != std::string::npos);
            const bool is_pvr = (name.find("pvr") != std::string::npos || name.find("rogue") != std::string::npos);
            const bool is_gpu = is_mali || is_xclipse || is_pvr || (name.find("gpu") != std::string::npos);
            if (!is_gpu) continue;

            surf.available = true;
            surf.sysfs_root = it->path().string();
            if (is_mali) {
                surf.vendor = GpuVendor::ArmMali;
                surf.vendor_label = "ARM Mali / Immortalis (" + name + ")";
            } else if (is_xclipse) {
                surf.vendor = GpuVendor::SamsungXclipse;
                surf.vendor_label = "Samsung Xclipse RDNA (" + name + ")";
            } else if (is_pvr) {
                surf.vendor = GpuVendor::ImaginationPowerVR;
                surf.vendor_label = "Imagination PowerVR (" + name + ")";
            } else {
                surf.vendor = GpuVendor::GenericSysfs;
                surf.vendor_label = "Universal Devfreq GPU (" + name + ")";
            }

            if (path_exists(surf.sysfs_root + "/governor")) {
                surf.active_governor = read_first_line(surf.sysfs_root + "/governor");
                surf.baseline_governor = surf.active_governor;
            }
            const uint32_t cur_f = read_uint32_node(surf.sysfs_root + "/cur_freq", 0);
            const uint32_t max_f = read_uint32_node(surf.sysfs_root + "/max_freq", 0);
            const uint32_t min_f = read_uint32_node(surf.sysfs_root + "/min_freq", 0);
            surf.current_freq_mhz = cur_f > 1000000U ? cur_f / 1000000U : cur_f / 1000U;
            surf.max_freq_mhz = max_f > 1000000U ? max_f / 1000000U : max_f / 1000U;
            surf.min_freq_mhz = min_f > 1000000U ? min_f / 1000000U : min_f / 1000U;

            if (path_exists(surf.sysfs_root + "/device/power_policy")) {
                mali_policy_node_ = surf.sysfs_root + "/device/power_policy";
                (void)baseline.capture(mali_policy_node_);
            }

            if (snap_.vendor == GpuVendor::Unknown) {
                snap_.vendor = surf.vendor;
            }
            if (!snap_.vulkan_available) {
                snap_.device_name = surf.vendor_label;
            }
            snap_.gpu_surface = surf;
            return;
        }
    }

    snap_.gpu_surface = surf;
}

void VulkanController::recompute_effective_capabilities() noexcept {
    // VULKAN-01 Safety Containment:
    // Never falsify physical GPU API version, extension count, feature bits, or limits.
    // All modes report strictly truthful hardware capabilities from the native ICD.
    const VulkanLayerMode mode =
        config_.mode == VulkanLayerMode::FullCapabilityOverride
            ? VulkanLayerMode::SafeHardware
            : config_.mode;
    snap_.active_mode = mode;
    snap_.layer_active = (mode != VulkanLayerMode::Disabled) && snap_.layer_installed;
    snap_.skiavk_active = config_.enable_skiavk_threaded && (mode != VulkanLayerMode::Disabled);
    snap_.adaptive_vrs_active =
        config_.enable_adaptive_vrs && (mode != VulkanLayerMode::Disabled) && snap_.hw_variable_rate_shading;
    snap_.pipeline_cache_prewarm_active =
        config_.enable_pipeline_cache_prewarm && (mode != VulkanLayerMode::Disabled);

    missing_safe_exts_ = 0;
    missing_override_exts_ = 0;
    snap_.layer_implemented_extensions_count = 0;
    snap_.application_enabled_extensions_count = 0;

    snap_.effective_api_version = snap_.api_version;
    snap_.effective_device_extensions_count = snap_.device_extensions_count;
    snap_.ray_tracing = snap_.hw_ray_tracing;
    snap_.mesh_shaders = snap_.hw_mesh_shaders;
    snap_.variable_rate_shading = snap_.hw_variable_rate_shading;
    snap_.cooperative_matrix = snap_.hw_cooperative_matrix;
    snap_.rt_pipeline = snap_.hw_rt_pipeline;
    snap_.ray_query = snap_.hw_ray_query;
    snap_.acceleration_structure = snap_.hw_acceleration_structure;
    snap_.portability_subset = snap_.hw_portability_subset;
    snap_.shader_float64 = snap_.hw_shader_float64;
    snap_.index_type_uint8 = snap_.hw_index_type_uint8;
    snap_.line_rasterization = snap_.hw_line_rasterization;
    snap_.push_descriptors = snap_.hw_push_descriptors;
    snap_.global_priority_query = snap_.hw_global_priority_query;
    snap_.shader_float_controls2 = snap_.hw_shader_float_controls2;
    snap_.max_push_descriptors = snap_.hw_max_push_descriptors;
    snap_.maintenance5 = snap_.hw_maintenance5;
    snap_.maintenance6 = snap_.hw_maintenance6;
    snap_.maintenance7 = snap_.hw_maintenance7;
    snap_.descriptor_buffer = snap_.hw_descriptor_buffer;
    snap_.graphics_pipeline_library = snap_.hw_graphics_pipeline_library;
}

void VulkanController::sync_layer_state_file() const noexcept {
    if (state_dir_.empty()) return;
    (void)::mkdir(state_dir_.c_str(), 0700);

    const std::string target = state_dir_ + "/vulkan_layer.state";
    const std::string tmp = target + ".tmp";

    struct stat st{};
    if (::lstat(target.c_str(), &st) == 0 && S_ISLNK(st.st_mode)) {
        return;
    }

    UniqueFd fd(::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0644));
    if (!fd.valid()) return;

    std::ostringstream oss;
    oss << "mode=" << mode_name(snap_.active_mode) << "\n"
        << "skiavk=" << (snap_.skiavk_active ? 1 : 0) << "\n"
        << "adaptive_vrs=" << (snap_.adaptive_vrs_active ? 1 : 0) << "\n"
        << "pipeline_cache_prewarm=" << (snap_.pipeline_cache_prewarm_active ? 1 : 0) << "\n"
        << "max_push_descriptors=" << snap_.hw_max_push_descriptors << "\n";
    const std::string payload = oss.str();
    const ssize_t written = ::write(fd.get(), payload.data(), payload.size());
    if (written == static_cast<ssize_t>(payload.size())) {
        (void)::fchmod(fd.get(), 0644);
        (void)::fsync(fd.get());
        fd.reset();
        (void)::rename(tmp.c_str(), target.c_str());
    } else {
        fd.reset();
        (void)::unlink(tmp.c_str());
    }
}

bool VulkanController::apply_workload_policy(
    WorkloadState state,
    double capacity_scale,
    bool thermal_guard) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!snap_.gpu_surface.available) return true;

    auto& baseline = BaselineStore::instance();
    bool ok = true;

    // Qualcomm Adreno KGSL surface coordination
    if (snap_.gpu_surface.vendor == GpuVendor::QualcommAdreno) {
        if (!adrenoboost_node_.empty()) {
            uint32_t target_boost_num = 0;
            std::string target_boost = "0";
            if (!thermal_guard && state == WorkloadState::SustainedLoad && capacity_scale >= 0.85) {
                target_boost_num = 2;
                target_boost = "2";
            } else if (!thermal_guard && state == WorkloadState::Balanced && capacity_scale >= 0.90) {
                target_boost_num = 1;
                target_boost = "1";
            }
            if (baseline.apply_mutation(adrenoboost_node_, target_boost)) {
                snap_.gpu_surface.adrenoboost_level = target_boost_num;
            } else {
                ok = false;
            }
        }
        if (!bus_split_node_.empty()) {
            const std::string bus_val = "1";
            if (baseline.apply_mutation(bus_split_node_, bus_val)) {
                snap_.gpu_surface.bus_split_enabled = true;
            }
        }
        if (!throttling_node_.empty()) {
            // Always keep hardware thermal throttling safety enabled
            const std::string throt_val = "1";
            if (baseline.apply_mutation(throttling_node_, throt_val)) {
                snap_.gpu_surface.throttling_enabled = true;
            }
        }
    }

    // ARM Mali / Immortalis power policy surface coordination
    if (snap_.gpu_surface.vendor == GpuVendor::ArmMali && !mali_policy_node_.empty()) {
        const std::string cur_policy = read_first_line(mali_policy_node_);
        if (cur_policy == "coarse_demand" || cur_policy == "always_on") {
            (void)baseline.apply_mutation(mali_policy_node_, "coarse_demand");
        }
    }

    return ok;
}

} // namespace coreflow
