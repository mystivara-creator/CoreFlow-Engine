#pragma once
#include <string>

// Deklarasi fungsi untuk CoreFlow AI
namespace CoreFlowAI {
    void initializeHardwareProfile();
    void evaluateDynamicLoad();
    void setUltraIdleMode();
}

// Deklarasi fungsi untuk Thermal Guardian
namespace ThermalGuardian {
    int getCurrentTemp();
    void applyCoolingMode();
    void triggerNotification();
}

// Deklarasi fungsi untuk deteksi layar
namespace EventListener {
    bool isScreenOn();
}
