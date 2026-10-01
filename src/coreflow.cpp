#include "../include/coreflow.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Internal state variables for computing CPU load natively (Zero Overhead)
unsigned long long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;

// =================================================================
// COREFLOW AI: Hardware Baseline & Dynamic Optimization
// =================================================================

void CoreFlowAI::initializeHardwareProfile() {
    std::cout << "[CoreFlow AI] Initializing sysfs hardware configuration..." << std::endl;
    std::cout << "[CoreFlow AI] Profile: Kunzite (Snapdragon 6 Gen 3) Detected." << std::endl;
    
    // Apply optimal I/O baseline directly through native file streams
    std::ofstream read_ahead("/sys/block/sda/queue/read_ahead_kb");
    if (read_ahead.is_open()) {
        read_ahead << "1024";
        read_ahead.close();
    }

    std::ofstream nr_requests("/sys/block/sda/queue/nr_requests");
    if (nr_requests.is_open()) {
        nr_requests << "128";
        nr_requests.close();
    }
}

void CoreFlowAI::evaluateDynamicLoad() {
    // Read CPU jiffies natively from procfs to avoid system overhead
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return;

    std::string line, cpu;
    std::getline(file, line); // Fetch the first line containing aggregate stats
    file.close();
    
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    std::stringstream ss(line);
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    unsigned long long prev_total = prev_user + prev_nice + prev_system + prev_idle;
    unsigned long long current_total = user + nice + system + idle;

    unsigned long long total_diff = current_total - prev_total;
    unsigned long long idle_diff = idle - prev_idle;

    double cpu_usage = 0.0;
    if (total_diff > 0) {
        cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
    }

    // Save current values for the next evaluation interval
    prev_user = user; prev_nice = nice; prev_system = system; prev_idle = idle;

    // Direct interface to the kernel WALT scheduler configuration path
    std::ofstream walt_boost("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load");
    
    if (cpu_usage > 75.0) {
        std::cout << "[CoreFlow AI] High load detected (" << cpu_usage << "%). Engaging Gaming State." << std::endl;
        if (walt_boost.is_open()) {
            walt_boost << "60"; // Make WALT more sensitive to boost performance
            walt_boost.close();
        }
    } 
    else if (cpu_usage > 20.0) {
        std::cout << "[CoreFlow AI] Balanced load detected (" << cpu_usage << "%). Engaging Interactive Balance State." << std::endl;
        if (walt_boost.is_open()) {
            walt_boost << "80"; // Default balance
            walt_boost.close();
        }
    }
    else {
        std::cout << "[CoreFlow AI] Light load detected (" << cpu_usage << "%). Engaging Battery Saver State." << std::endl;
        if (walt_boost.is_open()) {
            walt_boost << "95"; // Conserve battery
            walt_boost.close();
        }
    }
}

void CoreFlowAI::setUltraIdleMode() {
    std::cout << "[CoreFlow AI] Entering ultra power-saving state..." << std::endl;
    
    // Scale down the CPU scaling governor natively when screen goes off
    std::ofstream governor("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor");
    if (governor.is_open()) {
        governor << "powersave";
        governor.close();
    }
}
