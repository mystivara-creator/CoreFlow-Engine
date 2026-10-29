#include "coreflow/runtime_intelligence.hpp"
#include "coreflow/resource_actuator.hpp"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>

namespace coreflow {
namespace {
unsigned code(std::string_view text, unsigned max) noexcept {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\n' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\n' || text.back() == '\t' || text.back() == '\r')) text.remove_suffix(1);
    if (text.empty()) return 0U;
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) { text.remove_prefix(2); base = 16; }
    if (text.empty()) return 0U;
    unsigned value = 0;
    const auto r = std::from_chars(text.data(), text.data() + text.size(), value, base);
    return !text.empty() && r.ec == std::errc{} && r.ptr == text.data() + text.size() && value <= max ? value : 0U;
}
std::string readSmall(const std::filesystem::path& path) {
    std::ifstream input(path);
    char bytes[32]{};
    input.read(bytes, 32);
    const auto n = input.gcount();
    return n > 0 && n < 32 ? std::string(bytes, static_cast<std::size_t>(n)) : std::string{};
}
}
StorageHealth parseStorageHealth(std::string_view a, std::string_view b,
                                 std::string_view eol) noexcept {
    StorageHealth h;
    h.life_a = code(a, 11); h.life_b = code(b, 11); h.pre_eol = code(eol, 3);
    h.available = h.life_a != 0U || h.life_b != 0U || h.pre_eol != 0U;
    h.caution = h.life_a >= 10U || h.life_b >= 10U || h.pre_eol >= 2U;
    return h;
}
StorageHealth readStorageHealth(const std::string& directory) noexcept {
    try {
        const std::filesystem::path root(directory);
        return parseStorageHealth(readSmall(root / "life_time_estimation_a"),
            readSmall(root / "life_time_estimation_b"), readSmall(root / "eol_info"));
    } catch (...) { return {}; }
}
StorageHealth summarizeStorageHealth(const std::vector<std::string>& directories) noexcept {
    StorageHealth latest;
    for (const auto& directory : directories) {
        const auto h = readStorageHealth(directory);
        latest.available = latest.available || h.available;
        latest.caution = latest.caution || h.caution;
        latest.life_a = std::max(latest.life_a, h.life_a);
        latest.life_b = std::max(latest.life_b, h.life_b);
        latest.pre_eol = std::max(latest.pre_eol, h.pre_eol);
    }
    return latest;
}
std::vector<std::string> discoverStorageHealthDirectories(const std::string& root) {
    // ABI-defined UFS aliases; finite enumeration, never a recursive sysfs walk.
    std::vector<std::string> result;
    if (root.empty() || !std::filesystem::path(root).is_absolute()) return result;
    for (const char* parent : {"bus/platform/drivers/ufshcd", "bus/platform/devices"}) {
        std::error_code ec;
        std::filesystem::directory_iterator it(std::filesystem::path(root) / parent, ec), end;
        std::size_t visited = 0;
        while (!ec && it != end && visited++ < 4096U && result.size() < 16U) {
            const auto descriptor = it->path() / "health_descriptor";
            if (std::filesystem::is_directory(descriptor, ec)) {
                auto canonical = std::filesystem::canonical(descriptor, ec);
                if (!ec) result.push_back(canonical.string());
            }
            ec.clear();
            it.increment(ec);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}
ControlContinuity inspectOwnedSettings(
    const std::vector<std::pair<std::string, std::string>>& expected,
    bool cpu_policy_directories) noexcept {
    try {
    bool unreadable = false;
    for (const auto& setting : expected) {
        std::string live;
        if (!ResourceActuator::readComparable(cpu_policy_directories ?
                setting.first + "/scaling_governor" : setting.first, live)) { unreadable = true; continue; }
        if (live != ResourceActuator::comparableValue(setting.second)) return ControlContinuity::Changed;
    }
    return unreadable ? ControlContinuity::Unavailable : ControlContinuity::Intact;
    } catch (...) { return ControlContinuity::Unavailable; }
}
} // namespace coreflow
