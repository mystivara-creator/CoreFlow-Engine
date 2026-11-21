/**
 * @file oem_collaboration_map.hpp
 * @brief CoreFlow v6.1 Persistent OEM-Collaboration Map (auto-resets on kernel/ROM update).
 * @license Apache-2.0
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace coreflow::v6 {

struct OemContentionRecord {
    uint64_t retry_after_ms{0};
    uint32_t strikes{0};
    bool permanently_yielded{false};
    std::string observed_value;
};

class OemCollaborationMap {
public:
    static constexpr uint64_t kStrike1CooldownMs = 30000ULL;   // 30 seconds
    static constexpr uint64_t kStrike2CooldownMs = 120000ULL;  // 2 minutes
    static constexpr uint64_t kStrike3CooldownMs = 600000ULL;  // 10 minutes
    static constexpr uint32_t kPermanentYieldStrikes = 4U;

    OemCollaborationMap() noexcept = default;

    // Loads persisted OEM-yielded paths if kernel_release + rom_fingerprint match;
    // automatically clears and re-learns after a ROM/OTA or kernel update.
    void initialize(const std::string& state_dir,
                    const std::string& kernel_release,
                    const std::string& rom_fingerprint) noexcept;

    // Returns true if the node is permanently yielded to OEM or currently in cooldown.
    [[nodiscard]] bool is_blocked(const std::string& path, uint64_t now_ms) const noexcept;

    // Returns true if the node is permanently yielded to OEM for this kernel+ROM.
    [[nodiscard]] bool is_permanently_yielded(const std::string& path) const noexcept;

    // Returns the last OEM-observed value for a contended node (or nullptr if not tracked).
    [[nodiscard]] const std::string* observed_value_for(const std::string& path) const noexcept;

    // Checks a previously contended node whose cooldown has expired.
    // Returns true if it is safe to retry (OEM settled), or false if OEM is still
    // mutating the node (in which case strikes escalate automatically).
    bool check_cooldown_expired(const std::string& path,
                                const std::string& live_value,
                                bool equivalent_to_observed,
                                uint64_t now_ms) noexcept;

    bool check_cooldown_expired(const std::string& path,
                                const std::string& live_value,
                                uint64_t now_ms) noexcept;

    // Records an OEM write collision and escalates backoff (30s -> 2m -> 10m -> permanent yield).
    void record_contention(const std::string& path,
                           const std::string& observed_value,
                           uint64_t now_ms) noexcept;

    // Resets only transient in-session cooldown timers (e.g. on SIGHUP),
    // preserving confirmed permanently-yielded OEM-owned nodes.
    void reset_transient_cooldowns() noexcept;

    // Clears all state and removes the persisted map (for tests / explicit reset).
    void clear_all() noexcept;

    [[nodiscard]] std::vector<std::string> active_contended_paths(uint64_t now_ms = 0U) const;
    [[nodiscard]] std::vector<std::string> permanently_yielded_paths() const;
    [[nodiscard]] uint32_t active_count(uint64_t now_ms = 0U) const noexcept;
    [[nodiscard]] uint32_t yielded_count() const noexcept;

    // Discovers a stable ROM build fingerprint from Android properties or /proc/version.
    [[nodiscard]] static std::string discover_rom_fingerprint(
        const std::string& procfs_root = "/proc") noexcept;

private:
    void persist_locked() const noexcept;
    void load_locked() noexcept;

    std::string state_dir_;
    std::string map_path_;
    std::string kernel_release_;
    std::string rom_fingerprint_;
    std::unordered_map<std::string, OemContentionRecord> records_;
};

} // namespace coreflow::v6
