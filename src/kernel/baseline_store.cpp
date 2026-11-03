/**
 * CoreFlow v2 verified write-ahead journal and OEM baseline recovery.
 * The journal is intentionally tiny: only nodes currently modified are saved.
 * Never call these methods from a POSIX fatal signal handler.
 */
#include "coreflow/baseline_store.hpp"
#include "coreflow/unique_fd.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <utility>
#include <cmath>

namespace coreflow {
namespace {
std::string read_line(const std::string& path) {
    std::ifstream stream(path);
    std::string value;
    if (stream) std::getline(stream, value);
    return value;
}
bool path_safe(const std::string& path) {
    if (path.find_first_of("\r\n\t") != std::string::npos || path.find("..") != std::string::npos) return false;
    if (path.rfind("/sys/", 0) == 0 || path.rfind("/proc/sys/", 0) == 0) return true;

    // Android can mount the CPU controller under /dev (for example /dev/cpuctl).
    // Authorize only the semantic top-app UClamp leaf discovered by the engine,
    // not arbitrary /dev nodes.
    if (path.rfind("/dev/", 0) == 0) {
        const std::filesystem::path p(path);
        const auto leaf = p.filename().string();
        const auto parent = p.parent_path().filename().string();
        if (parent == "top-app" && (leaf == "cpu.uclamp.min" || leaf == "cpu.uclamp.max"))
            return true;
    }

    // Unit tests can use private mock files without granting arbitrary runtime writes.
    return std::getenv("COREFLOW_TEST_STATE_DIR") != nullptr && path.rfind("/tmp/", 0) == 0;
}
std::string hex_encode(std::string_view input) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char ch : input) { out.push_back(digits[ch >> 4]); out.push_back(digits[ch & 15]); }
    return out;
}
int nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
bool hex_decode(std::string_view input, std::string& out) {
    if (input.size() % 2 || input.size() > 8192) return false;
    out.clear();
    for (size_t i = 0; i < input.size(); i += 2) {
        int high = nibble(input[i]), low = nibble(input[i + 1]);
        if (high < 0 || low < 0) return false;
        out.push_back(static_cast<char>((high << 4) | low));
    }
    return !out.empty();
}
bool readback_matches(const std::string& path, const std::string& expected, const std::string& actual) {
    if (expected == actual) return true;
    if (path.find("/cpu.uclamp.") == std::string::npos) return false;
    const auto numeric = [](const std::string& value, double& percent) {
        if (value == "max") { percent = 100.0; return true; }
        char* end = nullptr;
        percent = std::strtod(value.c_str(), &end);
        return end != value.c_str() && *end == '\0' &&
               std::isfinite(percent) && percent >= 0.0 && percent <= 100.0;
    };
    double before = 0, after = 0;
    // cgroup v2 formats numeric utilization clamps as percentages (e.g. 31.25).
    return numeric(expected, before) && numeric(actual, after) &&
           std::abs(before - after) <= 0.05;
}
bool write_all(int fd, std::string_view data) {
    while (!data.empty()) {
        ssize_t n = ::write(fd, data.data(), data.size());
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        data.remove_prefix(static_cast<size_t>(n));
    }
    return true;
}
bool sync_directory(const std::string& path) {
    const auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return false;
    UniqueFd fd(::open(path.substr(0, pos).c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    return fd.valid() && ::fsync(fd.get()) == 0;
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
    return read_line(path);
}

bool BaselineStore::write_raw_node(const std::string& path, const std::string& value) {
    if (!path_safe(path) || value.empty() || value.size() > 4096) return false;
    struct stat st {};
    if (::stat(path.c_str(), &st) != 0) return false;
    const bool virtual_node = path.rfind("/sys/", 0) == 0 || path.rfind("/proc/", 0) == 0;
    const int flags = O_WRONLY | O_CLOEXEC | O_NOFOLLOW | (!virtual_node && S_ISREG(st.st_mode) ? O_TRUNC : 0);
    UniqueFd fd(::open(path.c_str(), flags));
    if (!fd.valid() || !write_all(fd.get(), value)) return false;
    if (!virtual_node && S_ISREG(st.st_mode) && ::fsync(fd.get()) != 0) return false;
    // Kernel nodes may normalize input; verify the actual read-back.
    return readback_matches(path, value, read_raw_node(path));
}

bool BaselineStore::persist_locked() {
    if (boot_id_.empty()) return false;
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(journal_path_).parent_path(), error);
    if (error) return false;
    std::string payload = "CFJ1\t" + boot_id_ + "\n";
    for (const auto& [path, snap] : store_) {
        if (snap.is_modified) {
            if (!path_safe(path) || snap.factory_value.empty()) return false;
            payload += path + "\t" + hex_encode(snap.factory_value) + "\n";
        }
    }
    const std::string temp = journal_path_ + ".tmp";
    UniqueFd fd(::open(temp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!fd.valid() || !write_all(fd.get(), payload) || ::fsync(fd.get()) != 0) {
        ::unlink(temp.c_str());
        return false;
    }
    fd.reset();
    if (::rename(temp.c_str(), journal_path_.c_str()) != 0) { ::unlink(temp.c_str()); return false; }
    return sync_directory(journal_path_);
}

bool BaselineStore::recover_pending() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (recovery_checked_) return true;
    std::ifstream file(journal_path_);
    if (!file) {
        if (errno != ENOENT && std::filesystem::exists(journal_path_)) return false;
        recovery_checked_ = true;
        return true;
    }
    std::string header;
    if (!std::getline(file, header) || header.rfind("CFJ1\t", 0) != 0 || boot_id_.empty()) return false;
    const std::string journal_boot = header.substr(5);
    if (journal_boot != boot_id_) {
        // Kernel settings reset on reboot; never overwrite the new boot's OEM state.
        file.close();
        if (::unlink(journal_path_.c_str()) != 0) return false;
        recovery_checked_ = true;
        return true;
    }
    std::vector<std::pair<std::string, std::string>> entries;
    std::string line;
    while (std::getline(file, line)) {
        auto sep = line.find('\t');
        std::string value;
        if (sep == std::string::npos || !path_safe(line.substr(0, sep)) ||
            !hex_decode(std::string_view(line).substr(sep + 1), value)) return false;
        entries.emplace_back(line.substr(0, sep), std::move(value));
        if (entries.size() > 256) return false;
    }
    if (!file.eof()) return false;
    for (const auto& [path, baseline] : entries) {
        store_[path] = NodeSnapshot{path, baseline, read_raw_node(path), true};
    }
    rollback_locked();
    for (const auto& [_, snap] : store_) if (snap.is_modified) return false;
    recovery_checked_ = true;
    return true;
}

bool BaselineStore::capture(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!recovery_checked_ || !path_safe(path)) return false;
    if (store_.contains(path)) return true;
    const std::string before = read_raw_node(path);
    if (before.empty()) return false;
    store_.emplace(path, NodeSnapshot{path, before, before, false});
    return true;
}

