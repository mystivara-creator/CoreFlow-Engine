#include "coreflow/semantic/semantic_evidence_serializer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace coreflow::semantic {
namespace {

constexpr std::uint64_t kFreshnessLimitNs = 5'000'000'000ULL;

const char* bindingStateName(DescriptorState state) noexcept {
    return state == DescriptorState::Validated ? "validated" : "candidate";
}

const char* domainName(FunctionalDomain domain) noexcept {
    switch (domain) {
        case FunctionalDomain::Cpu: return "cpu";
        case FunctionalDomain::Gpu: return "gpu";
        case FunctionalDomain::Vm: return "vm";
        case FunctionalDomain::Unresolved: return "unresolved";
    }
    return "unresolved";
}

const char* assessmentStateName(DomainAssessmentState state) noexcept {
    switch (state) {
        case DomainAssessmentState::Candidate: return "candidate";
        case DomainAssessmentState::Corroborated: return "corroborated";
        case DomainAssessmentState::Verified: return "verified";
        case DomainAssessmentState::Unresolved: return "unresolved";
    }
    return "unresolved";
}

struct Age final {
    bool observed{false};
    std::uint64_t nanoseconds{0};
    bool fresh{false};
};

Age calculateAge(std::uint64_t observed_at_ns, std::uint64_t now_ns) noexcept {
    if (observed_at_ns == 0U || now_ns < observed_at_ns) return {};
    const auto age_ns = now_ns - observed_at_ns;
    return {true, age_ns, age_ns <= kFreshnessLimitNs};
}

void writeAge(std::ostringstream& out, const Age& age) {
    if (!age.observed) {
        out << "null";
        return;
    }
    out << std::fixed << std::setprecision(6)
        << (static_cast<double>(age.nanoseconds) / 1'000'000.0);
}

} // namespace

std::string serializeSemanticEvidence(
    std::span<const DynamicSemanticDescriptor> descriptors,
    std::uint64_t now_ns) {
    std::ostringstream out;
    out << "{\"schemaVersion\":1";

    if (descriptors.empty()) {
        out << ",\"status\":\"unavailable\""
            << ",\"generation\":null"
            << ",\"observedAtMonotonicNs\":null"
            << ",\"ageMs\":null"
            << ",\"freshness\":\"stale\""
            << ",\"bindings\":[]}";
        return out.str();
    }

    const auto newest = std::max_element(
        descriptors.begin(), descriptors.end(),
        [](const DynamicSemanticDescriptor& lhs, const DynamicSemanticDescriptor& rhs) {
            return lhs.domainAssessment().assessed_at_ns <
                   rhs.domainAssessment().assessed_at_ns;
        });
    const auto newest_observed = newest->domainAssessment().assessed_at_ns != 0U
        ? newest->domainAssessment().assessed_at_ns
        : newest->provenance().observed_at_ns;
    const auto top_age = calculateAge(newest_observed, now_ns);

    out << ",\"status\":\"available\""
        << ",\"generation\":\"" << newest->generation() << "\""
        << ",\"observedAtMonotonicNs\":\"" << newest_observed << "\""
        << ",\"ageMs\":";
    writeAge(out, top_age);
    out << ",\"freshness\":\""
        << (top_age.fresh ? "fresh" : "stale") << "\""
        << ",\"bindings\":[";

    bool first = true;
    for (const auto& descriptor : descriptors) {
        if (!first) out << ',';
        first = false;

        const auto& assessment = descriptor.domainAssessment();
        const auto observed = assessment.assessed_at_ns != 0U
            ? assessment.assessed_at_ns
            : descriptor.provenance().observed_at_ns;
        const auto age = calculateAge(observed, now_ns);
        const double confidence = std::isfinite(assessment.confidence)
            ? std::clamp(static_cast<double>(assessment.confidence), 0.0, 1.0)
            : 0.0;

        out << "{\"bindingState\":\""
            << bindingStateName(descriptor.state())
            << "\",\"domain\":\""
            << domainName(descriptor.semantic().functional_domain)
            << "\",\"domainAssessment\":{\"state\":\""
            << assessmentStateName(assessment.state)
            << "\",\"domain\":\""
            << domainName(assessment.domain)
            << "\",\"confidence\":"
            << std::fixed << std::setprecision(6) << confidence
            << ",\"corroboratingRelations\":"
            << static_cast<unsigned>(assessment.corroborating_relations)
            << "},\"generation\":\"" << descriptor.generation() << "\""
            << ",\"observedAtMonotonicNs\":\"" << observed << "\""
            << ",\"ageMs\":";
        writeAge(out, age);
        out << ",\"freshness\":\""
            << (age.fresh ? "fresh" : "stale") << "\"}";
    }

    out << "]}";
    return out.str();
}

} // namespace coreflow::semantic
