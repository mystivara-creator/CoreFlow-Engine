#include "coreflow/signal_state.hpp"

#include <csignal>

namespace {
volatile std::sig_atomic_t g_stop = 0;
volatile std::sig_atomic_t g_refresh = 0;

void handleSignal(int signal) noexcept {
    if (signal == SIGTERM || signal == SIGINT) g_stop = 1;
    else if (signal == SIGUSR1) g_refresh = 1;
}
} // namespace

namespace coreflow::signal_state {

void install() noexcept {
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGUSR1, handleSignal);
}

bool stopRequested() noexcept {
    return g_stop != 0;
}

bool consumeRefresh() noexcept {
    if (g_refresh == 0) return false;
    g_refresh = 0;
    return true;
}

} // namespace coreflow::signal_state
