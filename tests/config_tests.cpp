#include "coreflow/config.hpp"

#include <fstream>
#include <iostream>
#include <string>

using namespace coreflow;

static bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }

    return true;
}

int main() {
    const std::string path = "/tmp/coreflow_config_test.ini";

    {
        std::ofstream file(path);
        if (!expect(file.is_open(), "unable to create test configuration")) {
            return 1;
        }

        file << "monitor_interval=99\n"
             << "min_confidence=0.42\n"
             << "mutation_mode=adaptive\n"
             << "allow_cpu_governor=no\n"
             << "runtime_refresh=false\n";
    }

    EngineConfig cfg;

    if (!expect(cfg.load(path), "configuration failed to load")) {
        return 1;
    }

    if (!expect(cfg.monitorIntervalSeconds() == 60,
                "monitor interval should be clamped to 60")) {
        return 1;
    }

    if (!expect(cfg.minConfidence() == 0.50,
                "minimum confidence should be clamped to 0.50")) {
        return 1;
    }

    if (!expect(cfg.mutationMode() == MutationMode::Adaptive,
                "mutation mode should be Adaptive")) {
        return 1;
    }

    if (!expect(cfg.allowCpuGovernor(),
                "CPU governor should be allowed by configuration")) {
        return 1;
    }

    if (!expect(cfg.runtimeRefreshEnabled(),
                "runtime refresh should be enabled")) {
        return 1;
    }

    return 0;
}
