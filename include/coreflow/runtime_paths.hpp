#pragma once

#include <string>


namespace coreflow {

struct RuntimePaths final {
    std::string root{"/data/adb/coreflow"};
    std::string config_ini;
    std::string mutation_journal_cpu;
    std::string mutation_journal_resource;
    std::string retained_cpu;
    std::string retained_resource;
    std::string experience_db;
    std::string kill_switch;
    std::string safe_mode;
    std::string status_json;
    std::string thermal_model;

    static RuntimePaths defaults() {
        RuntimePaths p;
        p.config_ini = p.root + "/config.ini";
        p.mutation_journal_cpu = p.root + "/mutation_journal.txt";
        p.mutation_journal_resource = p.root + "/resource_mutation_journal.txt";
        p.retained_cpu = p.root + "/retained_cpufreq.db";
        p.retained_resource = p.root + "/retained_resource.db";
        p.experience_db = p.root + "/experience.db";
        p.kill_switch = p.root + "/DISABLE";
        p.safe_mode = p.root + "/SAFE_MODE";
        p.status_json = p.root + "/status.json";
        p.thermal_model =
            "/data/adb/modules/coreflow_autonomous/system/etc/coreflow/thermal_predictor.onnx";
        return p;
    }
};

} // namespace coreflow