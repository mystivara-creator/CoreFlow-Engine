#include "coreflow/observation/pressure.hpp"

#include <cassert>
#include <cmath>
#include <string>
#include <string_view>

int main() {
    using coreflow::observation::pressure::kMaximumPressureTextBytes;
    using coreflow::observation::pressure::parseSomeAvg10;

    double avg10 = -1.0;
    assert(parseSomeAvg10(
        "some avg10=3.25 avg60=2.00 avg300=1.00 total=9876\n"
        "full avg10=0.50 avg60=0.20 avg300=0.10 total=123\n", avg10));
    assert(std::abs(avg10 - 3.25) < 1e-12);
    double resolution = -1.0;
    assert(parseSomeAvg10("some avg10=3.25 avg60=2.00 avg300=1.00 total=0\n", avg10, &resolution));
    assert(std::abs(resolution - 0.01) < 1e-12);
    assert(parseSomeAvg10("some avg10=0 avg60=0 avg300=0 total=0\n", avg10, &resolution));
    assert(std::abs(resolution - 1.0) < 1e-12);
    assert(parseSomeAvg10("some avg10=1e+2 avg60=0 avg300=0 total=0\n", avg10, &resolution));
    assert(std::abs(resolution - 100.0) < 1e-12);

    // Whitespace and future key/value fields are tolerated, while the 'full'
    // line must not be confused with the 'some' signal used by CoreFlow.
    avg10 = 99.0;
    assert(parseSomeAvg10("  some\tavg10=0 avg60=0 avg300=0 total=0 future=ok\r\n", avg10));
    assert(avg10 == 0.0);

    const std::string_view invalid[] = {
        "", "full avg10=5 avg60=5 avg300=5 total=1\n",
        "some avg60=5 avg300=5 total=1\n",
        "some avg10= avg60=5\n", "some avg10=-0.1\n",
        "some avg10=100.01\n", "some avg10=nan\n",
        "some avg10=inf\n", "some avg10=5junk\n",
        "some avg10=1 avg10=2\n", "somefoo avg10=5\n",
        "some avg10=1 unexpected\n"
    };
    for (const auto input : invalid) {
        avg10 = 42.0;
        assert(!parseSomeAvg10(input, avg10));
        assert(avg10 == 42.0); // failure never modifies the caller's value
    }

    std::string oversized(kMaximumPressureTextBytes + 1, 'x');
    avg10 = 42.0;
    assert(!parseSomeAvg10(oversized, avg10));
    assert(avg10 == 42.0);

    return 0;
}
