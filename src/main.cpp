#include "coreflow.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

// Fungsi sederhana mengecek status layar via dumpsys
bool EventListener::isScreenOn() {
    int result = std::system("dumpsys window | grep -q 'mScreenOn=true'");
    return (result == 0);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " CoreFlow Engine Daemon - Started" << std::endl;
    std::cout << "========================================" << std::endl;

    // Fase 1: Fast-Boot Kalibrasi
    CoreFlowAI::initializeHardwareProfile();

    // Fase 2 & 3: Dynamic State Machine
    while (true) {
        if (EventListener::isScreenOn()) {
            // LAYAR MENYALA (Screen ON) - Cek suhu dan performa
            int currentTemp = ThermalGuardian::getCurrentTemp();
            
            // Batas 40.0 C (Kernel membaca sebagai 400)
            if (currentTemp >= 400) {
                ThermalGuardian::applyCoolingMode();
                ThermalGuardian::triggerNotification();
                
                // Jeda 2 menit untuk pendinginan sebelum mengecek lagi
                std::this_thread::sleep_for(std::chrono::seconds(120));
            } else {
                CoreFlowAI::evaluateDynamicLoad();
                
                // Jeda 30 detik saat digunakan, sangat ringan untuk CPU
                std::this_thread::sleep_for(std::chrono::seconds(30));
            }
        } else {
            // LAYAR MATI (Screen OFF) - Ultra Deep Sleep
            CoreFlowAI::setUltraIdleMode();
            
            // Tidur panjang (5 menit) saat layar mati agar tidak mengganggu Doze/Deep Sleep
            std::this_thread::sleep_for(std::chrono::seconds(300));
        }
    }

    return 0;
}
