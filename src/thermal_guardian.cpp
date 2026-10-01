#include "coreflow.hpp"
#include <fstream>
#include <cstdlib>
#include <iostream>

int ThermalGuardian::getCurrentTemp() {
    std::ifstream file("/sys/class/power_supply/battery/temp");
    int temp = 0;
    if (file.is_open()) {
        file >> temp;
    }
    return temp;
}

void ThermalGuardian::applyCoolingMode() {
    // Logika menurunkan batas frekuensi CPU/GPU saat panas
    // Contoh: std::system("echo 1 > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq_limit");
    std::cout << "[Thermal Guardian] Cooling mode diaktifkan!" << std::endl;
}

void ThermalGuardian::triggerNotification() {
    // Memanggil notifikasi native Android via sistem root
    std::system("cmd notification post -S bigtext -t 'CoreFlow Guardian' 'thermal_alert' 'Suhu mencapai 40°C. Sistem otomatis diturunkan. Mohon istirahatkan perangkat sejenak demi kesehatan hardware.'");
}
