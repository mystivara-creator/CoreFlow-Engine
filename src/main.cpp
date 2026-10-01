#include "../include/coreflow.hpp"
#include <android/log.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/system_properties.h>
#include <thread>

namespace {
    constexpr const char* kLogTag = "CoreFlowEngine";
    constexpr int kThermalLimitCelsius = 43;
    constexpr int kSwappinessSleep = 120;
    constexpr int kSwappinessActive = 60;
    constexpr int kScreenOffTriggerCount = 30; // 30 * 10s = 300s deep-sleep delay
    constexpr int kHeartbeatSeconds = 60;

    void logToLogcat(const std::string& text) {
        __android_log_print(ANDROID_LOG_DEBUG, kLogTag, "%s", text.c_str());
    }

    void restoreDefaultFrequencies() {
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

        std::string msg = "[CoreFlow KernelTuner] Frequencies restored to hardware maximums.";
        std::cout << msg << std::endl;
        logToLogcat(msg);
    }

    void setSwappiness(int value) {
        std::ofstream file("/proc/sys/vm/swappiness");
        if (file.is_open()) {
            file << value;
            file.close();
            return;
        }
        std::string cmd = "sysctl -w vm.swappiness=" + std::to_string(value) + " >/dev/null 2>&1";
        std::system(cmd.c_str());
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " CoreFlow Engine - Native Daemon v1.0.0" << std::endl;
    std::cout << " Architecture: Hybrid Universal & Kunzite" << std::endl;
    std::cout << " Features: Auto-Throttling & Dynamic Swap" << std::endl;
    std::cout << "========================================" << std::endl;

    logToLogcat("Daemon started");

    try {
        CoreFlowAI::initializeHardwareProfile();
    } catch (const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Init failed: %s", e.what());
        return 1;
    }

    int screen_off_counter = 0;
    bool is_throttled = false;
    bool was_screen_on = true;
    auto last_heartbeat = std::chrono::steady_clock::now();

    while (true) {
        try {
            ThermalGuardian::applyChargingThermalProtection();

            bool screenOn = EventListener::isScreenOn();
            int currentTemp = ThermalGuardian::getCurrentTemp();

            if (currentTemp >= kThermalLimitCelsius && !is_throttled) {
                std::string msg = "[Thermal Guardian] Suhu kritis: " + std::to_string(currentTemp) +
                                  "C. Mengaktifkan Throttling...";
                std::cout << msg << std::endl;
                logToLogcat(msg);
                ThermalGuardian::applyCoolingMode();
                ThermalGuardian::triggerNotification();
                is_throttled = true;
            } else if (currentTemp < (kThermalLimitCelsius - 3) && is_throttled) {
                std::string msg = "[Thermal Guardian] Suhu normal: " + std::to_string(currentTemp) +
                                  "C. Memulihkan performa...";
                std::cout << msg << std::endl;
                logToLogcat(msg);
                restoreDefaultFrequencies();
                is_throttled = false;
            }

            if (screenOn) {
                if (!was_screen_on) {
                    std::string msg = "[CoreFlow] Layar menyala. Mengembalikan Swappiness ke " +
                                      std::to_string(kSwappinessActive) + "%";
                    std::cout << msg << std::endl;
                    logToLogcat(msg);
                    setSwappiness(kSwappinessActive);

                    if (is_throttled || currentTemp < kThermalLimitCelsius) {
                        restoreDefaultFrequencies();
                    }

                    screen_off_counter = 0;
                    was_screen_on = true;
                }

                CoreFlowAI::AppClass currentApp = CoreFlowAI::detectForegroundApp();
                CoreFlowAI::evaluateDynamicLoad(currentApp);

                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat).count();
                if (elapsed >= kHeartbeatSeconds) {
                    std::string beat = "[CoreFlow] Heartbeat OK. Temp=" + std::to_string(currentTemp) +
                                       "C Throttled=" + (is_throttled ? "yes" : "no");
                    std::cout << beat << std::endl;
                    logToLogcat(beat);
                    last_heartbeat = now;
                }

                std::this_thread::sleep_for(std::chrono::seconds(5));
            } else {
                if (was_screen_on) {
                    std::string msg = "[CoreFlow] Layar mati.";
                    std::cout << msg << std::endl;
                    logToLogcat(msg);
                    was_screen_on = false;
                }

                if (screen_off_counter == kScreenOffTriggerCount) {
                    std::string msg = "[CoreFlow] Memasuki Ultra Deep Sleep. Menaikkan Swappiness ke " +
                                      std::to_string(kSwappinessSleep) + "%";
                    std::cout << msg << std::endl;
                    logToLogcat(msg);
                    CoreFlowAI::setUltraIdleMode();
                    setSwappiness(kSwappinessSleep);
                }

                std::this_thread::sleep_for(std::chrono::seconds(10));
                if (screen_off_counter < INT_MAX) {
                    ++screen_off_counter;
                }
            }
        } catch (const std::exception& e) {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Runtime exception: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        } catch (...) {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Unknown runtime error");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    return 0;
}
