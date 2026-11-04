/**
 * CoreFlow v2 verified write-ahead journal and OEM baseline recovery.
 * The journal is intentionally tiny: only nodes currently modified are saved.
 * Never call these methods from a POSIX fatal signal handler.
 */
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {

std::string read_line(const std::string& path) {
    if (path.empty() || path.find('\0') != std::string::npos) return {};
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return {};

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return {};

    char buffer[4096];
    std::string raw;
    while (raw.size() < sizeof(buffer)) {
        const ssize_t n = ::read(fd.get(), buffer, sizeof(buffer) - raw.size());
        if (n < 0) {
            if (errno == EINTR) continue;
            return {};
        }
        if (n == 0) break;
        raw.append(buffer, static_cast<size_t>(n));
        if (raw.find('\n') != std::string::npos) break;
    }

    const size_t nl = raw.find('\n');
    if (nl != std::string::npos) raw.resize(nl);
    if (!raw.empty() && raw.back() == '\r') raw.pop_back();
    return raw;
}

bool path_safe(const std::string& path) {
    if (path.empty() || path.size() > 512 || path.front() != '/' || path.back() == '/') return false;
    if (path.find('\0') != std::string::npos ||
        path.find_first_of("\r\n\t") != std::string::npos ||
        path.find("..") != std::string::npos ||
        path.find("//") != std::string::npos) {
        return false;
    }
    if (path.rfind("/sys/", 0) == 0 || path.rfind("/proc/sys/", 0) == 0) return true;

    // Android can mount the CPU controller under /dev (for example /dev/cpuctl).
    // Authorize only the semantic top-app UClamp leaf discovered by the engine,
    // not arbitrary /dev nodes.
    if (path.rfind("/dev/", 0) == 0) {
        const std::filesystem::path p(path);
        const auto leaf = p.filename().string();
        const auto parent = p.parent_path().filename().string();
        if (parent == "top-app" && (leaf == "cpu.uclamp.min" || leaf == "cpu.uclamp.max")) {
            return true;
        }
    }

    // Unit tests can use private mock files only when non-root or explicitly isolated in /tmp.
    const char* test_dir = std::getenv("COREFLOW_TEST_STATE_DIR");
    return test_dir != nullptr && *test_dir != '\0' && path.rfind("/tmp/", 0) == 0;
}

std::string hex_encode(std::string_view input) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char ch : input) {
        out.push_back(digits[ch >> 4]);
        out.push_back(digits[ch & 15]);
    }
    return out;
}

