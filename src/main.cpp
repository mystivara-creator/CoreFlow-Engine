#include "coreflow/runtime/engine.hpp"
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <exception>
#include <iostream>
int main() {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO,"CoreFlowEngine","Autonomous Engine starting");
#else
    std::clog << "[CoreFlowEngine] Autonomous Engine starting\n";
#endif
    try {
        coreflow::AutonomousEngine engine;
        engine.run();
    } catch (const std::exception& e) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR,"CoreFlowEngine","Fatal: %s",e.what());
#else
        std::clog << "[CoreFlowEngine] Fatal: " << e.what() << '\n';
#endif
        return 1;
    }
    return 0;
}
