#include "coreflow/neural_predictor.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

uint32_t test_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint32_t>(data[i]);
        for (int b = 0; b < 8; ++b) {
            const uint32_t mask = (crc & 1U) ? 0xEDB88320U : 0U;
            crc = (crc >> 1U) ^ mask;
        }
    }
    return ~crc;
}

void append_le16(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFFU));
    buf.push_back(static_cast<uint8_t>((v >> 8U) & 0xFFU));
}

void append_le32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFFU));
    buf.push_back(static_cast<uint8_t>((v >> 8U) & 0xFFU));
    buf.push_back(static_cast<uint8_t>((v >> 16U) & 0xFFU));
    buf.push_back(static_cast<uint8_t>((v >> 24U) & 0xFFU));
}

void append_f32(std::vector<uint8_t>& buf, float v) {
    uint32_t bits = 0U;
    std::memcpy(&bits, &v, sizeof(bits));
    append_le32(buf, bits);
}

constexpr size_t kCfw2ParamCount =
    (16U * 10U) + 16U + (12U * 16U) + 12U + (3U * 12U) + 3U; // 419

std::vector<uint8_t> build_cfw2_blob(
    uint16_t flags,
    uint16_t in_dim,
    const std::vector<float>& weights,
    bool corrupt_crc = false) {
    std::vector<uint8_t> payload;
    payload.reserve(weights.size() * sizeof(float));
    for (float w : weights) append_f32(payload, w);

    uint32_t crc = test_crc32(payload.data(), payload.size());
    if (corrupt_crc) crc ^= 0x12345678U;

    std::vector<uint8_t> blob;
    blob.reserve(24U + payload.size());
    blob.push_back('C');
    blob.push_back('F');
    blob.push_back('W');
    blob.push_back('2');
    append_le16(blob, 2U);      // version
    append_le16(blob, flags);   // flags (0 = tanh, 1 = SiLU)
    append_le16(blob, in_dim);  // in_dim
    append_le16(blob, 16U);     // h1_dim
    append_le16(blob, 12U);     // h2_dim
    append_le16(blob, 3U);      // out_dim
    append_le32(blob, crc);     // crc32
    append_le32(blob, 0U);      // reserved
    blob.insert(blob.end(), payload.begin(), payload.end());
    return blob;
}

void write_bytes(const fs::path& p, const std::vector<uint8_t>& data) {
    std::ofstream ofs(p, std::ios::binary | std::ios::trunc);
    assert(ofs.is_open());
    ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    assert(ofs.good());
}

void write_text(const fs::path& p, const std::string& text) {
    std::ofstream ofs(p, std::ios::binary | std::ios::trunc);
    assert(ofs.is_open());
    ofs << text;
    assert(ofs.good());
}

template <typename Fn>
Fn load_sym(void* handle, const char* name) {
    void* raw = ::dlsym(handle, name);
    assert(raw != nullptr);
    Fn fn = nullptr;
    std::memcpy(&fn, &raw, sizeof(fn));
    return fn;
}

} // namespace

