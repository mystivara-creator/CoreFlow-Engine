/**
 * @file neural_predictor.cpp
 * @brief Hybrid ONNX Runtime C-API Bridge, CFW2 Micro-MLP & Online RLS Predictor
 * @license Apache-2.0
 */
#include "coreflow/neural_predictor.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <new>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace coreflow {
namespace {

// ---------------------------------------------------------------------------
// Verified ONNX Runtime C-API v1 (append-only frozen ABI across ORT 1.x).
// Exact function-pointer slot layout verified against official onnxruntime_c_api.h.
// ---------------------------------------------------------------------------
struct OrtEnv;
struct OrtStatus;
struct OrtMemoryInfo;
struct OrtSession;
struct OrtValue;
struct OrtRunOptions;
struct OrtTypeInfo;
struct OrtTensorTypeAndShapeInfo;
struct OrtSessionOptions;
struct OrtCustomOpDomain;
struct OrtAllocator;

using OrtLoggingLevel = int;
using OrtAllocatorType = int;
using OrtMemType = int;
using ONNXTensorElementDataType = int;

constexpr OrtLoggingLevel kOrtLoggingError = 3;
constexpr OrtAllocatorType kOrtDeviceAllocator = 0;
constexpr OrtMemType kOrtMemTypeDefault = 0;
constexpr ONNXTensorElementDataType kOnnxTensorFloat = 1;
constexpr uint32_t kOrtApiVersion = 1U;

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
using OrtGetApiBaseFn = const OrtApiBase* (*)() noexcept;

bool ort_ok(const OrtApiV1* api, OrtStatus* st) noexcept {
    if (st == nullptr) return true;
    if (api != nullptr && api->ReleaseStatus != nullptr) {
        api->ReleaseStatus(st);
    }
    return false;
}

// ---------------------------------------------------------------------------
// CFW2 Binary Constants & IEEE CRC32 (table-free to preserve tiny binary size)
// ---------------------------------------------------------------------------
constexpr size_t kCfw2HeaderBytes = 24U;
constexpr size_t kCfw2MaxFileBytes = 16384U;
constexpr float kCfw2MaxAbsWeight = 50.0f;

uint32_t ieee_crc32(const uint8_t* data, size_t len) noexcept {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint32_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            const uint32_t mask = (crc & 1U) ? 0xEDB88320U : 0U;
            crc = (crc >> 1U) ^ mask;
        }
    }
    return ~crc;
}

uint16_t read_le16(const uint8_t* p) noexcept {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8U);
}

uint32_t read_le32(const uint8_t* p) noexcept {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8U) |
           (static_cast<uint32_t>(p[2]) << 16U) |
           (static_cast<uint32_t>(p[3]) << 24U);
}

float read_le_f32(const uint8_t* p) noexcept {
    const uint32_t bits = read_le32(p);
    float out = 0.0f;
    std::memcpy(&out, &bits, sizeof(out));
    return out;
}

