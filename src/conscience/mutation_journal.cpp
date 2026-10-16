#include "coreflow/mutation_journal.hpp"
#ifdef COREFLOW_HOST_TEST_FIXTURES
#include <atomic>
#endif

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

namespace coreflow {
namespace {

constexpr const char* kHeader = "coreflow-mutation-journal v1";
#ifdef COREFLOW_HOST_TEST_FIXTURES
std::atomic<bool> g_fail_next_directory_sync{false};
#endif

bool fsyncDirectoryOf(const std::string& path) noexcept {
#ifdef COREFLOW_HOST_TEST_FIXTURES
    if (g_fail_next_directory_sync.exchange(false)) return false;
#endif
    const std::size_t slash = path.find_last_of('/');
    const std::string dir = (slash == std::string::npos) ? "." :
        (slash == 0 ? "/" : path.substr(0, slash));
    const int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return false;
    const bool ok = ::fsync(fd) == 0;
    ::close(fd);
    return ok;
}

bool validField(const std::string& value) noexcept {
    if (value.empty()) return false;
    for (const char c : value) {
        if (c == '\t' || c == '\n' || c == '\r') return false;
    }
    return true;
}

bool writeAll(int fd, const std::string& data) noexcept {
    std::size_t written = 0;
    while (written < data.size()) {
        const ssize_t n = ::write(fd, data.data() + written, data.size() - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        written += static_cast<std::size_t>(n);
    }
    return true;
}

} // namespace

#ifdef COREFLOW_HOST_TEST_FIXTURES
void testing::failNextJournalDirectorySync(bool fail) noexcept {
    g_fail_next_directory_sync.store(fail);
}
#endif


bool FileMutationJournal::commit(const Entries& factory_values) noexcept {
    if (factory_values.empty()) return false;

    std::unordered_set<std::string> seen_paths;
    seen_paths.reserve(factory_values.size());

    for (const auto& entry : factory_values) {
        if (!validField(entry.first) || !validField(entry.second)) {
            return false;
        }

        if (!seen_paths.insert(entry.first).second) {
            return false;
        }
    }

    std::string body;
    body.reserve(128 * factory_values.size());
    body += kHeader;
    body += '\n';
    for (const auto& entry : factory_values) {
        body += entry.first;
        body += '\t';
        body += entry.second;
        body += '\n';
    }
    body += "end\n";

    const std::string tmp = path_ + ".tmp";
    const int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0) return false;

    bool ok = writeAll(fd, body) && ::fsync(fd) == 0;
    ok = (::close(fd) == 0) && ok;
    if (!ok) {
        ::unlink(tmp.c_str());
        return false;
    }

    if (::rename(tmp.c_str(), path_.c_str()) != 0) {
        ::unlink(tmp.c_str());
        return false;
    }
    return fsyncDirectoryOf(path_);
}

bool FileMutationJournal::clear() noexcept {
    if (::unlink(path_.c_str()) != 0 && errno != ENOENT) return false;
    return fsyncDirectoryOf(path_);
}

MutationJournal::LoadState FileMutationJournal::load(Entries& out) noexcept {
    out.clear();

    const auto corrupt = [&out]() {
        out.clear();
        return MutationJournal::LoadState::Corrupt;
    };

    struct stat st {};
    if (::stat(path_.c_str(), &st) != 0) {
        return errno == ENOENT
            ? LoadState::Absent
            : LoadState::Corrupt;
    }

    std::ifstream file(path_, std::ios::binary);
    if (!file) return LoadState::Corrupt;

    std::string line;
    if (!std::getline(file, line) || line != kHeader) {
        return corrupt();
    }

    std::unordered_set<std::string> seen_paths;
    bool terminated = false;

    while (std::getline(file, line)) {
        if (line == "end") {
            terminated = true;

            // "end" must be the final content in the file.
            if (file.peek() != std::char_traits<char>::eof()) {
                return corrupt();
            }

            if (file.bad()) return corrupt();
            break;
        }

        const std::size_t tab = line.find('\t');
        if (tab == std::string::npos) {
            return corrupt();
        }

        std::string resource_path = line.substr(0, tab);
        std::string value = line.substr(tab + 1);

        if (!validField(resource_path) || !validField(value)) {
            return corrupt();
        }

        if (!seen_paths.insert(resource_path).second) {
            return corrupt();
        }

        out.emplace_back(
            std::move(resource_path),
            std::move(value));
    }

    if (file.bad() || !terminated || out.empty()) {
        return corrupt();
    }

    return LoadState::Pending;
}

} // namespace coreflow
