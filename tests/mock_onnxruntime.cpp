/**
 * @file mock_onnxruntime.cpp
 * @brief Test-only mock ONNX Runtime C-API v1 shared library for dlopen verification.
 * @license Apache-2.0
 *
 * Implements the exact frozen ORT v1 ABI slots used by NeuralPredictor so host C++,
 * ASan/LeakSanitizer, UBSan, and TSan verify dynamic session lifecycle, single-threaded
 * arena-free SessionOptions, tensor I/O, NaN fallback, and zero resource leaks.
 */
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <vector>

namespace {

using OrtLoggingLevel = int;
using OrtAllocatorType = int;
using OrtMemType = int;
using ONNXTensorElementDataType = int;

constexpr ONNXTensorElementDataType kOnnxTensorFloat = 1;
constexpr uint32_t kOrtApiVersion = 1U;

struct OrtStatus {
    int code{0};
    std::string msg;
};

struct OrtEnv {
    std::string name;
};

struct OrtSessionOptions {
    int intra_op{0};
    int inter_op{0};
    bool mem_pattern_disabled{false};
    bool cpu_arena_disabled{false};
};

struct OrtSession {
    bool emit_nan{false};
};

struct OrtMemoryInfo {
    OrtAllocatorType alloc_type{0};
    OrtMemType mem_type{0};
};

struct OrtValue {
    std::vector<float> data;
    std::vector<int64_t> shape;
    ONNXTensorElementDataType elem_type{kOnnxTensorFloat};
};

struct OrtTensorTypeAndShapeInfo {
    ONNXTensorElementDataType elem_type{kOnnxTensorFloat};
    size_t elem_count{0};
};

struct OrtRunOptions {};
struct OrtAllocator {
    int tag{1};
};

std::atomic<int> g_live_handles{0};
std::atomic<int> g_options_mask{0};
std::atomic<int> g_run_count{0};
OrtAllocator g_default_allocator{};

OrtStatus* make_status(int code, const char* msg) noexcept {
    auto* st = new (std::nothrow) OrtStatus{code, msg ? msg : "error"};
    if (st != nullptr) g_live_handles.fetch_add(1, std::memory_order_relaxed);
    return st;
}

void* api_create_status(int code, const char* msg) noexcept {
    return make_status(code, msg);
}

int api_get_error_code(const OrtStatus* st) noexcept {
    return st ? st->code : 0;
}

const char* api_get_error_message(const OrtStatus* st) noexcept {
    return st ? st->msg.c_str() : "";
}

OrtStatus* api_create_env(OrtLoggingLevel /*level*/, const char* logid, OrtEnv** out) noexcept {
    if (out == nullptr) return make_status(1, "null out");
    auto* env = new (std::nothrow) OrtEnv{logid ? logid : ""};
    if (env == nullptr) return make_status(2, "oom");
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = env;
    return nullptr;
}

OrtStatus* api_enable_telemetry(const OrtEnv* /*env*/) noexcept { return nullptr; }

OrtStatus* api_disable_telemetry(const OrtEnv* /*env*/) noexcept {
    g_options_mask.fetch_or(1 << 4, std::memory_order_relaxed);
    return nullptr;
}

OrtStatus* api_create_session_options(OrtSessionOptions** out) noexcept {
    if (out == nullptr) return make_status(1, "null opts");
    auto* opts = new (std::nothrow) OrtSessionOptions{};
    if (opts == nullptr) return make_status(2, "oom");
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = opts;
    return nullptr;
}

OrtStatus* api_disable_mem_pattern(OrtSessionOptions* opts) noexcept {
    if (opts == nullptr) return make_status(1, "null opts");
    opts->mem_pattern_disabled = true;
    g_options_mask.fetch_or(1 << 2, std::memory_order_relaxed);
    return nullptr;
}

OrtStatus* api_disable_cpu_mem_arena(OrtSessionOptions* opts) noexcept {
    if (opts == nullptr) return make_status(1, "null opts");
    opts->cpu_arena_disabled = true;
    g_options_mask.fetch_or(1 << 3, std::memory_order_relaxed);
    return nullptr;
}

OrtStatus* api_set_intra_op(OrtSessionOptions* opts, int n) noexcept {
    if (opts == nullptr) return make_status(1, "null opts");
    opts->intra_op = n;
    if (n == 1) g_options_mask.fetch_or(1 << 0, std::memory_order_relaxed);
    return nullptr;
}

OrtStatus* api_set_inter_op(OrtSessionOptions* opts, int n) noexcept {
    if (opts == nullptr) return make_status(1, "null opts");
    opts->inter_op = n;
    if (n == 1) g_options_mask.fetch_or(1 << 1, std::memory_order_relaxed);
    return nullptr;
}

OrtStatus* api_create_session(const OrtEnv* env, const char* model_path,
                              const OrtSessionOptions* opts, OrtSession** out) noexcept {
    if (env == nullptr || opts == nullptr || model_path == nullptr || out == nullptr) {
        return make_status(1, "invalid args");
    }
    std::ifstream ifs(model_path, std::ios::binary);
    if (!ifs.is_open()) return make_status(3, "missing model");
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (content.find("CORRUPT_ONNX") != std::string::npos) {
        return make_status(4, "corrupt model");
    }
    auto* sess = new (std::nothrow) OrtSession{};
    if (sess == nullptr) return make_status(2, "oom");
    sess->emit_nan = (content.find("NAN_OUTPUT") != std::string::npos);
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = sess;
    return nullptr;
}

OrtStatus* api_session_get_input_count(const OrtSession* sess, size_t* out) noexcept {
    if (sess == nullptr || out == nullptr) return make_status(1, "invalid");
    *out = 1U;
    return nullptr;
}

OrtStatus* api_session_get_output_count(const OrtSession* sess, size_t* out) noexcept {
    if (sess == nullptr || out == nullptr) return make_status(1, "invalid");
    *out = 1U;
    return nullptr;
}

OrtStatus* api_get_default_allocator(OrtAllocator** out) noexcept {
    if (out == nullptr) return make_status(1, "invalid");
    *out = &g_default_allocator;
    return nullptr;
}

OrtStatus* api_session_get_input_name(const OrtSession* sess, size_t idx,
                                      OrtAllocator* alloc, char** out) noexcept {
    if (sess == nullptr || alloc == nullptr || out == nullptr || idx != 0U) {
        return make_status(1, "invalid");
    }
    constexpr const char kName[] = "features";
    char* copy = static_cast<char*>(std::malloc(sizeof(kName)));
    if (copy == nullptr) return make_status(2, "oom");
    std::memcpy(copy, kName, sizeof(kName));
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = copy;
    return nullptr;
}

OrtStatus* api_session_get_output_name(const OrtSession* sess, size_t idx,
                                       OrtAllocator* alloc, char** out) noexcept {
    if (sess == nullptr || alloc == nullptr || out == nullptr || idx != 0U) {
        return make_status(1, "invalid");
    }
    constexpr const char kName[] = "logits";
    char* copy = static_cast<char*>(std::malloc(sizeof(kName)));
    if (copy == nullptr) return make_status(2, "oom");
    std::memcpy(copy, kName, sizeof(kName));
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = copy;
    return nullptr;
}

OrtStatus* api_allocator_free(OrtAllocator* alloc, void* ptr) noexcept {
    if (alloc == nullptr) return make_status(1, "null alloc");
    if (ptr != nullptr) {
        std::free(ptr);
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
    return nullptr;
}

OrtStatus* api_create_cpu_memory_info(OrtAllocatorType alloc_type, OrtMemType mem_type,
                                      OrtMemoryInfo** out) noexcept {
    if (out == nullptr) return make_status(1, "null out");
    auto* info = new (std::nothrow) OrtMemoryInfo{alloc_type, mem_type};
    if (info == nullptr) return make_status(2, "oom");
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = info;
    return nullptr;
}

OrtStatus* api_create_tensor_with_data(const OrtMemoryInfo* mem_info, void* data, size_t data_len,
                                       const int64_t* shape, size_t shape_len,
                                       ONNXTensorElementDataType elem_type,
                                       OrtValue** out) noexcept {
    if (mem_info == nullptr || data == nullptr || shape == nullptr || out == nullptr ||
        elem_type != kOnnxTensorFloat || data_len % sizeof(float) != 0U) {
        return make_status(1, "invalid tensor args");
    }
    auto* val = new (std::nothrow) OrtValue{};
    if (val == nullptr) return make_status(2, "oom");
    const size_t count = data_len / sizeof(float);
    val->data.resize(count);
    std::memcpy(val->data.data(), data, data_len);
    val->shape.assign(shape, shape + shape_len);
    val->elem_type = elem_type;
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = val;
    return nullptr;
}

OrtStatus* api_run(OrtSession* sess, const OrtRunOptions* /*run_opts*/,
                   const char* const* in_names, const OrtValue* const* inputs, size_t in_len,
                   const char* const* out_names, size_t out_len, OrtValue** outputs) noexcept {
    if (sess == nullptr || in_names == nullptr || inputs == nullptr || in_len != 1U ||
        out_names == nullptr || out_len != 1U || outputs == nullptr || inputs[0] == nullptr) {
        return make_status(1, "invalid run args");
    }
    if (std::strcmp(in_names[0], "features") != 0 || std::strcmp(out_names[0], "logits") != 0) {
        return make_status(5, "io name mismatch");
    }
    if (inputs[0]->data.size() < 10U) {
        return make_status(6, "input dim mismatch");
    }

    auto* out = new (std::nothrow) OrtValue{};
    if (out == nullptr) return make_status(2, "oom");
    out->shape = {1, 3};
    out->elem_type = kOnnxTensorFloat;
    const auto& x = inputs[0]->data;
    if (sess->emit_nan) {
        out->data = {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f};
    } else {
        const float out0 = 0.85f * x[0] + 0.50f * x[1] + 0.25f * x[9];
        const float out1 = -0.50f + 2.40f * x[7] + 0.60f * x[6];
        const float out2 = 1.20f - 1.60f * x[0] - 0.70f * x[1];
        out->data = {out0, out1, out2};
    }
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    g_run_count.fetch_add(1, std::memory_order_relaxed);
    outputs[0] = out;
    return nullptr;
}

OrtStatus* api_is_tensor(const OrtValue* val, int* out) noexcept {
    if (val == nullptr || out == nullptr) return make_status(1, "invalid");
    *out = 1;
    return nullptr;
}

OrtStatus* api_get_tensor_mutable_data(OrtValue* val, void** out) noexcept {
    if (val == nullptr || out == nullptr) return make_status(1, "invalid");
    *out = val->data.data();
    return nullptr;
}

OrtStatus* api_get_tensor_type_and_shape(const OrtValue* val,
                                         OrtTensorTypeAndShapeInfo** out) noexcept {
    if (val == nullptr || out == nullptr) return make_status(1, "invalid");
    auto* info = new (std::nothrow) OrtTensorTypeAndShapeInfo{val->elem_type, val->data.size()};
    if (info == nullptr) return make_status(2, "oom");
    g_live_handles.fetch_add(1, std::memory_order_relaxed);
    *out = info;
    return nullptr;
}

OrtStatus* api_get_tensor_element_type(const OrtTensorTypeAndShapeInfo* info,
                                       ONNXTensorElementDataType* out) noexcept {
    if (info == nullptr || out == nullptr) return make_status(1, "invalid");
    *out = info->elem_type;
    return nullptr;
}

OrtStatus* api_get_tensor_shape_element_count(const OrtTensorTypeAndShapeInfo* info,
                                              size_t* out) noexcept {
    if (info == nullptr || out == nullptr) return make_status(1, "invalid");
    *out = info->elem_count;
    return nullptr;
}

void api_release_env(OrtEnv* p) noexcept {
    if (p != nullptr) {
        delete p;
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
}

void api_release_status(OrtStatus* p) noexcept {
    if (p != nullptr) {
        delete p;
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
}

void api_release_memory_info(OrtMemoryInfo* p) noexcept {
    if (p != nullptr) {
        delete p;
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
}

void api_release_session(OrtSession* p) noexcept {
    if (p != nullptr) {
        delete p;
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
}

void api_release_value(OrtValue* p) noexcept {
    if (p != nullptr) {
        delete p;
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
}

void api_release_tensor_type_and_shape_info(OrtTensorTypeAndShapeInfo* p) noexcept {
    if (p != nullptr) {
        delete p;
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
}

void api_release_session_options(OrtSessionOptions* p) noexcept {
    if (p != nullptr) {
        delete p;
        g_live_handles.fetch_sub(1, std::memory_order_relaxed);
    }
}

struct OrtApiV1 {
    void* (*CreateStatus)(int, const char*) noexcept;                                // 0
    int (*GetErrorCode)(const OrtStatus*) noexcept;                                  // 1
    const char* (*GetErrorMessage)(const OrtStatus*) noexcept;                       // 2
    OrtStatus* (*CreateEnv)(OrtLoggingLevel, const char*, OrtEnv**) noexcept;        // 3
    void* CreateEnvWithCustomLogger;                                                 // 4
    OrtStatus* (*EnableTelemetryEvents)(const OrtEnv*) noexcept;                     // 5
    OrtStatus* (*DisableTelemetryEvents)(const OrtEnv*) noexcept;                    // 6
    OrtStatus* (*CreateSession)(const OrtEnv*, const char*, const OrtSessionOptions*,
                                OrtSession**) noexcept;                              // 7
    void* CreateSessionFromArray;                                                    // 8
    OrtStatus* (*Run)(OrtSession*, const OrtRunOptions*, const char* const*,
                      const OrtValue* const*, size_t, const char* const*, size_t,
                      OrtValue**) noexcept;                                          // 9
    OrtStatus* (*CreateSessionOptions)(OrtSessionOptions**) noexcept;                // 10
    void* SetOptimizedModelFilePath;                                                 // 11
    void* CloneSessionOptions;                                                       // 12
    void* SetSessionExecutionMode;                                                   // 13
    void* EnableProfiling;                                                           // 14
    void* DisableProfiling;                                                          // 15
    void* EnableMemPattern;                                                          // 16
    OrtStatus* (*DisableMemPattern)(OrtSessionOptions*) noexcept;                    // 17
    void* EnableCpuMemArena;                                                         // 18
    OrtStatus* (*DisableCpuMemArena)(OrtSessionOptions*) noexcept;                   // 19
    void* SetSessionLogId;                                                           // 20
    void* SetSessionLogVerbosityLevel;                                               // 21
    void* SetSessionLogSeverityLevel;                                                // 22
    void* SetSessionGraphOptimizationLevel;                                          // 23
    OrtStatus* (*SetIntraOpNumThreads)(OrtSessionOptions*, int) noexcept;            // 24
    OrtStatus* (*SetInterOpNumThreads)(OrtSessionOptions*, int) noexcept;            // 25
    void* CreateCustomOpDomain;                                                      // 26
    void* CustomOpDomain_Add;                                                        // 27
    void* AddCustomOpDomain;                                                         // 28
    void* RegisterCustomOpsLibrary;                                                  // 29
    OrtStatus* (*SessionGetInputCount)(const OrtSession*, size_t*) noexcept;         // 30
    OrtStatus* (*SessionGetOutputCount)(const OrtSession*, size_t*) noexcept;        // 31
    void* SessionGetOverridableInitializerCount;                                     // 32
    void* SessionGetInputTypeInfo;                                                   // 33
    void* SessionGetOutputTypeInfo;                                                  // 34
    void* SessionGetOverridableInitializerTypeInfo;                                  // 35
    OrtStatus* (*SessionGetInputName)(const OrtSession*, size_t, OrtAllocator*,
                                      char**) noexcept;                              // 36
    OrtStatus* (*SessionGetOutputName)(const OrtSession*, size_t, OrtAllocator*,
                                       char**) noexcept;                             // 37
    void* SessionGetOverridableInitializerName;                                      // 38
    void* run_options_reserved[9];                                                   // 39..47
    void* CreateTensorAsOrtValue;                                                    // 48
    OrtStatus* (*CreateTensorWithDataAsOrtValue)(const OrtMemoryInfo*, void*, size_t,
                                                 const int64_t*, size_t,
                                                 ONNXTensorElementDataType,
                                                 OrtValue**) noexcept;               // 49
    OrtStatus* (*IsTensor)(const OrtValue*, int*) noexcept;                          // 50
    OrtStatus* (*GetTensorMutableData)(OrtValue*, void**) noexcept;                  // 51
    void* tensor_misc_reserved[8];                                                   // 52..59
    OrtStatus* (*GetTensorElementType)(const OrtTensorTypeAndShapeInfo*,
                                       ONNXTensorElementDataType*) noexcept;         // 60
    void* GetDimensionsCount;                                                        // 61
    void* GetDimensions;                                                             // 62
    void* GetSymbolicDimensions;                                                     // 63
    OrtStatus* (*GetTensorShapeElementCount)(const OrtTensorTypeAndShapeInfo*,
                                             size_t*) noexcept;                      // 64
    OrtStatus* (*GetTensorTypeAndShape)(const OrtValue*,
                                        OrtTensorTypeAndShapeInfo**) noexcept;       // 65
    void* GetTypeInfo;                                                               // 66
    void* GetValueType;                                                              // 67
    void* CreateMemoryInfo;                                                          // 68
    OrtStatus* (*CreateCpuMemoryInfo)(OrtAllocatorType, OrtMemType,
                                      OrtMemoryInfo**) noexcept;                     // 69
    void* mem_info_reserved[5];                                                      // 70..74
    void* AllocatorAlloc;                                                            // 75
    OrtStatus* (*AllocatorFree)(OrtAllocator*, void*) noexcept;                      // 76
    void* AllocatorGetInfo;                                                          // 77
    OrtStatus* (*GetAllocatorWithDefaultOptions)(OrtAllocator**) noexcept;           // 78
    void* misc_kernel_reserved[13];                                                  // 79..91
    void (*ReleaseEnv)(OrtEnv*) noexcept;                                            // 92
    void (*ReleaseStatus)(OrtStatus*) noexcept;                                      // 93
    void (*ReleaseMemoryInfo)(OrtMemoryInfo*) noexcept;                              // 94
    void (*ReleaseSession)(OrtSession*) noexcept;                                    // 95
    void (*ReleaseValue)(OrtValue*) noexcept;                                        // 96
    void* ReleaseRunOptions;                                                         // 97
    void* ReleaseTypeInfo;                                                           // 98
    void (*ReleaseTensorTypeAndShapeInfo)(OrtTensorTypeAndShapeInfo*) noexcept;      // 99
    void (*ReleaseSessionOptions)(OrtSessionOptions*) noexcept;                      // 100
    void* ReleaseCustomOpDomain;                                                     // 101
};

struct OrtApiBase {
    const OrtApiV1* (*GetApi)(uint32_t version) noexcept;
    const char* (*GetVersionString)() noexcept;
};

OrtApiV1 build_api() noexcept {
    OrtApiV1 api{};
    api.CreateStatus = api_create_status;
    api.GetErrorCode = api_get_error_code;
    api.GetErrorMessage = api_get_error_message;
    api.CreateEnv = api_create_env;
    api.EnableTelemetryEvents = api_enable_telemetry;
    api.DisableTelemetryEvents = api_disable_telemetry;
    api.CreateSession = api_create_session;
    api.Run = api_run;
    api.CreateSessionOptions = api_create_session_options;
    api.DisableMemPattern = api_disable_mem_pattern;
    api.DisableCpuMemArena = api_disable_cpu_mem_arena;
    api.SetIntraOpNumThreads = api_set_intra_op;
    api.SetInterOpNumThreads = api_set_inter_op;
    api.SessionGetInputCount = api_session_get_input_count;
    api.SessionGetOutputCount = api_session_get_output_count;
    api.SessionGetInputName = api_session_get_input_name;
    api.SessionGetOutputName = api_session_get_output_name;
    api.CreateTensorWithDataAsOrtValue = api_create_tensor_with_data;
    api.IsTensor = api_is_tensor;
    api.GetTensorMutableData = api_get_tensor_mutable_data;
    api.GetTensorElementType = api_get_tensor_element_type;
    api.GetTensorShapeElementCount = api_get_tensor_shape_element_count;
    api.GetTensorTypeAndShape = api_get_tensor_type_and_shape;
    api.CreateCpuMemoryInfo = api_create_cpu_memory_info;
    api.AllocatorFree = api_allocator_free;
    api.GetAllocatorWithDefaultOptions = api_get_default_allocator;
    api.ReleaseEnv = api_release_env;
    api.ReleaseStatus = api_release_status;
    api.ReleaseMemoryInfo = api_release_memory_info;
    api.ReleaseSession = api_release_session;
    api.ReleaseValue = api_release_value;
    api.ReleaseTensorTypeAndShapeInfo = api_release_tensor_type_and_shape_info;
    api.ReleaseSessionOptions = api_release_session_options;
    return api;
}

const OrtApiV1* get_api(uint32_t version) noexcept {
    if (version != kOrtApiVersion) return nullptr;
    static const OrtApiV1 kApi = build_api();
    return &kApi;
}

const char* get_version_string() noexcept {
    return "1.19.0-coreflow-mock";
}

const OrtApiBase kApiBase{get_api, get_version_string};

} // namespace

extern "C" {

__attribute__((visibility("default")))
const OrtApiBase* OrtGetApiBase() noexcept {
    return &kApiBase;
}

__attribute__((visibility("default")))
int CoreFlowMockOrtLiveHandles() noexcept {
    return g_live_handles.load(std::memory_order_relaxed);
}

__attribute__((visibility("default")))
int CoreFlowMockOrtOptionsMask() noexcept {
    return g_options_mask.load(std::memory_order_relaxed);
}

__attribute__((visibility("default")))
int CoreFlowMockOrtRunCount() noexcept {
    return g_run_count.load(std::memory_order_relaxed);
}

__attribute__((visibility("default")))
void CoreFlowMockOrtResetStats() noexcept {
    g_options_mask.store(0, std::memory_order_relaxed);
    g_run_count.store(0, std::memory_order_relaxed);
}

} // extern "C"
