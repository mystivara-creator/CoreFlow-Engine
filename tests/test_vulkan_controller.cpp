/**
 * @file test_vulkan_controller.cpp
 * @brief Deterministic unit and layer dispatch tests for Vulkan Capability Controller,
 *        Multi-Vendor GPU Kernel Surface Rollback, and Truthful Pass-Through Layer.
 */

#include "coreflow/vulkan_controller.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace coreflow;

// Minimal ABI types to test VkLayer_coreflow entrypoints directly against a stub ICD
using VkResult = int32_t;
using VkFlags = uint32_t;
using VkBool32 = uint32_t;
using VkStructureType = int32_t;
using VkInstance = void*;
using VkPhysicalDevice = void*;
using VkDevice = void*;
using PFN_vkVoidFunction = void (*)();

constexpr VkResult VK_SUCCESS = 0;
constexpr VkResult VK_INCOMPLETE = 5;
constexpr VkResult VK_ERROR_EXTENSION_NOT_PRESENT = -7;
constexpr VkBool32 VK_FALSE = 0U;
constexpr VkBool32 VK_TRUE = 1U;
constexpr size_t VK_MAX_EXTENSION_NAME_SIZE = 256;
constexpr size_t VK_MAX_PHYSICAL_DEVICE_NAME_SIZE = 256;
constexpr size_t VK_UUID_SIZE = 16;
constexpr VkStructureType VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO = 47;
constexpr VkStructureType VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO = 48;
constexpr VkStructureType VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES = 1000547001;
constexpr VkStructureType VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR = 1000470000;
constexpr VkStructureType VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR = 1000347000;

constexpr uint32_t make_vk_version(uint32_t major, uint32_t minor, uint32_t patch) noexcept {
    return (major << 22U) | (minor << 12U) | patch;
}

struct VkBaseOutStructure {
    VkStructureType sType;
    VkBaseOutStructure* pNext;
};

struct VkExtensionProperties {
    char extensionName[VK_MAX_EXTENSION_NAME_SIZE];
    uint32_t specVersion;
};

struct VkPhysicalDeviceFeatures {
    VkBool32 words[55];
};

struct VkPhysicalDeviceFeatures2 {
    VkStructureType sType;
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
    char deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    uint8_t pipelineCacheUUID[VK_UUID_SIZE];
    VkPhysicalDeviceLimits limits;
    VkPhysicalDeviceSparseProperties sparseProperties;
};

struct VkPhysicalDeviceProperties2 {
    VkStructureType sType;
    void* pNext;
    VkPhysicalDeviceProperties properties;
};

struct VkInstanceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    const void* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkDeviceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    uint32_t queueCreateInfoCount;
    const void* pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    const VkPhysicalDeviceFeatures* pEnabledFeatures;
};

using PFN_vkGetInstanceProcAddr = PFN_vkVoidFunction (*)(VkInstance, const char*);
using PFN_vkGetDeviceProcAddr = PFN_vkVoidFunction (*)(VkDevice, const char*);
using PFN_vkCreateInstance = VkResult (*)(const VkInstanceCreateInfo*, const void*, VkInstance*);
using PFN_vkDestroyInstance = void (*)(VkInstance, const void*);
using PFN_vkCreateDevice = VkResult (*)(VkPhysicalDevice, const VkDeviceCreateInfo*, const void*, VkDevice*);
using PFN_vkDestroyDevice = void (*)(VkDevice, const void*);
using PFN_vkEnumeratePhysicalDevices = VkResult (*)(VkInstance, uint32_t*, VkPhysicalDevice*);
using PFN_vkGetPhysicalDeviceProperties = void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
using PFN_vkGetPhysicalDeviceProperties2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties2*);
using PFN_vkGetPhysicalDeviceFeatures = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
using PFN_vkGetPhysicalDeviceFeatures2 = void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures2*);
using PFN_vkEnumerateDeviceExtensionProperties =
    VkResult (*)(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*);

struct VkLayerInstanceLink {
    VkLayerInstanceLink* pNext;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
    PFN_vkVoidFunction pfnNextGetPhysicalDeviceProcAddr;
};

