#pragma once
#include "coreflow/environment.hpp"
#include "coreflow/baseline_intelligence.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace coreflow {
struct TrialMember {
    ResourceDomain domain{ResourceDomain::Vendor};
    std::string path, previous, target;
};
struct TrialConditions {
    // hottest C, CPU, memory, CPU PSI, memory PSI, I/O PSI, I/O ms, GPU, jank, load1
    std::array<double,10> values{};
    std::uint32_t available{0};
    unsigned mode{0}; // performance, thermal stabilization, memory stabilization
    bool charging_known{false}, charging{false};
    std::uint64_t foreground{0}, frame_period_ns{0}, configuration{0};
};
// Admission history, not independent component benefit attribution or a restore journal.
// A trial may involve a coherent group; these entries only prevent unchanged
// conditions from reopening the same path (including nearby numeric variants).
class TrialHistory {
public:
    bool initialize(std::string path,std::string scope) noexcept;
    bool flush() noexcept;
    bool ready() const noexcept { return healthy_ && !dirty_; }
    bool dirty() const noexcept { return dirty_; }
    std::size_t size() const noexcept { return entries_.size(); }
    void setForeground(std::string_view identity) noexcept;
    void setConfiguration(const std::vector<std::pair<std::string,std::string>>& cpu,
                          const std::vector<std::pair<std::string,std::string>>& resources) noexcept;
    TrialConditions conditions(const RuntimeSample&,RuntimeState) const noexcept;
    static bool materiallyChanged(const TrialConditions&,const TrialConditions&,ResourceDomain) noexcept;
    static bool updateQuietRelease(const TrialConditions& accepted,const TrialConditions& now,
                                   bool tuning_needed,unsigned& samples) noexcept;
    const char* blockReason(std::string_view path,std::string_view current,ResourceDomain,
                            const TrialConditions&,std::string_view requested={}) const noexcept;
    bool remember(const std::vector<TrialMember>&,const TrialConditions&,
                  BaselineIntelligence::Outcome,bool accepted,std::string_view group) noexcept;
private:
    struct Entry { TrialMember member; TrialConditions when; bool accepted{false}; unsigned outcome{0}; std::uint64_t group{0}; };
    static constexpr std::size_t kMaxEntries=128;
    std::string path_,scope_;
    std::vector<Entry> entries_;
    std::uint64_t foreground_{0}, configuration_{0};
    bool healthy_{false},dirty_{false};
};
} // namespace coreflow
