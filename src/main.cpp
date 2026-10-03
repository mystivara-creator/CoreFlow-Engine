#include "coreflow/autonomous.hpp"
#include <csignal>

namespace {

coreflow::AutonomousEngine* g_engine = nullptr;

void handleSignal(int) {
    if (g_engine != nullptr)
        g_engine->requestStop();
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
