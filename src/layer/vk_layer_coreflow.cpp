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
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <iterator>
#include <mutex>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
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
using VkPipelineCache = uintptr_t;
using PFN_vkVoidFunction = void (*)();

constexpr VkResult kVkSuccess = 0;
constexpr VkResult kVkIncomplete = 5;
constexpr VkResult kVkErrorInitializationFailed = -3;
constexpr int32_t kVkStructureTypePipelineCacheCreateInfo = 17;
constexpr uint32_t kVkPipelineCacheHeaderVersionOne = 1U;
constexpr size_t kVkPipelineCacheHeaderSize = 32U;
constexpr size_t kVkMaxPipelineCacheBytes = 32U * 1024U * 1024U;
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

struct VkPipelineCacheCreateInfo {
    int32_t sType;
    const void* pNext;
    VkFlags flags;
    size_t initialDataSize;
    const void* pInitialData;
};
using PFN_vkCreatePipelineCache = VkResult (*)(VkDevice, const VkPipelineCacheCreateInfo*, const void*, VkPipelineCache*);
using PFN_vkGetPipelineCacheData = VkResult (*)(VkDevice, VkPipelineCache, size_t*, void*);
using PFN_vkDestroyPipelineCache = void (*)(VkDevice, VkPipelineCache, const void*);

struct VkPipelineCacheHeaderOne {
    uint32_t header_size;
    uint32_t header_version;
    uint32_t vendor_id;
    uint32_t device_id;
    uint8_t pipeline_cache_uuid[16];
};
static_assert(sizeof(VkPipelineCacheHeaderOne) == kVkPipelineCacheHeaderSize);

struct InstanceDispatchEntry {
    PFN_vkGetInstanceProcAddr next_gipa{nullptr};
    PFN_vkVoidFunction next_gpdpa{nullptr};
};

struct DeviceDispatchEntry {
    PFN_vkGetDeviceProcAddr next_gdpa{nullptr};
    PFN_vkGetInstanceProcAddr next_gipa{nullptr};
    VkInstance instance{nullptr};
    uint32_t vendor_id{0U};
    uint32_t device_id{0U};
    uint32_t driver_version{0U};
    std::array<uint8_t, 16> pipeline_cache_uuid{};
    bool identity_valid{false};
};

struct PipelineCacheRecord {
    VkDevice device{nullptr};
};

std::mutex g_dispatch_mutex;
std::unordered_map<VkInstance, InstanceDispatchEntry> g_instance_dispatch;
std::unordered_map<VkPhysicalDevice, VkInstance> g_phys_to_instance;
std::unordered_map<VkDevice, DeviceDispatchEntry> g_device_dispatch;
std::unordered_map<VkPipelineCache, PipelineCacheRecord> g_pipeline_cache_dispatch;
std::atomic<uint64_t> g_cache_temp_sequence{0U};

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


bool write_all(int fd, const void* bytes, size_t length) noexcept {
    const auto* cursor = static_cast<const uint8_t*>(bytes);
    size_t written = 0U;
    while (written < length) {
        const ssize_t count = ::write(fd, cursor + written, length - written);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        written += static_cast<size_t>(count);
    }
    return true;
}

bool read_all(int fd, void* bytes, size_t length) noexcept {
    auto* cursor = static_cast<uint8_t*>(bytes);
    size_t received = 0U;
    while (received < length) {
        const ssize_t count = ::read(fd, cursor + received, length - received);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        received += static_cast<size_t>(count);
    }
    return true;
}

bool valid_cache_header(const void* bytes, size_t length,
                        const DeviceDispatchEntry& device) noexcept {
    if (!bytes || !device.identity_valid || length < kVkPipelineCacheHeaderSize ||
        length > kVkMaxPipelineCacheBytes) return false;
    VkPipelineCacheHeaderOne header{};
    std::memcpy(&header, bytes, sizeof(header));
    return header.header_size == kVkPipelineCacheHeaderSize &&
        header.header_version == kVkPipelineCacheHeaderVersionOne &&
        header.vendor_id == device.vendor_id && header.device_id == device.device_id &&
        std::memcmp(header.pipeline_cache_uuid, device.pipeline_cache_uuid.data(),
                    device.pipeline_cache_uuid.size()) == 0;
}