struct VkLayerInstanceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    int function; // 0 == VK_LAYER_LINK_INFO
    union {
        VkLayerInstanceLink* pLayerInfo;
    } u;
};

struct VkLayerDeviceLink {
    VkLayerDeviceLink* pNext;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr pfnNextGetDeviceProcAddr;
};

struct VkLayerDeviceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    int function; // 0 == VK_LAYER_LINK_INFO
    union {
        VkLayerDeviceLink* pLayerInfo;
    } u;
};

struct VkFeatureToggleHeader {
    VkStructureType sType;
    void* pNext;
    VkBool32 enabled;
};

struct VkPhysicalDeviceVulkan14PropertiesSubset {
    VkStructureType sType;
    void* pNext;
    uint32_t lineSubPixelPrecisionBits;
    VkBool32 stippledRectangularLines;
    VkBool32 stippledBresenhamLines;
    VkBool32 stippledSmoothLines;
    uint32_t maxVertexAttribDivisor;
    VkBool32 supportsNonZeroFirstInstance;
    uint32_t maxPushDescriptors;
};

extern "C" PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName);
extern "C" PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char* pName);

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

// Stub ICD state for multi-instance / multi-device dispatch verification
struct StubIcdState {
    int instance_a_calls = 0;
    int instance_b_calls = 0;
    int device_a_destroy_calls = 0;
    int device_b_destroy_calls = 0;
    uintptr_t inst_a_token = 0x1001;
    uintptr_t inst_b_token = 0x1002;
    uintptr_t phys_a_token = 0x2001;
    uintptr_t phys_b_token = 0x2002;
    uintptr_t dev_a_token = 0x3001;
    uintptr_t dev_b_token = 0x3002;
} g_stub;

VkResult stub_vkCreateInstance_A(const VkInstanceCreateInfo*, const void*, VkInstance* pInstance) {
    g_stub.instance_a_calls++;
    *pInstance = reinterpret_cast<VkInstance>(g_stub.inst_a_token);
    return VK_SUCCESS;
}

VkResult stub_vkCreateInstance_B(const VkInstanceCreateInfo*, const void*, VkInstance* pInstance) {
    g_stub.instance_b_calls++;
    *pInstance = reinterpret_cast<VkInstance>(g_stub.inst_b_token);
    return VK_SUCCESS;
}

void stub_vkDestroyInstance_A(VkInstance inst, const void*) {
    assert(inst == reinterpret_cast<VkInstance>(g_stub.inst_a_token));
}

void stub_vkDestroyInstance_B(VkInstance inst, const void*) {
    assert(inst == reinterpret_cast<VkInstance>(g_stub.inst_b_token));
}

VkResult stub_vkEnumeratePhysicalDevices_A(VkInstance inst, uint32_t* pCount, VkPhysicalDevice* pDevices) {
    assert(inst == reinterpret_cast<VkInstance>(g_stub.inst_a_token));
    if (!pDevices) {
        *pCount = 1;
        return VK_SUCCESS;
    }
    if (*pCount < 1) return VK_INCOMPLETE;
    pDevices[0] = reinterpret_cast<VkPhysicalDevice>(g_stub.phys_a_token);
    *pCount = 1;
    return VK_SUCCESS;
}

VkResult stub_vkEnumeratePhysicalDevices_B(VkInstance inst, uint32_t* pCount, VkPhysicalDevice* pDevices) {
    assert(inst == reinterpret_cast<VkInstance>(g_stub.inst_b_token));
    if (!pDevices) {
        *pCount = 1;
        return VK_SUCCESS;
    }
    if (*pCount < 1) return VK_INCOMPLETE;
    pDevices[0] = reinterpret_cast<VkPhysicalDevice>(g_stub.phys_b_token);
    *pCount = 1;
    return VK_SUCCESS;
}

void stub_vkGetPhysicalDeviceProperties_A(VkPhysicalDevice phys, VkPhysicalDeviceProperties* props) {
    assert(phys == reinterpret_cast<VkPhysicalDevice>(g_stub.phys_a_token));
    std::memset(props, 0, sizeof(*props));
    props->apiVersion = make_vk_version(1, 1, 128);
    props->vendorID = 0x5143;
}

