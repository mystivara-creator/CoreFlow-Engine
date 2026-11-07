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

UniqueFd open_directory_chain_nofollow(const std::string& path, bool create) {
    if (path.empty() || path.front() != '/') {
        errno = EINVAL;
        return {};
    }
    UniqueFd current(::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (!current.valid()) return {};
    for (const auto& component : std::filesystem::path(path).relative_path()) {
        const std::string name = component.string();
        if (name.empty() || name == ".") continue;
        if (name == ".." || name.find('/') != std::string::npos) {
            errno = EINVAL;
            return {};
        }
        if (create && ::mkdirat(current.get(), name.c_str(), 0700) != 0 && errno != EEXIST) {
            return {};
        }
        UniqueFd next(::openat(
            current.get(), name.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
        if (!next.valid()) return {};
        current = std::move(next);
    }
    return current;
}

std::string read_line_at(int directory_fd, const char* name) {
    if (directory_fd < 0 || name == nullptr || *name == '\0' || std::string_view(name).find('/') != std::string_view::npos) return {};
    UniqueFd fd(::openat(directory_fd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
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

std::string read_line_with_stat(
    const std::string& path,
    uint64_t* out_dev = nullptr,
    uint64_t* out_ino = nullptr) {
    if (path.empty() || path.find('\0') != std::string::npos) return {};
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return {};

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return {};
    if (out_dev) *out_dev = static_cast<uint64_t>(st.st_dev);
    if (out_ino) *out_ino = static_cast<uint64_t>(st.st_ino);

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

bool valid_boot_token(std::string_view token) noexcept {
    if (token.size() < 8 || token.size() > 128) return false;
    if (token == "fallback-boot-id") return false;
    for (unsigned char ch : token) {
        const bool ok = (ch >= 'a' && ch <= 'z') ||
                        (ch >= 'A' && ch <= 'Z') ||
                        (ch >= '0' && ch <= '9') ||
                        ch == '-' || ch == '_' || ch == '.' || ch == ':';
        if (!ok) return false;
    }
    return true;
}

bool parse_u64_strict(std::string_view text, uint64_t& out) noexcept {
    if (text.empty() || text.size() > 20) return false;
    uint64_t accum = 0;
    for (unsigned char ch : text) {
        if (ch < '0' || ch > '9') return false;
        const uint64_t digit = static_cast<uint64_t>(ch - '0');
        if (accum > (UINT64_MAX - digit) / 10ULL) return false;
        accum = accum * 10ULL + digit;
    }
    out = accum;
    return true;
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

std::vector<std::string_view> split_tabs(std::string_view line) {
    std::vector<std::string_view> parts;
    size_t start = 0;
    while (start <= line.size()) {
        const size_t tab = line.find('\t', start);
        if (tab == std::string_view::npos) {
            parts.push_back(line.substr(start));
            break;
        }
        parts.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
    return parts;
}

} // namespace

BaselineStore::BaselineStore() {
    init_paths_and_boot_id_locked();
}

void BaselineStore::init_paths_and_boot_id_locked() {
    const char* test = std::getenv("COREFLOW_TEST_STATE_DIR");
    state_dir_ = (test && *test) ? std::string(test) : "/data/adb/coreflow";
    journal_path_ = state_dir_ + "/baseline.journal";

    const char* test_boot_id_path = std::getenv("COREFLOW_TEST_BOOT_ID_PATH");
    const std::string kernel_boot_path =
        (test_boot_id_path && *test_boot_id_path)
            ? std::string(test_boot_id_path)
            : "/proc/sys/kernel/random/boot_id";

    std::string candidate = read_line_with_stat(kernel_boot_path);
    if (!valid_boot_token(candidate)) {
        // Secondary fallback: per-boot epoch token generated once per boot by post-fs-data.sh
        UniqueFd state_dir_fd = open_directory_chain_nofollow(state_dir_, false);
        if (state_dir_fd.valid()) candidate = read_line_at(state_dir_fd.get(), "boot_epoch");
    }

    if (valid_boot_token(candidate)) {
        boot_id_ = std::move(candidate);
        identity_unavailable_ = false;
    } else {
        boot_id_.clear();
        identity_unavailable_ = true;
    }
}

void BaselineStore::reinitialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    store_.clear();
    recovery_checked_ = false;
    init_paths_and_boot_id_locked();
}

BaselineStore& BaselineStore::instance() {
    static BaselineStore store;
    return store;
}

bool BaselineStore::identity_unavailable() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return identity_unavailable_;
}

std::string BaselineStore::boot_id() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return boot_id_;
}

std::string BaselineStore::read_raw_node(
    const std::string& path,
    uint64_t* out_dev,
    uint64_t* out_ino) const {
    return extract_selector_token(path, read_line_with_stat(path, out_dev, out_ino));
}

bool BaselineStore::write_raw_node(
    const std::string& path,
    const std::string& value,
    uint64_t expected_dev,
    uint64_t expected_ino) {
    if (!path_safe(path) || value.empty() || value.size() > 4096) return false;
    const bool virtual_node = path.rfind("/sys/", 0) == 0 ||
                              path.rfind("/proc/", 0) == 0 ||
                              path.rfind("/dev/", 0) == 0;

    // Open with O_NOFOLLOW | O_NONBLOCK first, then fstat the descriptor to eliminate TOCTOU.
    UniqueFd fd(::open(path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    if (expected_ino != 0 &&
        (static_cast<uint64_t>(st.st_dev) != expected_dev ||
         static_cast<uint64_t>(st.st_ino) != expected_ino)) {
        // Underlying inode or filesystem mount changed; refuse to overwrite replacement object.
        return false;
    }
    if (!virtual_node && ::ftruncate(fd.get(), 0) != 0) return false;
    if (!write_all(fd.get(), value)) return false;
    if (!virtual_node && ::fsync(fd.get()) != 0) return false;
    fd.reset();

    // Kernel nodes may normalize input; verify the actual read-back and object identity.
    uint64_t after_dev = 0;
    uint64_t after_ino = 0;
    const std::string readback = read_raw_node(path, &after_dev, &after_ino);
    if (expected_ino != 0 && (after_dev != expected_dev || after_ino != expected_ino)) {
        return false;
    }
    return readback_matches(path, value, readback);
}

bool BaselineStore::persist_locked() {
    if (identity_unavailable_ || !valid_boot_token(boot_id_)) return false;
    const auto parent = std::filesystem::path(journal_path_).parent_path();
    UniqueFd directory_fd = open_directory_chain_nofollow(parent.string(), true);
    if (!directory_fd.valid() || ::fchmod(directory_fd.get(), 0700) != 0) return false;

    std::string payload = "CFJ4\t" + boot_id_ + "\n";
    for (const auto& [path, snap] : store_) {
        if (snap.is_modified) {
            if (!path_safe(path) || snap.factory_value.empty()) return false;
            if (snap.active_value.empty() || snap.prior_value.empty()) return false;
            payload += path + "\t" + hex_encode(snap.factory_value) +
                       "\t" + hex_encode(snap.prior_value) +
                       "\t" + hex_encode(snap.active_value) +
                       "\t" + std::to_string(snap.dev) +
                       "\t" + std::to_string(snap.ino) + "\n";
        }
    }

    constexpr const char* journal_name = "baseline.journal";
    constexpr const char* temp_name = "baseline.journal.tmp";
    (void)::unlinkat(directory_fd.get(), temp_name, 0);
    UniqueFd fd(::openat(
        directory_fd.get(), temp_name,
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!fd.valid() || !write_all(fd.get(), payload) || ::fsync(fd.get()) != 0) {
        (void)::unlinkat(directory_fd.get(), temp_name, 0);
        return false;
    }
    fd.reset();
    if (::renameat(directory_fd.get(), temp_name, directory_fd.get(), journal_name) != 0) {
        (void)::unlinkat(directory_fd.get(), temp_name, 0);
        return false;
    }
    return ::fsync(directory_fd.get()) == 0;
}

bool BaselineStore::recover_pending() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Re-evaluate boot identity if not yet checked so env/state updates before first recovery take effect
    if (!recovery_checked_) {
        init_paths_and_boot_id_locked();
    }
    if (identity_unavailable_ || !valid_boot_token(boot_id_)) {
        std::cerr << "[CoreFlow] BaselineStore: boot identity unavailable; refusing automatic recovery or mutation.\n";
        return false;
    }
    if (recovery_checked_) return true;

    UniqueFd directory_fd = open_directory_chain_nofollow(state_dir_, false);
    if (!directory_fd.valid()) {
        if (errno == ENOENT) {
            recovery_checked_ = true;
            return true;
        }
        return false;
    }
    UniqueFd fd(::openat(
        directory_fd.get(), "baseline.journal",
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
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
    if (!std::getline(file, header)) return false;
    if (!header.empty() && header.back() == '\r') header.pop_back();

    const bool v1 = header.rfind("CFJ1\t", 0) == 0;
    const bool v2 = header.rfind("CFJ2\t", 0) == 0;
    const bool v3 = header.rfind("CFJ3\t", 0) == 0;
    const bool v4 = header.rfind("CFJ4\t", 0) == 0;
    if (!v1 && !v2 && !v3 && !v4) return false;

    const std::string journal_boot = header.substr(5);
    // Reject legacy constant "fallback-boot-id" or malformed tokens: never assume same-boot
    if (journal_boot == "fallback-boot-id" || !valid_boot_token(journal_boot)) {
        std::cerr << "[CoreFlow] BaselineStore: rejecting ambiguous/legacy journal boot token '"
                  << journal_boot << "'.\n";
        return false;
    }

    if (journal_boot != boot_id_) {
        // Kernel settings reset on reboot; never overwrite the new boot's OEM state.
        if (::unlinkat(directory_fd.get(), "baseline.journal", 0) != 0 && errno != ENOENT) return false;
        recovery_checked_ = true;
        return true;
    }

    std::vector<NodeSnapshot> entries;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        const auto parts = split_tabs(line);
        std::string path_str;
        std::string value, prior, applied;
        uint64_t dev = 0;
        uint64_t ino = 0;

        if (v1) {
            if (parts.size() != 2) return false;
            path_str = std::string(parts[0]);
            if (!path_safe(path_str) || !hex_decode(parts[1], value)) return false;
            applied = read_raw_node(path_str, &dev, &ino);
            prior = applied;
        } else if (v2) {
            if (parts.size() != 3) return false;
            path_str = std::string(parts[0]);
            if (!path_safe(path_str) ||
                !hex_decode(parts[1], value) ||
                !hex_decode(parts[2], applied)) {
                return false;
            }
            prior = applied;
        } else if (v3) {
            if (parts.size() != 4) return false;
            path_str = std::string(parts[0]);
            if (!path_safe(path_str) ||
                !hex_decode(parts[1], value) ||
                !hex_decode(parts[2], prior) ||
                !hex_decode(parts[3], applied)) {
                return false;
            }
        } else {
            // CFJ4: path \t factory_hex \t prior_hex \t active_hex \t dev \t ino
            if (parts.size() != 6) return false;
            path_str = std::string(parts[0]);
            if (!path_safe(path_str) ||
                !hex_decode(parts[1], value) ||
                !hex_decode(parts[2], prior) ||
                !hex_decode(parts[3], applied) ||
                !parse_u64_strict(parts[4], dev) ||
                !parse_u64_strict(parts[5], ino)) {
                return false;
            }
        }

        if (prior.empty() || applied.empty()) return false;
        entries.push_back(NodeSnapshot{
            std::move(path_str),
            std::move(value),
            std::move(applied),
            true,
            std::move(prior),
            dev,
            ino,
        });
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
    if (!recovery_checked_ || identity_unavailable_ || !path_safe(path)) return false;
    auto it = store_.find(path);
    if (it != store_.end()) {
        // Verify existing un-modified snapshot still points to the same underlying object
        uint64_t cur_dev = 0;
        uint64_t cur_ino = 0;
        const std::string current = read_raw_node(path, &cur_dev, &cur_ino);
        if (current.empty()) return false;
        if (!it->second.is_modified &&
            (it->second.ino != 0 && (cur_dev != it->second.dev || cur_ino != it->second.ino))) {
            it->second.factory_value = current;
            it->second.active_value = current;
            it->second.prior_value = current;
            it->second.dev = cur_dev;
            it->second.ino = cur_ino;
        }
        return true;
    }
    uint64_t dev = 0;
    uint64_t ino = 0;
    const std::string before = read_raw_node(path, &dev, &ino);
    if (before.empty()) return false;
    store_.emplace(path, NodeSnapshot{path, before, before, false, before, dev, ino});
    return true;
}

bool BaselineStore::rollback_path_locked(const std::string& path) {
    auto it = store_.find(path);
    if (it == store_.end() || !it->second.is_modified) return true;

    auto& snap = it->second;
    uint64_t cur_dev = 0;
    uint64_t cur_ino = 0;
    const std::string actual = read_raw_node(path, &cur_dev, &cur_ino);
    if (actual.empty()) return false;

    const bool inode_replaced =
        snap.ino != 0 && (cur_dev != snap.dev || cur_ino != snap.ino);
    if (inode_replaced ||
        (!readback_matches(path, snap.active_value, actual) &&
         !readback_matches(path, snap.prior_value, actual) &&
         !readback_matches(path, snap.factory_value, actual))) {
        // The kernel/OEM changed this node. Adopt only this path; do not disturb
        // unrelated transactions owned by other controllers.
        snap.factory_value = actual;
        snap.dev = cur_dev;
        snap.ino = cur_ino;
    } else if (!readback_matches(path, snap.factory_value, actual) &&
               !write_raw_node(path, snap.factory_value, snap.dev, snap.ino)) {
        return false;
    }

    snap.is_modified = false;
    snap.active_value = snap.factory_value;
    snap.prior_value = snap.factory_value;
    return persist_locked();
}

size_t BaselineStore::rollback_locked() {
    size_t restored = 0;
    // A clamp pair can require the other member to be restored first.
    for (int pass = 0; pass < 2; ++pass) {
        for (auto& [path, snap] : store_) {
            if (!snap.is_modified) continue;
            uint64_t cur_dev = 0;
            uint64_t cur_ino = 0;
            const std::string actual = read_raw_node(path, &cur_dev, &cur_ino);
            if (actual.empty()) continue;

            const bool inode_replaced =
                snap.ino != 0 && (cur_dev != snap.dev || cur_ino != snap.ino);

            // If the file/node was replaced (inode/dev changed) or modified by an external writer,
            // adopt the external state rather than overwriting a foreign object or OEM override.
            if (inode_replaced ||
                (!readback_matches(path, snap.active_value, actual) &&
                 !readback_matches(path, snap.prior_value, actual) &&
                 !readback_matches(path, snap.factory_value, actual))) {
                snap.factory_value = actual;
                snap.active_value = actual;
                snap.prior_value = actual;
                snap.dev = cur_dev;
                snap.ino = cur_ino;
                snap.is_modified = false;
                continue;
            }
            if (readback_matches(path, snap.factory_value, actual) ||
                write_raw_node(path, snap.factory_value, snap.dev, snap.ino)) {
                snap.active_value = snap.factory_value;
                snap.prior_value = snap.factory_value;
                snap.dev = cur_dev;
                snap.ino = cur_ino;
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
    if (!recovery_checked_ || identity_unavailable_) return false;
    if (!path_safe(path) || new_value.empty()) return false;
    auto it = store_.find(path);
    if (it == store_.end()) {
        uint64_t dev = 0;
        uint64_t ino = 0;
        const std::string before = read_raw_node(path, &dev, &ino);
        if (before.empty()) return false;
        it = store_.emplace(path, NodeSnapshot{path, before, before, false, before, dev, ino}).first;
    }
    uint64_t cur_dev = 0;
    uint64_t cur_ino = 0;
    const std::string actual = read_raw_node(path, &cur_dev, &cur_ino);
    const bool inode_replaced =
        it->second.ino != 0 && (cur_dev != it->second.dev || cur_ino != it->second.ino);

    if (actual.empty() || inode_replaced ||
        !readback_matches(path, it->second.active_value, actual)) {
        if (!actual.empty()) {
            it->second.factory_value = actual;
            it->second.active_value = actual;
            it->second.prior_value = actual;
            it->second.dev = cur_dev;
            it->second.ino = cur_ino;
            it->second.is_modified = false;
        }
        // Reconcile only the failed path. A write admission/readback failure for
        // one controller must never roll back another controller's active nodes.
        if (!actual.empty()) (void)persist_locked();
        return false;
    }
    if (it->second.active_value == new_value && actual == new_value) return true;
    // Write-ahead: fsync original value and inode identity before any kernel write.
    const bool previous_modified = it->second.is_modified;
    const std::string previous_value = it->second.active_value;
    const std::string previous_prior = it->second.prior_value;
    it->second.is_modified = true;
    it->second.prior_value = previous_value;
    it->second.active_value = new_value;
    it->second.dev = cur_dev;
    it->second.ino = cur_ino;
    if (!persist_locked()) {
        it->second.is_modified = previous_modified;
        it->second.active_value = previous_value;
        it->second.prior_value = previous_prior;
        if (!persist_locked()) {
            std::cerr << "[CoreFlow] BaselineStore: failed to restore journal after rejected mutation on "
                      << path << "\n";
        }
        return false;
    }
    if (!write_raw_node(path, new_value, cur_dev, cur_ino)) {
        if (!rollback_path_locked(path)) {
            std::cerr << "[CoreFlow] BaselineStore: path-scoped rollback failed after write error on "
                      << path << "\n";
        }
        return false;
    }
    it->second.active_value = new_value;
    it->second.is_modified = new_value != it->second.factory_value;
    if (!persist_locked()) {
        if (!rollback_path_locked(path)) {
            std::cerr << "[CoreFlow] BaselineStore: path-scoped rollback failed after journal update error on "
                      << path << "\n";
        }
        return false;
    }
    return true;
}

bool BaselineStore::rollback(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_path_locked(path);
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

bool BaselineStore::verify_and_persist_clear_state() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!recovery_checked_ || identity_unavailable_) return false;
    for (const auto& [_, snap] : store_) {
        if (snap.is_modified) return false;
    }
    // Replacing the journal with a durable empty CFJ4 header removes stale
    // write-ahead records left by an earlier failed persist operation.
    return persist_locked();
}

bool BaselineStore::owns_expected_state(const std::string& path, const std::string& expected_value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(path);
    uint64_t cur_dev = 0;
    uint64_t cur_ino = 0;
    const std::string actual = read_raw_node(path, &cur_dev, &cur_ino);
    if (actual.empty()) return false;
    if (it != store_.end() && it->second.ino != 0) {
        if (cur_dev != it->second.dev || cur_ino != it->second.ino) {
            return false;
        }
    }
    return readback_matches(path, expected_value, actual);
}

std::vector<NodeSnapshot> BaselineStore::get_audit_trail() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NodeSnapshot> trail;
    trail.reserve(store_.size());
    for (const auto& [_, snap] : store_) trail.push_back(snap);
    return trail;
}

} // namespace coreflow
