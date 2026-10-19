#pragma once

#include <cstdint>

#include "coreflow/integer_parse.hpp"
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace coreflow::observe {

// Parses a devfreq `load` node. Accepted grammar, with no internal whitespace:
//   percent            := digits [ "." digits ]          (0..100 inclusive)
//   load               := percent [ "@" digits ]          ("@" is the frequency in Hz)
// Surrounding ASCII whitespace is allowed. Anything else returns -1.0: trailing
// garbage, a missing or non-numeric frequency, a repeated delimiter, an empty
// part, and out-of-range or overflowing values. An invalid reading is never
// mistaken for an idle GPU.
[[nodiscard]] inline double parseDevfreqLoadPercent(std::string_view raw) noexcept {
    const std::string_view token = integer_parse::trim(raw);
    if (token.empty()) return -1.0;

    const std::size_t at = token.find('@');
    const std::string_view percent = token.substr(0, at);
    if (at != std::string_view::npos) {
        std::uint64_t frequency_hz = 0;
        if (!integer_parse::parseUint64(token.substr(at + 1U), frequency_hz)) return -1.0;
    }

    std::size_t i = 0;
    std::uint64_t whole = 0;
    std::size_t whole_digits = 0;
    while (i < percent.size() && percent[i] >= '0' && percent[i] <= '9') {
        whole = whole * 10U + static_cast<std::uint64_t>(percent[i] - '0');
        if (whole > 100U) return -1.0;  // checked before the next multiply
        ++whole_digits;
        ++i;
    }
    if (whole_digits == 0U) return -1.0;

    double result = static_cast<double>(whole);
    if (i < percent.size() && percent[i] == '.') {
        ++i;
        double place = 0.1;
        std::size_t fraction_digits = 0;
        while (i < percent.size() && percent[i] >= '0' && percent[i] <= '9') {
            result += static_cast<double>(percent[i] - '0') * place;
            place *= 0.1;
            ++fraction_digits;
            ++i;
        }
        if (fraction_digits == 0U) return -1.0;
    }
    if (i != percent.size()) return -1.0;  // trailing garbage in the percent part
    if (result < 0.0 || result > 100.0) return -1.0;
    return result;
}

// Discovers and polls GPU busy-ratio from kernel devfreq nodes (Adreno kgsl,
// Mali, generic *.gpu). Read-only; never writes device state.
class GpuBusySource final {
public:
    void addRoot(std::string path) { roots_.push_back(std::move(path)); }
    // Dynamically resolves the sysfs mount and searches the canonical device
    // model for readable GPU/devfreq load sources. No vendor alias is assumed.
    void discoverDefaultRoots() noexcept;
    [[nodiscard]] bool poll(double& ratio_out) noexcept; // ratio in [0,1]
    [[nodiscard]] const std::string& activeRoot() const noexcept { return active_root_; }

private:
    std::vector<std::string> roots_;
    std::string active_root_;
};

} // namespace coreflow::observe
