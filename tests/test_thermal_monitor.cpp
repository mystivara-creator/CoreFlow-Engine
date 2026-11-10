#include "coreflow/thermal_monitor.hpp"
#include "coreflow/cooling_feedback.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <algorithm>
#include <thread>
#include <unistd.h>

using namespace coreflow;

static void create_zone(const std::string& root, int index, const char* type, const char* temp) {
    const std::string dir = root + "/thermal_zone" + std::to_string(index);
    std::filesystem::create_directories(dir);
    {
        std::ofstream t(dir + "/type");
        t << type << "\n";
    }
    {
        std::ofstream v(dir + "/temp");
        v << temp << "\n";
    }
}

int main() {
    char tmp[] = "/tmp/coreflow-thermal-XXXXXX";
    assert(::mkdtemp(tmp) != nullptr);
    const std::string root(tmp);
    assert(!assessment_from_android_severity(-1).available);
    assert(!assessment_from_android_severity(7).available);
    assert(assessment_from_android_severity(0).pressure == 0);
    assert(assessment_from_android_severity(2).pressure > 70);
    assert(!assessment_from_android_severity(4).emergency);
    assert(assessment_from_android_severity(5).emergency);
    assert(assessment_from_android_severity(6).emergency);

    // Pixel Tensor BIG/MID/LITTLE clusters + non-compute zones that must be ignored.
    create_zone(root, 0, "socd", "95000");
    create_zone(root, 1, "battery", "49000");
    create_zone(root, 2, "LITTLE", "36250");
    create_zone(root, 3, "MID", "39500");
    create_zone(root, 4, "BIG", "42750");

    ThermalMonitor tensor_monitor(root);
    assert(std::fabs(tensor_monitor.hottest_celsius() - 42.75f) < 0.01f);
    const auto prof = tensor_monitor.multi_sensor_profile();
    assert(prof.valid_cpu_count == 3);
    assert(std::fabs(prof.cpu_average - 39.5f) < 0.01f);
    assert(std::fabs(prof.cpu_peak - 42.75f) < 0.01f);
    assert(std::fabs(prof.composite_soc - 40.475f) < 0.05f);

    // Sub-1C millidegree sensor scaling (500 mC == 0.5 C, not 500 C).
    const std::string cold_root = root + "/cold";
    create_zone(cold_root, 0, "cpu-0-0", "500");
    ThermalMonitor cold_monitor(cold_root);
    assert(std::fabs(cold_monitor.hottest_celsius() - 0.5f) < 0.01f);

    // PMIC BCL/IBAT levels are indicator values, not physical temperatures. They must
    // be excluded from both sensor readings and board/AP composite calculations.
    const auto pmic_indicator_root = root + "/pmic_indicators";
    create_zone(pmic_indicator_root, 0, "cpu-cluster0", "45000");
    create_zone(pmic_indicator_root, 1, "ap_therm", "48000");
    create_zone(pmic_indicator_root, 2, "pm7250b-ibat-lvl1", "120000");
    create_zone(pmic_indicator_root, 3, "PM7250B-BCL-LVL1", "125000");
    ThermalMonitor pmic_indicator_monitor(pmic_indicator_root);
    const auto pmic_profile = pmic_indicator_monitor.multi_sensor_profile();
    assert(pmic_profile.total_valid_sensors == 2);
    assert(pmic_profile.valid_cpu_count == 1);
    assert(std::fabs(pmic_profile.board_average - 48.0f) < 0.01f);
    assert(std::fabs(pmic_indicator_monitor.hottest_celsius() - 45.0f) < 0.01f);
    assert(pmic_indicator_monitor.assess().readings.size() == 2);

    // No per-model tuning: hotter CPU junction and cooler battery use their own trips.
    const auto oem = root + "/oem";
    create_zone(oem, 150, "cpuss-0", "78000");
    create_zone(oem, 151, "battery", "40000");
    create_zone(oem, 152, "skin", "41000");
    create_zone(oem, 153, "socd", "70000");
    auto trip = [](const std::string& r, int zone, int index, const char* type, const char* value) {
        const auto base = r + "/thermal_zone" + std::to_string(zone) + "/trip_point_" + std::to_string(index);
        std::ofstream(base + "_type") << type;
        std::ofstream(base + "_temp") << value;
    };
    trip(oem, 150, 0, "passive", "95000"); trip(oem, 150, 1, "critical", "110000");
    trip(oem, 151, 0, "passive", "45000"); trip(oem, 151, 1, "critical", "60000");
    trip(oem, 152, 0, "passive", "43000");
    ThermalMonitor adaptive(oem);
    assert(adaptive.hottest_celsius() == 78);
    assert(adaptive.battery_celsius() == 40);
    assert(adaptive.skin_celsius() == 41);
    const auto decision = adaptive.assess();
    assert(decision.available && !decision.emergency);
    assert(decision.readings.size() == 3);
    const auto fused = fuse_thermal_assessments(assessment_from_android_severity(0), decision);
    assert(fused.pressure == decision.pressure && fused.sensor == decision.sensor);
    assert(fused.readings.size() == 3);
    assert(decision.sensor == "skin"); // 41/43C is nearer its own limit than CPU 78/95C.
    assert(!tensor_monitor.assess().available); // A name/temperature alone is no thermal policy.
    create_zone(oem, 150, "cpuss-0", "111000");
    assert(adaptive.assess().emergency);
    assert(adaptive.assess().readings.size() == 3); // No emergency short-circuit.
    create_zone(oem, 150, "cpuss-0", "99000");
    trip(oem, 150, 2, "hot", "98000");
    const auto hot = adaptive.assess();
    assert(hot.available && !hot.emergency && hot.pressure >= 110);
    CoolingFeedback feedback;
    auto before = decision;
    assert(!feedback.sample(before, 0, true));
    auto after = before;
    after.sensor = "cpuss-0"; // Switching the limiting sensor must not fake cooling.
    after.readings[0].celsius += 1;
    after.readings[1].celsius -= 1;
    assert(feedback.sample(after, 10, true) == "mixed");
    assert(!feedback.sample(after, 20, false));
    assert(!feedback.sample(after, 21, true));
    assert(feedback.sample(after, 31, true) == "steady");
    create_zone(oem, 150, "cpuss-0", "78000");
    create_zone(oem, 151, "battery", "unavailable");
    const auto incomplete_sysfs = adaptive.assess();
    assert(!incomplete_sysfs.available); // Sysfs alone without platform status is marked incomplete
    assert(incomplete_sysfs.incomplete && incomplete_sysfs.degraded && !incomplete_sysfs.sensor_readings_complete);

    // THERMAL-01: Valid Android platform status MUST NOT be discarded when a sysfs sensor is incomplete
    const auto fused_degraded = fuse_thermal_assessments(assessment_from_android_severity(3), incomplete_sysfs);
    assert(fused_degraded.available);
    assert(fused_degraded.platform_status_valid);
    assert(fused_degraded.degraded);
    assert(!fused_degraded.sensor_readings_complete);
    assert(fused_degraded.pressure >= 95.0);
    assert(fused_degraded.source == "android+sysfs-degraded");

    // THERMAL-01: Android headroom semantics (<0, NaN, 0.5, 1.0, >1.0)
    const auto base_none = assessment_from_android_severity(0);
    const auto hr_neg = apply_android_headroom(base_none, -1.0f);
    assert(!hr_neg.headroom_valid && hr_neg.pressure == 0.0);
    const auto hr_nan = apply_android_headroom(base_none, std::numeric_limits<float>::quiet_NaN());
    assert(!hr_nan.headroom_valid && hr_nan.pressure == 0.0);
    const auto hr_half = apply_android_headroom(base_none, 0.5f);
    assert(hr_half.headroom_valid && std::fabs(hr_half.headroom_value - 0.5f) < 0.001f);
    assert(std::fabs(hr_half.pressure - 42.5) < 0.01);
    const auto hr_one = apply_android_headroom(base_none, 1.0f);
    assert(hr_one.headroom_valid && std::fabs(hr_one.pressure - 95.0) < 0.01 && !hr_one.emergency);
    // Headroom > 1.0 must NOT be linearly extrapolated to emergency (130.0)
    const auto hr_above = apply_android_headroom(base_none, 2.5f);
    assert(hr_above.headroom_valid && std::fabs(hr_above.headroom_value - 2.5f) < 0.001f);
    assert(std::fabs(hr_above.pressure - 95.0) < 0.01 && !hr_above.emergency);

    // THERMAL-03: Secondary hwmon discovery and diagnostic-only unknown category isolation
    const auto hwmon_test_root = root + "/with_hwmon";
    create_zone(hwmon_test_root, 0, "cpu-cluster0", "50000");
    const auto hwmon0 = hwmon_test_root + "/hwmon/hwmon0";
    std::filesystem::create_directories(hwmon0);
    { std::ofstream(hwmon0 + "/name") << "gpu_thermal\n"; }
    { std::ofstream(hwmon0 + "/temp1_label") << "gpuss-0\n"; }
    { std::ofstream(hwmon0 + "/temp1_input") << "60000\n"; }
    { std::ofstream(hwmon0 + "/temp2_label") << "mystery_aux_sensor\n"; }
    { std::ofstream(hwmon0 + "/temp2_input") << "99000\n"; }
    ThermalMonitor hwmon_mon(hwmon_test_root);
    const auto hw_prof = hwmon_mon.multi_sensor_profile();
    assert(hw_prof.valid_cpu_count == 1);
    assert(hw_prof.valid_gpu_count == 1);
    assert(std::fabs(hw_prof.gpu_average - 60.0f) < 0.01f);
    // Unknown diagnostic sensor (99C) must be recorded in readings but NOT pollute compute hottest/composite
    assert(std::fabs(hwmon_mon.hottest_celsius() - 60.0f) < 0.01f);
    assert(hwmon_mon.assess().readings.size() == 3);

    // Per-sensor histories must remain independent: a warming compute die must not be
    // averaged away by a cooling board sensor, and a failed read resets only that sensor.
    const auto trend_root = root + "/sensor_trends";
    create_zone(trend_root, 0, "cpu-cluster0", "40000");
    create_zone(trend_root, 1, "ap_therm", "35000");
    ThermalMonitor per_sensor(trend_root);
    const auto trend_first = per_sensor.assess();
    assert(trend_first.readings.size() == 2);
    for (const auto& reading : trend_first.readings) {
        assert(!reading.velocity_valid);
        assert(reading.velocity_samples_count == 1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1050));
    create_zone(trend_root, 0, "cpu-cluster0", "42000");
    create_zone(trend_root, 1, "ap_therm", "34000");
    const auto trend_second = per_sensor.assess();
    const auto cpu_trend = std::find_if(trend_second.readings.begin(), trend_second.readings.end(),
        [](const ThermalSensorReading& reading) { return reading.type == "cpu-cluster0"; });
    const auto board_trend = std::find_if(trend_second.readings.begin(), trend_second.readings.end(),
        [](const ThermalSensorReading& reading) { return reading.type == "ap_therm"; });
    assert(cpu_trend != trend_second.readings.end() && cpu_trend->velocity_valid);
    assert(board_trend != trend_second.readings.end() && board_trend->velocity_valid);
    assert(cpu_trend->velocity_celsius_per_sec > 1.5);
    assert(board_trend->velocity_celsius_per_sec < -0.5);

    // A transient sensor read failure invalidates that sensor's old trend history.
    create_zone(trend_root, 0, "cpu-cluster0", "unavailable");
    const auto trend_failed = per_sensor.assess();
    const auto failed_cpu = std::find_if(trend_failed.readings.begin(), trend_failed.readings.end(),
        [](const ThermalSensorReading& reading) { return reading.type == "cpu-cluster0"; });
    assert(failed_cpu != trend_failed.readings.end() && !failed_cpu->velocity_valid);
    create_zone(trend_root, 0, "cpu-cluster0", "43000");
    const auto trend_recovered = per_sensor.assess();
    const auto recovered_cpu = std::find_if(trend_recovered.readings.begin(), trend_recovered.readings.end(),
        [](const ThermalSensorReading& reading) { return reading.type == "cpu-cluster0"; });
    assert(recovered_cpu != trend_recovered.readings.end());
    assert(!recovered_cpu->velocity_valid && recovered_cpu->velocity_samples_count == 1);

    const auto invalid = root + "/invalid";
    create_zone(invalid, 0, "cpu", "-40000");
    create_zone(invalid, 1, "skin", "-273000");
    create_zone(invalid, 2, "battery", "garbage");
    ThermalMonitor bad(invalid);
    assert(bad.hottest_celsius() < 0 && !bad.assess().available);
    const auto units = root + "/units";
    create_zone(units, 0, "cpu", "100");
    assert(std::fabs(ThermalMonitor(units).hottest_celsius() - 0.1f) < 0.001f);

    // THERMAL-DECISION-CORRECTNESS: TemperatureTrendTracker & Physical Velocity Verification
    TemperatureTrendTracker tracker(8);
    assert(!tracker.evaluate_velocity().valid);
    assert(tracker.sample_count() == 0);

    // Single sample is insufficient for velocity
    tracker.record_sample(45.0f, 1000000000ULL);
    assert(!tracker.evaluate_velocity().valid);
    assert(tracker.sample_count() == 1);

    // Out of order / duplicate timestamp rejected
    tracker.record_sample(44.0f, 1000000000ULL);
    assert(tracker.sample_count() == 1);

    // Time gap > 10s resets history
    tracker.record_sample(45.0f, 15000000000ULL); // 14s later
    assert(tracker.sample_count() == 1);
    assert(!tracker.evaluate_velocity().valid);

    // Constant temperature -> velocity 0.0 °C/s, valid == true
    tracker.reset();
    tracker.record_sample(45.0f, 1000000000ULL);
    tracker.record_sample(45.0f, 2000000000ULL);
    tracker.record_sample(45.0f, 3000000000ULL);
    const auto const_vel = tracker.evaluate_velocity();
    assert(const_vel.valid);
    assert(const_vel.valid_samples_count == 3);
    assert(std::abs(const_vel.velocity_celsius_per_sec) < 1e-5);
    assert(std::abs(const_vel.sample_duration_sec - 2.0) < 0.01);

    // Negative temperature trend (physical cooling)
    tracker.reset();
    tracker.record_sample(46.0f, 1000000000ULL);
    tracker.record_sample(45.5f, 2000000000ULL);
    tracker.record_sample(45.0f, 3000000000ULL);
    const auto cool_vel = tracker.evaluate_velocity();
    assert(cool_vel.valid);
    assert(cool_vel.valid_samples_count == 3);
    assert(cool_vel.velocity_celsius_per_sec < -0.45 && cool_vel.velocity_celsius_per_sec > -0.55);

    // Positive temperature trend (warming)
    tracker.reset();
    tracker.record_sample(42.0f, 1000000000ULL);
    tracker.record_sample(42.6f, 2000000000ULL);
    tracker.record_sample(43.2f, 3000000000ULL);
    const auto warm_vel = tracker.evaluate_velocity();
    assert(warm_vel.valid);
    assert(warm_vel.velocity_celsius_per_sec > 0.55 && warm_vel.velocity_celsius_per_sec < 0.65);

    // Non-finite and negative temperatures are safely rejected without corrupting history
    tracker.record_sample(-1.0f, 4000000000ULL);
    tracker.record_sample(std::numeric_limits<float>::quiet_NaN(), 5000000000ULL);
    assert(tracker.sample_count() == 3);

    // P2-D sensor freshness: require warm-up, then detect a frozen physical value
    // even when sysfs reads continue to succeed synchronously.
    ThermalSensorFreshnessTracker freshness;
    freshness.record_sample(45.0F, 100ULL, 1'000'000'000ULL);
    assert(!freshness.evaluate(1'000'000'000ULL).valid);
    freshness.record_sample(45.0F, 200ULL, 1'250'000'000ULL); // source mtime advanced
    auto live = freshness.evaluate(1'250'000'000ULL);
    assert(live.valid && !live.stale && live.samples == 2U);

    uint64_t frozen_at = 1'250'000'000ULL;
    for (uint32_t i = 0; i < 8U; ++i) {
        frozen_at += 300'000'000ULL;
        freshness.record_sample(45.0F, 200ULL, frozen_at);
    }
    const auto frozen = freshness.evaluate(frozen_at);
    assert(frozen.stale && !frozen.valid && frozen.samples >= 8U);

    frozen_at += 300'000'000ULL;
    freshness.record_sample(45.1F, 200ULL, frozen_at);
    const auto recovered = freshness.evaluate(frozen_at);
    assert(recovered.valid && !recovered.stale);

    std::filesystem::remove_all(root);
    std::cout << "ThermalMonitor cluster discovery, fusion, headroom, hwmon, and TemperatureTrendTracker PASS\n";
    return 0;
}