int nibble(char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool hex_decode(std::string_view input, std::string& out) {
    if (input.empty() || (input.size() % 2) != 0 || input.size() > 8192) return false;
    out.clear();
    out.reserve(input.size() / 2);
    for (size_t i = 0; i < input.size(); i += 2) {
        const int high = nibble(input[i]);
        const int low = nibble(input[i + 1]);
        if (high < 0 || low < 0) return false;
        out.push_back(static_cast<char>((high << 4) | low));
    }
    return !out.empty();
}

bool readback_matches(const std::string& path, const std::string& expected, const std::string& actual) {
    if (expected == actual) return true;
    if (path.find("/cpu.uclamp.") == std::string::npos) return false;
    const auto numeric = [](const std::string& value, double& percent) {
        if (value == "max") {
            percent = 100.0;
            return true;
        }
        return parse_ascii_double(value, percent) && percent >= 0.0 && percent <= 100.0;
    };
    double before = 0.0, after = 0.0;
    // cgroup v2 formats numeric utilization clamps as percentages (e.g. 31.25).
    return numeric(expected, before) && numeric(actual, after) &&
           std::abs(before - after) <= 0.05;
}

bool write_all(int fd, std::string_view data) {
    while (!data.empty()) {
        const ssize_t n = ::write(fd, data.data(), data.size());
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        data.remove_prefix(static_cast<size_t>(n));
    }
    return true;
}

bool sync_directory(const std::string& path) {
    const auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return false;
    const std::string dir = pos == 0 ? "/" : path.substr(0, pos);
    UniqueFd fd(::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    return fd.valid() && ::fsync(fd.get()) == 0;
}

std::string extract_selector_token(const std::string& path, std::string raw) {
    if (path.find("/queue/scheduler") != std::string::npos) {
        const size_t open_b = raw.find('[');
        const size_t close_b =
            open_b == std::string::npos ? std::string::npos : raw.find(']', open_b + 1);
        if (open_b != std::string::npos && close_b != std::string::npos && close_b > open_b + 1) {
            return raw.substr(open_b + 1, close_b - open_b - 1);
        }
    }
    return raw;
}

} // namespace

BaselineStore::BaselineStore() {
    const char* test = std::getenv("COREFLOW_TEST_STATE_DIR");
    journal_path_ = (test && *test ? std::string(test) : "/data/adb/coreflow") + "/baseline.journal";
    boot_id_ = read_line("/proc/sys/kernel/random/boot_id");
}

BaselineStore& BaselineStore::instance() {
    static BaselineStore store;
    return store;
}

std::string BaselineStore::read_raw_node(const std::string& path) const {
    return extract_selector_token(path, read_line(path));
}

bool BaselineStore::write_raw_node(const std::string& path, const std::string& value) {
    if (!path_safe(path) || value.empty() || value.size() > 4096) return false;
    const bool virtual_node = path.rfind("/sys/", 0) == 0 ||
                              path.rfind("/proc/", 0) == 0 ||
                              path.rfind("/dev/", 0) == 0;

    // Open with O_NOFOLLOW | O_NONBLOCK first, then fstat the descriptor to eliminate TOCTOU.
    UniqueFd fd(::open(path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    if (!virtual_node && ::ftruncate(fd.get(), 0) != 0) return false;
    if (!write_all(fd.get(), value)) return false;
    if (!virtual_node && ::fsync(fd.get()) != 0) return false;
    fd.reset();

    // Kernel nodes may normalize input; verify the actual read-back.
    return readback_matches(path, value, read_raw_node(path));
}

bool BaselineStore::persist_locked() {
    if (boot_id_.empty()) return false;
    std::error_code error;
    const auto parent = std::filesystem::path(journal_path_).parent_path();
    std::filesystem::create_directories(parent, error);
    if (error) return false;
    (void)::chmod(parent.c_str(), 0700);

    std::string payload = "CFJ3\t" + boot_id_ + "\n";
    for (const auto& [path, snap] : store_) {
        if (snap.is_modified) {
            if (!path_safe(path) || snap.factory_value.empty()) return false;
            if (snap.active_value.empty() || snap.prior_value.empty()) return false;
            payload += path + "\t" + hex_encode(snap.factory_value) +
                       "\t" + hex_encode(snap.prior_value) +
                       "\t" + hex_encode(snap.active_value) + "\n";
        }
    }

    const std::string temp = journal_path_ + ".tmp";
    (void)::unlink(temp.c_str());
    UniqueFd fd(::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!fd.valid() || !write_all(fd.get(), payload) || ::fsync(fd.get()) != 0) {
        (void)::unlink(temp.c_str());
        return false;
    }
    fd.reset();
    if (::rename(temp.c_str(), journal_path_.c_str()) != 0) {
        (void)::unlink(temp.c_str());
        return false;
    }
    return sync_directory(journal_path_);
}

bool BaselineStore::recover_pending() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (recovery_checked_) return true;

    UniqueFd fd(::open(journal_path_.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) {
        if (errno == ENOENT) {
            recovery_checked_ = true;
            return true;
        }
        return false;
    }

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 || st.st_size > 65536) {
        return false;
    }

    std::string raw;
    raw.reserve(static_cast<size_t>(st.st_size));
    char buf[4096];
    while (raw.size() <= 65536) {
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) break;
        raw.append(buf, static_cast<size_t>(n));
    }
    fd.reset();
    if (raw.size() > 65536) return false;

    std::istringstream file(raw);
    std::string header;
    if (!std::getline(file, header) || boot_id_.empty()) return false;
    const bool legacy = header.rfind("CFJ1\t", 0) == 0;
    const bool v2 = header.rfind("CFJ2\t", 0) == 0;
    if (!legacy && !v2 && header.rfind("CFJ3\t", 0) != 0) return false;
    if (!header.empty() && header.back() == '\r') header.pop_back();
    const std::string journal_boot = header.substr(5);
    if (journal_boot != boot_id_) {
        // Kernel settings reset on reboot; never overwrite the new boot's OEM state.
        if (::unlink(journal_path_.c_str()) != 0 && errno != ENOENT) return false;
        recovery_checked_ = true;
        return true;
    }

    std::vector<NodeSnapshot> entries;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        const auto sep = line.find('\t');
        const auto second = sep == std::string::npos ? sep : line.find('\t', sep + 1);
        const auto third = second == std::string::npos ? second : line.find('\t', second + 1);
        std::string value, prior, applied;
        if (sep == std::string::npos ||
            !path_safe(line.substr(0, sep)) ||
            (legacy ? second != std::string::npos : second == std::string::npos) ||
            (v2 ? third != std::string::npos : (!legacy && third == std::string::npos)) ||
            !hex_decode(std::string_view(line).substr(
                sep + 1, legacy ? std::string_view::npos : second - sep - 1), value) ||
            (!legacy && !v2 && !hex_decode(
                std::string_view(line).substr(second + 1, third - second - 1), prior)) ||
            (!legacy && !hex_decode(std::string_view(line).substr(
                (v2 ? second : third) + 1), applied))) {
            return false;
        }
        if (legacy) applied = read_raw_node(line.substr(0, sep));
        if (v2 || legacy) prior = applied;
        if (prior.empty() || applied.empty()) return false;
        entries.push_back(NodeSnapshot{line.substr(0, sep), std::move(value),
                                       std::move(applied), true, std::move(prior)});
        if (entries.size() > 256) return false;
    }
    if (!file.eof()) return false;

    for (const auto& snap : entries) {
        store_[snap.path] = snap;
    }
    rollback_locked();
    for (const auto& [_, snap] : store_) {
        if (snap.is_modified) return false;
    }
    recovery_checked_ = true;
    return true;
}

