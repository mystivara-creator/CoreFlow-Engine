/**
 * @file decision_journal.hpp
 * @brief CoreFlow v6.0 Live Decision Episode Ring Buffer (verified read-back transparency).
 * @license CoreFlow Community Source-Available License (see LICENSE)
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace coreflow::v6 {

struct DecisionEpisode {
    uint64_t id{0};
    uint64_t ts_ms{0};
    std::string domain;   // "context" | "uclamp" | "storage" | "memory" | "thermal" | "power"
    std::string trigger;  // e.g. "LaunchBurst", "RenderPressure 78%", "SoC 47.2C"
    std::string action;   // human-readable verified action summary
    std::string outcome;  // "verified" | "oem_yield" | "rolled_back" | "skipped"
    double soc_temp_c{-1.0};
    double render_pressure{0.0};
    double capacity_scale{1.0};
};

class DecisionJournal {
public:
    static constexpr size_t kCapacity = 32U;

    void record(uint64_t ts_ms,
                std::string domain,
                std::string trigger,
                std::string action,
                std::string outcome,
                double soc_temp_c = -1.0,
                double render_pressure = 0.0,
                double capacity_scale = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Deduplicate identical consecutive domain+trigger+action+outcome within 4s
        if (count_ > 0U) {
            const size_t prev_idx = (head_ + kCapacity - 1U) % kCapacity;
            const auto& prev = ring_[prev_idx];
            if (prev.domain == domain && prev.trigger == trigger &&
                prev.action == action && prev.outcome == outcome &&
                ts_ms >= prev.ts_ms && (ts_ms - prev.ts_ms) < 4000U) {
                return;
            }
        }
        DecisionEpisode ep{};
        ep.id = ++next_id_;
        ep.ts_ms = ts_ms;
        ep.domain = std::move(domain);
        ep.trigger = std::move(trigger);
        ep.action = std::move(action);
        ep.outcome = std::move(outcome);
        ep.soc_temp_c = soc_temp_c;
        ep.render_pressure = render_pressure;
        ep.capacity_scale = capacity_scale;
        ring_[head_] = std::move(ep);
        head_ = (head_ + 1U) % kCapacity;
        if (count_ < kCapacity) {
            ++count_;
        }
    }

    // Returns up to max_count newest-first episodes.
    [[nodiscard]] std::vector<DecisionEpisode> recent(size_t max_count = kCapacity) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const size_t n = (max_count < count_) ? max_count : count_;
        std::vector<DecisionEpisode> out;
        out.reserve(n);
        for (size_t i = 0U; i < n; ++i) {
            const size_t idx = (head_ + kCapacity - 1U - i) % kCapacity;
            out.push_back(ring_[idx]);
        }
        return out;
    }

    [[nodiscard]] size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

    [[nodiscard]] uint64_t total_recorded() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return next_id_;
    }

    void clear() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = 0U;
        count_ = 0U;
    }

private:
    mutable std::mutex mutex_;
    std::array<DecisionEpisode, kCapacity> ring_{};
    size_t head_{0U};
    size_t count_{0U};
    uint64_t next_id_{0U};
};

} // namespace coreflow::v6
