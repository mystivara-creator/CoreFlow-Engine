#include "coreflow/render_collector.hpp"

#include "coreflow/observer_metrics.hpp"

#include <array>
#include <fstream>
#include <iterator>
#include <sstream>

namespace coreflow::observe {

std::size_t countCgroupProcs(const std::string& text) noexcept {
    std::size_t count = 0;
    std::size_t pos = 0;
    while (pos < text.size() && count < kMaxCgroupPids) {
        std::size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) eol = text.size();
        const std::size_t length = eol - pos;
        bool numeric = length > 0U;
        for (std::size_t i = pos; numeric && i < eol; ++i) {
            numeric = text[i] >= '0' && text[i] <= '9';
        }
        if (numeric) ++count;
        pos = eol + 1U;
    }
    return count;
}

bool readLaunchObservation(const std::string& foreground_procs_path,
                           const std::string& io_pressure_path,
                           std::uint64_t timestamp_monotonic_ns,
                           LaunchObservation& out) {
    constexpr std::size_t kMaxMembershipBytes = 1U << 20U; // 1 MiB hard cap
    std::ifstream procs(foreground_procs_path, std::ios::binary);
    if (!procs) return false;
    std::string procs_body(kMaxMembershipBytes + 1U, '\0');
    procs.read(procs_body.data(),
               static_cast<std::streamsize>(procs_body.size()));
    const std::streamsize bytes_read = procs.gcount();
    if (bytes_read < 0 ||
        static_cast<std::size_t>(bytes_read) > kMaxMembershipBytes) {
        return false; // oversized or hostile membership input: fail closed
    }
    procs_body.resize(static_cast<std::size_t>(bytes_read));

    out = LaunchObservation{};
    out.timestamp_monotonic_ns = timestamp_monotonic_ns;
    out.foreground_pids = countCgroupProcs(procs_body);
    out.io_pressure_some = -1.0;

    std::ifstream pressure(io_pressure_path);
    std::string line;
    if (pressure && std::getline(pressure, line)) {
        double some_avg10 = 0.0;
        double full_avg10 = 0.0;
        if (observer_metrics::parsePsiLine(line, some_avg10, full_avg10)) {
            out.io_pressure_some = some_avg10;
        }
    }
    return true;
}

} // namespace coreflow::observe