bool regular_file_exists(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    return ::fstat(fd.get(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}

double activate(double x, bool use_silu) noexcept {
    if (!use_silu) return std::tanh(x);
    const double clamped = std::clamp(x, -12.0, 12.0);
    return clamped / (1.0 + std::exp(-clamped));
}

std::string_view classify_workload(
    double thermal_pressure,
    double velocity,
    double cpu_psi,
    double io_psi,
    double gpu_busy,
    double render_pressure) noexcept {
    if (thermal_pressure >= 78.0 || (thermal_pressure >= 55.0 && velocity >= 1.5)) return "thermal_surge";
    if (gpu_busy >= 55.0) return "gpu_heavy";
    if (io_psi >= 10.0) return "io_bound";
    if (render_pressure >= 0.25 || cpu_psi >= 15.0) return "interactive";
    if (cpu_psi < 4.0 && gpu_busy < 12.0 && render_pressure < 0.08) return "idle";
    return "balanced";
}

} // namespace

struct NeuralPredictor::OrtRuntimeState {
    void* lib_handle{nullptr};
    const OrtApiV1* api{nullptr};
    OrtEnv* env{nullptr};
    OrtSessionOptions* opts{nullptr};
    OrtSession* session{nullptr};
    OrtMemoryInfo* mem_info{nullptr};
    std::string input_name;
    std::string output_name;
};

NeuralPredictor::NeuralPredictor(std::string model_dir) : model_dir_(std::move(model_dir)) {
    initialize(model_dir_);
}

NeuralPredictor::~NeuralPredictor() {
    close_onnx_runtime();
}

std::string_view NeuralPredictor::active_backend_name() const noexcept {
    if (onnx_loaded_) return "onnx-runtime-live";
    if (cfw2_loaded_) return "cfw2-micro-mlp";
    if (legacy_cfw1_loaded_) return "custom-cfw-rls-mlp";
    return "embedded-rls-mlp";
}

void NeuralPredictor::init_default_network() noexcept {
    // Initialize 4x4 RLS covariance matrix P = 10 * I
    rls_cov_.fill(0.0);
    for (size_t i = 0; i < 4; ++i) {
        rls_cov_[i * 4 + i] = 10.0;
    }
    rls_weights_ = {0.0, 0.02, 0.14, -0.02};

    // Physics-calibrated 10 -> 16 -> 12 -> 3 baseline weights (matches tools/onnx_to_cfw2.py)
    cfw2_w1_.fill(0.0f);
    cfw2_b1_.fill(0.0f);
    auto set_h1 = [&](size_t u, std::array<float, kCfw2InDim> coeffs, float bias) {
        for (size_t j = 0; j < kCfw2InDim; ++j) {
            cfw2_w1_[u * kCfw2InDim + j] = coeffs[j];
        }
        cfw2_b1_[u] = bias;
    };
    set_h1(0,  { 1.30f,  1.10f,  0.35f,  0.05f,  0.10f,  0.45f,  0.20f,  0.00f,  0.30f,  0.55f}, -0.35f);
    set_h1(1,  { 0.45f,  0.30f,  1.15f, -0.15f, -0.10f,  0.50f,  0.40f,  0.15f,  0.35f,  0.10f}, -0.25f);
    set_h1(2,  { 0.15f,  0.10f,  0.10f,  1.25f,  1.15f,  0.05f,  0.25f,  0.35f,  0.10f,  0.00f}, -0.40f);
    set_h1(3,  {-1.10f, -0.80f, -0.60f, -0.40f, -0.40f, -0.50f, -0.30f, -0.30f, -0.15f, -0.25f},  0.30f);
    set_h1(4,  { 0.85f,  1.45f,  0.50f,  0.15f,  0.20f,  0.45f,  0.15f,  0.00f,  0.45f,  0.65f}, -0.30f);
    set_h1(5,  { 0.05f,  0.05f,  0.35f,  0.25f,  0.25f,  0.45f,  0.95f,  1.45f, -0.20f,  0.00f}, -0.45f);
    set_h1(6,  { 1.55f,  0.75f,  0.25f,  0.55f,  0.50f,  0.40f,  0.15f,  0.10f,  0.40f,  0.45f}, -0.55f);
    set_h1(7,  {-0.70f, -0.90f, -0.25f, -0.15f, -0.15f, -0.20f, -0.10f, -0.15f,  0.10f, -0.35f},  0.20f);
    set_h1(8,  { 0.10f,  0.15f,  0.10f,  0.00f,  0.00f,  1.25f,  0.85f,  0.55f,  0.20f,  0.15f}, -0.35f);
    set_h1(9,  { 0.40f,  0.60f,  0.05f,  0.00f,  0.00f,  0.10f,  0.05f,  0.00f,  0.15f,  1.35f}, -0.45f);
    set_h1(10, {-0.25f,  0.10f,  0.85f, -0.35f, -0.25f,  0.15f,  0.80f,  0.45f, -0.10f,  0.00f}, -0.20f);
    set_h1(11, { 0.00f,  0.00f,  0.20f,  0.80f,  0.90f,  0.00f,  0.55f,  0.95f, -0.15f,  0.00f}, -0.40f);
    set_h1(12, { 0.70f,  0.25f,  0.40f,  0.20f,  0.20f,  0.80f,  0.35f,  0.20f,  0.50f,  0.25f}, -0.40f);
    set_h1(13, {-0.55f, -0.40f,  0.30f, -0.20f, -0.20f,  0.35f,  0.65f,  0.60f, -0.25f, -0.15f}, -0.15f);
    set_h1(14, { 0.95f,  0.45f,  0.15f,  0.30f,  0.20f,  0.20f,  0.05f,  0.05f,  0.30f,  0.70f}, -0.45f);
    set_h1(15, {-0.45f, -0.55f, -0.45f, -0.25f, -0.25f, -0.35f, -0.55f, -0.60f,  0.15f, -0.10f},  0.25f);

    cfw2_w2_.fill(0.0f);
    cfw2_b2_.fill(0.0f);
    for (size_t h = 0; h < kCfw2H2Dim; ++h) {
        cfw2_w2_[h * kCfw2H1Dim + (h % kCfw2H1Dim)] = 0.75f;
        cfw2_w2_[h * kCfw2H1Dim + ((h + 4U) % kCfw2H1Dim)] = 0.25f;
    }

    cfw2_w3_ = {
         0.75f,  0.35f,  0.20f, -0.55f,  0.85f,  0.10f,  0.80f, -0.45f,  0.40f,  0.55f,  0.05f,  0.10f,
         0.10f,  0.30f,  0.55f, -0.50f,  0.05f,  1.15f,  0.20f, -0.30f,  0.45f,  0.05f,  0.55f,  0.90f,
        -0.95f, -0.20f, -0.65f,  0.75f, -0.85f,  0.25f, -1.10f,  0.65f, -0.35f, -0.60f,  0.30f, -0.25f,
    };
    cfw2_b3_ = {0.05f, -0.65f, 0.70f};
    cfw2_flags_ = 0U;

    // Legacy 6 -> 8 -> 2 defaults (used only if CFW1 text weights are loaded)
    legacy_w1_.fill(0.0);
    legacy_b1_.fill(0.0);
    legacy_w2_.fill(0.0);
    legacy_b2_ = {0.05, 0.65};
}

void NeuralPredictor::reset() noexcept {
    init_default_network();
    prev_features_.fill(0.0);
    prev_h2_.fill(0.0);
    prev_legacy_hidden_.fill(0.0);
    prev_out0_ = 0.0;
    prev_pressure_ = -1.0;
    ema_abs_error_ = 1.5;
    samples_ = 0;
    last_ = NeuralPrediction{
        .valid = true,
        .onnx_active = onnx_loaded_,
        .cfw2_active = cfw2_loaded_,
        .cfw2_crc32 = cfw2_crc32_,
        .predicted_pressure_10s = 45.0,
        .jank_risk = 0.0,
        .recommended_scale = 1.0,
        .confidence = 0.5,
        .prediction_mae = 0.0,
        .predicted_velocity = 0.0,
        .samples_observed = 0,
        .workload_class = "balanced",
        .backend = active_backend_name(),
    };
}

void NeuralPredictor::close_onnx_runtime() noexcept {
    if (ort_state_ != nullptr) {
        const OrtApiV1* api = ort_state_->api;
        if (api != nullptr) {
            if (ort_state_->mem_info != nullptr && api->ReleaseMemoryInfo != nullptr) {
                api->ReleaseMemoryInfo(ort_state_->mem_info);
            }
            if (ort_state_->session != nullptr && api->ReleaseSession != nullptr) {
                api->ReleaseSession(ort_state_->session);
            }
            if (ort_state_->opts != nullptr && api->ReleaseSessionOptions != nullptr) {
                api->ReleaseSessionOptions(ort_state_->opts);
            }
            if (ort_state_->env != nullptr && api->ReleaseEnv != nullptr) {
                api->ReleaseEnv(ort_state_->env);
            }
        }
        if (ort_state_->lib_handle != nullptr) {
            (void)::dlclose(ort_state_->lib_handle);
        }
        delete ort_state_;
        ort_state_ = nullptr;
    }
    onnx_loaded_ = false;
}

bool NeuralPredictor::try_load_cfw2(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) ||
        st.st_size < static_cast<off_t>(kCfw2HeaderBytes) ||
        st.st_size > static_cast<off_t>(kCfw2MaxFileBytes)) {
        return false;
    }

    std::vector<uint8_t> raw;
    raw.reserve(static_cast<size_t>(st.st_size));
    uint8_t buf[4096];
    while (raw.size() <= kCfw2MaxFileBytes) {
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n > 0) {
            raw.insert(raw.end(), buf, buf + n);
            continue;
        }
        if (n == 0) break;
        if (errno == EINTR) continue;
        return false;
    }
    if (raw.size() < kCfw2HeaderBytes || raw.size() > kCfw2MaxFileBytes) return false;
    if (raw[0] != 'C' || raw[1] != 'F' || raw[2] != 'W' || raw[3] != '2') return false;

    const uint16_t format_ver = read_le16(raw.data() + 4U);
    const uint16_t flags = read_le16(raw.data() + 6U);
    const uint16_t in_dim = read_le16(raw.data() + 8U);
    const uint16_t h1_dim = read_le16(raw.data() + 10U);
    const uint16_t h2_dim = read_le16(raw.data() + 12U);
    const uint16_t out_dim = read_le16(raw.data() + 14U);
    const uint32_t expected_crc = read_le32(raw.data() + 16U);
    const uint32_t reserved = read_le32(raw.data() + 20U);

    if (format_ver != 2U || reserved != 0U || (flags & ~0x0001U) != 0U) return false;
    if (in_dim != kCfw2InDim || h1_dim != kCfw2H1Dim ||
        h2_dim != kCfw2H2Dim || out_dim != kCfw2OutDim) {
        return false;
    }

    constexpr size_t kFloatCount =
        (kCfw2H1Dim * kCfw2InDim) + kCfw2H1Dim +
        (kCfw2H2Dim * kCfw2H1Dim) + kCfw2H2Dim +
        (kCfw2OutDim * kCfw2H2Dim) + kCfw2OutDim;
    const size_t payload_bytes = raw.size() - kCfw2HeaderBytes;
    if (payload_bytes != kFloatCount * sizeof(float)) return false;

    const uint32_t actual_crc = ieee_crc32(raw.data() + kCfw2HeaderBytes, payload_bytes);
    if (actual_crc != expected_crc) return false;

    std::array<float, kCfw2H1Dim * kCfw2InDim> cand_w1{};
    std::array<float, kCfw2H1Dim> cand_b1{};
    std::array<float, kCfw2H2Dim * kCfw2H1Dim> cand_w2{};
    std::array<float, kCfw2H2Dim> cand_b2{};
    std::array<float, kCfw2OutDim * kCfw2H2Dim> cand_w3{};
    std::array<float, kCfw2OutDim> cand_b3{};

    const uint8_t* cursor = raw.data() + kCfw2HeaderBytes;
    auto read_tensor = [&](auto& dst) -> bool {
        for (float& v : dst) {
            const float f = read_le_f32(cursor);
            cursor += sizeof(float);
            if (!std::isfinite(f) || std::abs(f) > kCfw2MaxAbsWeight) return false;
            v = f;
        }
        return true;
    };

    if (!read_tensor(cand_w1) || !read_tensor(cand_b1) ||
        !read_tensor(cand_w2) || !read_tensor(cand_b2) ||
        !read_tensor(cand_w3) || !read_tensor(cand_b3)) {
        return false;
    }

    cfw2_w1_ = cand_w1;
    cfw2_b1_ = cand_b1;
    cfw2_w2_ = cand_w2;
    cfw2_b2_ = cand_b2;
    cfw2_w3_ = cand_w3;
    cfw2_b3_ = cand_b3;
    cfw2_flags_ = flags;
    cfw2_crc32_ = actual_crc;
    return true;
}