void stub_vkGetPhysicalDeviceProperties_B(VkPhysicalDevice phys, VkPhysicalDeviceProperties* props) {
    assert(phys == reinterpret_cast<VkPhysicalDevice>(g_stub.phys_b_token));
    std::memset(props, 0, sizeof(*props));
    props->apiVersion = make_vk_version(1, 3, 250);
    props->vendorID = 0x13B5;
}

void stub_vkGetPhysicalDeviceProperties2(VkPhysicalDevice phys, VkPhysicalDeviceProperties2* props2) {
    if (phys == reinterpret_cast<VkPhysicalDevice>(g_stub.phys_a_token)) {
        stub_vkGetPhysicalDeviceProperties_A(phys, &props2->properties);
    } else {
        stub_vkGetPhysicalDeviceProperties_B(phys, &props2->properties);
    }
    auto* cur = reinterpret_cast<VkBaseOutStructure*>(props2->pNext);
    while (cur) {
        if (cur->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES) {
            auto* p14 = reinterpret_cast<VkPhysicalDeviceVulkan14PropertiesSubset*>(cur);
            p14->maxPushDescriptors = 16U;
        }
        cur = cur->pNext;
    }
}

void stub_vkGetPhysicalDeviceFeatures(VkPhysicalDevice, VkPhysicalDeviceFeatures* feats) {
    std::memset(feats, 0, sizeof(*feats));
    // Feature 39 (shaderFloat64) and 38 (shaderInt64) explicitly false on this stub ICD
    feats->words[38] = VK_FALSE;
    feats->words[39] = VK_FALSE;
}

void stub_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice phys, VkPhysicalDeviceFeatures2* feats2) {
    stub_vkGetPhysicalDeviceFeatures(phys, &feats2->features);
    auto* cur = reinterpret_cast<VkBaseOutStructure*>(feats2->pNext);
    while (cur) {
        if (cur->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR ||
            cur->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR) {
            auto* toggle = reinterpret_cast<VkFeatureToggleHeader*>(cur);
            toggle->enabled = VK_FALSE;
        }
        cur = cur->pNext;
    }
}

const VkExtensionProperties kStubExtensions[] = {
    {"VK_KHR_swapchain", 70},
    {"VK_KHR_maintenance1", 2},
};

VkResult stub_vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) {
    if (pLayerName != nullptr) {
        *pPropertyCount = 0;
        return VK_SUCCESS;
    }
    constexpr uint32_t kTotal = 2;
    if (!pProperties) {
        *pPropertyCount = kTotal;
        return VK_SUCCESS;
    }
    const uint32_t cap = *pPropertyCount;
    const uint32_t to_copy = cap < kTotal ? cap : kTotal;
    for (uint32_t i = 0; i < to_copy; ++i) {
        pProperties[i] = kStubExtensions[i];
    }
    *pPropertyCount = to_copy;
    return (cap < kTotal) ? VK_INCOMPLETE : VK_SUCCESS;
}

