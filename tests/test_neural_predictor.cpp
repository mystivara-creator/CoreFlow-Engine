#include "coreflow/neural_predictor.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <unistd.h>

namespace fs = std::filesystem;

int main() {
    char pattern[] = "/tmp/coreflow-neural-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    assert(dir != nullptr);

    coreflow::NeuralPredictor predictor(dir);

    // 1. Reject non-finite or out-of-range telemetry
    const auto bad = predictor.infer_and_learn({
        .thermal_pressure = std::numeric_limits<double>::quiet_NaN(),
        .cpu_psi = 10.0,
        .memory_psi = 0.0,
        .io_psi = 0.0,
        .current_scale = 1.0,
        .elapsed_seconds = 1.0,
    });
    assert(!bad.valid);

    // 2. Cold/nominal workload yields high recommended capacity ceiling
    const auto nominal = predictor.infer_and_learn({
        .thermal_pressure = 42.0,
        .cpu_psi = 4.0,
        .memory_psi = 0.0,
        .io_psi = 0.0,
        .current_scale = 1.0,
        .elapsed_seconds = 1.0,
    });
    assert(nominal.valid);
    assert(nominal.backend == "embedded-rls-mlp");
    assert(nominal.recommended_scale >= 0.70 && nominal.recommended_scale <= 1.0);
    assert(nominal.predicted_pressure_10s >= 0.0 && nominal.predicted_pressure_10s <= 130.0);

    // 3. Rapid thermal escalation triggers proactive 10s horizon pre-cooling
    coreflow::NeuralPrediction hot{};
    for (double p : {65.0, 74.0, 82.0, 89.0, 95.0}) {
        hot = predictor.infer_and_learn({
            .thermal_pressure = p,
            .cpu_psi = 45.0,
            .memory_psi = 12.0,
            .io_psi = 10.0,
            .current_scale = 0.90,
            .elapsed_seconds = 1.0,
        });
        assert(hot.valid);
    }
    assert(hot.samples_observed == 6);
    assert(hot.predicted_pressure_10s > 85.0);
    assert(hot.recommended_scale < nominal.recommended_scale);
    assert(hot.confidence >= 0.35 && hot.confidence <= 0.99);

    // 4. Verify custom CFW1 weight loading
    {
        std::ofstream cfw(fs::path(dir) / "equilibrium.cfw");
        cfw << "CFW1\n";
        for (int i = 0; i < 48; ++i) cfw << "0.1 ";
        for (int i = 0; i < 8; ++i) cfw << "0.0 ";
        for (int i = 0; i < 16; ++i) cfw << "0.1 ";
        cfw << "0.0 0.5\n";
    }
    predictor.initialize(dir);
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

    fs::remove_all(dir);
    return 0;
}