bool NeuralPredictor::try_load_legacy_cfw1(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 16384) {
        return false;
    }

    std::string content;
    content.reserve(static_cast<size_t>(st.st_size));
    char buf[4096];
    while (content.size() < 16384U) {
        const size_t remaining = 16384U - content.size();
        const ssize_t n = ::read(fd.get(), buf, std::min(sizeof(buf), remaining));
        if (n > 0) {
            content.append(buf, static_cast<size_t>(n));
            continue;
        }
        if (n == 0) break;
        if (errno == EINTR) continue;
        return false;
    }
    if (content.empty()) return false;

    std::istringstream stream(content);
    std::string header;
    if (!(stream >> header) || header != "CFW1") return false;

    std::array<double, 48> cand_w1{};
    std::array<double, 8> cand_b1{};
    std::array<double, 16> cand_w2{};
    std::array<double, 2> cand_b2{};

    for (double& v : cand_w1) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_b1) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_w2) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_b2) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }

    legacy_w1_ = cand_w1;
    legacy_b1_ = cand_b1;
    legacy_w2_ = cand_w2;
    legacy_b2_ = cand_b2;
    return true;
}

bool NeuralPredictor::try_load_onnx_runtime(const std::string& onnx_path) noexcept {
    if (!regular_file_exists(onnx_path)) return false;

    std::vector<const char*> candidates;
#ifdef COREFLOW_ENABLE_TEST_HOOKS
    const char* test_ort_lib = std::getenv("COREFLOW_TEST_ONNX_LIB");
    if (test_ort_lib != nullptr && test_ort_lib[0] != '\0') {
        candidates.push_back(test_ort_lib);
    }
#endif
    candidates.push_back("/data/adb/coreflow/lib/libonnxruntime.so");
    candidates.push_back("/system/lib64/libonnxruntime.so");
    candidates.push_back("/vendor/lib64/libonnxruntime.so");
    candidates.push_back("libonnxruntime.so");

    for (const char* candidate : candidates) {
        void* handle = ::dlopen(candidate, RTLD_NOW | RTLD_LOCAL);
        if (handle == nullptr) continue;

        auto* sym = ::dlsym(handle, "OrtGetApiBase");
        if (sym == nullptr) {
            (void)::dlclose(handle);
            continue;
        }

        OrtGetApiBaseFn get_api_base = nullptr;
        std::memcpy(&get_api_base, &sym, sizeof(get_api_base));
        const OrtApiBase* base = get_api_base ? get_api_base() : nullptr;
        if (base == nullptr || base->GetApi == nullptr) {
            (void)::dlclose(handle);
            continue;
        }
        const OrtApiV1* api = base->GetApi(kOrtApiVersion);
        if (api == nullptr ||
            api->CreateEnv == nullptr || api->CreateSessionOptions == nullptr ||
            api->CreateSession == nullptr || api->Run == nullptr ||
            api->SessionGetInputCount == nullptr || api->SessionGetOutputCount == nullptr ||
            api->SessionGetInputName == nullptr || api->SessionGetOutputName == nullptr ||
            api->GetAllocatorWithDefaultOptions == nullptr || api->AllocatorFree == nullptr ||
            api->CreateCpuMemoryInfo == nullptr ||
            api->CreateTensorWithDataAsOrtValue == nullptr ||
            api->IsTensor == nullptr || api->GetTensorTypeAndShape == nullptr ||
            api->GetTensorElementType == nullptr || api->GetTensorShapeElementCount == nullptr ||
            api->GetTensorMutableData == nullptr ||
            api->ReleaseEnv == nullptr || api->ReleaseSessionOptions == nullptr ||
            api->ReleaseSession == nullptr || api->ReleaseMemoryInfo == nullptr ||
            api->ReleaseValue == nullptr || api->ReleaseTensorTypeAndShapeInfo == nullptr) {
            (void)::dlclose(handle);
            continue;
        }

        auto* state = new (std::nothrow) OrtRuntimeState{};
        if (state == nullptr) {
            (void)::dlclose(handle);
            return false;
        }
        state->lib_handle = handle;
        state->api = api;

        bool ok = ort_ok(api, api->CreateEnv(kOrtLoggingError, "coreflowd", &state->env)) &&
                  state->env != nullptr;
        if (ok && api->DisableTelemetryEvents != nullptr) {
            (void)ort_ok(api, api->DisableTelemetryEvents(state->env));
        }
        ok = ok && ort_ok(api, api->CreateSessionOptions(&state->opts)) && state->opts != nullptr;
        if (ok && api->SetIntraOpNumThreads != nullptr) {
            ok = ort_ok(api, api->SetIntraOpNumThreads(state->opts, 1));
        }
        if (ok && api->SetInterOpNumThreads != nullptr) {
            ok = ort_ok(api, api->SetInterOpNumThreads(state->opts, 1));
        }
        if (ok && api->DisableMemPattern != nullptr) {
            ok = ort_ok(api, api->DisableMemPattern(state->opts));
        }
        if (ok && api->DisableCpuMemArena != nullptr) {
            ok = ort_ok(api, api->DisableCpuMemArena(state->opts));
        }
        ok = ok && ort_ok(api, api->CreateSession(state->env, onnx_path.c_str(), state->opts, &state->session)) &&
             state->session != nullptr;

        size_t in_count = 0U;
        size_t out_count = 0U;
        ok = ok && ort_ok(api, api->SessionGetInputCount(state->session, &in_count)) &&
             ort_ok(api, api->SessionGetOutputCount(state->session, &out_count)) &&
             in_count >= 1U && out_count >= 1U;

        OrtAllocator* allocator = nullptr;
        ok = ok && ort_ok(api, api->GetAllocatorWithDefaultOptions(&allocator)) && allocator != nullptr;

        char* in_name_raw = nullptr;
        char* out_name_raw = nullptr;
        if (ok) {
            ok = ort_ok(api, api->SessionGetInputName(state->session, 0U, allocator, &in_name_raw)) &&
                 in_name_raw != nullptr && in_name_raw[0] != '\0';
            if (in_name_raw != nullptr) {
                state->input_name = in_name_raw;
                (void)ort_ok(api, api->AllocatorFree(allocator, in_name_raw));
            }
        }
        if (ok) {
            ok = ort_ok(api, api->SessionGetOutputName(state->session, 0U, allocator, &out_name_raw)) &&
                 out_name_raw != nullptr && out_name_raw[0] != '\0';
            if (out_name_raw != nullptr) {
                state->output_name = out_name_raw;
                (void)ort_ok(api, api->AllocatorFree(allocator, out_name_raw));
            }
        }

        ok = ok && ort_ok(api, api->CreateCpuMemoryInfo(kOrtDeviceAllocator, kOrtMemTypeDefault, &state->mem_info)) &&
             state->mem_info != nullptr;

        if (ok) {
            ort_state_ = state;
            onnx_loaded_ = true;
            return true;
        }

        ort_state_ = state;
        close_onnx_runtime();
    }
    return false;
}

