#include "coreflow/autonomous.hpp"

#include <android/log.h>
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
}

} // namespace

int main() {
    __android_log_print(ANDROID_LOG_INFO, "CoreFlowAutonomous",
                        "CoreFlowMainEntry version=1.4.2 stage=MAIN");

    coreflow::AutonomousEngine engine;

    g_engine = &engine;

    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);

    const int result = engine.run();

    __android_log_print(ANDROID_LOG_INFO, "CoreFlowAutonomous",
                        "CoreFlowMainExit version=1.4.2 result=%d", result);

    g_engine = nullptr;
    return result;
}