VkResult stub_vkCreateDevice(
    VkPhysicalDevice phys,
    const VkDeviceCreateInfo* pCreateInfo,
    const void*,
    VkDevice* pDevice) {
    // Reject any extension not in kStubExtensions (as a real ICD does)
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
        const char* req = pCreateInfo->ppEnabledExtensionNames[i];
        bool supported = false;
        for (const auto& ext : kStubExtensions) {
            if (std::strcmp(req, ext.extensionName) == 0) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    if (phys == reinterpret_cast<VkPhysicalDevice>(g_stub.phys_a_token)) {
        *pDevice = reinterpret_cast<VkDevice>(g_stub.dev_a_token);
    } else {
        *pDevice = reinterpret_cast<VkDevice>(g_stub.dev_b_token);
    }
    return VK_SUCCESS;
}

void stub_vkDestroyDevice_A(VkDevice dev, const void*) {
    assert(dev == reinterpret_cast<VkDevice>(g_stub.dev_a_token));
    g_stub.device_a_destroy_calls++;
}

void stub_vkDestroyDevice_B(VkDevice dev, const void*) {
    assert(dev == reinterpret_cast<VkDevice>(g_stub.dev_b_token));
    g_stub.device_b_destroy_calls++;
}

PFN_vkVoidFunction stub_gipa_A(VkInstance, const char* name) {
    if (std::strcmp(name, "vkCreateInstance") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkCreateInstance_A);
    if (std::strcmp(name, "vkDestroyInstance") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkDestroyInstance_A);
    if (std::strcmp(name, "vkEnumeratePhysicalDevices") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkEnumeratePhysicalDevices_A);
    if (std::strcmp(name, "vkGetPhysicalDeviceProperties") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceProperties_A);
    if (std::strcmp(name, "vkGetPhysicalDeviceProperties2") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceProperties2);
    if (std::strcmp(name, "vkGetPhysicalDeviceFeatures") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceFeatures);
    if (std::strcmp(name, "vkGetPhysicalDeviceFeatures2") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceFeatures2);
    if (std::strcmp(name, "vkEnumerateDeviceExtensionProperties") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(stub_vkEnumerateDeviceExtensionProperties);
    if (std::strcmp(name, "vkCreateDevice") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkCreateDevice);
    return nullptr;
}

PFN_vkVoidFunction stub_gipa_B(VkInstance, const char* name) {
    if (std::strcmp(name, "vkCreateInstance") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkCreateInstance_B);
    if (std::strcmp(name, "vkDestroyInstance") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkDestroyInstance_B);
    if (std::strcmp(name, "vkEnumeratePhysicalDevices") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkEnumeratePhysicalDevices_B);
    if (std::strcmp(name, "vkGetPhysicalDeviceProperties") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceProperties_B);
    if (std::strcmp(name, "vkGetPhysicalDeviceProperties2") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceProperties2);
    if (std::strcmp(name, "vkGetPhysicalDeviceFeatures") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceFeatures);
    if (std::strcmp(name, "vkGetPhysicalDeviceFeatures2") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkGetPhysicalDeviceFeatures2);
    if (std::strcmp(name, "vkEnumerateDeviceExtensionProperties") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(stub_vkEnumerateDeviceExtensionProperties);
    if (std::strcmp(name, "vkCreateDevice") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkCreateDevice);
    return nullptr;
}

PFN_vkVoidFunction stub_gdpa_A(VkDevice, const char* name) {
    if (std::strcmp(name, "vkDestroyDevice") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkDestroyDevice_A);
    return nullptr;
}

PFN_vkVoidFunction stub_gdpa_B(VkDevice, const char* name) {
    if (std::strcmp(name, "vkDestroyDevice") == 0) return reinterpret_cast<PFN_vkVoidFunction>(stub_vkDestroyDevice_B);
    return nullptr;
}

void test_vulkan_layer_truthfulness_and_dispatch() {
    auto create_inst = reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
    assert(create_inst != nullptr);

    // Create two distinct instances with distinct next-layer dispatch tables (VULKAN-02)
    VkLayerInstanceLink link_a{nullptr, stub_gipa_A, nullptr};
    VkLayerInstanceCreateInfo loader_info_a{
        VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO, nullptr, 0, {&link_a}};
    VkInstanceCreateInfo ci_a{};
    ci_a.pNext = &loader_info_a;
    VkInstance inst_a = nullptr;
    assert(create_inst(&ci_a, nullptr, &inst_a) == VK_SUCCESS);
    assert(inst_a == reinterpret_cast<VkInstance>(g_stub.inst_a_token));

    VkLayerInstanceLink link_b{nullptr, stub_gipa_B, nullptr};
    VkLayerInstanceCreateInfo loader_info_b{
        VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO, nullptr, 0, {&link_b}};
    VkInstanceCreateInfo ci_b{};
    ci_b.pNext = &loader_info_b;
    VkInstance inst_b = nullptr;
    assert(create_inst(&ci_b, nullptr, &inst_b) == VK_SUCCESS);
    assert(inst_b == reinterpret_cast<VkInstance>(g_stub.inst_b_token));

    // Enumerate physical devices on both instances; ensure inst_b creation did NOT overwrite inst_a's dispatch
    auto enum_phys_a = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        vkGetInstanceProcAddr(inst_a, "vkEnumeratePhysicalDevices"));
    auto enum_phys_b = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        vkGetInstanceProcAddr(inst_b, "vkEnumeratePhysicalDevices"));
    uint32_t count = 1;
    VkPhysicalDevice phys_a = nullptr;
    VkPhysicalDevice phys_b = nullptr;
    assert(enum_phys_a(inst_a, &count, &phys_a) == VK_SUCCESS);
    assert(phys_a == reinterpret_cast<VkPhysicalDevice>(g_stub.phys_a_token));
    assert(enum_phys_b(inst_b, &count, &phys_b) == VK_SUCCESS);
    assert(phys_b == reinterpret_cast<VkPhysicalDevice>(g_stub.phys_b_token));

    // Verify properties are NOT promoted to 1.4.303 and maxPushDescriptors is NOT inflated (VULKAN-01)
    auto get_props = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        vkGetInstanceProcAddr(inst_a, "vkGetPhysicalDeviceProperties"));
    VkPhysicalDeviceProperties props_a{};
    get_props(phys_a, &props_a);
    assert(props_a.apiVersion == make_vk_version(1, 1, 128));

    VkPhysicalDeviceProperties props_b{};
    get_props(phys_b, &props_b);
    assert(props_b.apiVersion == make_vk_version(1, 3, 250));

    auto get_props2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        vkGetInstanceProcAddr(inst_a, "vkGetPhysicalDeviceProperties2"));
    VkPhysicalDeviceVulkan14PropertiesSubset props14{};
    props14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES;
    props14.pNext = nullptr;
    VkPhysicalDeviceProperties2 props2{};
    props2.pNext = &props14;
    get_props2(phys_a, &props2);
    assert(props2.properties.apiVersion == make_vk_version(1, 1, 128));
    assert(props14.maxPushDescriptors == 16U);

    // Verify features and pNext chains are NOT patched to VK_TRUE when hardware reports VK_FALSE (VULKAN-01)
    auto get_feats2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        vkGetInstanceProcAddr(inst_a, "vkGetPhysicalDeviceFeatures2"));
    VkFeatureToggleHeader rt_feat{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, nullptr, VK_FALSE};
    VkFeatureToggleHeader m5_feat{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR, &rt_feat, VK_FALSE};
    VkPhysicalDeviceFeatures2 feats2{};
    feats2.pNext = &m5_feat;
    get_feats2(phys_a, &feats2);
    assert(feats2.features.words[39] == VK_FALSE); // shaderFloat64 stays false
    assert(m5_feat.enabled == VK_FALSE);            // maintenance5 stays false
    assert(rt_feat.enabled == VK_FALSE);            // rayTracingPipeline stays false

    // Verify extension enumeration reports exact ICD extensions without injecting fake extensions (VULKAN-01)
    auto enum_ext = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        vkGetInstanceProcAddr(inst_a, "vkEnumerateDeviceExtensionProperties"));
    uint32_t ext_count = 0;
    assert(enum_ext(phys_a, nullptr, &ext_count, nullptr) == VK_SUCCESS);
    assert(ext_count == 2U);

    uint32_t partial_count = 1;
    VkExtensionProperties partial_ext{};
    assert(enum_ext(phys_a, nullptr, &partial_count, &partial_ext) == VK_INCOMPLETE);
    assert(partial_count == 1U);

    std::vector<VkExtensionProperties> all_exts(ext_count);
    assert(enum_ext(phys_a, nullptr, &ext_count, all_exts.data()) == VK_SUCCESS);
    assert(ext_count == 2U);
    assert(std::strcmp(all_exts[0].extensionName, "VK_KHR_swapchain") == 0);
    assert(std::strcmp(all_exts[1].extensionName, "VK_KHR_maintenance1") == 0);

    // Verify vkCreateDevice does NOT strip unsupported extensions and returns VK_ERROR_EXTENSION_NOT_PRESENT
    auto create_dev = reinterpret_cast<PFN_vkCreateDevice>(
        vkGetInstanceProcAddr(inst_a, "vkCreateDevice"));
    const char* unsupported_exts[] = {"VK_KHR_swapchain", "VK_KHR_ray_tracing_pipeline"};
    VkLayerDeviceLink dev_link_a{nullptr, stub_gipa_A, stub_gdpa_A};
    VkLayerDeviceCreateInfo dev_loader_a{
        VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO, nullptr, 0, {&dev_link_a}};
    VkDeviceCreateInfo dci_bad{};
    dci_bad.pNext = &dev_loader_a;
    dci_bad.enabledExtensionCount = 2;
    dci_bad.ppEnabledExtensionNames = unsupported_exts;
    VkDevice dev_bad = nullptr;
    assert(create_dev(phys_a, &dci_bad, nullptr, &dev_bad) == VK_ERROR_EXTENSION_NOT_PRESENT);

    // Verify valid device creation on both physical devices keeps independent device dispatch tables
    const char* valid_exts[] = {"VK_KHR_swapchain"};
    VkLayerDeviceLink dev_link_a_ok{nullptr, stub_gipa_A, stub_gdpa_A};
    VkLayerDeviceCreateInfo dev_loader_a_ok{
        VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO, nullptr, 0, {&dev_link_a_ok}};
    VkDeviceCreateInfo dci_a{};
    dci_a.pNext = &dev_loader_a_ok;
    dci_a.enabledExtensionCount = 1;
    dci_a.ppEnabledExtensionNames = valid_exts;
    VkDevice dev_a = nullptr;
    assert(create_dev(phys_a, &dci_a, nullptr, &dev_a) == VK_SUCCESS);

    VkLayerDeviceLink dev_link_b_ok{nullptr, stub_gipa_B, stub_gdpa_B};
    VkLayerDeviceCreateInfo dev_loader_b_ok{
        VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO, nullptr, 0, {&dev_link_b_ok}};
    VkDeviceCreateInfo dci_b{};
    dci_b.pNext = &dev_loader_b_ok;
    dci_b.enabledExtensionCount = 1;
    dci_b.ppEnabledExtensionNames = valid_exts;
    VkDevice dev_b = nullptr;
    assert(create_dev(phys_b, &dci_b, nullptr, &dev_b) == VK_SUCCESS);

    auto destroy_dev_a = reinterpret_cast<PFN_vkDestroyDevice>(vkGetDeviceProcAddr(dev_a, "vkDestroyDevice"));
    auto destroy_dev_b = reinterpret_cast<PFN_vkDestroyDevice>(vkGetDeviceProcAddr(dev_b, "vkDestroyDevice"));
    destroy_dev_a(dev_a, nullptr);
    destroy_dev_b(dev_b, nullptr);
    assert(g_stub.device_a_destroy_calls == 1);
    assert(g_stub.device_b_destroy_calls == 1);

    auto destroy_inst_a = reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(inst_a, "vkDestroyInstance"));
    auto destroy_inst_b = reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(inst_b, "vkDestroyInstance"));
    destroy_inst_a(inst_a, nullptr);
    destroy_inst_b(inst_b, nullptr);
}

} // namespace

