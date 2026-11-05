/**
 * @file vk_layer_coreflow.cpp
 * @brief Universal Multi-Vendor Vulkan 1.4 Implicit Layer (VK_LAYER_COREFLOW_universal_bridge)
 *        Implements Opsi A (Safe Hardware & Vulkan 1.4 Extension Bridge) and
 *        Opsi B (Full Capability Completion + vkCreateDevice Extension Guard).
 * @author mystivara-creator
 * @license MIT
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

#define CF_VK_EXPORT extern "C" __attribute__((visibility("default")))

namespace {

using VkBool32 = uint32_t;
using VkFlags = uint32_t;
using VkResult = int32_t;
using VkInstance = void*;
using VkPhysicalDevice = void*;
using VkDevice = void*;
using PFN_vkVoidFunction = void (*)();

constexpr VkResult kVkSuccess = 0;
constexpr VkResult kVkIncomplete = 5;
constexpr VkResult kVkErrorInitializationFailed = -3;
constexpr VkBool32 kVkTrue = 1U;
constexpr size_t kVkMaxExtensionNameSize = 256;
constexpr size_t kVkMaxDescriptionSize = 256;

// Khronos Structure Types
constexpr int32_t kVkStructureTypeLoaderInstanceCreateInfo = 47;
constexpr int32_t kVkStructureTypeLoaderDeviceCreateInfo = 48;
constexpr int32_t kVkStructureTypePhysicalDeviceFeatures2 = 1000059000;
constexpr int32_t kVkStructureTypePhysicalDeviceProperties2 = 1000059001;
constexpr int32_t kVkStructureTypePhysicalDeviceVulkan11Features = 49;
constexpr int32_t kVkStructureTypePhysicalDeviceVulkan12Features = 51;
constexpr int32_t kVkStructureTypePhysicalDeviceVulkan13Features = 53;
constexpr int32_t kVkStructureTypePhysicalDeviceVulkan14Features = 55;
constexpr int32_t kVkStructureTypePhysicalDevicePushDescriptorPropertiesKhr = 1000080000;

// Extension Feature Structure Types (Khronos vk.xml values)
constexpr int32_t kVkStructureTypePhysicalDeviceLineRasterizationFeaturesExt = 1000259000;
constexpr int32_t kVkStructureTypePhysicalDeviceIndexTypeUint8FeaturesExt = 1000265000;
constexpr int32_t kVkStructureTypePhysicalDeviceFragmentShadingRateFeaturesKhr = 1000226000;
constexpr int32_t kVkStructureTypePhysicalDeviceRayTracingPipelineFeaturesKhr = 1000347000;
constexpr int32_t kVkStructureTypePhysicalDeviceRayQueryFeaturesKhr = 1000348001;
constexpr int32_t kVkStructureTypePhysicalDeviceAccelerationStructureFeaturesKhr = 1000150013;
constexpr int32_t kVkStructureTypePhysicalDeviceMeshShaderFeaturesExt = 1000328000;
constexpr int32_t kVkStructureTypePhysicalDeviceCooperativeMatrixFeaturesKhr = 1000506000;
constexpr int32_t kVkStructureTypePhysicalDeviceGlobalPriorityQueryFeaturesKhr = 1000388000;
constexpr int32_t kVkStructureTypePhysicalDeviceShaderFloatControls2FeaturesKhr = 1000528000;
constexpr int32_t kVkStructureTypePhysicalDeviceMaintenance5FeaturesKhr = 1000470000;
constexpr int32_t kVkStructureTypePhysicalDeviceMaintenance6FeaturesKhr = 1000545000;
constexpr int32_t kVkStructureTypePhysicalDeviceMaintenance7FeaturesKhr = 1000562000;
constexpr int32_t kVkStructureTypePhysicalDeviceDescriptorBufferFeaturesExt = 1000316000;
constexpr int32_t kVkStructureTypePhysicalDeviceGraphicsPipelineLibraryFeaturesExt = 1000320000;

constexpr uint32_t make_vk_version(uint32_t major, uint32_t minor, uint32_t patch) noexcept {
    return (major << 22U) | (minor << 12U) | patch;
}

struct VkBaseOutStructure {
    int32_t sType;
    VkBaseOutStructure* pNext;
};

struct VkLayerProperties {
    char layerName[kVkMaxExtensionNameSize];
    uint32_t specVersion;
    uint32_t implementationVersion;
    char description[kVkMaxDescriptionSize];
};

struct VkExtensionProperties {
    char extensionName[kVkMaxExtensionNameSize];
    uint32_t specVersion;
};

using PFN_vkGetInstanceProcAddr = PFN_vkVoidFunction (*)(VkInstance, const char*);
using PFN_vkGetDeviceProcAddr = PFN_vkVoidFunction (*)(VkDevice, const char*);

struct VkLayerInstanceLink {
    VkLayerInstanceLink* pNext;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
    PFN_vkVoidFunction pfnNextGetPhysicalDeviceProcAddr;
};

struct VkLayerDeviceLink {
    VkLayerDeviceLink* pNext;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr pfnNextGetDeviceProcAddr;
};

struct VkLayerInstanceCreateInfo {
    int32_t sType;
    const void* pNext;
    int32_t function; // 0 == VK_LAYER_LINK_INFO
    union {
        VkLayerInstanceLink* pLayerInfo;
    } u;
};

struct VkLayerDeviceCreateInfo {
    int32_t sType;
    const void* pNext;
    int32_t function; // 0 == VK_LAYER_LINK_INFO
    union {
        VkLayerDeviceLink* pLayerInfo;
    } u;
};

struct VkInstanceCreateInfo {
    int32_t sType;
    const void* pNext;
    VkFlags flags;
    const void* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkDeviceCreateInfo {
    int32_t sType;
    const void* pNext;
    VkFlags flags;
    uint32_t queueCreateInfoCount;
    const void* pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    const void* pEnabledFeatures;
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

struct VkPhysicalDeviceFeatures2 {
    int32_t sType;
    void* pNext;
    VkPhysicalDeviceFeatures features;
};

struct VkPhysicalDevicePropertiesPrefix {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
};

struct VkPhysicalDeviceProperties2Prefix {
    int32_t sType;
    void* pNext;
    VkPhysicalDevicePropertiesPrefix properties;
};

struct VkPhysicalDevicePushDescriptorPropertiesKHR {
    int32_t sType;
    void* pNext;
    uint32_t maxPushDescriptors;
};

struct VkNegotiateLayerInterface {
    int32_t sType;
    void* pNext;
    uint32_t loaderLayerInterfaceVersion;
    PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr;
    PFN_vkVoidFunction pfnGetPhysicalDeviceProcAddr;
};

using PFN_vkCreateInstance = VkResult (*)(const VkInstanceCreateInfo*, const void*, VkInstance*);
using PFN_vkDestroyInstance = void (*)(VkInstance, const void*);
using PFN_vkCreateDevice = VkResult (*)(VkPhysicalDevice, const VkDeviceCreateInfo*, const void*, VkDevice*);
using PFN_vkDestroyDevice = void (*)(VkDevice, const void*);
using PFN_vkGetPhysicalDeviceFeatures = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
using PFN_vkGetPhysicalDeviceFeatures2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures2*);
using PFN_vkGetPhysicalDeviceProperties2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties2Prefix*);
using PFN_vkEnumerateDeviceExtensionProperties = VkResult (*)(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*);

enum class LayerMode : uint8_t {
    Disabled = 0,
    SafeHardware = 1,
    FullCapabilityOverride = 2
};

struct LayerRuntimeConfig {
    LayerMode mode{LayerMode::SafeHardware};
    uint32_t max_push_descriptors{32};
    bool adaptive_vrs{true};
    bool pipeline_cache_prewarm{true};
};

LayerRuntimeConfig read_layer_runtime_config() noexcept {
    LayerRuntimeConfig cfg{};
    const char* env_disable = ::getenv("DISABLE_COREFLOW_VULKAN_LAYER");
    if (env_disable && std::strcmp(env_disable, "1") == 0) {
        cfg.mode = LayerMode::Disabled;
        return cfg;
    }

    const int fd = ::open("/data/adb/coreflow/vulkan_layer.state", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return cfg;

    char buf[512];
    const ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    (void)::close(fd);
    if (n <= 0) return cfg;
    buf[n] = '\0';

    std::string_view content(buf, static_cast<size_t>(n));
    size_t pos = 0;
    while (pos < content.size()) {
        const size_t eol = content.find('\n', pos);
        const std::string_view line = eol == std::string_view::npos ? content.substr(pos) : content.substr(pos, eol - pos);
        pos = eol == std::string_view::npos ? content.size() : eol + 1;

        if (line.rfind("mode=", 0) == 0) {
            const std::string_view val = line.substr(5);
            if (val == "disabled") cfg.mode = LayerMode::Disabled;
            else if (val == "full_capability_override") cfg.mode = LayerMode::FullCapabilityOverride;
            else cfg.mode = LayerMode::SafeHardware;
        } else if (line.rfind("max_push_descriptors=", 0) == 0) {
            const std::string_view val = line.substr(21);
            if (val == "64") cfg.max_push_descriptors = 64;
            else cfg.max_push_descriptors = 32;
        } else if (line.rfind("adaptive_vrs=", 0) == 0) {
            cfg.adaptive_vrs = (line.substr(13) != "0");
        } else if (line.rfind("pipeline_cache_prewarm=", 0) == 0) {
            cfg.pipeline_cache_prewarm = (line.substr(23) != "0");
        }
    }
    return cfg;
}

std::mutex g_dispatch_mutex;
PFN_vkGetInstanceProcAddr g_next_gipa = nullptr;
PFN_vkGetDeviceProcAddr g_next_gdpa = nullptr;
VkInstance g_last_instance = nullptr;

// Safe Vulkan 1.4 & pipeline extensions (Opsi A + Opsi B)
constexpr std::array<VkExtensionProperties, 9> kSafeBridgedExtensions = {{
    {"VK_KHR_push_descriptor", 2},
    {"VK_EXT_index_type_uint8", 1},
    {"VK_KHR_index_type_uint8", 1},
    {"VK_EXT_line_rasterization", 1},
    {"VK_KHR_line_rasterization", 1},
    {"VK_KHR_global_priority", 1},
    {"VK_EXT_global_priority_query", 1},
    {"VK_KHR_shader_float_controls2", 1},
    {"VK_KHR_maintenance7", 1},
}};

// Full Capability Override extensions (Opsi B only)
constexpr std::array<VkExtensionProperties, 7> kOverrideExtensions = {{
    {"VK_EXT_descriptor_buffer", 1},
    {"VK_EXT_graphics_pipeline_library", 1},
    {"VK_KHR_acceleration_structure", 13},
    {"VK_KHR_ray_tracing_pipeline", 1},
    {"VK_KHR_ray_query", 1},
    {"VK_EXT_mesh_shader", 1},
    {"VK_KHR_cooperative_matrix", 2},
}};

void patch_feature_chain(VkBaseOutStructure* chain, const LayerRuntimeConfig& cfg) noexcept {
    if (cfg.mode == LayerMode::Disabled) return;

    for (VkBaseOutStructure* cur = chain; cur != nullptr; cur = cur->pNext) {
        auto* raw_bools = reinterpret_cast<VkBool32*>(reinterpret_cast<char*>(cur) + sizeof(VkBaseOutStructure));
        switch (cur->sType) {
            case kVkStructureTypePhysicalDeviceIndexTypeUint8FeaturesExt:
            case kVkStructureTypePhysicalDeviceGlobalPriorityQueryFeaturesKhr:
            case kVkStructureTypePhysicalDeviceShaderFloatControls2FeaturesKhr:
            case kVkStructureTypePhysicalDeviceMaintenance5FeaturesKhr:
            case kVkStructureTypePhysicalDeviceMaintenance6FeaturesKhr:
            case kVkStructureTypePhysicalDeviceMaintenance7FeaturesKhr:
                raw_bools[0] = kVkTrue;
                break;

            case kVkStructureTypePhysicalDeviceGraphicsPipelineLibraryFeaturesExt:
                if (cfg.pipeline_cache_prewarm) {
                    raw_bools[0] = kVkTrue;
                }
                break;

            case kVkStructureTypePhysicalDeviceLineRasterizationFeaturesExt:
                // rectangularLines, bresenhamLines, smoothLines
                raw_bools[0] = kVkTrue;
                raw_bools[1] = kVkTrue;
                raw_bools[2] = kVkTrue;
                break;

            case kVkStructureTypePhysicalDeviceDescriptorBufferFeaturesExt:
                // descriptorBuffer, descriptorBufferCaptureReplay, descriptorBufferImageLayoutIgnored, descriptorBufferPushDescriptors
                raw_bools[0] = kVkTrue;
                raw_bools[3] = kVkTrue;
                break;

            case kVkStructureTypePhysicalDeviceFragmentShadingRateFeaturesKhr:
                if (cfg.adaptive_vrs) {
                    // pipelineFragmentShadingRate, primitiveFragmentShadingRate, attachmentFragmentShadingRate
                    raw_bools[0] = kVkTrue;
                    raw_bools[1] = kVkTrue;
                    raw_bools[2] = kVkTrue;
                }
                break;

            case kVkStructureTypePhysicalDeviceVulkan14Features:
                // Vulkan 1.4 consolidated feature struct:
                // [0] globalPriorityQuery, [3] shaderFloatControls2, [5..7] rectangular/bresenham/smoothLines,
                // [13] indexTypeUint8, [15] maintenance5, [16] maintenance6, [20] pushDescriptor
                raw_bools[0] = kVkTrue;
                raw_bools[3] = kVkTrue;
                raw_bools[5] = kVkTrue;
                raw_bools[6] = kVkTrue;
                raw_bools[7] = kVkTrue;
                raw_bools[13] = kVkTrue;
                raw_bools[15] = kVkTrue;
                raw_bools[16] = kVkTrue;
                raw_bools[20] = kVkTrue;
                break;

            case kVkStructureTypePhysicalDeviceRayTracingPipelineFeaturesKhr:
            case kVkStructureTypePhysicalDeviceRayQueryFeaturesKhr:
            case kVkStructureTypePhysicalDeviceAccelerationStructureFeaturesKhr:
            case kVkStructureTypePhysicalDeviceMeshShaderFeaturesExt:
            case kVkStructureTypePhysicalDeviceCooperativeMatrixFeaturesKhr:
                if (cfg.mode == LayerMode::FullCapabilityOverride) {
                    raw_bools[0] = kVkTrue;
                }
                break;

            default:
                break;
        }
    }
}

VkResult layer_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const void* pAllocator,
    VkInstance* pInstance) {
    if (!pCreateInfo || !pInstance) return kVkErrorInitializationFailed;

    auto* chain = reinterpret_cast<VkLayerInstanceCreateInfo*>(const_cast<void*>(pCreateInfo->pNext));
    while (chain && !(chain->sType == kVkStructureTypeLoaderInstanceCreateInfo && chain->function == 0)) {
        chain = reinterpret_cast<VkLayerInstanceCreateInfo*>(const_cast<void*>(chain->pNext));
    }
    if (!chain || !chain->u.pLayerInfo) return kVkErrorInitializationFailed;

    PFN_vkGetInstanceProcAddr next_gipa = chain->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;

    auto create_func = reinterpret_cast<PFN_vkCreateInstance>(next_gipa(nullptr, "vkCreateInstance"));
    if (!create_func) return kVkErrorInitializationFailed;

    const VkResult res = create_func(pCreateInfo, pAllocator, pInstance);
    if (res == kVkSuccess) {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_next_gipa = next_gipa;
        g_last_instance = *pInstance;
    }
    return res;
}

void layer_vkDestroyInstance(VkInstance instance, const void* pAllocator) {
    PFN_vkGetInstanceProcAddr next_gipa = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gipa = g_next_gipa;
    }
    if (!next_gipa) return;
    auto destroy_func = reinterpret_cast<PFN_vkDestroyInstance>(next_gipa(instance, "vkDestroyInstance"));
    if (destroy_func) destroy_func(instance, pAllocator);
}

void layer_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) {
    PFN_vkGetInstanceProcAddr next_gipa = nullptr;
    VkInstance inst = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gipa = g_next_gipa;
        inst = g_last_instance;
    }
    if (!next_gipa || !pFeatures) return;
    auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(next_gipa(inst, "vkGetPhysicalDeviceFeatures"));
    if (fn) fn(physicalDevice, pFeatures);

    const LayerRuntimeConfig cfg = read_layer_runtime_config();
    if (cfg.mode == LayerMode::FullCapabilityOverride) {
        pFeatures->shaderFloat64 = kVkTrue;
    }
}

void layer_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures) {
    PFN_vkGetInstanceProcAddr next_gipa = nullptr;
    VkInstance inst = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gipa = g_next_gipa;
        inst = g_last_instance;
    }
    if (!next_gipa || !pFeatures) return;
    auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(next_gipa(inst, "vkGetPhysicalDeviceFeatures2"));
    if (!fn) {
        fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(next_gipa(inst, "vkGetPhysicalDeviceFeatures2KHR"));
    }
    if (fn) fn(physicalDevice, pFeatures);

    const LayerRuntimeConfig cfg = read_layer_runtime_config();
    if (cfg.mode == LayerMode::FullCapabilityOverride) {
        pFeatures->features.shaderFloat64 = kVkTrue;
    }
    patch_feature_chain(reinterpret_cast<VkBaseOutStructure*>(pFeatures->pNext), cfg);
}

void layer_vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2Prefix* pProperties) {
    PFN_vkGetInstanceProcAddr next_gipa = nullptr;
    VkInstance inst = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gipa = g_next_gipa;
        inst = g_last_instance;
    }
    if (!next_gipa || !pProperties) return;
    auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(next_gipa(inst, "vkGetPhysicalDeviceProperties2"));
    if (!fn) {
        fn = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(next_gipa(inst, "vkGetPhysicalDeviceProperties2KHR"));
    }
    if (fn) fn(physicalDevice, pProperties);

    const LayerRuntimeConfig cfg = read_layer_runtime_config();
    if (cfg.mode == LayerMode::Disabled) return;

    if (pProperties->properties.apiVersion < make_vk_version(1, 4, 303)) {
        pProperties->properties.apiVersion = make_vk_version(1, 4, 303);
    }

    for (auto* cur = reinterpret_cast<VkBaseOutStructure*>(pProperties->pNext); cur != nullptr; cur = cur->pNext) {
        if (cur->sType == kVkStructureTypePhysicalDevicePushDescriptorPropertiesKhr) {
            auto* push = reinterpret_cast<VkPhysicalDevicePushDescriptorPropertiesKHR*>(cur);
            if (push->maxPushDescriptors < cfg.max_push_descriptors) {
                push->maxPushDescriptors = cfg.max_push_descriptors;
            }
        }
    }
}

VkResult layer_vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) {
    if (pLayerName && std::strcmp(pLayerName, "VK_LAYER_COREFLOW_universal_bridge") == 0) {
        if (!pPropertyCount) return kVkErrorInitializationFailed;
        *pPropertyCount = 0;
        return kVkSuccess;
    }

    PFN_vkGetInstanceProcAddr next_gipa = nullptr;
    VkInstance inst = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gipa = g_next_gipa;
        inst = g_last_instance;
    }
    if (!next_gipa || !physicalDevice) return kVkErrorInitializationFailed;

    auto fn = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        next_gipa(inst, "vkEnumerateDeviceExtensionProperties"));
    if (!fn) return kVkErrorInitializationFailed;

    const LayerRuntimeConfig cfg = read_layer_runtime_config();
    if (cfg.mode == LayerMode::Disabled) {
        return fn(physicalDevice, pLayerName, pPropertyCount, pProperties);
    }

    uint32_t hw_count = 0;
    VkResult res = fn(physicalDevice, pLayerName, &hw_count, nullptr);
    if (res != kVkSuccess && res != kVkIncomplete) return res;

    std::vector<VkExtensionProperties> merged(hw_count);
    if (hw_count > 0) {
        res = fn(physicalDevice, pLayerName, &hw_count, merged.data());
        if (res != kVkSuccess && res != kVkIncomplete) return res;
        merged.resize(hw_count);
    }

    auto append_unique = [&merged](const VkExtensionProperties& ext) {
        for (const auto& existing : merged) {
            if (std::strcmp(existing.extensionName, ext.extensionName) == 0) return;
        }
        merged.push_back(ext);
    };

    for (const auto& ext : kSafeBridgedExtensions) {
        append_unique(ext);
    }
    if (cfg.mode == LayerMode::FullCapabilityOverride) {
        for (const auto& ext : kOverrideExtensions) {
            append_unique(ext);
        }
    }

    if (!pProperties) {
        *pPropertyCount = static_cast<uint32_t>(merged.size());
        return kVkSuccess;
    }

    const uint32_t to_copy = std::min(*pPropertyCount, static_cast<uint32_t>(merged.size()));
    for (uint32_t i = 0; i < to_copy; ++i) {
        pProperties[i] = merged[i];
    }
    *pPropertyCount = to_copy;
    return to_copy < merged.size() ? kVkIncomplete : kVkSuccess;
}

VkResult layer_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const void* pAllocator,
    VkDevice* pDevice) {
    if (!pCreateInfo || !pDevice) return kVkErrorInitializationFailed;

    auto* chain = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(pCreateInfo->pNext));
    while (chain && !(chain->sType == kVkStructureTypeLoaderDeviceCreateInfo && chain->function == 0)) {
        chain = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(chain->pNext));
    }
    if (!chain || !chain->u.pLayerInfo) return kVkErrorInitializationFailed;

    PFN_vkGetInstanceProcAddr next_gipa = chain->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr next_gdpa = chain->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;

    VkInstance inst = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        inst = g_last_instance;
    }

    auto create_dev = reinterpret_cast<PFN_vkCreateDevice>(next_gipa(inst, "vkCreateDevice"));
    if (!create_dev) return kVkErrorInitializationFailed;

    // Filter ppEnabledExtensionNames against actual hardware extensions so the vendor ICD
    // never rejects vkCreateDevice with VK_ERROR_EXTENSION_NOT_PRESENT.
    VkDeviceCreateInfo sanitized = *pCreateInfo;
    std::vector<const char*> allowed_exts;

    auto enum_ext = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        next_gipa(inst, "vkEnumerateDeviceExtensionProperties"));
    if (enum_ext && pCreateInfo->enabledExtensionCount > 0 && pCreateInfo->ppEnabledExtensionNames) {
        uint32_t hw_count = 0;
        if (enum_ext(physicalDevice, nullptr, &hw_count, nullptr) == kVkSuccess && hw_count > 0) {
            std::vector<VkExtensionProperties> hw_exts(hw_count);
            if (enum_ext(physicalDevice, nullptr, &hw_count, hw_exts.data()) == kVkSuccess) {
                for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
                    const char* req = pCreateInfo->ppEnabledExtensionNames[i];
                    if (!req) continue;
                    for (const auto& hw : hw_exts) {
                        if (std::strcmp(hw.extensionName, req) == 0) {
                            allowed_exts.push_back(req);
                            break;
                        }
                    }
                }
                sanitized.enabledExtensionCount = static_cast<uint32_t>(allowed_exts.size());
                sanitized.ppEnabledExtensionNames = allowed_exts.empty() ? nullptr : allowed_exts.data();
            }
        }
    }

    const VkResult res = create_dev(physicalDevice, &sanitized, pAllocator, pDevice);
    if (res == kVkSuccess) {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_next_gdpa = next_gdpa;
    }
    return res;
}

void layer_vkDestroyDevice(VkDevice device, const void* pAllocator) {
    PFN_vkGetDeviceProcAddr next_gdpa = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gdpa = g_next_gdpa;
    }
    if (!next_gdpa) return;
    auto fn = reinterpret_cast<PFN_vkDestroyDevice>(next_gdpa(device, "vkDestroyDevice"));
    if (fn) fn(device, pAllocator);
}

} // namespace

CF_VK_EXPORT VkResult vkEnumerateInstanceLayerProperties(
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties) {
    if (!pPropertyCount) return kVkErrorInitializationFailed;
    if (!pProperties) {
        *pPropertyCount = 1;
        return kVkSuccess;
    }
    if (*pPropertyCount < 1) return kVkIncomplete;

    std::memset(&pProperties[0], 0, sizeof(VkLayerProperties));
    std::snprintf(pProperties[0].layerName, kVkMaxExtensionNameSize, "VK_LAYER_COREFLOW_universal_bridge");
    pProperties[0].specVersion = make_vk_version(1, 4, 303);
    pProperties[0].implementationVersion = 1;
    std::snprintf(
        pProperties[0].description,
        kVkMaxDescriptionSize,
        "CoreFlow Universal Vulkan 1.4 Capability & Extension Bridge Layer");
    *pPropertyCount = 1;
    return kVkSuccess;
}

CF_VK_EXPORT VkResult vkEnumerateDeviceLayerProperties(
    VkPhysicalDevice,
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties) {
    return vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

CF_VK_EXPORT VkResult vkEnumerateInstanceExtensionProperties(
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties*) {
    if (pLayerName && std::strcmp(pLayerName, "VK_LAYER_COREFLOW_universal_bridge") == 0) {
        if (!pPropertyCount) return kVkErrorInitializationFailed;
        *pPropertyCount = 0;
        return kVkSuccess;
    }
    return kVkErrorInitializationFailed;
}

CF_VK_EXPORT VkResult vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) {
    return layer_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

CF_VK_EXPORT PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    if (!pName) return nullptr;
    if (std::strcmp(pName, "vkGetDeviceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkGetDeviceProcAddr);
    }
    if (std::strcmp(pName, "vkDestroyDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkDestroyDevice);
    }

    PFN_vkGetDeviceProcAddr next_gdpa = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gdpa = g_next_gdpa;
    }
    return next_gdpa ? next_gdpa(device, pName) : nullptr;
}

CF_VK_EXPORT PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (!pName) return nullptr;
    if (std::strcmp(pName, "vkGetInstanceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkGetInstanceProcAddr);
    }
    if (std::strcmp(pName, "vkCreateInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreateInstance);
    }
    if (std::strcmp(pName, "vkDestroyInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkDestroyInstance);
    }
    if (std::strcmp(pName, "vkCreateDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreateDevice);
    }
    if (std::strcmp(pName, "vkGetPhysicalDeviceFeatures") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetPhysicalDeviceFeatures);
    }
    if (std::strcmp(pName, "vkGetPhysicalDeviceFeatures2") == 0 ||
        std::strcmp(pName, "vkGetPhysicalDeviceFeatures2KHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetPhysicalDeviceFeatures2);
    }
    if (std::strcmp(pName, "vkGetPhysicalDeviceProperties2") == 0 ||
        std::strcmp(pName, "vkGetPhysicalDeviceProperties2KHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetPhysicalDeviceProperties2);
    }
    if (std::strcmp(pName, "vkEnumerateDeviceExtensionProperties") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkEnumerateDeviceExtensionProperties);
    }
    if (std::strcmp(pName, "vkEnumerateInstanceLayerProperties") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkEnumerateInstanceLayerProperties);
    }
    if (std::strcmp(pName, "vkEnumerateDeviceLayerProperties") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkEnumerateDeviceLayerProperties);
    }
    if (std::strcmp(pName, "vkEnumerateInstanceExtensionProperties") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkEnumerateInstanceExtensionProperties);
    }

    PFN_vkGetInstanceProcAddr next_gipa = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        next_gipa = g_next_gipa;
    }
    return next_gipa ? next_gipa(instance, pName) : nullptr;
}

CF_VK_EXPORT VkResult vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* pVersionStruct) {
    if (!pVersionStruct) return kVkErrorInitializationFailed;
    if (pVersionStruct->loaderLayerInterfaceVersion > 2) {
        pVersionStruct->loaderLayerInterfaceVersion = 2;
    }
    pVersionStruct->pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    pVersionStruct->pfnGetDeviceProcAddr = &vkGetDeviceProcAddr;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
    return kVkSuccess;
}
