#include "coreflow/memory_warmup_controller.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <unistd.h>

namespace {
coreflow::MemoryWarmupAdmission admitted() {
    coreflow::MemoryWarmupAdmission a{};
    a.available_headroom_bytes = 256ULL * 1024ULL * 1024ULL;
    a.configured_budget_bytes = 64ULL * 1024ULL * 1024ULL;
    a.max_single_file_bytes = 64ULL * 1024ULL * 1024ULL;
    a.telemetry_timestamp_ns = 100U;
    a.now_ns = 110U;
    a.max_telemetry_age_ns = 50U;
    a.memory_pressure_pct = 2.0;
    a.io_pressure_pct = 1.0;
    a.memory_pressure_valid = true;
    a.io_pressure_valid = true;
    a.thermal_admission_valid = true;
    a.thermal_admission_granted = true;
    a.working_set_prediction_valid = true;
    a.foreground_identity_valid = true;
    return a;
}
}

int main() {
    char path[] = "/tmp/coreflow-warmup-XXXXXX";
    const int fd = ::mkstemp(path);
    assert(fd >= 0);
    constexpr off_t file_size = 48LL * 1024LL * 1024LL;
    assert(::ftruncate(fd, file_size) == 0);
    assert(::close(fd) == 0);

    coreflow::MemoryWarmupController controller;
    assert(controller.hard_budget_bytes() == 32ULL * 1024ULL * 1024ULL);
    const std::vector<coreflow::WarmupFileRange> ranges{{path, 0U, 48ULL * 1024ULL * 1024ULL}};

    auto result = controller.warmup(ranges, admitted());
    assert(result.files_advised == 1U);
    assert(result.admitted_bytes == 32ULL * 1024ULL * 1024ULL);
    assert(result.admitted_bytes <= controller.hard_budget_bytes());

    auto high_memory = admitted();
    high_memory.memory_pressure_pct = 10.01;
    result = controller.warmup(ranges, high_memory);
    assert(result.admitted_bytes == 0U && result.files_rejected == 1U);

    auto high_io = admitted();
    high_io.io_pressure_pct = 10.01;
    result = controller.warmup(ranges, high_io);
    assert(result.admitted_bytes == 0U);

    auto stale = admitted();
    stale.now_ns = 1000U;
    result = controller.warmup(ranges, stale);
    assert(result.admitted_bytes == 0U);

    auto thermal_denied = admitted();
    thermal_denied.thermal_admission_granted = false;
    result = controller.warmup(ranges, thermal_denied);
    assert(result.admitted_bytes == 0U);

    coreflow::MemoryWarmupController cancelled;
    cancelled.cancel();
    result = cancelled.warmup(ranges, admitted());
    assert(result.cancelled && result.admitted_bytes == 0U);

    const std::string link = std::string(path) + ".link";
    assert(::symlink(path, link.c_str()) == 0);
    result = controller.warmup({{link, 0U, 4096U}}, admitted());
    assert(result.admitted_bytes == 0U && result.files_rejected == 1U);
    (void)::unlink(link.c_str());
    (void)::unlink(path);
    return 0;
}
