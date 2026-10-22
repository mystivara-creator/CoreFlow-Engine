#include "coreflow/outcome/outcome.hpp"
#include <cassert>

int main() {
    using namespace coreflow;
    OutcomeVerifier verifier;

    SystemObservation before;
    before.memory_valid = true;
    before.memory_available_ratio = 0.30;
    before.temperature_valid = true;
    before.maximum_temperature_c = 43.0;
    before.cpu_pressure_valid = true;
    before.cpu_pressure_some_avg10 = 4.0;
    before.memory_pressure_valid = true;
    before.memory_pressure_some_avg10 = 2.0;
    before.io_pressure_valid = true;
    before.io_pressure_some_avg10 = 5.0;

    auto after = before;
    after.memory_available_ratio = 0.32;
    after.maximum_temperature_c = 42.5;
    after.cpu_pressure_some_avg10 = 3.0;
    after.memory_pressure_some_avg10 = 1.2;
    after.io_pressure_some_avg10 = 3.0;
    auto outcome = verifier.verify(before, after);
    assert(outcome.measurable && outcome.improved);
    assert(OutcomePolicy{}.decide(outcome) == Disposition::Retain);

    // A measurable regression in any comparable pressure signal prevents retention.
    after.io_pressure_some_avg10 = 8.0;
    outcome = verifier.verify(before, after);
    assert(outcome.measurable && !outcome.improved);
    assert(OutcomePolicy{}.decide(outcome) == Disposition::Rollback);

    SystemObservation absent;
    outcome = verifier.verify(absent, SystemObservation{});
    assert(!outcome.measurable && !outcome.improved);
    assert(OutcomePolicy{}.decide(outcome) == Disposition::Unresolved);
    return 0;
}
