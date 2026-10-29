#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace coreflow::observe {

// Parses the first PID from a cgroup.procs file. Whitespace tolerant; rejects
// non-numeric content so a malformed file is not read as PID 0.
[[nodiscard]] inline bool parseFirstPid(std::string_view content,
                                        std::uint32_t& pid) noexcept {
    std::uint64_t value = 0;
    bool any = false;
    for (const char c : content) {
        if (c >= '0' && c <= '9') {
            value = value * 10U + static_cast<std::uint64_t>(c - '0');
            if (value > 4000000U) return false;
            any = true;
        } else if (c == '\n' || c == ' ' || c == '\t' || c == '\r') {
            if (any) {
                pid = static_cast<std::uint32_t>(value);
                return true;
            }
        } else {
            return false;
        }
    }
    if (any) {
        pid = static_cast<std::uint32_t>(value);
        return true;
    }
    return false;
}

// Tracks the foreground application via the top-app cgroup (no binder, no
// Java). A changed first PID is a foreground-process heuristic, not proof
// of an application launch or a measurement of launch completion.
class ForegroundTracker final {
public:
    void setProcsPath(std::string path) { procs_path_ = std::move(path); }
    void setProcRoot(std::string path) { proc_root_ = std::move(path); }
    [[nodiscard]] const std::string& procsPath() const noexcept { return procs_path_; }
    // Returns true once per detected foreground change; fills the resolved
    // process identity when /proc/<pid>/cmdline is complete and readable.
    // Unresolved identities are retried and do not emit an event.
    void invalidate() noexcept { available_ = false; }
    [[nodiscard]] bool available() const noexcept { return available_; }
    [[nodiscard]] const std::string& identity() const noexcept { return identity_; }
    [[nodiscard]] bool poll(std::string& package_out) noexcept;

private:
    std::string procs_path_{"/dev/cpuctl/top-app/cgroup.procs"};
    std::string proc_root_{"/proc"};
    std::uint32_t last_pid_{0};
    std::string identity_;
    bool available_{false};
};

} // namespace coreflow::observe
