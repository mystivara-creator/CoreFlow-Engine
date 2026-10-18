#include "coreflow/mutation_journal.hpp"
#ifdef COREFLOW_HOST_TEST_FIXTURES
#include <atomic>
#endif

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

namespace coreflow {
namespace {

constexpr const char* kHeaderV1 = "coreflow-mutation-journal v1";
constexpr const char* kHeaderV2 = "coreflow-mutation-journal v2";
constexpr std::size_t kMaxEntries = 256U;
constexpr std::size_t kMaxFieldBytes = 4096U;
constexpr std::size_t kMaxFileBytes = 1024U * 1024U;
#ifdef COREFLOW_HOST_TEST_FIXTURES
std::atomic<bool> g_fail_next_directory_sync{false};
#endif

bool fsyncDirectoryOf(const std::string& path) noexcept {
#ifdef COREFLOW_HOST_TEST_FIXTURES
    if (g_fail_next_directory_sync.exchange(false)) return false;
#endif
    try {
        const std::size_t slash = path.find_last_of('/');
        const std::string dir = (slash == std::string::npos) ? "." :
            (slash == 0 ? "/" : path.substr(0, slash));
        const int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (fd < 0) return false;
        const bool ok = ::fsync(fd) == 0;
        (void)::close(fd);
        return ok;
    } catch (...) {
        return false;
    }
}

bool validField(const std::string& value) noexcept {
    if (value.empty() || value.size() > kMaxFieldBytes) return false;
    for (const char c : value) {
        if (c == '\t' || c == '\n' || c == '\r' || c == '\0') return false;
    }
    return true;
}

std::uint32_t checksum32(const std::string& bytes) noexcept {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const char character : bytes) {
        crc ^= static_cast<std::uint32_t>(static_cast<unsigned char>(character));
        for (unsigned bit = 0; bit < 8U; ++bit) {
            const std::uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

bool parseHex32(const std::string& text, std::uint32_t& value) noexcept {
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
    std::size_t written = 0U;
    while (written < data.size()) {
        const ssize_t count = ::write(fd, data.data() + written, data.size() - written);
        if (count < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (count == 0) return false;
        written += static_cast<std::size_t>(count);
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

} // namespace

#ifdef COREFLOW_HOST_TEST_FIXTURES
void testing::failNextJournalDirectorySync(bool fail) noexcept {
    g_fail_next_directory_sync.store(fail);
}
#endif

bool FileMutationJournal::commit(const Entries& factory_values) noexcept {
    if (path_.empty() || path_.find('\0') != std::string::npos ||
        factory_values.empty() || factory_values.size() > kMaxEntries) {
        return false;
    }

    try {
        std::unordered_set<std::string> seen_paths;
        seen_paths.reserve(factory_values.size());

        std::string body{kHeaderV2};
        body.push_back('\n');
        for (const auto& entry : factory_values) {
            if (!validField(entry.first) || !validField(entry.second) ||
                !seen_paths.insert(entry.first).second) {
                return false;
            }
            const std::size_t addition = entry.first.size() + entry.second.size() + 2U;
            if (addition > kMaxFileBytes || body.size() > kMaxFileBytes - addition) {
                return false;
            }
            body += entry.first;
            body.push_back('\t');
            body += entry.second;
            body.push_back('\n');
        }

        char checksum_text[9]{};
        const int length = std::snprintf(
            checksum_text, sizeof(checksum_text), "%08x", checksum32(body));
        if (length != 8) return false;
        body += "checksum ";
        body.append(checksum_text, 8U);
        body += "\nend\n";
        if (body.size() > kMaxFileBytes) return false;

        // Use a unique O_EXCL temporary rather than a predictable "path.tmp".
        // O_NOFOLLOW alone does not protect a fixed temp path from a hard link:
        // opening that link with O_TRUNC could damage an unrelated file.
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

bool FileMutationJournal::clear() noexcept {
    if (path_.empty() || path_.find('\0') != std::string::npos) return false;
    if (::unlink(path_.c_str()) != 0 && errno != ENOENT) return false;
    return fsyncDirectoryOf(path_);
}

MutationJournal::LoadState FileMutationJournal::load(Entries& out) noexcept {
    out.clear();
    const auto corrupt = [&out]() {
        out.clear();
        return MutationJournal::LoadState::Corrupt;
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
        std::istringstream file(contents);
        std::string line;
        if (!std::getline(file, line)) return corrupt();
        const bool version_one = line == kHeaderV1;
        const bool version_two = line == kHeaderV2;
        if (!version_one && !version_two) return corrupt();

        std::unordered_set<std::string> seen_paths;
        Entries loaded;
        loaded.reserve(16U);
        std::string checksum_payload;
        if (version_two) {
            checksum_payload = kHeaderV2;
            checksum_payload.push_back('\n');
        }
        bool checksum_seen = false;
        bool terminated = false;
        std::uint32_t expected_checksum = 0U;

        while (std::getline(file, line)) {
            if (line == "end") {
                terminated = true;
                if (file.peek() != std::char_traits<char>::eof()) return corrupt();
                break;
            }

            if (version_two && line.rfind("checksum ", 0U) == 0U) {
                if (checksum_seen || !parseHex32(line.substr(9U), expected_checksum)) {
                    return corrupt();
                }
                checksum_seen = true;
                continue;
            }
            if (checksum_seen || loaded.size() >= kMaxEntries) return corrupt();

            const std::size_t tab = line.find('\t');
            if (tab == std::string::npos || line.find('\t', tab + 1U) != std::string::npos) {
                return corrupt();
            }
            std::string resource_path = line.substr(0U, tab);
            std::string value = line.substr(tab + 1U);
            if (!validField(resource_path) || !validField(value) ||
                !seen_paths.insert(resource_path).second) {
                return corrupt();
            }
            if (version_two) {
                checksum_payload += line;
                checksum_payload.push_back('\n');
            }
            loaded.emplace_back(std::move(resource_path), std::move(value));
        }

        if (file.bad() || !terminated || loaded.empty() ||
            (version_two && (!checksum_seen || checksum32(checksum_payload) != expected_checksum))) {
            return corrupt();
        }
        out = std::move(loaded);
        return LoadState::Pending;
    } catch (...) {
        return corrupt();
    }
}

} // namespace coreflow
