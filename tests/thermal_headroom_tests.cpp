#include "coreflow/observation/thermal_policy.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <string_view>

using namespace coreflow::observation::thermal_policy;

int main() {
    double celsius = -1.0;
    assert(isPassiveTripType("passive"));
    assert(!isPassiveTripType("critical"));
    assert(!isPassiveTripType(""));

    // Kernel trip points are millidegrees; the vendor passive trip defines headroom.
    assert(parseTripCelsius("95000", celsius) && std::abs(celsius - 95.0) < 1e-9);
    assert(parseTripCelsius("85500", celsius) && std::abs(celsius - 85.5) < 1e-9);
    constexpr std::string_view rejected[] = {"", "5000", "200000", "95000junk", "nan", "-40000"};
    for (const auto raw : rejected) {
        double out = 42.0;
        assert(!parseTripCelsius(raw, out));
        assert(out == 42.0);  // output untouched on failure
    }

    // Retention is vetoed only when the margin to the vendor trip is both consumed
    // and near the limit. The same drop far from the trip is acceptable.
    assert(shouldVetoHeadroom(true, 6.0, true, 2.0, 3.0, 0.5));
    assert(!shouldVetoHeadroom(true, 6.0, true, 5.8, 3.0, 0.5));   // within deadband
    assert(!shouldVetoHeadroom(true, 10.0, true, 4.0, 3.0, 0.5));  // consumed margin, but still above the 3 C floor
    assert(!shouldVetoHeadroom(false, 6.0, true, 2.0, 3.0, 0.5));  // missing before sample
    assert(!shouldVetoHeadroom(true, 6.0, false, 2.0, 3.0, 0.5));  // missing after sample
    assert(!shouldVetoHeadroom(true, std::numeric_limits<double>::quiet_NaN(), true, 2.0, 3.0, 0.5));
    assert(shouldVetoHeadroom(true, 3.5, true, -1.0, 3.0, 0.5));   // above trip is also vetoed
    return 0;
}