bool BaselineStore::capture(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!recovery_checked_ || !path_safe(path)) return false;
    if (store_.contains(path)) return true;
    const std::string before = read_raw_node(path);
    if (before.empty()) return false;
    store_.emplace(path, NodeSnapshot{path, before, before, false, before});
    return true;
}

size_t BaselineStore::rollback_locked() {
    size_t restored = 0;
    // A clamp pair can require the other member to be restored first.
    for (int pass = 0; pass < 2; ++pass) {
        for (auto& [path, snap] : store_) {
            if (!snap.is_modified) continue;
            const std::string actual = read_raw_node(path);
            if (actual.empty()) continue;
            // Recovery accepts either side of a write in flight, or the baseline.
            if (!readback_matches(path, snap.active_value, actual) &&
                !readback_matches(path, snap.prior_value, actual) &&
                !readback_matches(path, snap.factory_value, actual)) {
                snap.factory_value = actual;
                snap.active_value = actual;
                snap.prior_value = actual;
                snap.is_modified = false;
                continue;
            }
            if (readback_matches(path, snap.factory_value, actual) ||
                write_raw_node(path, snap.factory_value)) {
                snap.active_value = snap.factory_value;
                snap.prior_value = snap.factory_value;
                snap.is_modified = false;
                ++restored;
            }
        }
    }
    // A failed rollback keeps the original journal records for next process start.
    if (!persist_locked()) std::cerr << "[CoreFlow] Failed to persist rollback journal\n";
    return restored;
}

bool BaselineStore::apply_mutation(const std::string& path, const std::string& new_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!recovery_checked_) return false;
    if (!path_safe(path) || new_value.empty()) {
        rollback_locked();
        return false;
    }
    auto it = store_.find(path);
    if (it == store_.end()) {
        const std::string before = read_raw_node(path);
        if (before.empty()) {
            rollback_locked();
            return false;
        }
        it = store_.emplace(path, NodeSnapshot{path, before, before, false, before}).first;
    }
    const std::string actual = read_raw_node(path);
    if (actual.empty() ||
        !readback_matches(path, it->second.active_value, actual)) {
        if (!actual.empty()) {
            it->second.factory_value = actual;
            it->second.active_value = actual;
            it->second.prior_value = actual;
            it->second.is_modified = false;
        }
        rollback_locked();
        return false;
    }
    if (it->second.active_value == new_value && actual == new_value) return true;
    // Write-ahead: fsync original value before any kernel write.
    const bool previous_modified = it->second.is_modified;
    const std::string previous_value = it->second.active_value;
    const std::string previous_prior = it->second.prior_value;
    it->second.is_modified = true;
    it->second.prior_value = previous_value;
    it->second.active_value = new_value;
    if (!persist_locked()) {
        it->second.is_modified = previous_modified;
        it->second.active_value = previous_value;
        it->second.prior_value = previous_prior;
        rollback_locked();
        return false;
    }
    if (!write_raw_node(path, new_value)) {
        rollback_locked();
        return false;
    }
    it->second.active_value = new_value;
    it->second.is_modified = new_value != it->second.factory_value;
    if (!persist_locked()) {
        rollback_locked();
        return false;
    }
    return true;
}

bool BaselineStore::rollback(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(path);
    if (it == store_.end() || !it->second.is_modified) return true;
    const std::string actual = read_raw_node(path);
    if (actual.empty()) return false;
    if (!readback_matches(path, it->second.active_value, actual) &&
        !readback_matches(path, it->second.prior_value, actual) &&
        !readback_matches(path, it->second.factory_value, actual)) {
        it->second.factory_value = actual;
    } else if (!readback_matches(path, it->second.factory_value, actual) &&
               !write_raw_node(path, it->second.factory_value)) {
        return false;
    }
    it->second.is_modified = false;
    it->second.active_value = it->second.factory_value;
    it->second.prior_value = it->second.factory_value;
    return persist_locked();
}

size_t BaselineStore::rollback_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_locked();
}

bool BaselineStore::has_active_mutations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [_, snap] : store_) {
        if (snap.is_modified) return true;
    }
    return false;
}

std::vector<NodeSnapshot> BaselineStore::get_audit_trail() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NodeSnapshot> trail;
    trail.reserve(store_.size());
    for (const auto& [_, snap] : store_) trail.push_back(snap);
    return trail;
}

ScopedBaselineLock::ScopedBaselineLock(std::vector<std::pair<std::string, std::string>> mutations) {
    for (const auto& [path, value] : mutations) {
        if (BaselineStore::instance().apply_mutation(path, value)) modified_paths_.push_back(path);
    }
}

ScopedBaselineLock::~ScopedBaselineLock() {
    for (const auto& path : modified_paths_) {
        (void)BaselineStore::instance().rollback(path);
    }
}

} // namespace coreflow