bool NeuralPredictor::run_onnx_inference(
    const std::array<float, kCfw2InDim>& features,
    std::array<double, kCfw2OutDim>& out_logits) noexcept {
    if (!onnx_loaded_ || ort_state_ == nullptr || ort_state_->api == nullptr ||
        ort_state_->session == nullptr || ort_state_->mem_info == nullptr) {
        return false;
    }
    const OrtApiV1* api = ort_state_->api;

    std::array<float, kCfw2InDim> input_buf = features;
    const int64_t shape[2] = {1, static_cast<int64_t>(kCfw2InDim)};
    OrtValue* in_tensor = nullptr;
    if (!ort_ok(api, api->CreateTensorWithDataAsOrtValue(
            ort_state_->mem_info,
            input_buf.data(),
            input_buf.size() * sizeof(float),
            shape,
            2U,
            kOnnxTensorFloat,
            &in_tensor)) || in_tensor == nullptr) {
        return false;
    }

    const char* in_names[1] = {ort_state_->input_name.c_str()};
    const char* out_names[1] = {ort_state_->output_name.c_str()};
    OrtValue* out_tensor = nullptr;
    const bool ran = ort_ok(api, api->Run(
        ort_state_->session,
        nullptr,
        in_names,
        &in_tensor,
        1U,
        out_names,
        1U,
        &out_tensor));
    api->ReleaseValue(in_tensor);
    if (!ran || out_tensor == nullptr) return false;

    bool valid = false;
    int is_tensor = 0;
    OrtTensorTypeAndShapeInfo* info = nullptr;
    ONNXTensorElementDataType elem_type = 0;
    size_t elem_count = 0U;
    void* raw_out = nullptr;

    if (ort_ok(api, api->IsTensor(out_tensor, &is_tensor)) && is_tensor != 0 &&
        ort_ok(api, api->GetTensorTypeAndShape(out_tensor, &info)) && info != nullptr &&
        ort_ok(api, api->GetTensorElementType(info, &elem_type)) && elem_type == kOnnxTensorFloat &&
        ort_ok(api, api->GetTensorShapeElementCount(info, &elem_count)) && elem_count >= kCfw2OutDim &&
        ort_ok(api, api->GetTensorMutableData(out_tensor, &raw_out)) && raw_out != nullptr) {
        const auto* out_f = static_cast<const float*>(raw_out);
        if (std::isfinite(out_f[0]) && std::isfinite(out_f[1]) && std::isfinite(out_f[2])) {
            out_logits[0] = static_cast<double>(out_f[0]);
            out_logits[1] = static_cast<double>(out_f[1]);
            out_logits[2] = static_cast<double>(out_f[2]);
            valid = true;
        }
    }

    if (info != nullptr) api->ReleaseTensorTypeAndShapeInfo(info);
    api->ReleaseValue(out_tensor);
    return valid;
}

