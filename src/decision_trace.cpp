#include "coreflow/decision_trace.hpp"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include <system_error>
#include <filesystem>
#include <sys/stat.h>

namespace coreflow {
namespace {

constexpr const char* kDecisionTracePath = "/data/adb/coreflow/decision_trace.jsonl";
constexpr std::uintmax_t kDecisionTraceMaxBytes = 512U * 1024U;

std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8U);
    for (const char raw : value) {
        const auto ch = static_cast<unsigned char>(raw);
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20U) {
                    constexpr char hex[] = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(ch >> 4U) & 0x0fU];
                    out += hex[ch & 0x0fU];
                } else {
                    out += static_cast<char>(ch);
                }
        }
    }
    return out;
}

const char* mutationResultTraceName(MutationResult result) noexcept {
    switch (result) {
        case MutationResult::Skipped: return "SKIPPED";
        case MutationResult::Applied: return "APPLIED";
        case MutationResult::Verified: return "VERIFIED";
        case MutationResult::Failed: return "FAILED";
        case MutationResult::RolledBack: return "ROLLED_BACK";
    }
    return "UNKNOWN";
}

void rotateDecisionTraceIfNeeded() noexcept {
    try {
        std::error_code ec;
        const auto size = std::filesystem::file_size(kDecisionTracePath, ec);
        if (ec || size < kDecisionTraceMaxBytes) return;
        const std::string previous = std::string(kDecisionTracePath) + ".1";
        (void)std::remove(previous.c_str());
        (void)std::rename(kDecisionTracePath, previous.c_str());
    } catch (...) {
        // Diagnostics must never interrupt the control loop.
    }
}


} // namespace

void appendDecisionTrace(
    std::uint64_t cycle,
    RuntimeState previous,
    RuntimeState current,
    Decision decision,
    double confidence,
    const SystemContext& context,
    const PolicyPlan& plan,
    HoldReason hold,
    MutationMode mode,
    bool armed,
    bool cpuGovernorAllowed,
    MutationResult cpuMutation,
    MutationResult resourceMutation
) noexcept {
    try {
        rotateDecisionTraceIfNeeded();
        const std::time_t now = std::time(nullptr);
        std::tm tm{};
        char timestamp[32] = "unknown";
        if (now != static_cast<std::time_t>(-1) && gmtime_r(&now, &tm) != nullptr) {
            (void)std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &tm);
        }
        const char* reason = "NO_VERIFIED_MUTATION";
        if (hold != HoldReason::None) reason = "SAFETY_HOLD";
        else if (mode != MutationMode::Adaptive || !armed) reason = "OBSERVE_OR_NOT_ARMED";
        else if (!plan.mutation_eligible) reason = "POLICY_BLOCKED";
        else if (cpuMutation == MutationResult::Failed ||
                 resourceMutation == MutationResult::Failed) reason = "MUTATION_FAILED";
        else if (cpuMutation == MutationResult::Verified ||
                 resourceMutation == MutationResult::Verified) reason = "MUTATION_VERIFIED";
        else if (cpuMutation == MutationResult::RolledBack ||
                 resourceMutation == MutationResult::RolledBack) reason = "MUTATION_ROLLED_BACK";

        std::ofstream out(kDecisionTracePath, std::ios::out | std::ios::app);
        if (!out) return;
        out << "{\"schema\":\"coreflow.decision.v1\""
            << ",\"timestamp\":\"" << timestamp << "\""
            << ",\"cycle\":" << cycle
            << ",\"previous_state\":\"" << jsonEscape(stateName(previous)) << "\""
            << ",\"state\":\"" << jsonEscape(stateName(current)) << "\""
            << ",\"decision\":\"" << jsonEscape(decisionName(decision)) << "\""
            << ",\"confidence\":" << confidence
            << ",\"workload\":\"" << jsonEscape(workloadClassName(context.workload)) << "\""
            << ",\"context_confidence\":" << context.confidence
            << ",\"mutation_eligible\":" << (plan.mutation_eligible ? "true" : "false")
            << ",\"stabilizing_only\":" << (plan.stabilizing_only ? "true" : "false")
            << ",\"candidate_count\":" << plan.candidates.size()
            << ",\"safety_hold\":\"" << jsonEscape(holdReasonName(hold)) << "\""
            << ",\"mode\":\"" << (mode == MutationMode::Adaptive ? "adaptive" :
                                      hold != HoldReason::None ? "disabled" : "observe") << "\""
            << ",\"mutation_armed\":" << (armed ? "true" : "false")
            << ",\"allow_cpu_governor\":" << (cpuGovernorAllowed ? "true" : "false")
            << ",\"cpu_mutation\":\"" << mutationResultTraceName(cpuMutation) << "\""
            << ",\"resource_mutation\":\"" << mutationResultTraceName(resourceMutation) << "\""
            << ",\"reason\":\"" << reason << "\"}\n";
        out.flush();
        (void)::chmod(kDecisionTracePath, 0600);
    } catch (...) {
        // A trace failure is non-fatal and never changes mutation authorization.
    }
}


} // namespace coreflow
