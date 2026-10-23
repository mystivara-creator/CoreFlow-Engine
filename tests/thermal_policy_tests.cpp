#include "coreflow/observation/thermal_policy.hpp"

#include <cassert>
#include <cmath>
#include <string_view>

using coreflow::observation::thermal_policy::isTrustedComputeThermalType;
using coreflow::observation::thermal_policy::parseTrustedThermalCelsius;
using coreflow::observation::thermal_policy::shouldVetoRetention;

int main() {
    double value = -1.0;

    assert(isTrustedComputeThermalType("cpu-0-0"));
    assert(isTrustedComputeThermalType("cpuss-0"));
    assert(isTrustedComputeThermalType("gpuss-0"));
    assert(!isTrustedComputeThermalType("battery"));
    assert(!isTrustedComputeThermalType("pm7250b-bcl-lvl0"));
    assert(!isTrustedComputeThermalType("unknown"));

    assert(parseTrustedThermalCelsius("cpu-0-0", "52100", value));
    assert(std::abs(value - 52.1) < 1e-9);

    assert(parseTrustedThermalCelsius("gpuss-0", "46800", value));
    assert(std::abs(value - 46.8) < 1e-9);

    constexpr std::string_view invalid_values[] = {
        "", "-40000", "-273000", "0", "9999", "125001",
        "52100junk", "nan", "inf", "1e9999"
    };

    for (const auto raw : invalid_values) {
        value = 77.0;
        assert(!parseTrustedThermalCelsius("cpu-0-0", raw, value));
        assert(value == 77.0);
    }

    value = 77.0;
    assert(!parseTrustedThermalCelsius("battery", "39000", value));
    assert(value == 77.0);

    // Retention veto boundaries are explicit and deterministic. Missing or
    // non-finite readings do not masquerade as a measured thermal regression.
    assert(!shouldVetoRetention(true, 40.0, true, 40.5));
    assert(shouldVetoRetention(true, 40.0, true, 40.5001));
    assert(shouldVetoRetention(true, 44.0, true, 45.0));

    // Absolute temperature is not a universal retention veto. A high but stable
    // reading cannot prove that this mutation caused a thermal regression.
    assert(!shouldVetoRetention(true, 48.0, true, 48.0));
    assert(!shouldVetoRetention(true, 50.0, true, 50.1));
    assert(!shouldVetoRetention(true, 48.0, true, 48.5));

    // A thermal rise is not inferred when either side of the comparison is
    // unavailable or non-finite.
    assert(!shouldVetoRetention(false, 0.0, true, 44.9));
    assert(!shouldVetoRetention(false, 0.0, true, 125.0));
    assert(!shouldVetoRetention(true, 40.0, false, 99.0));
    assert(!shouldVetoRetention(true, std::nan(""), true, 50.0));
    assert(!shouldVetoRetention(true, 40.0, true, std::nan("")));

    return 0;
}
