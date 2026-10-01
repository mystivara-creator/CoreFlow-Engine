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
    
    // Terapkan throttling langsung via native file stream (Zero Overhead)
    std::ofstream cpu0("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    if (cpu0.is_open()) {
        cpu0 << "1400000";
        cpu0.close();
    }

    std::ofstream cpu4("/sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq");
    if (cpu4.is_open()) {
        cpu4 << "1800000";
        cpu4.close();
    }
}

void ThermalGuardian::triggerNotification() {
    // Ambil suhu saat ini secara real-time untuk dimasukkan ke notifikasi
    int current_temp = getCurrentTemp();
    
    // Siapkan perintah notification bar via terminal Android root
    std::string text_msg = "Suhu mencapai " + std::to_string(current_temp) + "°C. Performa otomatis dibatasi. Mohon istirahatkan perangkat sejenak demi menjaga kesehatan hardware.";
    std::string cmd = "cmd notification post -S bigtext -t 'CoreFlow Guardian' 'thermal_alert' '" + text_msg + "'";
    
    // Eksekusi notifikasi ke sistem Android
    std::system(cmd.c_str());
    
    // Tulis juga peringatan ke sistem Logcat Android
    std::system("log -t CoreFlowEngine 'WARNING: Device temperature exceeded critical limit!'");
}
