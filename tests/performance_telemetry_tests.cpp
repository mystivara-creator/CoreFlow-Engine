#include "coreflow/performance_telemetry.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::cerr << "FAIL: " #expr " at line " << __LINE__ << '\n'; } } while (false)

coreflow::performance::Sample frame(double value = 16'666'667.0) {
    using namespace coreflow::performance;
    return {Signal::FrameInterval, Availability::Available, Unit::Nanoseconds,
            1'000'000U, value};
}
} // namespace

int main() {
    using namespace coreflow::performance;

    CHECK(validate(frame()).valid);
    CHECK(normalize(frame()).has_value());

    auto bad = frame(0.0);
    CHECK(!validate(bad).valid);
    CHECK(validate(bad).availability == Availability::InvalidSample);

    bad = frame(std::numeric_limits<double>::quiet_NaN());
    CHECK(!normalize(bad).has_value());
    bad = frame(std::numeric_limits<double>::infinity());
    CHECK(!validate(bad).valid);

    bad = frame();
    bad.timestamp_monotonic_ns = 0U;
    CHECK(!validate(bad).valid);

    Sample gpu{Signal::GpuBusyRatio, Availability::Available, Unit::Ratio,
               2'000'000U, 0.75};
    CHECK(validate(gpu).valid);
    gpu.value = 1.01;
    CHECK(!validate(gpu).valid);
    gpu.value = 0.5;
    gpu.unit = Unit::Nanoseconds;
    CHECK(!validate(gpu).valid);

    Sample denied{Signal::GpuBusyRatio, Availability::PermissionDenied,
                  Unit::Ratio, 2'000'000U, 0.5};
    CHECK(!validate(denied).valid);
    CHECK(validate(denied).availability == Availability::PermissionDenied);

    Sample run_queue{Signal::CpuRunQueueDelay, Availability::Available,
                     Unit::Nanoseconds, 3'000'000U, 0.0};
    CHECK(validate(run_queue).valid);

    CHECK(signalName(Signal::FrameInterval)[0] == 'f');
    CHECK(availabilityName(Availability::Unsupported)[0] == 'u');
    CHECK(unitName(Unit::Ratio)[0] == 'r');

    if (failures != 0) {
        std::cerr << failures << " telemetry test(s) failed\n";
        return 1;
    }
    std::cout << "Performance telemetry tests passed\n";
    return 0;
}
