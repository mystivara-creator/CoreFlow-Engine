/**
 * @file vk_layer_coreflow.cpp
 * @brief Truthful Pass-Through Vulkan Implicit Layer (VK_LAYER_COREFLOW_universal_bridge)
 *        Implements per-instance and per-device dispatch tables (VULKAN-02) and strictly
 *        preserves physical ICD API versions, extensions, feature chains, and error
 *        semantics without falsification or silent extension stripping (VULKAN-01).
 * @author mystivara-creator
 * @license MIT
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <string_view>
#include <unistd.h>
#include <unordered_map>
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
constexpr size_t kVkMaxExtensionNameSize = 256;
constexpr size_t kVkMaxDescriptionSize = 256;

// Khronos Loader-Layer Structure Types
constexpr int32_t kVkStructureTypeLoaderInstanceCreateInfo = 47;
constexpr int32_t kVkStructureTypeLoaderDeviceCreateInfo = 48;

constexpr uint32_t make_vk_version(uint32_t major, uint32_t minor, uint32_t patch) noexcept {
    return (major << 22U) | (minor << 12U) | patch;
}

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
    VkBool32 features[55];
};

struct VkPhysicalDeviceFeatures2 {
    int32_t sType;
    void* pNext;
    VkPhysicalDeviceFeatures features;
};

struct VkPhysicalDeviceLimits {
    uint8_t opaque[504];
};

struct VkPhysicalDeviceSparseProperties {
    VkBool32 words[5];
};

struct VkPhysicalDeviceProperties {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t deviceType;
    char deviceName[256];
    uint8_t pipelineCacheUUID[16];
    VkPhysicalDeviceLimits limits;
    VkPhysicalDeviceSparseProperties sparseProperties;
};

struct VkPhysicalDeviceProperties2 {
    int32_t sType;
    void* pNext;
    VkPhysicalDeviceProperties properties;
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
using PFN_vkEnumeratePhysicalDevices = VkResult (*)(VkInstance, uint32_t*, VkPhysicalDevice*);
using PFN_vkCreateDevice = VkResult (*)(VkPhysicalDevice, const VkDeviceCreateInfo*, const void*, VkDevice*);
using PFN_vkDestroyDevice = void (*)(VkDevice, const void*);
using PFN_vkGetPhysicalDeviceFeatures = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
using PFN_vkGetPhysicalDeviceFeatures2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures2*);
using PFN_vkGetPhysicalDeviceProperties = void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
using PFN_vkGetPhysicalDeviceProperties2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties2*);
using PFN_vkEnumerateDeviceExtensionProperties = VkResult (*)(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*);

struct InstanceDispatchEntry {
    PFN_vkGetInstanceProcAddr next_gipa{nullptr};
    PFN_vkVoidFunction next_gpdpa{nullptr};
};

struct DeviceDispatchEntry {
    PFN_vkGetDeviceProcAddr next_gdpa{nullptr};
    PFN_vkGetInstanceProcAddr next_gipa{nullptr};
    VkInstance instance{nullptr};
};

std::mutex g_dispatch_mutex;
std::unordered_map<VkInstance, InstanceDispatchEntry> g_instance_dispatch;
std::unordered_map<VkPhysicalDevice, VkInstance> g_phys_to_instance;
std::unordered_map<VkDevice, DeviceDispatchEntry> g_device_dispatch;

bool lookup_instance_dispatch(VkInstance instance, InstanceDispatchEntry& out) noexcept {
    std::lock_guard<std::mutex> lock(g_dispatch_mutex);
    auto it = g_instance_dispatch.find(instance);
    if (it != g_instance_dispatch.end()) {
        out = it->second;
        return out.next_gipa != nullptr;
    }
    if (g_instance_dispatch.size() == 1) {
        out = g_instance_dispatch.begin()->second;
        return out.next_gipa != nullptr;
    }
    return false;
}

bool lookup_phys_dispatch(VkPhysicalDevice phys, VkInstance& inst_out, InstanceDispatchEntry& out) noexcept {
    std::lock_guard<std::mutex> lock(g_dispatch_mutex);
    auto pit = g_phys_to_instance.find(phys);
    if (pit != g_phys_to_instance.end()) {
        auto iit = g_instance_dispatch.find(pit->second);
        if (iit != g_instance_dispatch.end()) {
            inst_out = pit->second;
            out = iit->second;
            return out.next_gipa != nullptr;
        }
    }
    if (g_instance_dispatch.size() == 1) {
        inst_out = g_instance_dispatch.begin()->first;
        out = g_instance_dispatch.begin()->second;
        return out.next_gipa != nullptr;
    }
    return false;
}

bool lookup_device_dispatch(VkDevice device, DeviceDispatchEntry& out) noexcept {
    std::lock_guard<std::mutex> lock(g_dispatch_mutex);
    auto it = g_device_dispatch.find(device);
    if (it != g_device_dispatch.end()) {
        out = it->second;
        return out.next_gdpa != nullptr;
    }
    if (g_device_dispatch.size() == 1) {
        out = g_device_dispatch.begin()->second;
        return out.next_gdpa != nullptr;
    }
    return false;
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
    PFN_vkVoidFunction next_gpdpa = chain->u.pLayerInfo->pfnNextGetPhysicalDeviceProcAddr;
    if (!next_gipa) return kVkErrorInitializationFailed;
    chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;

    auto create_func = reinterpret_cast<PFN_vkCreateInstance>(next_gipa(nullptr, "vkCreateInstance"));
    if (!create_func) return kVkErrorInitializationFailed;

    const VkResult res = create_func(pCreateInfo, pAllocator, pInstance);
    if (res == kVkSuccess && *pInstance != nullptr) {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_instance_dispatch[*pInstance] = InstanceDispatchEntry{next_gipa, next_gpdpa};

        // Pre-bind physical devices for this instance if enumeration is available
        auto enum_phys = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
            next_gipa(*pInstance, "vkEnumeratePhysicalDevices"));
        if (enum_phys) {
            uint32_t count = 0;
            if (enum_phys(*pInstance, &count, nullptr) == kVkSuccess && count > 0 && count <= 64) {
                std::vector<VkPhysicalDevice> devs(count, nullptr);
                if (enum_phys(*pInstance, &count, devs.data()) == kVkSuccess) {
                    for (uint32_t i = 0; i < count; ++i) {
                        if (devs[i] != nullptr) {
                            g_phys_to_instance[devs[i]] = *pInstance;
                        }
                    }
                }
            }
        }
    }
    return res;
}

void layer_vkDestroyInstance(VkInstance instance, const void* pAllocator) {
    InstanceDispatchEntry entry{};
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        auto it = g_instance_dispatch.find(instance);
        if (it != g_instance_dispatch.end()) {
            entry = it->second;
            g_instance_dispatch.erase(it);
        }
        for (auto pit = g_phys_to_instance.begin(); pit != g_phys_to_instance.end();) {
            if (pit->second == instance) {
                pit = g_phys_to_instance.erase(pit);
            } else {
                ++pit;
            }
        }
    }
    if (!entry.next_gipa) return;
    auto destroy_func = reinterpret_cast<PFN_vkDestroyInstance>(entry.next_gipa(instance, "vkDestroyInstance"));
    if (destroy_func) destroy_func(instance, pAllocator);
}

VkResult layer_vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices) {
    InstanceDispatchEntry entry{};
    if (!lookup_instance_dispatch(instance, entry)) return kVkErrorInitializationFailed;
    auto fn = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        entry.next_gipa(instance, "vkEnumeratePhysicalDevices"));
    if (!fn) return kVkErrorInitializationFailed;

    const VkResult res = fn(instance, pPhysicalDeviceCount, pPhysicalDevices);
    if ((res == kVkSuccess || res == kVkIncomplete) && pPhysicalDevices && pPhysicalDeviceCount) {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        for (uint32_t i = 0; i < *pPhysicalDeviceCount; ++i) {
            if (pPhysicalDevices[i] != nullptr) {
                g_phys_to_instance[pPhysicalDevices[i]] = instance;
            }
        }
    }
    return res;
}

void layer_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) {
    if (!physicalDevice || !pFeatures) return;
    VkInstance inst = nullptr;
    InstanceDispatchEntry entry{};
    if (!lookup_phys_dispatch(physicalDevice, inst, entry)) return;

    auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(
        entry.next_gipa(inst, "vkGetPhysicalDeviceFeatures"));
    if (fn) {
        // Strictly pass-through hardware features without falsifying shaderFloat64 or any ALU bit
        fn(physicalDevice, pFeatures);
    }
}

void layer_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures) {
    if (!physicalDevice || !pFeatures) return;
    VkInstance inst = nullptr;
    InstanceDispatchEntry entry{};
    if (!lookup_phys_dispatch(physicalDevice, inst, entry)) return;

    auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        entry.next_gipa(inst, "vkGetPhysicalDeviceFeatures2"));
    if (!fn) {
        fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
            entry.next_gipa(inst, "vkGetPhysicalDeviceFeatures2KHR"));
    }
    if (fn) {
        // Strictly pass-through the entire pNext feature chain without modifying any VkBool32
        fn(physicalDevice, pFeatures);
    }
}

void layer_vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties) {
    if (!physicalDevice || !pProperties) return;
    VkInstance inst = nullptr;
    InstanceDispatchEntry entry{};
    if (!lookup_phys_dispatch(physicalDevice, inst, entry)) return;

    auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        entry.next_gipa(inst, "vkGetPhysicalDeviceProperties"));
    if (fn) {
        // Strictly pass-through physical device properties without inflating apiVersion
        fn(physicalDevice, pProperties);
    }
}

void layer_vkGetPhysicalDeviceProperties2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2* pProperties) {
    if (!physicalDevice || !pProperties) return;
    VkInstance inst = nullptr;
    InstanceDispatchEntry entry{};
    if (!lookup_phys_dispatch(physicalDevice, inst, entry)) return;

    auto fn = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        entry.next_gipa(inst, "vkGetPhysicalDeviceProperties2"));
    if (!fn) {
        fn = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
            entry.next_gipa(inst, "vkGetPhysicalDeviceProperties2KHR"));
    }
    if (fn) {
        // Strictly pass-through physical device properties and limits without inflating apiVersion or maxPushDescriptors
        fn(physicalDevice, pProperties);
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

    if (!physicalDevice || !pPropertyCount) return kVkErrorInitializationFailed;
    VkInstance inst = nullptr;
    InstanceDispatchEntry entry{};
    if (!lookup_phys_dispatch(physicalDevice, inst, entry)) return kVkErrorInitializationFailed;

    auto fn = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        entry.next_gipa(inst, "vkEnumerateDeviceExtensionProperties"));
    if (!fn) return kVkErrorInitializationFailed;

    // Strictly enumerate native ICD extensions without injecting unimplemented extensions
    return fn(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult layer_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const void* pAllocator,
    VkDevice* pDevice) {
    if (!physicalDevice || !pCreateInfo || !pDevice) return kVkErrorInitializationFailed;

    auto* chain = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(pCreateInfo->pNext));
    while (chain && !(chain->sType == kVkStructureTypeLoaderDeviceCreateInfo && chain->function == 0)) {
        chain = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(chain->pNext));
    }
    if (!chain || !chain->u.pLayerInfo) return kVkErrorInitializationFailed;

    PFN_vkGetInstanceProcAddr next_gipa = chain->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr next_gdpa = chain->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    if (!next_gipa || !next_gdpa) return kVkErrorInitializationFailed;
    chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;

    VkInstance inst = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        auto pit = g_phys_to_instance.find(physicalDevice);
        if (pit != g_phys_to_instance.end()) {
            inst = pit->second;
        } else if (g_instance_dispatch.size() == 1) {
            inst = g_instance_dispatch.begin()->first;
        }
    }

    auto create_dev = reinterpret_cast<PFN_vkCreateDevice>(next_gipa(inst, "vkCreateDevice"));
    if (!create_dev) return kVkErrorInitializationFailed;

    // Pass pCreateInfo unmodified so unsupported extensions/features return authentic
    // VK_ERROR_EXTENSION_NOT_PRESENT or VK_ERROR_FEATURE_NOT_PRESENT from the ICD.
    const VkResult res = create_dev(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (res == kVkSuccess && *pDevice != nullptr) {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_device_dispatch[*pDevice] = DeviceDispatchEntry{next_gdpa, next_gipa, inst};
    }
    return res;
}

void layer_vkDestroyDevice(VkDevice device, const void* pAllocator) {
    DeviceDispatchEntry entry{};
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        auto it = g_device_dispatch.find(device);
        if (it != g_device_dispatch.end()) {
            entry = it->second;
            g_device_dispatch.erase(it);
        }
    }
    if (!entry.next_gdpa) return;
    auto fn = reinterpret_cast<PFN_vkDestroyDevice>(entry.next_gdpa(device, "vkDestroyDevice"));
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
    pProperties[0].specVersion = make_vk_version(1, 3, 0);
    pProperties[0].implementationVersion = 2;
    std::snprintf(
        pProperties[0].description,
        kVkMaxDescriptionSize,
        "CoreFlow Truthful Hardware Pass-Through & Telemetry Layer");
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

    DeviceDispatchEntry entry{};
    if (!lookup_device_dispatch(device, entry)) return nullptr;
    return entry.next_gdpa ? entry.next_gdpa(device, pName) : nullptr;
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
    if (std::strcmp(pName, "vkEnumeratePhysicalDevices") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkEnumeratePhysicalDevices);
    }
    if (std::strcmp(pName, "vkCreateDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreateDevice);
    }
    if (std::strcmp(pName, "vkDestroyDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkDestroyDevice);
    }
    if (std::strcmp(pName, "vkGetDeviceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkGetDeviceProcAddr);
    }
    if (std::strcmp(pName, "vkGetPhysicalDeviceFeatures") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetPhysicalDeviceFeatures);
    }
    if (std::strcmp(pName, "vkGetPhysicalDeviceFeatures2") == 0 ||
        std::strcmp(pName, "vkGetPhysicalDeviceFeatures2KHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetPhysicalDeviceFeatures2);
    }
    if (std::strcmp(pName, "vkGetPhysicalDeviceProperties") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetPhysicalDeviceProperties);
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

    InstanceDispatchEntry entry{};
    if (!lookup_instance_dispatch(instance, entry)) return nullptr;
    return entry.next_gipa ? entry.next_gipa(instance, pName) : nullptr;
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
