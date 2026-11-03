/**
 * @file psi_monitor.cpp
 * @brief PSI Trigger Implementation
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/psi_monitor.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>

namespace coreflow {

PsiMonitor::PsiMonitor(EpollReactor& reactor)
    : reactor_(reactor) {}

bool PsiMonitor::register_trigger(PsiResource res, uint32_t stall_us, uint32_t window_us, PsiCallback callback) {
    std::string path;
    switch (res) {
        case PsiResource::Cpu:    path = "/proc/pressure/cpu"; break;
        case PsiResource::Memory: path = "/proc/pressure/memory"; break;
        case PsiResource::Io:     path = "/proc/pressure/io"; break;
    }

    int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        std::cerr << "[CoreFlow][PSI] Unable to open " << path << " (Kernel PSI unsupported)" << std::endl;
        return false;
    }

    // Format: "some <threshold_us> <window_us>"
    std::string trigger_spec = "some " + std::to_string(stall_us) + " " + std::to_string(window_us);
    if (::write(fd, trigger_spec.c_str(), trigger_spec.size()) < 0) {
        ::close(fd);
        std::cerr << "[CoreFlow][PSI] Failed to register trigger on " << path << std::endl;
        return false;
    }

    fd_map_[fd] = res;

    // Epoll on EPOLLPRI (exceptional condition) as mandated by Linux PSI docs
    const bool registered = reactor_.register_fd(fd, EPOLLPRI, [this, fd, cb = std::move(callback)](uint32_t) {
        PsiResource res = fd_map_[fd];
        double current = get_current_pressure(res);
        cb(res, current);
    });
    if (!registered) {
        fd_map_.erase(fd);
        ::close(fd);
    }
    return registered;
}

double PsiMonitor::get_current_pressure(PsiResource res) const {
    std::string path;
    switch (res) {
        case PsiResource::Cpu:    path = "/proc/pressure/cpu"; break;
        case PsiResource::Memory: path = "/proc/pressure/memory"; break;
        case PsiResource::Io:     path = "/proc/pressure/io"; break;
    }

    std::ifstream f(path);
    if (!f.is_open()) return 0.0;

    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("some", 0) == 0) {
            // parse "avg10=X.XX"
            auto pos = line.find("avg10=");
            if (pos != std::string::npos) {
                return std::stod(line.substr(pos + 6));
            }
        }
    }
    return 0.0;
}

} // namespace coreflow
