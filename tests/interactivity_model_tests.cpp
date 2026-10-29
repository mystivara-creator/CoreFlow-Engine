#include "coreflow/interactivity_model.hpp"

#include <cstdint>
#include <iostream>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::cerr << "FAIL: " #expr " line " << __LINE__ << '\n'; } } while (false)

constexpr std::uint64_t kTarget = 16'666'667ULL;

void feedGoodFrames(coreflow::understand::InteractivityModel& model, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i)
        model.ingestFrame(10'000'000ULL + static_cast<std::uint64_t>(i), kTarget);
}
void feedJankyFrames(coreflow::understand::InteractivityModel& model, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i)
        model.ingestFrame(28'000'000ULL, kTarget); // deadline miss + streak
}
} // namespace

int main() {
    using namespace coreflow::understand;
    InteractivityModel model;

    InteractivitySample s;
    s.now_ms = 1000;
    CHECK(model.evaluate(s).state == InteractionState::Idle);

    // Launch window: opens on foreground change, expires by deadline.
    model.onForegroundChanged(1000);
    s.now_ms = 2000;
    CHECK(model.evaluate(s).state == InteractionState::Launching);
    s.now_ms = 1000 + 2500 + 1;
    CHECK(model.evaluate(s).state == InteractionState::Idle);

    // Jank: one evaluating tick raises the streak, the next crosses the
    // 2-tick entry threshold.
    feedJankyFrames(model, 120);
    s.now_ms = 10'000;
    CHECK(model.evaluate(s).state == InteractionState::Interactive);
    feedJankyFrames(model, 10);
    s.now_ms = 11'000;
    CHECK(model.evaluate(s).state == InteractionState::Stuttering);

    model.reset();
    feedGoodFrames(model, 120);
    s.now_ms = 20'000;
    CHECK(model.evaluate(s).state == InteractionState::Interactive);

    // Thermal guard suppresses the launch window entirely.
    model.onForegroundChanged(21'000);
    s.now_ms = 21'500;
    s.thermal_guard = true;
    CHECK(model.evaluate(s).state == InteractionState::Idle);
    s.thermal_guard = false;
    s.now_ms = 21'600;
    CHECK(model.evaluate(s).state != InteractionState::Launching);

    // Hysteresis exit: the window must be clean for the full exit count.
    model.reset();
    feedJankyFrames(model, 120);
    s.now_ms = 30'000;
    CHECK(model.evaluate(s).state == InteractionState::Interactive);
    feedJankyFrames(model, 10);
    s.now_ms = 30'500;
    CHECK(model.evaluate(s).state == InteractionState::Stuttering);
    feedGoodFrames(model, 120);
    for (int i = 0; i < 7; ++i) {
        s.now_ms += 100;
        CHECK(model.evaluate(s).state == InteractionState::Stuttering);
    }
    s.now_ms += 100;
    CHECK(model.evaluate(s).state == InteractionState::Interactive);

    model.onForegroundChanged(40'000);
    s.now_ms = 42'500;
    s.frame_fresh = false;
    CHECK(model.evaluate(s).state != InteractionState::Launching); // exclusive deadline
    model.onForegroundChanged(50'000);
    model.clearFrames();
    s.now_ms = 50'100;
    CHECK(model.evaluate(s).state == InteractionState::Launching); // missing frames do not extend deadline
    CHECK(!model.evaluate(s).frame.valid);

    if (failures != 0) {
        std::cerr << failures << " interactivity model test(s) failed\n";
        return 1;
    }
    std::cout << "Interactivity model tests passed\n";
    return 0;
}
