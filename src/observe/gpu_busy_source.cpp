#include "coreflow/gpu_busy_source.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>

namespace coreflow::observe {

void GpuBusySource::discoverDefaultRoots() noexcept {
    roots_.clear();
    roots_.emplace_back("/sys/class/kgsl/kgsl-3d0/devfreq");
    if (DIR* dir = ::opendir("/sys/class/devfreq")) {
        while (dirent* entry = ::readdir(dir)) {
            const std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            if (name.find("gpu") != std::string::npos ||
                name.find("kgsl") != std::string::npos ||
                name.find("mali") != std::string::npos) {
                roots_.emplace_back("/sys/class/devfreq/" + name);
            }
        }
        ::closedir(dir);
    }
}

bool GpuBusySource::poll(double& ratio_out) noexcept {
    for (const auto& root : roots_) {
        const std::string path = root + "/load";
        const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) continue;
        std::array<char, 64> buffer{};
        const ssize_t n = ::read(fd, buffer.data(), buffer.size() - 1U);
        ::close(fd);
        if (n <= 0) continue;
        const double percent = parseDevfreqLoadPercent(
            std::string_view(buffer.data(), static_cast<std::size_t>(n)));
        if (percent < 0.0) continue;
        ratio_out = percent / 100.0;
        active_root_ = root;
        return true;
    }
    return false;
}

} // namespace coreflow::observe
