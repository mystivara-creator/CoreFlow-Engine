#pragma once
#include "coreflow/mutation_journal.hpp"
#include <cstdint>
namespace coreflow {
// Diagnostic identity only: never restores observations or authorizes tuning.
// Reserve IDs durably in batches, so a crash skips unused IDs rather than reusing them.
class SampleSequence final {
public:
    explicit SampleSequence(std::string path) : store_(std::move(path)) {}
    bool initialize() noexcept;
    bool next(std::uint64_t& value) noexcept;
    bool checkpoint() noexcept;
    bool healthy() const noexcept { return healthy_; }
    static constexpr std::uint64_t kBatch = 256;
    static constexpr std::uint64_t kMaximum = (1ULL << 53U) - 1U;
private:
    FileMutationJournal store_;
    std::uint64_t next_{0};
    std::uint64_t reserved_{0};
    bool healthy_{false};
};
}
