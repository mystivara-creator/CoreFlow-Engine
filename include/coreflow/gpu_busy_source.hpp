#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace coreflow::observe {

// Parses a devfreq `load` node ("<percent>@<freq>") or a plain percentage.
// Returns -1.0 for empty, non-numeric, or out-of-range (>100) input so an
// invalid reading is never mistaken for an idle GPU.
[[nodiscard]] inline double parseDevfreqLoadPercent(std::string_view raw) noexcept {
    std::size_t i = 0;
    while (i < raw.size() &&
           (raw[i] == ' ' || raw[i] == '\t' || raw[i] == '\n' || raw[i] == '\r')) {
        ++i;
    }
    bool any = false;
    std::uint64_t whole = 0;
    while (i < raw.size() && raw[i] >= '0' && raw[i] <= '9') {
        const auto digit = static_cast<std::uint64_t>(raw[i] - '0');
        if (whole > (100U - digit) / 10U) return -1.0;
        whole = whole * 10U + digit;
        any = true;
        ++i;
    }
    double result = static_cast<double>(whole);
    if (i < raw.size() && raw[i] == '.') {
        ++i;
        double place = 0.1;
        while (i < raw.size() && raw[i] >= '0' && raw[i] <= '9') {
            result += static_cast<double>(raw[i] - '0') * place;
            place *= 0.1;
            any = true;
            ++i;
        }
    }
    if (!any || result < 0.0 || result > 100.0) return -1.0;
    return result;
}

// Discovers and polls GPU busy-ratio from kernel devfreq nodes (Adreno kgsl,
// Mali, generic *.gpu). Read-only; never writes device state.
class GpuBusySource final {
public:
    void addRoot(std::string path) { roots_.push_back(std::move(path)); }
    // Well-known roots: /sys/class/kgsl/kgsl-3d0/devfreq plus every
    // /sys/class/devfreq/* entry whose name suggests a GPU device.
    void discoverDefaultRoots() noexcept;
    [[nodiscard]] bool poll(double& ratio_out) noexcept; // ratio in [0,1]
    [[nodiscard]] const std::string& activeRoot() const noexcept { return active_root_; }

private:
    std::vector<std::string> roots_;
    std::string active_root_;
};

} // namespace coreflow::observe
