#include "coreflow-engine/src/bridge.rs.h"
#include <cassert>
#include <cmath>
int main() {
    using namespace coreflow::engine;
    auto controller = new_controller({2.5, 0.15, 1.2, 42.5});
    Snapshot input{38, 1, 30, 0, 0, 44, 40, 3, 25, 15, 15, true};
    auto decision = controller->step(input);
    assert(decision.valid && decision.state == 1);
    input.temperature = 46;
    decision = controller->step(input);
    assert(decision.valid && decision.state == 3);
    assert(std::abs(decision.capacity_scale - 0.98) < 1e-9);
    input.temperature = NAN;
    assert(!controller->step(input).valid);
}
