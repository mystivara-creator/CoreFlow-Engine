#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include "coreflow/environment.hpp"

namespace coreflow {

// Semantic prior for one Android/Linux surface node.
// Sources: AOSP (source.android.com) + Linux kernel docs (docs.kernel.org).
// Presence in this catalog does NOT grant mutation permission.
enum class KnowledgeRisk : std::uint8_t {
    ObserveOnly = 0,  // Safe to read; writes forbidden or meaningless
    BoundedTune,      // May be tuned under explicit policy + journal
    VendorVolatile,   // Path/semantics vary by OEM/ACK branch
    Quarantined,      // Known dangerous without topology proof
    FrameworkOwned    // Owned by Android framework / lmkd / AMS — do not fight
};

enum class KnowledgeLayer : std::uint8_t {
    LinuxKernel = 0,
    AndroidCommonKernel,
    AndroidFramework,
    VendorHAL
};

struct EcosystemKnowledgeEntry {
    const char* key;           // stable id, e.g. "vm.swappiness"
    const char* path;          // canonical path or path prefix
    ResourceDomain domain;
    KnowledgeLayer layer;
    KnowledgeRisk risk;
    bool typically_writable;
    const char* summary;       // short semantic note for logs / status
    const char* reference;     // doc anchor (AOSP or kernel)
};

// Compile-time catalog size.
std::size_t ecosystemKnowledgeCount() noexcept;

// Linear lookup by exact path or by key. Returns nullptr if unknown.
const EcosystemKnowledgeEntry* lookupEcosystemKnowledgeByPath(
    const std::string& path) noexcept;
const EcosystemKnowledgeEntry* lookupEcosystemKnowledgeByKey(
    const std::string& key) noexcept;

const char* knowledgeRiskName(KnowledgeRisk risk) noexcept;
const char* knowledgeLayerName(KnowledgeLayer layer) noexcept;

// Annotate a discovered capability with catalog risk text (empty if unknown).
std::string describeEcosystemNode(const ResourceCapability& cap) noexcept;

} // namespace coreflow
