#pragma once

namespace coreflow {

class EngineConfig {
public:
    EngineConfig() = default;

    // Fungsi membaca interval dari sistem konfigurasi.
    // Sementara kita set hardcode 5 detik untuk standard safety,
    // fungsi ini siap diekspansi membaca file properti Android.
    int getMonitorInterval() const noexcept {
        return monitor_interval_;
    }

    // Fungsi untuk memperbarui interval saat runtime jika dibutuhkan
    void setMonitorInterval(int seconds) noexcept {
        if (seconds > 0) {
            monitor_interval_ = seconds;
        }
    }

private:
    int monitor_interval_{5};
};

} // namespace coreflow
