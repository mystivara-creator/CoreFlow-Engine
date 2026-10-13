#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace coreflow {

// What the device vendor itself configures, harvested read-only from the
// firmware image at runtime. This replaces hard-coded parameter tables: the
// values a vendor writes at boot or in its power-HAL profiles are known-good,
// device-validated operating points for that exact hardware.
//
// Sources (all optional, all read-only):
//   - init *.rc files:    `write <path> <value>` (+ the `on ...` trigger text)
//   - vendor shell init:  `echo <value> > <path>` (e.g. init.*.post_boot.sh)
//   - power HAL JSON:     powerhint*.json Nodes/Actions (per-hint value sets)
enum class VendorProfile : unsigned char {
    Boot = 0,     // written unconditionally during vendor boot configuration
    Default,      // power-HAL default / reset value
    Efficiency,   // low-power, sustained, thermal or battery-saver profile
    Performance,  // launch, interaction, boost, game or rendering profile
};

struct VendorValue {
    std::string value;
    VendorProfile profile{VendorProfile::Boot};
    std::string source;
};

struct VendorKnob {
    std::string path;  // canonical (realpath when the node exists)
    std::vector<VendorValue> values;

    bool declares(std::string_view value) const noexcept;
    bool declaresWithProfile(std::string_view value, VendorProfile profile) const noexcept;
};

class VendorKnowledge {
public:
    static constexpr std::size_t kMaxFiles = 512;
    static constexpr std::size_t kMaxFileBytes = 1024U * 1024U;
    static constexpr std::size_t kMaxValuesPerKnob = 16;

    void clear() noexcept;

    // Scan the default vendor/odm/system configuration locations.
    std::size_t harvest() noexcept;
    // Scan explicit roots (directories or files); used by tests and harvest().
    std::size_t harvest(const std::vector<std::string>& roots) noexcept;

    // Parsers are public so host tests can feed fixture text directly.
    std::size_t parseInitRc(std::string_view text, std::string_view source) noexcept;
    std::size_t parseShell(std::string_view text, std::string_view source) noexcept;
    std::size_t parsePowerHintJson(std::string_view text, std::string_view source) noexcept;

    const VendorKnob* find(std::string_view path) const noexcept;
    const std::vector<VendorKnob>& knobs() const noexcept { return knobs_; }
    std::size_t filesParsed() const noexcept { return files_parsed_; }

    static std::string canonicalPath(std::string_view path) noexcept;
    static VendorProfile profileFromLabel(std::string_view label) noexcept;
    static const char* profileName(VendorProfile profile) noexcept;

private:
    bool add(std::string_view path, std::string_view value,
             VendorProfile profile, std::string_view source) noexcept;
    std::size_t parseFile(const std::string& path) noexcept;
    void scan(const std::string& root, std::size_t depth) noexcept;

    std::vector<VendorKnob> knobs_;
    std::unordered_map<std::string, std::size_t> index_;
    std::unordered_set<std::string> visited_;
    std::size_t files_parsed_{0};
};

} // namespace coreflow
