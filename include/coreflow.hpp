#pragma once
#include <string>

// Deklarasi fungsi untuk CoreFlow AI (Termasuk perluasan Pilar 6)
namespace CoreFlowAI {
    enum AppClass { APP_DEFAULT, APP_GAME, APP_LAUNCHER };

    void initializeHardwareProfile();
    void evaluateDynamicLoad(AppClass foreground_app); // <-- Menerima status aplikasi
    void setUltraIdleMode();
    AppClass detectForegroundApp();
}

// Deklarasi fungsi untuk Thermal Guardian & Smart Charging Guardian
namespace ThermalGuardian {
    int getCurrentTemp();
    void applyCoolingMode();
    void triggerNotification();
    void applyChargingThermalProtection();
}

// Deklarasi fungsi untuk deteksi status layar
namespace EventListener {
    bool isScreenOn();
}