size_t BaselineStore::rollback_locked() {
    size_t restored = 0;
    for (auto& [path, snap] : store_) {
        if (snap.is_modified && write_raw_node(path, snap.factory_value)) {
            snap.active_value = snap.factory_value;
            snap.is_modified = false;
            ++restored;
        }
    }
    // A failed rollback keeps the original journal records for next process start.
    if (!persist_locked()) std::cerr << "[CoreFlow] Failed to persist rollback journal\n";
    return restored;
}

bool BaselineStore::apply_mutation(const std::string& path, const std::string& new_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!recovery_checked_ || !path_safe(path) || new_value.empty()) return false;
    auto it = store_.find(path);
    if (it == store_.end()) {
        const std::string before = read_raw_node(path);
        if (before.empty()) { rollback_locked(); return false; }
        it = store_.emplace(path, NodeSnapshot{path, before, before, false}).first;
    }
    if (it->second.active_value == new_value && read_raw_node(path) == new_value) return true;
    // Write-ahead: fsync original value before any kernel write. This flag also
    // covers partial writes and crashes between the write and read-back.
    const bool previous_modified = it->second.is_modified;
    it->second.is_modified = true;
    if (!persist_locked()) {
        it->second.is_modified = previous_modified;
        rollback_locked();
        return false;
    }
    if (!write_raw_node(path, new_value)) {
        rollback_locked();
        return false;
    }
    it->second.active_value = new_value;
    it->second.is_modified = new_value != it->second.factory_value;
    if (!persist_locked()) { rollback_locked(); return false; }
    return true;
}

bool BaselineStore::rollback(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(path);
    if (it == store_.end() || !it->second.is_modified) return true;
    if (!write_raw_node(path, it->second.factory_value)) return false;
    it->second.is_modified = false;
    it->second.active_value = it->second.factory_value;
    return persist_locked();
}

size_t BaselineStore::rollback_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_locked();
}

bool BaselineStore::has_active_mutations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [_, snap] : store_) if (snap.is_modified) return true;
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
    for (const auto& [path, value] : mutations)
        if (BaselineStore::instance().apply_mutation(path, value)) modified_paths_.push_back(path);
}
ScopedBaselineLock::~ScopedBaselineLock() {
    for (const auto& path : modified_paths_) BaselineStore::instance().rollback(path);
}
} // namespace coreflow
