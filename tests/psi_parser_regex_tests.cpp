#include "coreflow/observer_metrics.hpp"

#include <cstdio>
#include <cmath>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

namespace {

// Test oracle for the documented Linux PSI line grammar. Do not use regex in
// production sampling: std::regex can allocate and has less predictable cost.
const std::regex kPsiLineGrammar(
    R"(^(some|full)[ \t]+avg10=([0-9]+(?:\.[0-9]+)?)[ \t]+avg60=([0-9]+(?:\.[0-9]+)?)[ \t]+avg300=([0-9]+(?:\.[0-9]+)?)[ \t]+total=([0-9]+)[ \t\r]*$)",
    std::regex::ECMAScript);

bool regexAccepts(const std::string& line) {
    // The parser accepts a terminal newline as line whitespace.
    std::string normalized = line;
    if (!normalized.empty() && normalized.back() == '\n') {
        normalized.pop_back();
        if (!normalized.empty() && normalized.back() == '\r') {
            normalized.pop_back();
        }
    }

    std::smatch match;
    if (!std::regex_match(normalized, match, kPsiLineGrammar)) {
        return false;
    }

    // Regex validates syntax; numeric bounds are checked separately.
    try {
        const double avg10 = std::stod(match[2].str());
        const double avg60 = std::stod(match[3].str());
        const double avg300 = std::stod(match[4].str());

        return std::isfinite(avg10) && avg10 >= 0.0 && avg10 <= 100.0 &&
               std::isfinite(avg60) && avg60 >= 0.0 && avg60 <= 100.0 &&
               std::isfinite(avg300) && avg300 >= 0.0 && avg300 <= 100.0;
    } catch (...) {
        return false;
    }
}

}  // namespace

int main() {
    using coreflow::observer_metrics::parsePsiLine;
    const std::vector<std::pair<std::string, bool>> cases{
        {"some avg10=0.00 avg60=0.00 avg300=0.00 total=0", true},
        {"full avg10=100.00 avg60=99.50 avg300=1.25 total=184467", true},
        {"some\tavg10=12.5\tavg60=2.0\tavg300=0.0\ttotal=42", true},
        {"some avg10=0 avg60=0 avg300=0 total=1", true},
        {"some avg10=.5 avg60=0 avg300=0 total=1", false},
        {"some avg10=1. avg60=0 avg300=0 total=1", false},
        {"some avg10=1e2 avg60=0 avg300=0 total=1", false},
        {"some avg10=nan avg60=0 avg300=0 total=1", false},
        {"some avg10=inf avg60=0 avg300=0 total=1", false},
        {"some avg10=-1 avg60=0 avg300=0 total=1", false},
        {"some avg10=100.01 avg60=0 avg300=0 total=1", false},
        {"some xavg10=1 avg60=0 avg300=0 total=1", false},
        {"some avg10=1 avg10=2 avg60=0 avg300=0 total=1", false},
        {"some avg10=1avg60=0 avg300=0 total=1", false},
        {"some avg10=1 avg60=0 avg300=0 total=x", false},
        {"sometimes avg10=1 avg60=0 avg300=0 total=1", false},
        {"full avg10=1 avg60=0 avg300=0 total=1 trailing", false},
        {"some avg10=1 avg60=0 avg300=0", false},
        {"some avg10=1 avg60=0 avg300=0 total=1\n", true},

        {"some avg10=0 avg60=1e2 avg300=0 total=1", false},
        {"some avg10=0 avg60=0 avg300=1e2 total=1", false},
    };

    for (const auto& [line, expected] : cases) {
        const bool regex_result = regexAccepts(line);
        if (regex_result != expected) {
            std::fprintf(stderr,
                "Regex mismatch: expected=%d actual=%d line=[%s]\n",
                static_cast<int>(expected),
                static_cast<int>(regex_result),
                line.c_str());
            return 1;
        }
        double some = 7.0;
        double full = 9.0;
        const bool parsed = parsePsiLine(line, some, full);
        if (parsed != expected) {
            std::fprintf(stderr,
                "Mismatch: expected=%d parsed=%d line=[%s]\n",
                static_cast<int>(expected),
                static_cast<int>(parsed),
                line.c_str());
            return 1;
        }
        if (!expected) {
            CHECK(some == 7.0 && full == 9.0);
        } else if (line.rfind("some", 0) == 0) {
            CHECK(some >= 0.0 && some <= 100.0 && full == 9.0);
        } else {
            CHECK(full >= 0.0 && full <= 100.0 && some == 7.0);
        }
    }
    return 0;
}
