#include "coreflow/render_collector.hpp"

#include "coreflow/observer_metrics.hpp"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <string_view>
#include <unistd.h>

namespace coreflow::observe {

namespace {

bool readFirstLineBounded(const std::string& path, std::string& line_out) noexcept {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;

    std::array<char, 256> buffer{};
    ssize_t count = 0;
    do {
        count = ::read(fd, buffer.data(), buffer.size());
    } while (count < 0 && errno == EINTR);
    ::close(fd);
    if (count <= 0) return false;

    const std::string_view raw(buffer.data(), static_cast<std::size_t>(count));
    const std::size_t eol = raw.find('\n');
    line_out.assign(raw.substr(0U, eol == std::string_view::npos ? raw.size() : eol));
    return true;
}

} // namespace

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
    const int fd = ::open(foreground_procs_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;

    std::string procs_body;
    procs_body.reserve(256U);
    std::array<char, 4096> chunk{};
    for (;;) {
        const ssize_t n = ::read(fd, chunk.data(), chunk.size());
        if (n < 0) {
            if (errno == EINTR) continue;
            ::close(fd);
            return false;
        }
        if (n == 0) break;
        const auto got = static_cast<std::size_t>(n);
        if (got > kMaxMembershipBytes - procs_body.size()) {
            ::close(fd);
            return false; // oversized or hostile membership input: fail closed
        }
        procs_body.append(chunk.data(), got);
    }
    ::close(fd);

    out = LaunchObservation{};
    out.timestamp_monotonic_ns = timestamp_monotonic_ns;
    out.foreground_pids = countCgroupProcs(procs_body);
    out.io_pressure_some = -1.0;

    std::string line;
    if (readFirstLineBounded(io_pressure_path, line)) {
        double some_avg10 = 0.0;
        double full_avg10 = 0.0;
        if (observer_metrics::parsePsiLine(line, some_avg10, full_avg10)) {
            out.io_pressure_some = some_avg10;
        }
    }
    return true;
}

} // namespace coreflow::observe
