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

        void restoreDefaultFrequencies() {
        // Driver Qualcomm cpufreq-hw membaca nilai "max" atau frekuensi absolut dari table kernel
        // Membaca batas maksimum asli langsung dari cpuinfo_max_freq untuk menghindari penolakan kernel
        std::ifstream max_freq_0("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq");
        std::string freq_val_0;
        if (max_freq_0 >> freq_val_0) {
            std::ofstream cpu0("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
            if (cpu0.is_open()) cpu0 << freq_val_0;
        }
        max_freq_0.close();

        std::ifstream max_freq_4("/sys/devices/system/cpu/cpufreq/policy4/cpuinfo_max_freq");
        std::string freq_val_4;
        if (max_freq_4 >> freq_val_4) {
            std::ofstream cpu4("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq");
            if (cpu4.is_open()) cpu4 << freq_val_4;
        }
        max_freq_4.close();
        
        std::cout << "[CoreFlow KernelTuner] Performa frekuensi CPU dipulihkan ke batas maksimum hardware." << std::endl;
    }
}


// ==========================================
// MAIN ENTRY POINT: Native Daemon Orchestrator
// ==========================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " CoreFlow Engine - Native Daemon v1.0.0" << std::endl;
    std::cout << " Architecture: Hybrid Universal & Kunzite" << std::endl;
    std::cout << " Features: Auto-Throttling & Dynamic Swap" << std::endl;
    std::cout << "========================================" << std::endl;

    // Inisialisasi awal hardware profil Kunzite
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
            KernelTuner::restoreDefaultFrequencies();
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
