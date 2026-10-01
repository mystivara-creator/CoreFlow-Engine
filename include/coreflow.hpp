#pragma once
#include <string>

namespace CoreFlowAI {
    enum AppClass { APP_DEFAULT, APP_GAME, APP_LAUNCHER };

    struct GovernorTunables {
        unsigned int hispeed_freq = 0;
        unsigned int hispeed_load = 0;
        unsigned int rtg_boost_freq = 0;
        unsigned int up_rate_limit_us = 0;
        unsigned int down_rate_limit_us = 0;
        unsigned int min_freq = 0;
        unsigned int max_freq = 0;
        bool valid = false;
    };

    void initializeHardwareProfile();
    void evaluateDynamicLoad(AppClass foreground_app);
    void setUltraIdleMode();
    AppClass detectForegroundApp();
}

namespace ThermalGuardian {
    int getCurrentTemp();
    void applyCoolingMode();
    void triggerNotification();
    void applyChargingThermalProtection();
}

namespace EventListener {
    bool isScreenOn();
}