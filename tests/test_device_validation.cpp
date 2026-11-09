#include "coreflow/device_validation.hpp"
#include "coreflow/device_profile.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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

    auto profile = std::make_shared<coreflow::DeviceProfileSnapshot>();
    profile->generation = 7;
    profile->captured_monotonic_ns = 123456789;
    profile->fingerprint = "0123456789abcdef";
    profile->kernel_release = "6.1-test";
    profile->device_model = "Synthetic Device";
    profile->soc_model = "Synthetic SoC";
    profile->possible_cpus = {0, 1, 2, 3};
    profile->present_cpus = {0, 1, 2, 3};
    profile->online_cpus = {0, 1, 2, 3};
    profile->cpu_topology_valid = true;
    profile->memory_telemetry_available = true;

    coreflow::CpuPolicyProfile policy;
    policy.policy_path = "/sys/devices/system/cpu/cpufreq/policy0";
    policy.related_cpus = {0, 1, 2, 3};
    policy.available_frequencies_khz = {300000, 600000, 1200000, 1800000};
    policy.cpuinfo_min_freq_khz = 300000;
    policy.cpuinfo_max_freq_khz = 1800000;
    policy.scaling_min_freq_khz = 300000;
    policy.scaling_max_freq_khz = 1800000;
    policy.scaling_cur_freq_khz = 1200000;
    policy.governor = "schedutil";
    policy.related_cpus_valid = true;
    policy.frequency_table_valid = true;
    profile->cpu_policies.push_back(policy);

    coreflow::NodeCapability capability;
    capability.path = "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
    capability.domain = coreflow::HardwareDomain::Cpu;
    capability.source = coreflow::ProfileSource::Sysfs;
    capability.state = coreflow::NodeCapabilityState::MutationEligible;
    capability.semantic_id = "cpufreq.scaling_max_freq";
    capability.unit = "kHz";
    capability.constraints = "verified range and frequency table";
    capability.evidence = "synthetic CDDP fixture";
    profile->node_capabilities.push_back(capability);
    coreflow::DeviceProfileHandle profile_handle = profile;

    coreflow::DeviceValidationRecorder recorder(root.string());
    std::string error;
    assert(!recorder.start("NOT_A_PROFILE", profile_handle, &error));
    assert(!recorder.active());
    assert(!recorder.start("INTERACTIVE_CPU", {}, &error));
    assert(recorder.start("INTERACTIVE_CPU", profile_handle, &error));
    assert(recorder.active());
    assert(!recorder.start("GAMING_LOAD", profile_handle, &error));

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
    sample.ownership_contention_retries = 2;
    sample.recovery_pending = false;
    sample.journal_clear = false;
    sample.active_mutations = true;
    sample.rollback_verified = false;
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
    assert(report.find("\"schema_version\":\"1.1\"") != std::string::npos);
    assert(report.find("\"verdict\":\"INCONCLUSIVE\"") != std::string::npos);
    assert(report.find("\"value\":null") != std::string::npos);
    assert(report.find("\"rollback_verified\":false") != std::string::npos);
    assert(report.find("\"active_mutations\":true") != std::string::npos);
    assert(report.find("\"device_profile\"") != std::string::npos);
    assert(report.find("\"generation\":7") != std::string::npos);
    assert(report.find("0123456789abcdef") != std::string::npos);
    assert(report.find("\"related_cpus\":[0,1,2,3]") != std::string::npos);
    assert(report.find("\"frequency_table_khz\":[300000,600000,1200000,1800000]") != std::string::npos);
    assert(report.find("MUTATION_ELIGIBLE") != std::string::npos);

    std::ifstream events_in(std::filesystem::path(bundle) / "events.jsonl");
    std::string first_event;
    std::getline(events_in, first_event);
    assert(first_event.find("\"event_type\":\"session.started\"") != std::string::npos);
    assert(first_event.find("\"device_profile_generation\":7") != std::string::npos);
    assert(first_event.find("0123456789abcdef") != std::string::npos);
    std::string sample_event;
    while (std::getline(events_in, sample_event)) {
        if (sample_event.find("\"event_type\":\"telemetry.sample\"") != std::string::npos) break;
    }
    assert(sample_event.find("\"ownership_contention_retries\":2") != std::string::npos);
    std::ifstream metrics_in(std::filesystem::path(bundle) / "metrics.csv");
    std::stringstream metrics_buf;
    metrics_buf << metrics_in.rdbuf();
    assert(metrics_buf.str().find("ownership_contention_retries") != std::string::npos);
    assert(metrics_buf.str().find(",2\n") != std::string::npos);

    struct stat st{};
    assert(::stat(bundle.c_str(), &st) == 0);
    assert((st.st_mode & 0777) == 0700);

    std::cout << "CF_DVS_BUNDLE=" << bundle << "\n";
    if (own_root) std::filesystem::remove_all(root);
    std::cout << "CF-DVS CDDP snapshot export, bounded report, and privacy permissions PASS\n";
    return 0;
}
