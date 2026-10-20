#include "coreflow/retained_state.hpp"

#include "coreflow/types.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

namespace coreflow {
namespace {

constexpr std::string_view kHeader{"coreflow-retained-state v1"};
constexpr std::size_t kMaxEntries = 256;
constexpr std::size_t kMaxFileBytes = 128U * 1024U;

bool validField(const std::string& value) noexcept {
    if (value.empty()) return false;
    for (const char c : value) {
        if (c == '\t' || c == '\n' || c == '\r' || c == '\0') return false;
    }
    return true;
}

std::uint32_t checksum32(std::string_view bytes) noexcept {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const char character : bytes) {
        const auto byte = static_cast<unsigned char>(character);
        crc ^= static_cast<std::uint32_t>(byte);
        for (unsigned bit = 0; bit < 8U; ++bit) {
            const std::uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

bool parseHex32(std::string_view text, std::uint32_t& value) noexcept {
    if (text.size() != 8U) return false;
    std::uint32_t parsed = 0U;
    for (const char c : text) {
        std::uint32_t digit = 0U;
        if (c >= '0' && c <= '9') digit = static_cast<std::uint32_t>(c - '0');
        else if (c >= 'a' && c <= 'f') digit = static_cast<std::uint32_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = static_cast<std::uint32_t>(c - 'A' + 10);
        else return false;
        parsed = (parsed << 4U) | digit;
    }
    value = parsed;
    return true;
}

bool writeAll(int fd, const std::string& data) noexcept {
    std::size_t offset = 0;
    while (offset < data.size()) {
        const ssize_t written = ::write(fd, data.data() + offset, data.size() - offset);
        if (written < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (written == 0) return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool readExact(int fd, std::string& data) noexcept {
    std::size_t offset = 0U;
    while (offset < data.size()) {
        const ssize_t count = ::read(fd, data.data() + offset, data.size() - offset);
        if (count < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (count == 0) return false;
        offset += static_cast<std::size_t>(count);
    }
    char extra = '\0';
    for (;;) {
        const ssize_t count = ::read(fd, &extra, 1U);
        if (count < 0 && errno == EINTR) continue;
        return count == 0;
    }
}

bool fsyncDirectoryOf(const std::string& path) noexcept {
    try {
        const std::size_t slash = path.find_last_of('/');
        const std::string directory = slash == std::string::npos ? "." :
            (slash == 0U ? "/" : path.substr(0U, slash));
        const int fd = ::open(directory.c_str(),
                              O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) return false;
        const bool ok = ::fsync(fd) == 0;
        (void)::close(fd);
        return ok;
    } catch (...) {
        return false;
    }
}

std::string fieldOr(const std::string& value, const char* fallback) {
    return value.empty() ? std::string(fallback) : value;
}

} // namespace

bool RetainedStateStore::upsert(const RetainedStateEntry& entry) noexcept {
    try {
        if (!validField(entry.scope) || !validField(entry.path) ||
            !validField(entry.baseline) || !validField(entry.target) ||
            entry.baseline == entry.target) {
            return false;
        }

        Entries entries;
        const LoadState state = load(entries);
        if (state == LoadState::Corrupt) return false;
        entries.erase(std::remove_if(entries.begin(), entries.end(),
            [&entry](const RetainedStateEntry& current) {
                return current.path == entry.path;
            }), entries.end());
        if (entries.size() >= kMaxEntries) return false;
        entries.push_back(entry);
        return commit(entries);
    } catch (...) {
        return false;
    }
}

bool RetainedStateStore::erasePath(const std::string& path) noexcept {
    try {
        if (!validField(path)) return false;
        Entries entries;
        const LoadState state = load(entries);
        if (state == LoadState::Corrupt) return false;
        const std::size_t old_size = entries.size();
        entries.erase(std::remove_if(entries.begin(), entries.end(),
            [&path](const RetainedStateEntry& current) {
                return current.path == path;
            }), entries.end());
        if (entries.size() == old_size) return true;
        return entries.empty() ? clear() : commit(entries);
    } catch (...) {
        return false;
    }
}

bool FileRetainedStateStore::commit(const Entries& entries) noexcept {
    try {
        if (entries.empty()) return clear();
        if (entries.size() > kMaxEntries) return false;

        std::unordered_set<std::string> seen;
        std::string body;
        body.reserve(96U + entries.size() * 160U);
        body.append(kHeader);
        body.push_back('\n');
        for (const auto& entry : entries) {
            if (!validField(entry.scope) || !validField(entry.path) ||
                !validField(entry.baseline) || !validField(entry.target) ||
                entry.baseline == entry.target || !seen.insert(entry.path).second) {
                return false;
            }
            body += entry.scope;
            body.push_back('\t');
            body += entry.path;
            body.push_back('\t');
            body += entry.baseline;
            body.push_back('\t');
            body += entry.target;
            body.push_back('\n');
            if (body.size() > kMaxFileBytes) return false;
        }

        char checksum_text[9]{};
        const int length = std::snprintf(
            checksum_text, sizeof(checksum_text), "%08x", checksum32(body));
        if (length != 8) return false;
        body += "checksum ";
        body.append(checksum_text, 8U);
        body.push_back('\n');
        body += "end\n";
        if (body.size() > kMaxFileBytes) return false;

        std::string temporary = path_ + ".tmp.XXXXXX";
        const int fd = ::mkstemp(temporary.data());
        if (fd < 0) return false;

        struct stat st {};
        bool ok = ::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0 &&
                  ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
                  ::fchmod(fd, 0600) == 0 && writeAll(fd, body) && ::fsync(fd) == 0;
        if (::close(fd) != 0) ok = false;
        if (!ok) {
            (void)::unlink(temporary.c_str());
            return false;
        }
        if (::rename(temporary.c_str(), path_.c_str()) != 0) {
            (void)::unlink(temporary.c_str());
            return false;
        }
        return fsyncDirectoryOf(path_);
    } catch (...) {
        return false;
    }
}

bool FileRetainedStateStore::clear() noexcept {
    try {
        if (::unlink(path_.c_str()) != 0 && errno != ENOENT) return false;
        return fsyncDirectoryOf(path_);
    } catch (...) {
        return false;
    }
}

RetainedStateStore::LoadState FileRetainedStateStore::load(Entries& out) noexcept {
    out.clear();
    const auto corrupt = [&out]() {
        out.clear();
        return LoadState::Corrupt;
    };
    if (path_.empty() || path_.find('\0') != std::string::npos) return corrupt();

    const int fd = ::open(path_.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return errno == ENOENT ? LoadState::Absent : LoadState::Corrupt;
    }

    struct stat st {};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        static_cast<std::uintmax_t>(st.st_size) > kMaxFileBytes) {
        (void)::close(fd);
        return corrupt();
    }

    std::string contents;
    try {
        contents.resize(static_cast<std::size_t>(st.st_size));
    } catch (...) {
        (void)::close(fd);
        return corrupt();
    }

    const bool read_ok = readExact(fd, contents);
    const bool close_ok = ::close(fd) == 0;
    if (!read_ok || !close_ok || contents.empty()) return corrupt();

    try {
        std::istringstream input(contents);
        std::string line;
        if (!std::getline(input, line) || line != kHeader) return corrupt();

        std::string payload(kHeader);
        payload.push_back('\n');
        Entries loaded;
        loaded.reserve(16U);
        std::unordered_set<std::string> seen;
        bool checksum_seen = false;
        std::uint32_t expected_checksum = 0U;
        bool terminated = false;

        while (std::getline(input, line)) {
            if (line == "end") {
                terminated = true;
                break;
            }
            if (line.rfind("checksum ", 0U) == 0U) {
                if (checksum_seen || !parseHex32(
                        std::string_view(line).substr(9U), expected_checksum)) {
                    return corrupt();
                }
                checksum_seen = true;
                continue;
            }
            if (checksum_seen) return corrupt();

            const std::size_t first = line.find('\t');
            const std::size_t second = first == std::string::npos ? first :
                line.find('\t', first + 1U);
            const std::size_t third = second == std::string::npos ? second :
                line.find('\t', second + 1U);
            if (first == std::string::npos || second == std::string::npos ||
                third == std::string::npos ||
                line.find('\t', third + 1U) != std::string::npos) {
                return corrupt();
            }

            RetainedStateEntry entry;
            entry.scope = line.substr(0U, first);
            entry.path = line.substr(first + 1U, second - first - 1U);
            entry.baseline = line.substr(second + 1U, third - second - 1U);
            entry.target = line.substr(third + 1U);
            if (!validField(entry.scope) || !validField(entry.path) ||
                !validField(entry.baseline) || !validField(entry.target) ||
                entry.baseline == entry.target || !seen.insert(entry.path).second ||
                loaded.size() >= kMaxEntries) {
                return corrupt();
            }
            payload += line;
            payload.push_back('\n');
            loaded.push_back(std::move(entry));
        }

        if (!terminated || !checksum_seen || loaded.empty() ||
            checksum32(payload) != expected_checksum ||
            input.peek() != std::char_traits<char>::eof()) {
            return corrupt();
        }
        out = std::move(loaded);
        return LoadState::Valid;
    } catch (...) {
        out.clear();
        return LoadState::Corrupt;
    }
}

std::string retainedScopeFor(const DeviceProfile& profile) {
    const auto& env = profile.environment;
    const std::string manufacturer = fieldOr(env.manufacturer, "unknown-vendor");
    const std::string device = fieldOr(env.device, "unknown-device");
    const std::string soc = fieldOr(env.soc_model, "unknown-soc");
    const std::string android = !profile.android_release.empty() ? profile.android_release :
        fieldOr(env.release, "unknown-android");
    const std::string kernel = !profile.kernel_release.empty() ? profile.kernel_release :
        fieldOr(env.kernel_release, "unknown-kernel");
    const std::string abi = !profile.abi.empty() ? profile.abi :
        fieldOr(env.abi, "unknown-abi");

    std::string scope;
    scope.reserve(manufacturer.size() + device.size() + soc.size() +
                  android.size() + kernel.size() + abi.size() + 5U);
    scope += manufacturer; scope.push_back('|');
    scope += device; scope.push_back('|');
    scope += soc; scope.push_back('|');
    scope += android; scope.push_back('|');
    scope += kernel; scope.push_back('|');
    scope += abi;
    return scope;
}

} // namespace coreflow