void NeuralPredictor::initialize(std::string_view model_dir) noexcept {
    model_dir_ = std::string(model_dir);
    close_onnx_runtime();
    cfw2_loaded_ = false;
    legacy_cfw1_loaded_ = false;
    cfw2_crc32_ = 0U;

    reset();
    (void)try_load_onnx_runtime(model_dir_ + "/equilibrium.onnx");
    cfw2_loaded_ = try_load_cfw2(model_dir_ + "/equilibrium.cfw2");
    if (!cfw2_loaded_) {
        legacy_cfw1_loaded_ = try_load_legacy_cfw1(model_dir_ + "/equilibrium.cfw");
    }
    last_.onnx_active = onnx_loaded_;
    last_.cfw2_active = cfw2_loaded_;
    last_.cfw2_crc32 = cfw2_crc32_;
    last_.backend = active_backend_name();
}

NeuralPrediction NeuralPredictor::infer_and_learn(const NeuralInput& input) noexcept {
    const bool finite = std::isfinite(input.thermal_pressure) &&
                        (!input.cpu_psi_valid || std::isfinite(input.cpu_psi)) &&
                        (!input.memory_psi_valid || std::isfinite(input.memory_psi)) &&
                        (!input.io_psi_valid || std::isfinite(input.io_psi)) &&
                        std::isfinite(input.current_scale) &&
                        std::isfinite(input.elapsed_seconds) &&
                        std::isfinite(input.gpu_busy_pct) &&
                        std::isfinite(input.render_pressure) &&
                        std::isfinite(input.jank_ratio) &&
                        std::isfinite(input.charging_power_mw);
    if (!finite || input.elapsed_seconds <= 0.0 || input.elapsed_seconds > 120.0 ||
        input.thermal_pressure < 0.0 || input.thermal_pressure > 130.0) {
        NeuralPrediction invalid{};
        invalid.valid = false;
        return invalid;
    }

    const double dt = std::clamp(input.elapsed_seconds, 0.05, 30.0);
    const double norm_p = std::clamp((input.thermal_pressure - 60.0) / 40.0, -1.5, 2.0);
    const double norm_cpu = input.cpu_psi_valid ? std::clamp(input.cpu_psi / 50.0, 0.0, 2.0) : 0.0;
    const double norm_mem = input.memory_psi_valid ? std::clamp(input.memory_psi / 25.0, 0.0, 2.0) : 0.0;
    const double norm_io = input.io_psi_valid ? std::clamp(input.io_psi / 25.0, 0.0, 2.0) : 0.0;
    const bool pressure_inputs_valid =
        input.cpu_psi_valid && input.memory_psi_valid && input.io_psi_valid;
    const double norm_scale = std::clamp((input.current_scale - 0.4) / 0.6, 0.0, 1.0);
    const double norm_gpu = std::clamp(input.gpu_busy_pct / 100.0, 0.0, 1.0);
    const double norm_render = std::clamp(input.render_pressure, 0.0, 1.0);
    const double norm_jank = std::clamp(input.jank_ratio, 0.0, 1.0);
    const double norm_charge = std::clamp(input.charging_power_mw / 15000.0, 0.0, 1.5);

    double velocity = 0.0;

    // Step 1: Online Recursive Least Squares (RLS) + NLMS residual adaptation
    if (pressure_inputs_valid && prev_pressure_ >= 0.0) {
        const double actual_rate = (input.thermal_pressure - prev_pressure_) / dt;
        velocity = std::clamp(actual_rate, -10.0, 10.0);

        double predicted_rate = 0.0;
        for (size_t i = 0; i < 4; ++i) {
            predicted_rate += rls_weights_[i] * prev_features_[i];
        }
        const double innovation = std::clamp(actual_rate - predicted_rate, -8.0, 8.0);
        ema_abs_error_ = 0.88 * ema_abs_error_ + 0.12 * std::abs(innovation);

        std::array<double, 4> px{};
        for (size_t i = 0; i < 4; ++i) {
            double sum = 0.0;
            for (size_t j = 0; j < 4; ++j) {
                sum += rls_cov_[i * 4 + j] * prev_features_[j];
            }
            px[i] = sum;
        }

        constexpr double kForgettingFactor = 0.96;
        double denom = kForgettingFactor;
        for (size_t i = 0; i < 4; ++i) {
            denom += prev_features_[i] * px[i];
        }
        if (denom > 1e-6) {
            std::array<double, 4> gain{};
            for (size_t i = 0; i < 4; ++i) {
                gain[i] = px[i] / denom;
                rls_weights_[i] = std::clamp(rls_weights_[i] + gain[i] * innovation, -5.0, 5.0);
            }
            std::array<double, 16> next_cov{};
            for (size_t i = 0; i < 4; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    const double updated = (rls_cov_[i * 4 + j] - gain[i] * px[j]) / kForgettingFactor;
                    next_cov[i * 4 + j] = std::clamp(updated, -25.0, 25.0);
                }
            }
            rls_cov_ = next_cov;
        }

        // Online NLMS adaptation on output-0 thermal residual head
        const double tanh_out0 = std::tanh(prev_out0_);
        const double grad_scale = (1.0 - tanh_out0 * tanh_out0);
        const double residual_err = std::clamp(innovation / 8.0 - tanh_out0, -1.0, 1.0);
        constexpr double kMlpStep = 0.015;
        if (legacy_cfw1_loaded_) {
            for (size_t h = 0; h < 8U; ++h) {
                legacy_w2_[h] = std::clamp(
                    legacy_w2_[h] + kMlpStep * residual_err * grad_scale * prev_legacy_hidden_[h],
                    -3.0, 3.0);
            }
            legacy_b2_[0] = std::clamp(legacy_b2_[0] + kMlpStep * residual_err * grad_scale, -1.5, 1.5);
        } else {
            for (size_t h = 0; h < kCfw2H2Dim; ++h) {
                const double w = static_cast<double>(cfw2_w3_[h]) +
                                 kMlpStep * residual_err * grad_scale * prev_h2_[h];
                cfw2_w3_[h] = static_cast<float>(std::clamp(w, -3.0, 3.0));
            }
            cfw2_b3_[0] = static_cast<float>(
                std::clamp(static_cast<double>(cfw2_b3_[0]) + kMlpStep * residual_err * grad_scale,
                           -1.5, 1.5));
        }
    }

    const std::array<double, 4> current_rls_features{
        1.0,
        norm_p,
        std::clamp(norm_cpu + 0.5 * norm_io, 0.0, 2.5),
        norm_scale,
    };
    if (pressure_inputs_valid) {
        prev_features_ = current_rls_features;
        prev_pressure_ = input.thermal_pressure;
    } else {
        prev_pressure_ = -1.0;
    }
    ++samples_;

    // Step 2: RLS 10-second linear horizon projection
    double rls_rate_forecast = 0.0;
    for (size_t i = 0; i < 4; ++i) {
        rls_rate_forecast += rls_weights_[i] * current_rls_features[i];
    }
    rls_rate_forecast = std::clamp(rls_rate_forecast, -4.0, 4.0);

    // Step 3: Three-Tier Neural Forward Pass
    const std::array<float, kCfw2InDim> features10{
        static_cast<float>(norm_p),
        static_cast<float>(std::clamp(velocity / 2.0, -2.0, 2.0)),
        static_cast<float>(norm_cpu),
        static_cast<float>(norm_mem),
        static_cast<float>(norm_io),
        static_cast<float>(norm_gpu),
        static_cast<float>(norm_render),
        static_cast<float>(norm_jank),
        static_cast<float>(norm_scale),
        static_cast<float>(norm_charge),
    };

    std::array<double, kCfw2OutDim> out{0.0, -0.65, 0.70};
    bool used_onnx = false;

    if (onnx_loaded_) {
        used_onnx = run_onnx_inference(features10, out);
    }

    if (!used_onnx) {
        if (legacy_cfw1_loaded_) {
            const std::array<double, 6> mlp_in{
                norm_p,
                std::clamp(velocity / 2.0, -2.0, 2.0),
                norm_cpu,
                norm_mem,
                norm_io,
                norm_scale,
            };
            std::array<double, 8> hidden{};
            for (size_t h = 0; h < 8U; ++h) {
                double acc = legacy_b1_[h];
                for (size_t k = 0; k < 6U; ++k) {
                    acc += legacy_w1_[h * 6U + k] * mlp_in[k];
                }
                hidden[h] = std::tanh(acc);
            }
            prev_legacy_hidden_ = hidden;
            double legacy_out0 = legacy_b2_[0];
            double legacy_out1 = legacy_b2_[1];
            for (size_t h = 0; h < 8U; ++h) {
                legacy_out0 += legacy_w2_[h] * hidden[h];
                legacy_out1 += legacy_w2_[8U + h] * hidden[h];
            }
            out[0] = legacy_out0;
            out[1] = -0.8 + 2.2 * norm_jank + 0.6 * norm_io;
            out[2] = legacy_out1;
        } else {
            // Primary 10 -> 16 -> 12 -> 3 zero-allocation CFW2 / built-in forward pass
            const bool use_silu = (cfw2_flags_ & 0x0001U) != 0U;
            std::array<double, kCfw2H1Dim> h1{};
            for (size_t i = 0; i < kCfw2H1Dim; ++i) {
                double acc = static_cast<double>(cfw2_b1_[i]);
                const size_t row = i * kCfw2InDim;
                for (size_t k = 0; k < kCfw2InDim; ++k) {
                    acc += static_cast<double>(cfw2_w1_[row + k]) * static_cast<double>(features10[k]);
                }
                h1[i] = activate(acc, use_silu);
            }

            std::array<double, kCfw2H2Dim> h2{};
            for (size_t i = 0; i < kCfw2H2Dim; ++i) {
                double acc = static_cast<double>(cfw2_b2_[i]);
                const size_t row = i * kCfw2H1Dim;
                for (size_t k = 0; k < kCfw2H1Dim; ++k) {
                    acc += static_cast<double>(cfw2_w2_[row + k]) * h1[k];
                }
                h2[i] = activate(acc, use_silu);
            }
            prev_h2_ = h2;

            for (size_t o = 0; o < kCfw2OutDim; ++o) {
                double acc = static_cast<double>(cfw2_b3_[o]);
                const size_t row = o * kCfw2H2Dim;
                for (size_t k = 0; k < kCfw2H2Dim; ++k) {
                    acc += static_cast<double>(cfw2_w3_[row + k]) * h2[k];
                }
                out[o] = acc;
            }
        }
    }

    prev_out0_ = out[0];

    const double nonlinear_pressure_delta = std::tanh(std::clamp(out[0], -6.0, 6.0)) * 10.0;
    const double predicted_10s = std::clamp(
        input.thermal_pressure + rls_rate_forecast * 6.0 + nonlinear_pressure_delta,
        0.0,
        130.0);

    const double jank_risk = std::clamp(
        1.0 / (1.0 + std::exp(-std::clamp(out[1], -8.0, 8.0))),
        0.0,
        1.0);

    const double sigmoid_scale = 1.0 / (1.0 + std::exp(-std::clamp(out[2], -8.0, 8.0)));
    const double recommended_scale = std::clamp(0.40 + 0.60 * sigmoid_scale, 0.40, 1.00);

    const double warmup_factor = std::min(1.0, static_cast<double>(samples_) / 5.0);
    const double error_confidence = 1.0 / (1.0 + 0.35 * ema_abs_error_);
    const double confidence = pressure_inputs_valid
        ? std::clamp(0.35 + 0.65 * warmup_factor * error_confidence, 0.10, 0.99)
        : std::min(0.35, std::clamp(0.35 + 0.65 * warmup_factor * error_confidence, 0.10, 0.99));

    const std::string_view effective_backend = used_onnx
        ? "onnx-runtime-live"
        : (cfw2_loaded_ ? "cfw2-micro-mlp"
                        : (legacy_cfw1_loaded_ ? "custom-cfw-rls-mlp" : "embedded-rls-mlp"));

    last_ = NeuralPrediction{
        .valid = true,
        .onnx_active = used_onnx,
        .cfw2_active = cfw2_loaded_ && !used_onnx,
        .cfw2_crc32 = cfw2_crc32_,
        .predicted_pressure_10s = predicted_10s,
        .jank_risk = jank_risk,
        .recommended_scale = recommended_scale,
        .confidence = confidence,
        .prediction_mae = ema_abs_error_,
        .predicted_velocity = rls_rate_forecast,
        .samples_observed = samples_,
        .workload_class = classify_workload(
            input.thermal_pressure, velocity,
            input.cpu_psi_valid ? input.cpu_psi : 0.0,
            input.io_psi_valid ? input.io_psi : 0.0,
            input.gpu_busy_pct, input.render_pressure),
        .backend = effective_backend,
    };
    return last_;
}

NeuralPrediction NeuralPredictor::last_prediction() const noexcept {
    return last_;
}

} // namespace coreflow
