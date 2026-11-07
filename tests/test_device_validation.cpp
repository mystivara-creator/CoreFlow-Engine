#include "coreflow/device_validation.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    const bool own_root = argc < 2;
    std::filesystem::path root;
    if (own_root) {
        std::string pattern = (std::filesystem::temp_directory_path() / "cf-dvs-test-XXXXXX").string();
        char* created = ::mkdtemp(pattern.data());
        assert(created != nullptr);
        root = created;
    } else {
        root = argv[1];
        std::filesystem::create_directories(root);
    }

    coreflow::DeviceValidationRecorder recorder(root.string());
    std::string error;
    assert(!recorder.start("NOT_A_PROFILE", &error));
    assert(!recorder.active());
    assert(recorder.start("INTERACTIVE_CPU", &error));
    assert(recorder.active());
    assert(!recorder.start("GAMING_LOAD", &error));

    coreflow::DeviceValidationSample sample;
    sample.cpu_psi = 12.5;
    sample.memory_psi = 0.0;
    sample.io_psi = std::nullopt;
    sample.thermal_pressure = 61.0;
    sample.temperature_celsius = 47.25;
    sample.thermal_headroom = std::nullopt;
    sample.ai_prediction_10s = 72.0;
    sample.ai_confidence = 0.82;
    sample.thermal_source = "android+sysfs-oem-trips";
    sample.workload = "interactive";
    sample.tuner_stage = "RETAINED";
    sample.tuner_verdict = "IMPROVED";
    sample.candidate = "cpufreq.schedutil_interactive_response";
    sample.capacity_scale = 0.91;
    sample.valid_thermal_sensors = 12;
    sample.recovery_pending = false;
    sample.journal_clear = true;
    sample.rollback_verified = false; // Active transaction is not a recovery failure.
    assert(recorder.record_sample(sample));

    std::string bundle;
    assert(recorder.stop(&bundle, &error));
    assert(!recorder.active());
    assert(std::filesystem::is_directory(bundle));
    assert(std::filesystem::exists(std::filesystem::path(bundle) / "report.json"));
    assert(std::filesystem::exists(std::filesystem::path(bundle) / "events.jsonl"));
    assert(std::filesystem::exists(std::filesystem::path(bundle) / "metrics.csv"));
    assert(std::filesystem::exists(std::filesystem::path(bundle) / "summary.txt"));
    assert(std::filesystem::exists(std::filesystem::path(bundle) / "manifest.json"));

    std::ifstream report_in(std::filesystem::path(bundle) / "report.json");
    std::stringstream report_buf;
    report_buf << report_in.rdbuf();
    const std::string report = report_buf.str();
    assert(report.find("\"verdict\":\"INCONCLUSIVE\"") != std::string::npos);
    assert(report.find("\"value\":null") != std::string::npos);
    assert(report.find("\"rollback_verified\":false") != std::string::npos);

    std::ifstream events_in(std::filesystem::path(bundle) / "events.jsonl");
    std::string first_event;
    std::getline(events_in, first_event);
    assert(first_event.find("\"event_type\":\"session.started\"") != std::string::npos);

    struct stat st{};
    assert(::stat(bundle.c_str(), &st) == 0);
    assert((st.st_mode & 0777) == 0700);

    std::cout << "CF_DVS_BUNDLE=" << bundle << "\n";
    if (own_root) std::filesystem::remove_all(root);
    std::cout << "CF-DVS runtime session capture, bounded report, and privacy permissions PASS\n";
    return 0;
}