int main() {
    test_vulkan_layer_truthfulness_and_dispatch();

    char pattern[] = "/tmp/coreflow-vk-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    assert(dir != nullptr);
    const fs::path tmp_root(dir);
    const fs::path state_dir = tmp_root / "state";
    const fs::path adreno_root = tmp_root / "adreno_sysfs";
    const fs::path mali_root = tmp_root / "mali_sysfs";
    const fs::path boot_id_file = tmp_root / "boot_id";

    fs::create_directories(state_dir);
    write_file(boot_id_file, "11111111-2222-3333-4444-555555555555\n");
    ::setenv("COREFLOW_TEST_STATE_DIR", state_dir.c_str(), 1);
    ::setenv("COREFLOW_TEST_BOOT_ID_PATH", boot_id_file.c_str(), 1);

    auto& baseline = BaselineStore::instance();
    assert(baseline.recover_pending());
    auto& coordinator = CentralMutationCoordinator::instance();
    coordinator.reset();

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
    // Effective capabilities must strictly mirror hardware ICD capabilities (no falsification)
    assert(snap_safe.effective_api_version == snap_safe.api_version);
    assert(snap_safe.effective_device_extensions_count == snap_safe.device_extensions_count);
    assert(snap_safe.layer_implemented_extensions_count == 0U);
    assert(snap_safe.index_type_uint8 == snap_safe.hw_index_type_uint8);
    assert(snap_safe.line_rasterization == snap_safe.hw_line_rasterization);
    assert(snap_safe.push_descriptors == snap_safe.hw_push_descriptors);
    assert(snap_safe.global_priority_query == snap_safe.hw_global_priority_query);
    assert(snap_safe.shader_float_controls2 == snap_safe.hw_shader_float_controls2);
    assert(snap_safe.maintenance5 == snap_safe.hw_maintenance5);
    assert(snap_safe.maintenance6 == snap_safe.hw_maintenance6);
    assert(snap_safe.maintenance7 == snap_safe.hw_maintenance7);
    assert(snap_safe.descriptor_buffer == snap_safe.hw_descriptor_buffer);
    assert(snap_safe.graphics_pipeline_library == snap_safe.hw_graphics_pipeline_library);
    assert(snap_safe.ray_tracing == snap_safe.hw_ray_tracing);
    assert(snap_safe.shader_float64 == snap_safe.hw_shader_float64);
    assert(snap_safe.gpu_surface.available);
    assert(snap_safe.gpu_surface.vendor == GpuVendor::QualcommAdreno);
    assert(snap_safe.gpu_surface.current_freq_mhz == 950U);
    assert(snap_safe.gpu_surface.max_freq_mhz == 1100U);

    // Verify workload policy boosts adrenoboost under SustainedLoad and rolls back cleanly
    assert(vk.apply_workload_policy(WorkloadState::SustainedLoad, 0.95, false));
    assert(read_file(kgsl / "devfreq/adrenoboost") == "2");
    const auto gpu_lease = coordinator.active_leases();
    assert(gpu_lease.size() == 1);
    assert(gpu_lease.front().domain == MutationDomain::GpuDevfreq);
    assert(gpu_lease.front().owner_controller == "VulkanController");
    coordinator.emergency_preempt_all("test-gpu-thermal-preemption");
    assert(read_file(kgsl / "devfreq/adrenoboost") == "0");
    assert(coordinator.active_leases().empty());

    // 2. Test legacy FullCapabilityOverride request is clamped to SafeHardware and never spoofs hardware
    vk.configure(VulkanConfig{
        .mode = VulkanLayerMode::FullCapabilityOverride,
        .enable_skiavk_threaded = true,
        .enable_adaptive_vrs = true,
        .enable_pipeline_cache_prewarm = true,
    });
    const auto snap_full = vk.snapshot();
    assert(snap_full.active_mode == VulkanLayerMode::SafeHardware);
    assert(snap_full.effective_api_version == snap_full.api_version);
    assert(snap_full.ray_tracing == snap_full.hw_ray_tracing);
    assert(snap_full.mesh_shaders == snap_full.hw_mesh_shaders);
    assert(snap_full.variable_rate_shading == snap_full.hw_variable_rate_shading);
    assert(snap_full.cooperative_matrix == snap_full.hw_cooperative_matrix);
    assert(snap_full.rt_pipeline == snap_full.hw_rt_pipeline);
    assert(snap_full.ray_query == snap_full.hw_ray_query);
    assert(snap_full.acceleration_structure == snap_full.hw_acceleration_structure);
    assert(snap_full.shader_float64 == snap_full.hw_shader_float64);

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
    write_file(mali_dev / "device/power_policy", "always_on\n");

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
    assert(vk.apply_workload_policy(WorkloadState::Balanced, 0.95, false));
    assert(read_file(mali_dev / "device/power_policy") == "coarse_demand");
    assert(coordinator.active_leases().size() == 1);
    coordinator.emergency_preempt_all("test-mali-policy-preemption");
    assert(read_file(mali_dev / "device/power_policy") == "always_on");
    assert(coordinator.active_leases().empty());

    assert(!baseline.has_active_mutations());
    fs::remove_all(tmp_root);
    std::cout << "[PASS] Vulkan Truthful Pass-Through Controller, Layer Dispatch & Multi-Vendor GPU Surface tests succeeded.\n";
    return 0;
}
