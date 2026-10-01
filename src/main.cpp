#include "../include/coreflow.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <string>

// ==========================================
// CONFIGURATION CONSTANTS
// ==========================================
const int THERMAL_LIMIT_CELSIUS = 43; // Batas suhu untuk throttling (43°C)
const int SWAPPINESS_SLEEP = 120;     // Swappiness agresif saat deep sleep
const int SWAPPINESS_ACTIVE = 60;     // Swappiness normal saat aktif (bawaan Android)

// ==========================================
// HELPER LOKAL: Eksekusi Perintah Kernel Sysctl
// ==========================================
namespace KernelTuner {
    void setSwappiness(int value) {
        std::ofstream file("/proc/sys/vm/swappiness");
        if (file.is_open()) {
            file << value;
            file.close();
        } else {
            std::string cmd = "sysctl -w vm.swappiness=" + std::to_string(value) + " > /dev/null 2>&1";
            std::system(cmd.c_str());
        }
    }

    void applyThermalThrottling(bool throttle) {
        if (throttle) {
            std::system("echo '1400000' > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq 2>/dev/null");
            std::system("echo '1800000' > /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq 2>/dev/null");
        } else {
            std::system("echo 'max' > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq 2>/dev/null");
            std::system("echo 'max' > /sys/devices/system/cpu/policy4/scaling_max_freq 2>/dev/null");
        }
    }
}

// ==========================================
// EVENT LISTENER: Implementasi Sesuai Namespace hpp
// ==========================================
bool EventListener::isScreenOn() {
    std::ifstream file("/sys/class/drm/card0-DSI-1/status"); 
    if (!file.is_open()) {
        file.open("/sys/class/graphics/fb0/blank");
    }

    if (file.is_open()) {
        std::string status;
        file >> status;
        return (status == "connected" || status == "0");
    }

    // Fallback sub-shell hanya berjalan jika sysfs tidak dapat diakses
    FILE* pipe = popen("dumpsys power | grep -q 'Display Power: state=ON'", "r");
    if (!pipe) return true;
    int res = pclose(pipe);
    return (res == 0);
}

// ==========================================
// THERMAL GUARDIAN: Implementasi Sesuai Namespace hpp
// ==========================================
int ThermalGuardian::getCurrentTemp() {
    std::ifstream file("/sys/class/power_supply/battery/temp");
    int temp = 0;
    if (file.is_open()) {
        file >> temp; 
        if (temp > 1000) temp /= 1000;
        else if (temp > 100) temp /= 10;
    }
    return temp;
}

void ThermalGuardian::applyCoolingMode() {
    KernelTuner::applyThermalThrottling(true);
}

void ThermalGuardian::triggerNotification() {
    std::system("log -t CoreFlowEngine 'WARNING: Device temperature exceeded critical limit!'");
    std::cout << "[Thermal Guardian] Peringatan suhu kritis dikirim ke Logcat." << std::endl;
}

// ==========================================
// MAIN ENTRY POINT: Native Daemon Orchestrator
// ==========================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " CoreFlow Engine - Native Daemon v2.3" << std::endl;
    std::cout << " Architecture: Hybrid Universal & Kunzite" << std::endl;
    std::cout << " Features: Auto-Throttling & Dynamic Swap" << std::endl;
    std::cout << "========================================" << std::endl;

    CoreFlowAI::initializeHardwareProfile();

    int screen_off_counter = 0;
    bool is_throttled = false;
    bool was_screen_on = true;

    while (true) {
        bool screenOn = EventListener::isScreenOn();
        int currentTemp = ThermalGuardian::getCurrentTemp();

        // 1. EVALUASI THERMAL GUARDIAN
        if (currentTemp >= THERMAL_LIMIT_CELSIUS && !is_throttled) {
            std::cout << "[Thermal Guardian] Suhu kritis: " << currentTemp << "°C. Mengaktifkan Throttling..." << std::endl;
            ThermalGuardian::applyCoolingMode();
            ThermalGuardian::triggerNotification();
            is_throttled = true;
        } else if (currentTemp < (THERMAL_LIMIT_CELSIUS - 3) && is_throttled) { 
            std::cout << "[Thermal Guardian] Suhu normal: " << currentTemp << "°C. Memulihkan performa..." << std::endl;
            KernelTuner::applyThermalThrottling(false);
            is_throttled = false;
        }

        // 2. STATE MACHINE STRATEGI DAYA & SWAPPINESS
        if (screenOn) {
            if (!was_screen_on) {
                std::cout << "[CoreFlow] Layar menyala. Mengembalikan Swappiness ke " << SWAPPINESS_ACTIVE << "%" << std::endl;
                KernelTuner::setSwappiness(SWAPPINESS_ACTIVE);
                screen_off_counter = 0;
                was_screen_on = true;
            }

            CoreFlowAI::evaluateDynamicLoad();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        } 
        else {
            if (was_screen_on) {
                was_screen_on = false;
            }

            if (screen_off_counter == 30) { 
                std::cout << "[CoreFlow] Memasuki Ultra Deep Sleep. Menaikkan Swappiness ke " << SWAPPINESS_SLEEP << "%" << std::endl;
                CoreFlowAI::setUltraIdleMode();
                KernelTuner::setSwappiness(SWAPPINESS_SLEEP);
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (screen_off_counter < 31) screen_off_counter++; 
        }
    }

    return 0;
}
