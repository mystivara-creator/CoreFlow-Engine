#include "coreflow/autonomous.hpp"

#include <android/log.h>
#include <chrono>
#include <cstdarg>
#include <thread>
#include <stdexcept>

namespace coreflow {
namespace {

constexpr const char* kLogTag = "CoreFlowAutonomous";

// Helper untuk mencatat informasi normal
void logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, kLogTag, fmt, args);
    va_end(args);
}

// Helper untuk mencatat peringatan atau kegagalan
void logError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, kLogTag, fmt, args);
    va_end(args);
}

} // namespace

AutonomousEngine::AutonomousEngine() = default;

bool AutonomousEngine::initialize() {
    try {
        snapshot_.profile = discovery_.discover();
        snapshot_.state = RuntimeState::Idle;

        // Verifikasi ketersediaan subsistem utama
        return snapshot_.profile.proc_available ||
               snapshot_.profile.sys_available;
    } catch (const std::exception& e) {
        logError("Failed to initialize AutonomousEngine: %s", e.what());
        return false;
    }
}

void AutonomousEngine::logStartup() const {
    logInfo(
        "CoreFlow Autonomous %s | abi=%s kernel=%s cpu_policies=%zu thermal_zones=%zu",
        CORE_FLOW_VERSION,
        snapshot_.profile.abi.c_str(),
        snapshot_.profile.kernel_release.c_str(),
        snapshot_.profile.cpu_policies.size(),
        snapshot_.profile.thermal_zones.size()
    );
}

void AutonomousEngine::logStateTransition(
    RuntimeState oldState,
    RuntimeState newState
) const {
    logInfo(
        "state: %s -> %s",
        stateName(oldState),
        stateName(newState)
    );
}

void AutonomousEngine::tick() {
    try {
        const RuntimeSample sample = observer_.sample();

        const RuntimeState previous = snapshot_.state;
        const RuntimeState next = policy_.evaluate(sample, previous);
        const Decision decision = policy_.decide(sample, next);

        if (next != previous) {
            snapshot_.state = next;
            logStateTransition(previous, next);
        }

        // Eksekusi mutasi hanya jika keputusan membutuhkan tindakan
        if (decision != Decision::NoAction) {
            logInfo(
                "sample=%llu state=%s decision=%s load=%.2f mem=%lluKB thermal=%ldmC",
                static_cast<unsigned long long>(sample_count_),
                stateName(snapshot_.state),
                decisionName(decision),
                sample.load1,
                static_cast<unsigned long long>(sample.mem_available_kb),
                sample.hottest_thermal_millidegrees
            );

            // [INTEGRASI MUTASI]: Controller memverifikasi writable & baseline sebelum mengubah state
            if (controller_.isReady()) {
                controller_.execute(decision, sample);
            } else {
                logError("Mutation controller is not ready. Skipping execution.");
            }
        }
    } catch (const std::exception& e) {
        // Mencegah daemon crash total jika observer gagal membaca file sementara (misal file sysfs terkunci)
        logError("Exception caught during tick processing: %s", e.what());
    }

    ++sample_count_;
}

int AutonomousEngine::run() {
    if (!initialize()) return 2;
    logStartup();

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        tick(); // Eksekusi observasi & mutasi

        // 1. Ambil interval dasar dari config (misal: 5 detik)
        int sleep_seconds = config_.getMonitorInterval(); 
        if (sleep_seconds <= 0) sleep_seconds = 5;

        // 2. ADAPTIVE POLLING (Deep Sleep Safe)
        // Jika sistem santai/layar mati, perlambat interval 4x lipat (misal jadi 20 detik)
        // agar tidak memicu I/O kernel dan membiarkan Doze Mode bekerja penuh.
        if (snapshot_.state == RuntimeState::Idle) {
            sleep_seconds *= 4; 
        }

        // 3. INTERRUPTIBLE SLEEP
        // Tidur dalam pecahan 100 milidetik menggunakan jam CLOCK_MONOTONIC.
        // Aman dari Wakelock dan daemon bisa dihentikan instan tanpa menunggu detik habis.
        for (int i = 0; i < sleep_seconds * 10; ++i) {
            if (stop_requested_.load(std::memory_order_relaxed)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    return 0;
}

void AutonomousEngine::requestStop() noexcept {
    logInfo("Stop requested via signal. Initiating shutdown...");
    stop_requested_.store(true, std::memory_order_relaxed);
}

} // namespace coreflow
