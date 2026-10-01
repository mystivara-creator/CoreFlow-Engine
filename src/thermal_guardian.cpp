#include "../include/coreflow.hpp"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <string>

// =================================================================
// THERMAL GUARDIAN: Manajemen Suhu & Proteksi Perangkat
// =================================================================

int ThermalGuardian::getCurrentTemp() {
    std::ifstream file("/sys/class/power_supply/battery/temp");
    int temp = 0;
    if (file.is_open()) {
        file >> temp;
        file.close();
        
        // Normalisasi format sensor kernel Android (Membulatkan ke °C)
        if (temp > 1000) temp /= 1000;
        else if (temp > 100) temp /= 10;
    }
    return temp;
}

void ThermalGuardian::applyCoolingMode() {
    std::cout << "[Thermal Guardian] Membatasi clock speed CPU secara native..." << std::endl;
    
    std::ofstream cpu0("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
    if (cpu0.is_open()) {
        cpu0 << "1400000";
        cpu0.close();
    }

    std::ofstream cpu4("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq");
    if (cpu4.is_open()) {
        cpu4 << "1800000";
        cpu4.close();
    }
}

void ThermalGuardian::triggerNotification() {
    int current_temp = getCurrentTemp();
    
    std::string text_msg = "Suhu mencapai " + std::to_string(current_temp) + "°C. Performa otomatis dibatasi. Mohon istirahatkan perangkat sejenak demi menjaga kesehatan hardware.";
    std::string cmd = "cmd notification post -S bigtext -t 'CoreFlow Guardian' 'thermal_alert' '" + text_msg + "'";
    
    std::system(cmd.c_str());
    std::system("log -t CoreFlowEngine 'WARNING: Device temperature exceeded critical limit!'");
}

// =================================================================
// TAMBAHAN BARU: FUNGSI SMART CHARGING THERMAL PROTECTION
// =================================================================

// Fungsi pembantu lokal untuk memeriksa status charging secara native
static bool localIsCharging() {
    std::ifstream file("/sys/class/power_supply/battery/status");
    if (!file.is_open()) return false;
    std::string status;
    std::getline(file, status);
    file.close();
    return (status.find("Charging") != std::string::npos);
}

void ThermalGuardian::applyChargingThermalProtection() {
    static bool lastProtectionState = false; // Caching status penulisan sysfs
    bool chargingNow = localIsCharging();
    int tempNow = ThermalGuardian::getCurrentTemp(); // Resolusi scope fungsi yang benar

    // Menerapkan batas aman proteksi saat charging di atas suhu 40°C
    bool needProtection = (chargingNow && tempNow > 40);

    // KONDISI A: Mengaktifkan Batasan Frekuensi (Hanya tulis sekali saat transisi)
    if (needProtection && !lastProtectionState) {
        std::cout << "[Smart Charging Guardian] Suhu baterai kritis: " << tempNow 
                  << "°C saat charging. Membatasi Core Performa (policy4).\n";
        
        std::ofstream maxFreq("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq");
        if (maxFreq.is_open()) {
            maxFreq << "1800000";
            maxFreq.close();
        }
        lastProtectionState = true;
    } 
    // KONDISI B: Mengembalikan Frekuensi Asli (Hanya tulis sekali saat suhu normal)
    else if (!needProtection && lastProtectionState) {
        std::cout << "[Smart Charging Guardian] Kondisi termal kembali normal. Memulihkan performa kluster.\n";
        
        std::ifstream max_freq_4("/sys/devices/system/cpu/cpufreq/policy4/cpuinfo_max_freq");
        std::string freq_val_4;
        if (max_freq_4 >> freq_val_4) {
            std::ofstream cpu4("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq");
            if (cpu4.is_open()) {
                cpu4 << freq_val_4;
                cpu4.close();
            }
        }
        max_freq_4.close();
        lastProtectionState = false;
    }
}
