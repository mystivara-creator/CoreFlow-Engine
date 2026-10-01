#include "../include/coreflow.hpp"
#include <android/log.h>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace {
    constexpr const char* kLogTag = "CoreFlowEngine";

    bool parseIntSafe(const std::string& input, int& out, int fallback) {
        if (input.empty()) {
            out = fallback;
            return false;
        }
        for (char c : input) {
            if (!std::isdigit(static_cast<unsigned char>(c)) && !std::isspace(static_cast<unsigned char>(c))) {
                out = fallback;
                return false;
            }
        }
        char* end = nullptr;
        errno = 0;
        long val = std::strtol(input.c_str(), &end, 10);
        if (errno == ERANGE || val > static_cast<long>(INT_MAX) || end == input.c_str()) {
            out = fallback;
            return false;
        }
        out = static_cast<int>(val);
        return true;
    }

    int readTempFromPath(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return INT_MIN;
        }
        std::string raw;
        if (!(file >> raw)) {
            return INT_MIN;
        }
        int temp = 0;
        parseIntSafe(raw, temp, INT_MIN);
        return temp;
    }
}

int ThermalGuardian::getCurrentTemp() {
    int raw = readTempFromPath("/sys/class/power_supply/battery/temp");
    if (raw == INT_MIN) {
        raw = readTempFromPath("/sys/class/thermal/thermal_zone0/temp");
    }

    if (raw == INT_MIN) {
        __android_log_print(ANDROID_LOG_WARN, kLogTag, "Unable to read temperature sensor");
        return 0;
    }

    if (raw > 1000) {
        return raw / 1000;
    }
    if (raw > 100) {
        return raw / 10;
    }
    return raw;
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
    std::string text_msg = "Suhu mencapai " + std::to_string(current_temp) +
                           "C. Performa dibatasi. Istirahatkan perangkat.";

    std::string safe_text;
    for (char c : text_msg) {
        if (c == '\'' || c == '\"' || c == '\\' || c == '`' || c == '$') {
            safe_text.push_back(' ');
        } else {
            safe_text.push_back(c);
        }
    }

    std::string cmd = "cmd notification post -S bigtext -t CoreFlowGuardian thermal_alert '" +
                      safe_text + "'";
    std::system(cmd.c_str());
    __android_log_print(ANDROID_LOG_WARN, kLogTag,
                        "WARNING: Device temperature exceeded critical limit!");
}

namespace {
    bool localIsCharging() {
        std::ifstream file("/sys/class/power_supply/battery/status");
        if (!file.is_open()) {
            return false;
        }
        std::string status;
        std::getline(file, status);
        file.close();
        return (status.find("Charging") != std::string::npos ||
                status.find("Full") != std::string::npos);
    }
}

void ThermalGuardian::applyChargingThermalProtection() {
    static bool lastProtectionState = false;

    bool chargingNow = localIsCharging();
    int tempNow = getCurrentTemp();
    bool needProtection = (chargingNow && tempNow > 40);

    if (needProtection && !lastProtectionState) {
        std::cout << "[Smart Charging Guardian] Suhu baterai kritis: " << tempNow
                  << "C saat charging. Membatasi Core Performa (policy4)." << std::endl;

        std::ofstream maxFreq("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq");
        if (maxFreq.is_open()) {
            maxFreq << "1800000";
            maxFreq.close();
        }
        lastProtectionState = true;
    } else if (!needProtection && lastProtectionState) {
        std::cout << "[Smart Charging Guardian] Kondisi termal kembali normal. Memulihkan performa kluster."
                  << std::endl;

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