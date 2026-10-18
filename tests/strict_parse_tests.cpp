#include "coreflow/frame_telemetry.hpp"
#include "coreflow/gpu_busy_source.hpp"
#include "coreflow/integer_parse.hpp"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

using namespace coreflow;
using coreflow::integer_parse::bumpedFrequency;
using coreflow::integer_parse::parseUint64;
using coreflow::integer_parse::parseUint64List;
using coreflow::observe::parseDevfreqLoadPercent;

namespace {

constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();

int testUint64Accepts() {
    std::uint64_t v = 99;
    CHECK(parseUint64("0", v) && v == 0U);
    CHECK(parseUint64("1000000", v) && v == 1000000U);
    CHECK(parseUint64("  512\n", v) && v == 512U);       // surrounding whitespace is part of the contract
    CHECK(parseUint64("\t7\r", v) && v == 7U);
    CHECK(parseUint64("18446744073709551615", v) && v == kMax);  // UINT64_MAX exactly
    return 0;
}

int testUint64Rejects() {
    std::uint64_t v = 42;
    CHECK(!parseUint64("", v));
    CHECK(!parseUint64("   ", v));
    CHECK(!parseUint64("12ab", v));                       // trailing garbage
    CHECK(!parseUint64("1 2", v));                        // internal whitespace
    CHECK(!parseUint64("-1", v));                         // no sign
    CHECK(!parseUint64("+1", v));
    CHECK(!parseUint64("18446744073709551616", v));       // UINT64_MAX + 1
    CHECK(!parseUint64("99999999999999999999999", v));
    CHECK(v == 42U);                                      // a failed parse never writes out
    return 0;
}

int testUint64ListIsAllOrNothing() {
    std::vector<std::uint64_t> out;
    CHECK(parseUint64List("300000 1000000\n1800000", out));
    CHECK(out.size() == 3U && out[2] == 1800000U);
    CHECK(!parseUint64List("300000 bad 1800000", out));
    CHECK(out.empty());                                   // one bad token rejects the list
    CHECK(!parseUint64List("", out));
    return 0;
}

int testBumpedFrequency() {
    std::string requested;
    const std::string list = "300000000 400000000 500000000 600000000";
    CHECK(bumpedFrequency(list, 400000000U, 1, requested) && requested == "500000000");
    CHECK(bumpedFrequency(list, 400000000U, 2, requested) && requested == "600000000");
    CHECK(bumpedFrequency(list, 400000000U, 9, requested) && requested == "600000000");  // partial bump
    CHECK(!bumpedFrequency(list, 600000000U, 1, requested));  // no higher step
    CHECK(!bumpedFrequency(list, 400000000U, 0, requested));  // steps must be >= 1
    CHECK(!bumpedFrequency("300 x 500", 300U, 1, requested)); // invalid list is rejected
    CHECK(!bumpedFrequency("", 300U, 1, requested));
    return 0;
}

int testBumpedFrequencyAtLimits() {
    std::string requested;
    const std::string list = "1 18446744073709551615";
    CHECK(bumpedFrequency(list, 1U, 1, requested) && requested == "18446744073709551615");
    CHECK(!bumpedFrequency(list, kMax, 1, requested));    // nothing above UINT64_MAX
    return 0;
}

int testDevfreqGrammarAccepts() {
    CHECK(parseDevfreqLoadPercent("12@1000000") == 12.0);
    CHECK(parseDevfreqLoadPercent("0@0") == 0.0);
    CHECK(parseDevfreqLoadPercent("100") == 100.0);
    CHECK(parseDevfreqLoadPercent("100.0@5") == 100.0);
    CHECK(parseDevfreqLoadPercent("8.5@1000") == 8.5);
    CHECK(parseDevfreqLoadPercent(" 8.5@1000 ") == 8.5);
    CHECK(parseDevfreqLoadPercent("42") == 42.0);
    return 0;
}

int testDevfreqGrammarRejects() {
    CHECK(parseDevfreqLoadPercent("") < 0.0);
    CHECK(parseDevfreqLoadPercent("12garbage") < 0.0);    // trailing garbage
    CHECK(parseDevfreqLoadPercent("12@1000 extra") < 0.0);
    CHECK(parseDevfreqLoadPercent("12@") < 0.0);          // frequency missing
    CHECK(parseDevfreqLoadPercent("@5") < 0.0);           // percent missing
    CHECK(parseDevfreqLoadPercent("12@@5") < 0.0);        // repeated delimiter
    CHECK(parseDevfreqLoadPercent("8.5@abc") < 0.0);      // non-numeric frequency
    CHECK(parseDevfreqLoadPercent("1.5.2") < 0.0);
    CHECK(parseDevfreqLoadPercent("5.") < 0.0);           // empty fraction
    CHECK(parseDevfreqLoadPercent(".5") < 0.0);           // no integer part
    CHECK(parseDevfreqLoadPercent("150") < 0.0);          // out of range
    CHECK(parseDevfreqLoadPercent("100.5") < 0.0);
    CHECK(parseDevfreqLoadPercent("99999999999999999999") < 0.0);  // overflow guard
    CHECK(parseDevfreqLoadPercent("-5") < 0.0);
    return 0;
}

} // namespace

int testLatencyRowsAreStrict() {
    using coreflow::observe::parseLatencyDurations;
    const auto ok = parseLatencyDurations("100 1000 1500\n200 2000 2600\n");
    CHECK(ok.size() == 2U && ok[0] == 500U && ok[1] == 600U);
    // Overflowing field, a fourth field, non-numeric field, ERROR row, reversed row.
    CHECK(parseLatencyDurations("1 99999999999999999999 5\n").empty());
    CHECK(parseLatencyDurations("1 2 3 4\n").empty());
    CHECK(parseLatencyDurations("1 x 3\n").empty());
    CHECK(parseLatencyDurations("1 ERROR 3\n").empty());
    CHECK(parseLatencyDurations("1 500 100\n").empty());
    CHECK(parseLatencyDurations("").empty());
    return 0;
}

int main() {
    int failures = 0;
    failures += testUint64Accepts() != 0;
    failures += testUint64Rejects() != 0;
    failures += testUint64ListIsAllOrNothing() != 0;
    failures += testBumpedFrequency() != 0;
    failures += testBumpedFrequencyAtLimits() != 0;
    failures += testDevfreqGrammarAccepts() != 0;
    failures += testDevfreqGrammarRejects() != 0;
    failures += testLatencyRowsAreStrict() != 0;
    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow strict parse tests: FAIL (%d)\n", failures);
        return 1;
    }
    std::puts("CoreFlow strict parse tests: PASS");
    return 0;
}
