/**
 * @file oem_collaboration_map.cpp
 * @brief CoreFlow v6.1 Persistent OEM-Collaboration Map (auto-resets on kernel/ROM update).
 * @license Apache-2.0
 */
#include "coreflow/v6/oem_collaboration_map.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <sys/system_properties.h>
#endif

namespace coreflow::v6 {
namespace fs = std::filesystem;

namespace {

std::string sanitize_token(std::string_view raw) {
    std::string out;
    out.reserve(std::min<size_t>(raw.size(), 128U));
    for (unsigned char c : raw) {
        if (out.size() >= 128U) break;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' ||
            c == ':' || c == '/') {
            out.push_back(static_cast<char>(c));
        } else if (c == ' ') {
            out.push_back('_');
        }
    }
    return out;
}

bool valid_sys_or_proc_path(std::string_view path) noexcept {
    if (path.empty() || path.size() > 512U || path.front() != '/') return false;
    if (path.find("..") != std::string_view::npos ||
        path.find('\n') != std::string_view::npos ||
        path.find('\r') != std::string_view::npos ||
        path.find('\t') != std::string_view::npos) {
        return false;
    }
    return path.rfind("/sys/", 0) == 0 ||
           path.rfind("/proc/sys/", 0) == 0 ||
           path.rfind("/dev/", 0) == 0 ||
           path.rfind("/tmp/", 0) == 0;
}

bool write_all_fd(int fd, std::string_view data) noexcept {
    size_t off = 0U;
    while (off < data.size()) {
        const ssize_t n = ::write(fd, data.data() + off, data.size() - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

std::string read_small_raw(const std::string& path, size_t max_bytes = 16384U) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    char buf[1024];
    std::string out;
    while (out.size() < max_bytes) {
        const ssize_t n = ::read(fd, buf, std::min(sizeof(buf), max_bytes - out.size()));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        out.append(buf, static_cast<size_t>(n));
    }
    (void)::close(fd);
    return out;
}

uint64_t fnv1a_64(std::string_view data) noexcept {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : data) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

} // namespace

std::string OemCollaborationMap::discover_rom_fingerprint(
    const std::string& procfs_root) noexcept {
#ifdef __ANDROID__
    char prop[PROP_VALUE_MAX]{};
    const char* keys[] = {
        "ro.build.fingerprint",
        "ro.system.build.fingerprint",
        "ro.miui.ui.version.name",
        "ro.build.version.incremental",
    };
    for (const char* key : keys) {
        const int len = __system_property_get(key, prop);
        if (len > 0) {
            const std::string tok = sanitize_token(std::string_view(prop, static_cast<size_t>(len)));
            if (!tok.empty()) return tok;
        }
    }
#endif
    const std::string ver = read_small_raw(procfs_root + "/version", 512U);
    if (!ver.empty()) {
        char hex[32];
        std::snprintf(hex, sizeof(hex), "procver-%016llx",
                      static_cast<unsigned long long>(fnv1a_64(ver)));
        return std::string(hex);
    }
    return "generic-rom";
}

void OemCollaborationMap::initialize(
    const std::string& state_dir,
    const std::string& kernel_release,
    const std::string& rom_fingerprint) noexcept {
    state_dir_ = state_dir;
    map_path_ = state_dir.empty() ? std::string{} : (state_dir + "/oem_collab.map");
    kernel_release_ = sanitize_token(kernel_release);
    if (kernel_release_.empty()) kernel_release_ = "unknown-kernel";
    rom_fingerprint_ = sanitize_token(rom_fingerprint);
    if (rom_fingerprint_.empty()) rom_fingerprint_ = "unknown-rom";
    records_.clear();
    load_locked();
}

bool OemCollaborationMap::is_permanently_yielded(const std::string& path) const noexcept {
    const auto it = records_.find(path);
    return it != records_.end() && it->second.permanently_yielded;
}

bool OemCollaborationMap::is_blocked(const std::string& path, uint64_t now_ms) const noexcept {
    const auto it = records_.find(path);
    if (it == records_.end()) return false;
    if (it->second.permanently_yielded) return true;
    return it->second.retry_after_ms > 0U && now_ms < it->second.retry_after_ms;
}

const std::string* OemCollaborationMap::observed_value_for(
    const std::string& path) const noexcept {
    const auto it = records_.find(path);
    return it == records_.end() ? nullptr : &it->second.observed_value;
}

bool OemCollaborationMap::check_cooldown_expired(
    const std::string& path,
    const std::string& live_value,
    uint64_t now_ms) noexcept {
    const auto it = records_.find(path);
    if (it == records_.end()) return true;
    return check_cooldown_expired(path, live_value, live_value == it->second.observed_value, now_ms);
}

bool OemCollaborationMap::check_cooldown_expired(
    const std::string& path,
    const std::string& live_value,
    bool equivalent_to_observed,
    uint64_t now_ms) noexcept {
    const auto it = records_.find(path);
    if (it == records_.end()) return true;
    if (it->second.permanently_yielded) return false;
    if (it->second.retry_after_ms == 0U) return true;
    if (now_ms < it->second.retry_after_ms) return false;

    if (!equivalent_to_observed) {
        // OEM daemon is still mutating this node even while we yielded; escalate without writing.
        record_contention(path, live_value, now_ms);
        return false;
    }
    // Cooldown expired and OEM appears settled: allow retry while PRESERVING strike count
    // so immediate re-contention escalates to 2m -> 10m -> permanent OEM yield.
    it->second.retry_after_ms = 0U;
    return true;
}

void OemCollaborationMap::record_contention(
    const std::string& path,
    const std::string& observed_value,
    uint64_t now_ms) noexcept {
    if (!valid_sys_or_proc_path(path)) return;
    auto& rec = records_[path];
    ++rec.strikes;
    rec.observed_value = observed_value;
    if (rec.strikes >= kPermanentYieldStrikes) {
        const bool newly_yielded = !rec.permanently_yielded;
        rec.permanently_yielded = true;
        rec.retry_after_ms = UINT64_MAX;
        if (newly_yielded) persist_locked();
        return;
    }
    const uint64_t backoff =
        rec.strikes == 1U ? kStrike1CooldownMs :
        rec.strikes == 2U ? kStrike2CooldownMs : kStrike3CooldownMs;
    rec.retry_after_ms = now_ms + backoff;
}

void OemCollaborationMap::reset_transient_cooldowns() noexcept {
    for (auto it = records_.begin(); it != records_.end();) {
        if (it->second.permanently_yielded) {
            ++it;
        } else {
            it = records_.erase(it);
        }
    }
}

void OemCollaborationMap::clear_all() noexcept {
    records_.clear();
    if (!map_path_.empty()) {
        (void)::unlink(map_path_.c_str());
    }
}

std::vector<std::string> OemCollaborationMap::active_contended_paths(uint64_t now_ms) const {
    std::vector<std::string> out;
    for (const auto& [path, rec] : records_) {
        if (rec.permanently_yielded ||
            (rec.retry_after_ms > 0U && (now_ms == 0U || now_ms < rec.retry_after_ms))) {
            out.push_back(path);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string> OemCollaborationMap::permanently_yielded_paths() const {
    std::vector<std::string> out;
    for (const auto& [path, rec] : records_) {
        if (rec.permanently_yielded) out.push_back(path);
    }
    std::sort(out.begin(), out.end());
    return out;
}

uint32_t OemCollaborationMap::active_count(uint64_t now_ms) const noexcept {
    uint32_t n = 0U;
    for (const auto& [_, rec] : records_) {
        if (rec.permanently_yielded ||
            (rec.retry_after_ms > 0U && (now_ms == 0U || now_ms < rec.retry_after_ms))) {
            ++n;
        }
    }
    return n;
}

uint32_t OemCollaborationMap::yielded_count() const noexcept {
    uint32_t n = 0U;
    for (const auto& [_, rec] : records_) {
        if (rec.permanently_yielded) ++n;
    }
    return n;
}

void OemCollaborationMap::load_locked() noexcept {
    if (map_path_.empty()) return;
    const std::string raw = read_small_raw(map_path_, 32768U);
    if (raw.empty()) return;

    std::istringstream ss(raw);
    std::string header;
    if (!std::getline(ss, header)) return;
    if (!header.empty() && header.back() == '\r') header.pop_back();

    const std::string expected_header =
        "OEMCM1\t" + kernel_release_ + "\t" + rom_fingerprint_;
    if (header != expected_header) {
        // Kernel or ROM updated -> auto-reset map so CoreFlow re-learns on the new OS build.
        (void)::unlink(map_path_.c_str());
        return;
    }

    std::string line;
    size_t count = 0U;
    while (std::getline(ss, line) && count < 128U) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || !valid_sys_or_proc_path(line)) continue;
        OemContentionRecord rec{};
        rec.strikes = kPermanentYieldStrikes;
        rec.permanently_yielded = true;
        rec.retry_after_ms = UINT64_MAX;
        records_[line] = std::move(rec);
        ++count;
    }
}

void OemCollaborationMap::persist_locked() const noexcept {
    if (state_dir_.empty() || map_path_.empty()) return;
    struct stat st{};
    if (::lstat(state_dir_.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) {
        return;
    }

    std::string payload = "OEMCM1\t" + kernel_release_ + "\t" + rom_fingerprint_ + "\n";
    auto paths = permanently_yielded_paths();
    for (const auto& p : paths) {
        payload += p + "\n";
    }

    const std::string tmp_path = map_path_ + ".tmp." + std::to_string(::getpid());
    (void)::unlink(tmp_path.c_str());
    const int fd = ::open(tmp_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return;
    const bool ok = write_all_fd(fd, payload) && (::fsync(fd) == 0);
    (void)::close(fd);
    if (!ok) {
        (void)::unlink(tmp_path.c_str());
        return;
    }
    if (::rename(tmp_path.c_str(), map_path_.c_str()) != 0) {
        (void)::unlink(tmp_path.c_str());
        return;
    }
    const int dfd = ::open(state_dir_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dfd >= 0) {
        (void)::fsync(dfd);
        (void)::close(dfd);
    }
}

} // namespace coreflow::v6