int main() {
    char pattern[] = "/tmp/coreflow-neural-XXXXXX";
    const char* dir_raw = ::mkdtemp(pattern);
    assert(dir_raw != nullptr);
    const fs::path dir(dir_raw);

    // Ensure test starts clean without ONNX env override
    ::unsetenv("COREFLOW_TEST_ONNX_LIB");

    // -----------------------------------------------------------------------
    // 1. Tier 3 Embedded RLS-MLP: NaN rejection, partial PSI, workload classes,
    //    jank_risk response, and proactive thermal pre-cooling
    // -----------------------------------------------------------------------
    {
        coreflow::NeuralPredictor predictor(dir.string());

        const auto bad = predictor.infer_and_learn({
            .thermal_pressure = std::numeric_limits<double>::quiet_NaN(),
            .cpu_psi = 10.0,
            .memory_psi = 0.0,
            .io_psi = 0.0,
            .current_scale = 1.0,
            .elapsed_seconds = 1.0,
        });
        assert(!bad.valid);

        // Empty or relative model_dir must safely fall back to embedded Tier 3
        {
            coreflow::NeuralPredictor empty_dir_pred("");
            assert(empty_dir_pred.last_prediction().backend == "embedded-rls-mlp");
            empty_dir_pred.initialize("relative/models");
            assert(empty_dir_pred.last_prediction().backend == "embedded-rls-mlp");
        }

        // Missing PSI must remain explicit and must not train RLS with synthetic zeros,
        // yet pure thermal velocity should still detect rapid heating!
        {
            coreflow::NeuralPredictor no_psi_pred(dir.string());
            const auto p1 = no_psi_pred.infer_and_learn({
                .thermal_pressure = 56.0,
                .cpu_psi = 0.0,
                .memory_psi = 0.0,
                .io_psi = 0.0,
                .cpu_psi_valid = false,
                .memory_psi_valid = false,
                .io_psi_valid = false,
                .current_scale = 1.0,
                .elapsed_seconds = 1.0,
            });
            assert(p1.valid && p1.confidence <= 0.35);
            const auto p2 = no_psi_pred.infer_and_learn({
                .thermal_pressure = 60.0,
                .cpu_psi = 0.0,
                .memory_psi = 0.0,
                .io_psi = 0.0,
                .cpu_psi_valid = false,
                .memory_psi_valid = false,
                .io_psi_valid = false,
                .current_scale = 1.0,
                .elapsed_seconds = 1.0,
            });
            assert(p2.valid && p2.confidence <= 0.35);
            assert(p2.workload_class == "thermal_surge");
        }

        const auto partial = predictor.infer_and_learn({
            .thermal_pressure = 43.0,
            .cpu_psi = 0.0,
            .memory_psi = 0.0,
            .io_psi = 0.0,
            .cpu_psi_valid = false,
            .memory_psi_valid = true,
            .io_psi_valid = false,
            .current_scale = 1.0,
            .elapsed_seconds = 1.0,
        });
        assert(partial.valid);
        assert(partial.confidence <= 0.35);

        const auto idle = predictor.infer_and_learn({
            .thermal_pressure = 38.0,
            .cpu_psi = 1.5,
            .memory_psi = 0.0,
            .io_psi = 0.0,
            .current_scale = 1.0,
            .elapsed_seconds = 1.0,
            .gpu_busy_pct = 5.0,
            .render_pressure = 0.02,
            .jank_ratio = 0.0,
            .charging_power_mw = 0.0,
        });
        assert(idle.valid);
        assert(idle.backend == "embedded-rls-mlp");
        assert(!idle.cfw2_active && !idle.onnx_active);
        assert(idle.workload_class == "idle");
        assert(idle.recommended_scale >= 0.70 && idle.recommended_scale <= 1.0);
        assert(idle.predicted_pressure_10s >= 0.0 && idle.predicted_pressure_10s <= 130.0);
        assert(idle.jank_risk >= 0.0 && idle.jank_risk <= 0.45);

        const auto gpu_jank = predictor.infer_and_learn({
            .thermal_pressure = 56.0,
            .cpu_psi = 22.0,
            .memory_psi = 6.0,
            .io_psi = 2.0,
            .current_scale = 0.95,
            .elapsed_seconds = 15.0,
            .gpu_busy_pct = 82.0,
            .render_pressure = 0.85,
            .jank_ratio = 0.75,
            .charging_power_mw = 0.0,
        });
        assert(gpu_jank.valid);
        assert(gpu_jank.workload_class == "gpu_heavy");
        assert(gpu_jank.jank_risk > idle.jank_risk + 0.15);

        const auto io_stall = predictor.infer_and_learn({
            .thermal_pressure = 52.0,
            .cpu_psi = 10.0,
            .memory_psi = 8.0,
            .io_psi = 18.0,
            .current_scale = 0.90,
            .elapsed_seconds = 1.0,
            .gpu_busy_pct = 15.0,
            .render_pressure = 0.30,
            .jank_ratio = 0.25,
            .charging_power_mw = 0.0,
        });
        assert(io_stall.valid);
        assert(io_stall.workload_class == "io_bound");

        coreflow::NeuralPrediction hot{};
        for (double p : {65.0, 74.0, 82.0, 89.0, 95.0}) {
            hot = predictor.infer_and_learn({
                .thermal_pressure = p,
                .cpu_psi = 45.0,
                .memory_psi = 12.0,
                .io_psi = 10.0,
                .current_scale = 0.90,
                .elapsed_seconds = 1.0,
                .gpu_busy_pct = 70.0,
                .render_pressure = 0.50,
                .jank_ratio = 0.30,
                .charging_power_mw = 12000.0,
            });
            assert(hot.valid);
        }
        assert(hot.samples_observed == 9);
        assert(hot.workload_class == "thermal_surge");
        assert(hot.predicted_pressure_10s > 85.0);
        assert(hot.recommended_scale < idle.recommended_scale);
        assert(hot.confidence >= 0.35 && hot.confidence <= 0.99);
        assert(std::isfinite(hot.predicted_velocity));
    }

    // -----------------------------------------------------------------------
    // 2. Tier 2 CFW2 Micro-MLP: valid tanh, valid SiLU, corrupt CRC, symlink,
    //    wrong dims, and NaN weight rejection
    // -----------------------------------------------------------------------
    {
        std::vector<float> valid_weights(kCfw2ParamCount, 0.08f);
        const fs::path cfw2_path = dir / "equilibrium.cfw2";

        // 2a. Valid CFW2 (tanh, flags = 0) + lossless O(1) reset() preservation
        write_bytes(cfw2_path, build_cfw2_blob(0U, 10U, valid_weights));
        coreflow::NeuralPredictor predictor(dir.string());
        const coreflow::NeuralInput cfw2_probe{
            .thermal_pressure = 50.0,
            .cpu_psi = 8.0,
            .memory_psi = 1.0,
            .io_psi = 1.0,
            .current_scale = 0.95,
            .elapsed_seconds = 1.0,
        };
        const auto pred_tanh = predictor.infer_and_learn(cfw2_probe);
        assert(pred_tanh.valid);
        assert(pred_tanh.cfw2_active);
        assert(!pred_tanh.onnx_active);
        assert(pred_tanh.cfw2_crc32 != 0U);
        assert(pred_tanh.backend == "cfw2-micro-mlp");

        // Train online for a few ticks, then reset(): must preserve CFW2 weights & exact fresh output!
        for (int i = 0; i < 4; ++i) {
            (void)predictor.infer_and_learn({
                .thermal_pressure = 75.0 + i * 3.0,
                .cpu_psi = 40.0,
                .memory_psi = 10.0,
                .io_psi = 8.0,
                .current_scale = 0.80,
                .elapsed_seconds = 1.0,
            });
        }
        predictor.reset();
        assert(predictor.last_prediction().cfw2_active);
        assert(predictor.last_prediction().backend == "cfw2-micro-mlp");
        const auto pred_after_reset = predictor.infer_and_learn(cfw2_probe);
        assert(pred_after_reset.cfw2_active);
        assert(pred_after_reset.backend == "cfw2-micro-mlp");
        assert(std::abs(pred_after_reset.predicted_pressure_10s - pred_tanh.predicted_pressure_10s) < 1e-6);

        // 2b. Valid CFW2 (SiLU, flags = 1)
        write_bytes(cfw2_path, build_cfw2_blob(1U, 10U, valid_weights));
        predictor.initialize(dir.string());
        const auto pred_silu = predictor.infer_and_learn(cfw2_probe);
        assert(pred_silu.valid);
        assert(pred_silu.cfw2_active);
        assert(pred_silu.backend == "cfw2-micro-mlp");
        assert(std::isfinite(pred_silu.predicted_pressure_10s));

        // 2c. Corrupt CRC32 must be rejected cleanly
        write_bytes(cfw2_path, build_cfw2_blob(0U, 10U, valid_weights, /*corrupt_crc=*/true));
        predictor.initialize(dir.string());
        assert(predictor.last_prediction().backend == "embedded-rls-mlp");
        assert(!predictor.last_prediction().cfw2_active);

        // 2d. Wrong input dimension must be rejected cleanly
        write_bytes(cfw2_path, build_cfw2_blob(0U, 8U, valid_weights));
        predictor.initialize(dir.string());
        assert(predictor.last_prediction().backend == "embedded-rls-mlp");
        assert(!predictor.last_prediction().cfw2_active);

        // 2e. Non-finite weight (NaN) even with valid CRC32 must be rejected cleanly
        std::vector<float> nan_weights = valid_weights;
        nan_weights[17] = std::numeric_limits<float>::quiet_NaN();
        write_bytes(cfw2_path, build_cfw2_blob(0U, 10U, nan_weights));
        predictor.initialize(dir.string());
        assert(predictor.last_prediction().backend == "embedded-rls-mlp");
        assert(!predictor.last_prediction().cfw2_active);

        // 2f. World-writable CFW2 must be rejected cleanly
        write_bytes(cfw2_path, build_cfw2_blob(0U, 10U, valid_weights));
        fs::permissions(cfw2_path, fs::perms::owner_read | fs::perms::owner_write |
                                   fs::perms::group_read | fs::perms::others_read | fs::perms::others_write,
                        fs::perm_options::replace);
        predictor.initialize(dir.string());
        assert(predictor.last_prediction().backend == "embedded-rls-mlp");
        assert(!predictor.last_prediction().cfw2_active);

        // 2g. Symlink equilibrium.cfw2 must be rejected via O_NOFOLLOW
        const fs::path real_target = dir / "real_target.cfw2";
        write_bytes(real_target, build_cfw2_blob(0U, 10U, valid_weights));
        fs::remove(cfw2_path);
        fs::create_symlink(real_target, cfw2_path);
        predictor.initialize(dir.string());
        assert(predictor.last_prediction().backend == "embedded-rls-mlp");
        assert(!predictor.last_prediction().cfw2_active);
        fs::remove(cfw2_path);
        fs::remove(real_target);
    }

    // -----------------------------------------------------------------------
    // 3. Backward-compatible Legacy CFW1 text weights
    // -----------------------------------------------------------------------
    {
        const fs::path cfw1_path = dir / "equilibrium.cfw";
        {
            std::ofstream cfw(cfw1_path);
            cfw << "CFW1\n";
            for (int i = 0; i < 48; ++i) cfw << "0.1 ";
            for (int i = 0; i < 8; ++i) cfw << "0.0 ";
            for (int i = 0; i < 16; ++i) cfw << "0.1 ";
            cfw << "0.0 0.5\n";
        }
        coreflow::NeuralPredictor predictor(dir.string());
        const auto custom = predictor.infer_and_learn({
            .thermal_pressure = 50.0,
            .cpu_psi = 5.0,
            .memory_psi = 0.0,
            .io_psi = 0.0,
            .current_scale = 1.0,
            .elapsed_seconds = 1.0,
        });
        assert(custom.valid);
        assert(custom.backend == "custom-cfw-rls-mlp");
        fs::remove(cfw1_path);
    }

    // -----------------------------------------------------------------------
    // 4. Tier 1 Dynamic ONNX Runtime C-API v1 (real dlopen of mock_onnxruntime)
    // -----------------------------------------------------------------------
#ifdef COREFLOW_MOCK_ORT_PATH
    {
        void* inspect_handle = ::dlopen(COREFLOW_MOCK_ORT_PATH, RTLD_NOW | RTLD_LOCAL);
        assert(inspect_handle != nullptr);
        using IntFn = int (*)() noexcept;
        using VoidFn = void (*)() noexcept;
        auto live_handles = load_sym<IntFn>(inspect_handle, "CoreFlowMockOrtLiveHandles");
        auto options_mask = load_sym<IntFn>(inspect_handle, "CoreFlowMockOrtOptionsMask");
        auto run_count = load_sym<IntFn>(inspect_handle, "CoreFlowMockOrtRunCount");
        auto reset_stats = load_sym<VoidFn>(inspect_handle, "CoreFlowMockOrtResetStats");

        assert(::setenv("COREFLOW_TEST_ONNX_LIB", COREFLOW_MOCK_ORT_PATH, 1) == 0);

        const fs::path onnx_path = dir / "equilibrium.onnx";
        const fs::path cfw2_path = dir / "equilibrium.cfw2";
        std::vector<float> fallback_weights(kCfw2ParamCount, 0.05f);
        write_bytes(cfw2_path, build_cfw2_blob(0U, 10U, fallback_weights));

        // 4a. Live ONNX session + startup dry-run + single-threaded/arena-free options + 0 leaks
        reset_stats();
        write_text(onnx_path, "MOCK_VALID_ONNX_V1");
        {
            coreflow::NeuralPredictor predictor(dir.string());
            assert(options_mask() == 0x1F);
            assert(run_count() == 1); // Startup dry-run self-test succeeded
            assert(predictor.last_prediction().onnx_active);
            assert(!predictor.last_prediction().cfw2_active);
            assert(predictor.last_prediction().backend == "onnx-runtime-live");

            const auto low_jank = predictor.infer_and_learn({
                .thermal_pressure = 42.0,
                .cpu_psi = 4.0,
                .memory_psi = 0.0,
                .io_psi = 0.0,
                .current_scale = 1.0,
                .elapsed_seconds = 1.0,
                .gpu_busy_pct = 10.0,
                .render_pressure = 0.05,
                .jank_ratio = 0.0,
                .charging_power_mw = 0.0,
            });
            assert(low_jank.valid);
            assert(low_jank.onnx_active);
            assert(!low_jank.cfw2_active);
            assert(low_jank.backend == "onnx-runtime-live");

            const auto high_jank = predictor.infer_and_learn({
                .thermal_pressure = 78.0,
                .cpu_psi = 35.0,
                .memory_psi = 8.0,
                .io_psi = 6.0,
                .current_scale = 0.85,
                .elapsed_seconds = 1.0,
                .gpu_busy_pct = 80.0,
                .render_pressure = 0.90,
                .jank_ratio = 0.85,
                .charging_power_mw = 10000.0,
            });
            assert(high_jank.valid);
            assert(high_jank.onnx_active);
            assert(high_jank.jank_risk > low_jank.jank_risk + 0.30);
            assert(high_jank.recommended_scale < low_jank.recommended_scale);
            assert(run_count() == 3); // 1 dry-run + 2 live ticks
        }
        assert(live_handles() == 0);

        // 4b. Corrupt ONNX model must cleanly release status & partial handles, falling back to CFW2
        reset_stats();
        write_text(onnx_path, "CORRUPT_ONNX");
        {
            coreflow::NeuralPredictor predictor(dir.string());
            assert(!predictor.last_prediction().onnx_active);
            assert(predictor.last_prediction().cfw2_active);
            assert(predictor.last_prediction().backend == "cfw2-micro-mlp");
        }
        assert(live_handles() == 0);

        // 4c. Always-NaN ONNX model must be rejected immediately during startup dry-run self-test
        reset_stats();
        write_text(onnx_path, "NAN_OUTPUT");
        {
            coreflow::NeuralPredictor predictor(dir.string());
            assert(run_count() == 1);
            assert(!predictor.last_prediction().onnx_active);
            assert(predictor.last_prediction().cfw2_active);
            assert(predictor.last_prediction().backend == "cfw2-micro-mlp");
            assert(live_handles() == 0);
        }
        assert(live_handles() == 0);

        // 4d. Runtime NaN on hot input triggers per-tick fallback, and 3 strikes trip circuit breaker
        reset_stats();
        write_text(onnx_path, "NAN_ON_HOT");
        {
            coreflow::NeuralPredictor predictor(dir.string());
            assert(predictor.last_prediction().onnx_active);
            assert(run_count() == 1); // dry-run passed

            // Cool tick runs normally on ORT
            const auto cool_ok = predictor.infer_and_learn({
                .thermal_pressure = 48.0,
                .cpu_psi = 6.0,
                .memory_psi = 1.0,
                .io_psi = 1.0,
                .current_scale = 0.95,
                .elapsed_seconds = 1.0,
            });
            assert(cool_ok.valid && cool_ok.onnx_active && cool_ok.backend == "onnx-runtime-live");
            assert(run_count() == 2);

            // 3 consecutive hot ticks produce NaN -> per-tick CFW2 fallback -> 3rd strike closes ORT!
            for (int strike = 0; strike < 3; ++strike) {
                const auto fb = predictor.infer_and_learn({
                    .thermal_pressure = 84.0,
                    .cpu_psi = 25.0,
                    .memory_psi = 4.0,
                    .io_psi = 3.0,
                    .current_scale = 0.85,
                    .elapsed_seconds = 1.0,
                });
                assert(fb.valid && !fb.onnx_active && fb.cfw2_active);
                assert(fb.backend == "cfw2-micro-mlp");
                assert(std::isfinite(fb.predicted_pressure_10s));
            }
            assert(run_count() == 5);
            // Circuit breaker must have already closed ORT cleanly (0 live handles BEFORE destructor!)
            assert(live_handles() == 0);

            // Subsequent cool tick stays on CFW2 without invoking broken ORT again
            const auto after_trip = predictor.infer_and_learn({
                .thermal_pressure = 48.0,
                .cpu_psi = 6.0,
                .memory_psi = 1.0,
                .io_psi = 1.0,
                .current_scale = 0.95,
                .elapsed_seconds = 1.0,
            });
            assert(after_trip.valid && !after_trip.onnx_active && after_trip.cfw2_active);
            assert(run_count() == 5);
        }
        assert(live_handles() == 0);

        // 4e. Symlinked ORT shared library must be rejected via O_NOFOLLOW
        const fs::path sym_ort = dir / "symlink_ort.so";
        fs::create_symlink(COREFLOW_MOCK_ORT_PATH, sym_ort);
        assert(::setenv("COREFLOW_TEST_ONNX_LIB", sym_ort.c_str(), 1) == 0);
        write_text(onnx_path, "MOCK_VALID_ONNX_V1");
        {
            coreflow::NeuralPredictor predictor(dir.string());
            assert(!predictor.last_prediction().onnx_active);
            assert(predictor.last_prediction().cfw2_active);
            assert(predictor.last_prediction().backend == "cfw2-micro-mlp");
        }
        fs::remove(sym_ort);

        ::unsetenv("COREFLOW_TEST_ONNX_LIB");
        (void)::dlclose(inspect_handle);
    }
#endif

    fs::remove_all(dir);
    return 0;
}
