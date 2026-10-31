#include "coreflow/sample_sequence.hpp"
#include "coreflow/integer_parse.hpp"
#include <algorithm>
namespace coreflow {
bool SampleSequence::initialize() noexcept {
    healthy_ = false;
    MutationJournal::Entries entries;
    const auto state = store_.load(entries);
    if (state == MutationJournal::LoadState::Corrupt) return false;
    std::uint64_t start = 0;
    if (state == MutationJournal::LoadState::Pending &&
        (entries.size() != 1U || entries[0].first != "next_sample" ||
         !integer_parse::parseUint64(entries[0].second, start) || start > kMaximum)) return false;
    next_ = start;
    reserved_ = start;
    healthy_ = true;
    return true;
}
bool SampleSequence::next(std::uint64_t& value) noexcept {
    if (!healthy_ || next_ >= kMaximum) { healthy_ = false; return false; }
    try {
        if (next_ == reserved_) {
            const auto limit = next_ + std::min(kBatch, kMaximum - next_);
            if (!store_.commit({{"next_sample", std::to_string(limit)}})) { healthy_ = false; return false; }
            reserved_ = limit;
        }
        value = next_++;
        return true;
    } catch (...) { healthy_ = false; return false; }
}
bool SampleSequence::checkpoint() noexcept {
    if (!healthy_) return false;
    try {
        if (!store_.commit({{"next_sample", std::to_string(next_)}})) { healthy_ = false; return false; }
        reserved_ = next_;
        return true;
    } catch (...) { healthy_ = false; return false; }
}
}
