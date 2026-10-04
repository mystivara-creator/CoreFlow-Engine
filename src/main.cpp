#include "coreflow/autonomous.hpp"

#include <csignal>

namespace {

coreflow::AutonomousEngine* g_engine = nullptr;

void handleSignal(int signal) {
    if (g_engine == nullptr)
        return;

    if (signal == SIGTERM || signal == SIGINT) {
        g_engine->requestStop();
        return;
    }

    if (signal == SIGUSR1) {
        g_engine->requestDiscoveryRefresh();
    }
}

} // namespace

int main() {
    coreflow::AutonomousEngine engine;

    g_engine = &engine;

    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
    
    const int result = engine.run();

    g_engine = nullptr;
    return result;
}
