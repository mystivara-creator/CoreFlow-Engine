#include "coreflow/coreflow_context.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

int main() {
    char pattern[] = "/tmp/coreflow-context-XXXXXX";
    const char* root = ::mkdtemp(pattern);
    assert(root != nullptr);

    const std::filesystem::path state(root);
    const auto boot_id = state / "boot_id";
    {
        std::ofstream out(boot_id);
        out << "11111111-2222-3333-4444-555555555555\n";
        assert(out.good());
    }
    assert(::setenv("COREFLOW_TEST_STATE_DIR", root, 1) == 0);
    assert(::setenv("COREFLOW_TEST_BOOT_ID_PATH", boot_id.c_str(), 1) == 0);

    for (int cycle = 0; cycle < 3; ++cycle) {
        coreflow::CoreFlowContext read_only;
        assert(read_only.Start(coreflow::CoreFlowContext::Mode::ReadOnly));
        assert(read_only.started());
        assert(!read_only.mutation_runtime_started());
        assert(read_only.transactional_frequency_actuator() == nullptr);
        assert(read_only.mutation_coordinator().active_leases().empty());
        assert(read_only.device_profile());
        assert(read_only.Stop("read-only test complete"));
        assert(read_only.state() == coreflow::CoreFlowContext::State::Stopped);

        coreflow::CoreFlowContext daemon;
        assert(daemon.Start(coreflow::CoreFlowContext::Mode::Daemon));
        assert(daemon.started());
        assert(daemon.mutation_runtime_started());
        assert(daemon.transactional_frequency_actuator() != nullptr);
        assert(daemon.autonomous_tuner().frequency_actuator_configured());
        assert(!daemon.autonomous_tuner().frequency_mutation_active());
        assert(daemon.device_profile());
        assert(daemon.mutation_coordinator().active_leases().empty());
        assert(daemon.Stop("deterministic lifecycle test shutdown"));
        assert(daemon.state() == coreflow::CoreFlowContext::State::Stopped);
        assert(daemon.mutation_coordinator().active_leases().empty());
    }

    std::filesystem::remove_all(state);
    return 0;
}
