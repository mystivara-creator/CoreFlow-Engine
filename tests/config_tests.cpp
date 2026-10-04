#include "coreflow/config.hpp"

#include <cassert>
#include <fstream>
#include <string>

int main() {
    using namespace coreflow;
    const std::string path = "/tmp/coreflow_config_test.ini";
    {
        std::ofstream file(path);
        file << "monitor_interval=99\n"
             << "min_confidence=0.42\n"
             << "mutation_mode=adaptive\n"
             << "allow_cpu_governor=no\n"
             << "runtime_refresh=false\n";
    }

    EngineConfig cfg;
    assert(cfg.load(path));
    assert(cfg.monitorIntervalSeconds() == 60);
    assert(cfg.minConfidence() == 0.50);
    assert(cfg.mutationMode() == MutationMode::Adaptive);
    assert(!cfg.allowCpuGovernor());
    assert(!cfg.runtimeRefreshEnabled());

    return 0;
}
