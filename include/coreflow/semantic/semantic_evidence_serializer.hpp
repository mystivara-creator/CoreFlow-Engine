#pragma once

#include "coreflow/semantic/domain_evidence_aggregator.hpp"
#include "coreflow/semantic/dynamic_semantic_descriptor.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace coreflow::semantic {

// Emits the versioned, daemon-owned semantic telemetry contract. An empty
// descriptor span is explicitly unavailable; the serializer never promotes
// Candidate to Validated or infers a domain.
[[nodiscard]] std::string serializeSemanticEvidence(
    std::span<const DynamicSemanticDescriptor> descriptors,
    std::uint64_t now_ns);

} // namespace coreflow::semantic
