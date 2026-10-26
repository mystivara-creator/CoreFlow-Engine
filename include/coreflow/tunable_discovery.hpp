#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "coreflow/environment.hpp"
#include "coreflow/vendor_knowledge.hpp"

namespace coreflow {

// Generic, capability-driven tunable discovery.
//
// Roots are not written into this module. They come from the kernel's own
// mount table (see kernel_roots.hpp): every procfs mount contributes its sysctl
// tree, every sysfs mount its attribute tree, and every cgroup mount its
// controller files. Each tree is walked breadth-first, bounded by depth, visit
// budget and filesystem boundary, and never through symlinks.
//
// A regular file becomes a candidate only when all of these hold:
//   - the kernel marks it writable (mode bits, not root's access() answer);
//   - it is not a universally fatal surface (fatal_surface.hpp);
//   - known subsystems receive a semantic domain from path structure; unknown
//     writable kernel nodes are retained in the generic Vendor domain rather
//     than discarded because they are absent from a catalog.
//
// Admissibility beyond that is decided by classifySurface(). No parameter names
// are listed anywhere in this module.
struct TunableDiscoveryStats {
    std::size_t walked{0};          // writable-candidate files examined
    std::size_t added{0};           // candidates admitted into the matrix
    std::size_t vendor_paths{0};    // vendor-declared nodes admitted
    std::size_t fatal_skipped{0};   // refused by the universal fatal guard
    std::size_t unclaimed{0};       // generic writable kernel nodes with no semantic prior
    std::size_t visited_entries{0}; // shared budget across every mount and vendor node
    bool truncated{false};          // a traversal/input cap was reached; inventory may be incomplete
};

class TunableDiscovery {
public:
    static constexpr std::size_t kMaxTunables = 16384;
    static constexpr std::size_t kMaxVisits = 40000;
    static constexpr std::size_t kMaxDirectoryEntries = 4096;
    static constexpr std::size_t kMaxFirstLineBytes = 4096;
    static constexpr std::size_t kMaxOptionListBytes = 64U * 1024U;
    static constexpr std::size_t kSysfsDepth = 16;
    static constexpr std::size_t kSysctlDepth = 8;
    static constexpr std::size_t kCgroupDepth = 8;

    // root_prefix lets host tests mount a fixture tree: the mount table is read
    // from <root_prefix>/proc/mounts. On the device it is empty.
    explicit TunableDiscovery(std::string root_prefix = {}) : prefix_(std::move(root_prefix)) {}

    TunableDiscoveryStats discover(EnvironmentCapabilityMatrix& matrix,
                                   const VendorKnowledge& vendor) const noexcept;

    // Attach kernel-advertised options and vendor values to every tunable
    // already present in the matrix (including ones added by other probes).
    void enrich(EnvironmentCapabilityMatrix& matrix,
                const VendorKnowledge& vendor) const noexcept;

    // Exposed for tests.
    ResourceDomain domainFor(std::string_view device_path) const noexcept;
    std::string nameFor(std::string_view device_path) const;
    static bool kernelWritable(const std::string& path) noexcept;

private:
    std::string_view strip(std::string_view path) const noexcept;
    bool addTunable(EnvironmentCapabilityMatrix& matrix, const std::string& path,
                    const VendorKnowledge& vendor, TunableDiscoveryStats& stats) const noexcept;
    void walkTree(EnvironmentCapabilityMatrix& matrix, const VendorKnowledge& vendor,
                  const std::string& root, std::size_t max_depth,
                  bool (*accept_dir)(std::string_view), TunableDiscoveryStats& stats) const noexcept;

    std::string prefix_;
};

} // namespace coreflow
