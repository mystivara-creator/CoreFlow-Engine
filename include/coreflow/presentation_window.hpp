#pragma once
#include "coreflow/integer_parse.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace coreflow::observe {
// AOSP FrameTracker dumps: desiredPresentTime, actualPresentTime, frameReadyTime.
// Track actual presentation intervals; never subtract ready from present and
// call it frame time. Repeated history contributes no new evidence.
class PresentationWindow final {
public:
    std::vector<std::uint64_t> consume(std::string_view dump) {
        std::array<std::uint64_t, 128> timestamps{};
        std::size_t count = 0;
        std::size_t pos = 0;
        while (pos < dump.size()) {
            auto end = dump.find('\n', pos);
            if (end == std::string_view::npos) end = dump.size();
            auto line = dump.substr(pos, end - pos);
            pos = end + (end < dump.size() ? 1U : 0U);
            std::uint64_t row[3]{};
            std::size_t fields = 0;
            bool valid = true;
            while (!line.empty()) {
                while (!line.empty() && integer_parse::isSpace(line.front())) line.remove_prefix(1);
                if (line.empty()) break;
                std::size_t n = 0;
                while (n < line.size() && !integer_parse::isSpace(line[n])) ++n;
                if (fields == 3U || !integer_parse::parseUint64(line.substr(0, n), row[fields])) {
                    valid = false; break;
                }
                ++fields;
                line.remove_prefix(n);
            }
            constexpr auto pending = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
            if (!valid || fields != 3U || row[0] == 0U || row[1] == 0U || row[2] == 0U ||
                row[0] >= pending || row[1] >= pending || row[2] >= pending) continue;
            if (count < timestamps.size()) timestamps[count++] = row[1];
            else { // Retain the newest bounded history if a vendor emits extra rows.
                auto oldest = std::min_element(timestamps.begin(), timestamps.end());
                if (row[1] > *oldest) *oldest = row[1];
            }
        }
        std::sort(timestamps.begin(), timestamps.begin() + static_cast<std::ptrdiff_t>(count));
        std::vector<std::uint64_t> intervals;
        intervals.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto stamp = timestamps[i];
            if (stamp <= last_present_ns_) continue;
            if (last_present_ns_ != 0U && stamp - last_present_ns_ < 1000000000U)
                intervals.push_back(stamp - last_present_ns_);
            last_present_ns_ = stamp;
        }
        return intervals;
    }
    void reset() noexcept { last_present_ns_ = 0; }
private:
    std::uint64_t last_present_ns_{0};
};
} // namespace coreflow::observe
