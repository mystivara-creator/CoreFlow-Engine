#include "coreflow/interactivity_model.hpp"

#include <cstdint>
#include <cstdio>
#include <limits>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

using namespace coreflow::understand;

namespace {

constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();

std::uint64_t deadlineAfterForeground(std::uint64_t now_ms) {
    InteractivityModel model;
    model.onForegroundChanged(now_ms);
    InteractivitySample sample;
    sample.now_ms = now_ms;
    return model.evaluate(sample).launch_deadline_ms;
}

int testNormalDeadlineIsNowPlusWindow() {
    // Default launch window is 2500 ms.
    CHECK(deadlineAfterForeground(1000U) == 3500U);
    CHECK(deadlineAfterForeground(0U + 1U) == 2501U);
    return 0;
}

int testDeadlineSaturatesInsteadOfWrapping() {
    // now + 2500 would wrap to a small value; the deadline must saturate instead.
    CHECK(deadlineAfterForeground(kMax - 100U) == kMax);
    CHECK(deadlineAfterForeground(kMax) == kMax);
    // Exactly at the limit: now + window == kMax, still representable.
    CHECK(deadlineAfterForeground(kMax - 2500U) == kMax);
    // One past the limit saturates rather than wrapping.
    CHECK(deadlineAfterForeground(kMax - 2499U) == kMax);
    return 0;
}

} // namespace

int main() {
    int failures = 0;
    failures += testNormalDeadlineIsNowPlusWindow() != 0;
    failures += testDeadlineSaturatesInsteadOfWrapping() != 0;
    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow interactivity deadline tests: FAIL (%d)\n", failures);
        return 1;
    }
    std::puts("CoreFlow interactivity deadline tests: PASS");
    return 0;
}
