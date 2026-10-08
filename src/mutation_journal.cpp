#include "coreflow/mutation_journal.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {

constexpr const char* kHeader = "coreflow-mutation-journal v1";

bool fsyncDirectoryOf(const std::string& path) noexcept {
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

bool FileMutationJournal::commit(const Entries& factory_values) noexcept {
    if (factory_values.empty()) return false;

    std::string body;
    body.reserve(128 * factory_values.size());
    body += kHeader;
    body += '\n';
    for (const auto& entry : factory_values) {
        if (!validField(entry.first) || !validField(entry.second)) return false;
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

    struct stat st {};
    if (::stat(path_.c_str(), &st) != 0) {
        return errno == ENOENT ? LoadState::Absent : LoadState::Corrupt;
    }

    std::ifstream file(path_);
    if (!file) return LoadState::Corrupt;

    std::string line;
    if (!std::getline(file, line) || line != kHeader) return LoadState::Corrupt;

    bool terminated = false;
    while (std::getline(file, line)) {
        if (line == "end") {
            terminated = true;
            break;
        }
        const std::size_t tab = line.find('\t');
        if (tab == std::string::npos) return LoadState::Corrupt;
        std::string path = line.substr(0, tab);
        std::string value = line.substr(tab + 1);
        if (!validField(path) || !validField(value)) return LoadState::Corrupt;
        out.emplace_back(std::move(path), std::move(value));
    }

    if (!terminated || out.empty()) {
        out.clear();
        return LoadState::Corrupt;
    }
    return LoadState::Pending;
}

} // namespace coreflow