int open_app_pipeline_cache_dir() noexcept {
    std::array<char, 256> cmdline{};
    const int cmd_fd = ::open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (cmd_fd < 0) return -1;
    const ssize_t length = ::read(cmd_fd, cmdline.data(), cmdline.size() - 1U);
    ::close(cmd_fd);
    if (length <= 0) return -1;
    std::string package(cmdline.data(), static_cast<size_t>(length));
    const size_t terminator = package.find('\0');
    if (terminator != std::string::npos) package.resize(terminator);
    const size_t process_suffix = package.find(':');
    if (process_suffix != std::string::npos) package.resize(process_suffix);
    if (package.empty() || package.size() > 200U) return -1;
    for (const unsigned char ch : package) {
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '_' || ch == '.')) return -1;
    }

    const uid_t uid = ::getuid();
    const uint32_t android_user = static_cast<uint32_t>(uid) / 100000U;
    const std::string base = "/data/user/" + std::to_string(android_user) + "/" + package + "/cache";
    const int cache_fd = ::open(base.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (cache_fd < 0) return -1;
    struct stat base_stat{};
    if (::fstat(cache_fd, &base_stat) != 0 || !S_ISDIR(base_stat.st_mode) ||
        base_stat.st_uid != uid) {
        ::close(cache_fd);
        return -1;
    }
    if (::mkdirat(cache_fd, "coreflow-vulkan", 0700) != 0 && errno != EEXIST) {
        ::close(cache_fd);
        return -1;
    }
    const int dir_fd = ::openat(cache_fd, "coreflow-vulkan",
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    ::close(cache_fd);
    if (dir_fd < 0) return -1;
    struct stat dir_stat{};
    if (::fstat(dir_fd, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode) ||
        dir_stat.st_uid != uid || (dir_stat.st_mode & 0077) != 0) {
        ::close(dir_fd);
        return -1;
    }
    return dir_fd;
}

bool pipeline_cache_filename(const DeviceDispatchEntry& device,
                              std::array<char, 160>& filename) noexcept {
    if (!device.identity_valid) return false;
    char uuid[33]{};
    for (size_t i = 0U; i < device.pipeline_cache_uuid.size(); ++i) {
        std::snprintf(uuid + (i * 2U), 3U, "%02x", device.pipeline_cache_uuid[i]);
    }
    const int length = std::snprintf(filename.data(), filename.size(),
        "vkcache-v1-%08x-%08x-%08x-%s.bin", device.vendor_id,
        device.device_id, device.driver_version, uuid);
    return length > 0 && static_cast<size_t>(length) < filename.size();
}

bool load_persistent_pipeline_cache(const DeviceDispatchEntry& device,
                                    std::vector<uint8_t>& data) noexcept {
    std::array<char, 160> filename{};
    if (!pipeline_cache_filename(device, filename)) return false;
    const int dir_fd = open_app_pipeline_cache_dir();
    if (dir_fd < 0) return false;
    const int fd = ::openat(dir_fd, filename.data(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ::close(dir_fd);
    if (fd < 0) return false;
    struct stat st{};
    const bool safe_file = ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
        st.st_uid == ::getuid() && st.st_size >= static_cast<off_t>(kVkPipelineCacheHeaderSize) &&
        st.st_size <= static_cast<off_t>(kVkMaxPipelineCacheBytes);
    if (!safe_file) {
        ::close(fd);
        return false;
    }
    data.resize(static_cast<size_t>(st.st_size));
    const bool read_ok = read_all(fd, data.data(), data.size());
    ::close(fd);
    if (!read_ok || !valid_cache_header(data.data(), data.size(), device)) {
        data.clear();
        return false;
    }
    return true;
}

bool persist_pipeline_cache(VkDevice device, VkPipelineCache cache) noexcept {
    DeviceDispatchEntry entry{};
    if (!lookup_device_dispatch(device, entry) || !entry.identity_valid) return false;
    auto get_data = reinterpret_cast<PFN_vkGetPipelineCacheData>(
        entry.next_gdpa(device, "vkGetPipelineCacheData"));
    if (!get_data) return false;
    size_t size = 0U;
    if (get_data(device, cache, &size, nullptr) != kVkSuccess ||
        size < kVkPipelineCacheHeaderSize || size > kVkMaxPipelineCacheBytes) return false;
    std::vector<uint8_t> data(size);
    size_t actual_size = data.size();
    const VkResult result = get_data(device, cache, &actual_size, data.data());
    if (result != kVkSuccess || actual_size < kVkPipelineCacheHeaderSize ||
        actual_size > data.size() || !valid_cache_header(data.data(), actual_size, entry)) return false;
    data.resize(actual_size);

    std::array<char, 160> filename{};
    if (!pipeline_cache_filename(entry, filename)) return false;
    const int dir_fd = open_app_pipeline_cache_dir();
    if (dir_fd < 0) return false;
    std::array<char, 224> temporary{};
    const uint64_t sequence = g_cache_temp_sequence.fetch_add(1U, std::memory_order_relaxed) + 1U;
    const int temp_length = std::snprintf(temporary.data(), temporary.size(),
        ".%s.%u.%llu.tmp", filename.data(), static_cast<unsigned>(::getpid()),
        static_cast<unsigned long long>(sequence));
    if (temp_length <= 0 || static_cast<size_t>(temp_length) >= temporary.size()) {
        ::close(dir_fd);
        return false;
    }
    const int out_fd = ::openat(dir_fd, temporary.data(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (out_fd < 0) {
        ::close(dir_fd);
        return false;
    }
    bool ok = write_all(out_fd, data.data(), data.size()) &&
        ::fchmod(out_fd, 0600) == 0 && ::fsync(out_fd) == 0;
    if (::close(out_fd) != 0) ok = false;
    if (ok && ::renameat(dir_fd, temporary.data(), dir_fd, filename.data()) != 0) ok = false;
    if (ok && ::fsync(dir_fd) != 0) ok = false;
    if (!ok) (void)::unlinkat(dir_fd, temporary.data(), 0);
    ::close(dir_fd);
    return ok;
}

void flush_device_pipeline_caches(VkDevice device) noexcept {
    std::vector<VkPipelineCache> caches;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        for (const auto& item : g_pipeline_cache_dispatch) {
            if (item.second.device == device) caches.push_back(item.first);
        }
    }
    for (const VkPipelineCache cache : caches) (void)persist_pipeline_cache(device, cache);
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        for (const VkPipelineCache cache : caches) g_pipeline_cache_dispatch.erase(cache);
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


VkResult layer_vkCreatePipelineCache(
    VkDevice device,
    const VkPipelineCacheCreateInfo* pCreateInfo,
    const void* pAllocator,
    VkPipelineCache* pPipelineCache) {
    if (!pCreateInfo || !pPipelineCache) return kVkErrorInitializationFailed;
    DeviceDispatchEntry entry{};
    if (!lookup_device_dispatch(device, entry)) return kVkErrorInitializationFailed;
    auto create_cache = reinterpret_cast<PFN_vkCreatePipelineCache>(
        entry.next_gdpa(device, "vkCreatePipelineCache"));
    if (!create_cache) return kVkErrorInitializationFailed;

    VkPipelineCacheCreateInfo adjusted = *pCreateInfo;
    std::vector<uint8_t> initial_data;
    if (pCreateInfo->sType == kVkStructureTypePipelineCacheCreateInfo &&
        pCreateInfo->initialDataSize == 0U && pCreateInfo->pInitialData == nullptr &&
        load_persistent_pipeline_cache(entry, initial_data)) {
        adjusted.initialDataSize = initial_data.size();
        adjusted.pInitialData = initial_data.data();
    }
    const VkResult result = create_cache(device, &adjusted, pAllocator, pPipelineCache);
    if (result == kVkSuccess && *pPipelineCache != 0U) {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_pipeline_cache_dispatch[*pPipelineCache] = PipelineCacheRecord{device};
    }
    return result;
}

VkResult layer_vkGetPipelineCacheData(
    VkDevice device, VkPipelineCache cache, size_t* pDataSize, void* pData) {
    if (!pDataSize) return kVkErrorInitializationFailed;
    DeviceDispatchEntry entry{};
    if (!lookup_device_dispatch(device, entry)) return kVkErrorInitializationFailed;
    auto get_data = reinterpret_cast<PFN_vkGetPipelineCacheData>(
        entry.next_gdpa(device, "vkGetPipelineCacheData"));
    return get_data ? get_data(device, cache, pDataSize, pData) : kVkErrorInitializationFailed;
}

void layer_vkDestroyPipelineCache(VkDevice device, VkPipelineCache cache, const void* pAllocator) {
    DeviceDispatchEntry entry{};
    if (!lookup_device_dispatch(device, entry)) return;
    (void)persist_pipeline_cache(device, cache);
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_pipeline_cache_dispatch.erase(cache);
    }
    auto destroy_cache = reinterpret_cast<PFN_vkDestroyPipelineCache>(
        entry.next_gdpa(device, "vkDestroyPipelineCache"));
    if (destroy_cache) destroy_cache(device, cache, pAllocator);
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

    VkPhysicalDeviceProperties native_properties{};
    bool identity_valid = false;
    auto get_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        next_gipa(inst, "vkGetPhysicalDeviceProperties"));
    if (get_properties) {
        get_properties(physicalDevice, &native_properties);
        identity_valid = native_properties.vendorID != 0U && native_properties.deviceID != 0U;
    }
    auto create_dev = reinterpret_cast<PFN_vkCreateDevice>(next_gipa(inst, "vkCreateDevice"));
    if (!create_dev) return kVkErrorInitializationFailed;

    // Pass pCreateInfo unmodified so unsupported extensions/features return authentic
    // VK_ERROR_EXTENSION_NOT_PRESENT or VK_ERROR_FEATURE_NOT_PRESENT from the ICD.
    const VkResult res = create_dev(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (res == kVkSuccess && *pDevice != nullptr) {
        DeviceDispatchEntry dispatch{};
        dispatch.next_gdpa = next_gdpa;
        dispatch.next_gipa = next_gipa;
        dispatch.instance = inst;
        dispatch.vendor_id = native_properties.vendorID;
        dispatch.device_id = native_properties.deviceID;
        dispatch.driver_version = native_properties.driverVersion;
        std::copy(std::begin(native_properties.pipelineCacheUUID),
                  std::end(native_properties.pipelineCacheUUID),
                  dispatch.pipeline_cache_uuid.begin());
        dispatch.identity_valid = identity_valid;
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_device_dispatch[*pDevice] = dispatch;
    }
    return res;
}

void layer_vkDestroyDevice(VkDevice device, const void* pAllocator) {
    flush_device_pipeline_caches(device);
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
    if (std::strcmp(pName, "vkCreatePipelineCache") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreatePipelineCache);
    }
    if (std::strcmp(pName, "vkGetPipelineCacheData") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetPipelineCacheData);
    }
    if (std::strcmp(pName, "vkDestroyPipelineCache") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&layer_vkDestroyPipelineCache);
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
