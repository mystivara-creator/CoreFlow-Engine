#include "coreflow/runtime_paths.hpp"

namespace coreflow {

RuntimePaths RuntimePaths::defaults() {
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
    p.thermal_model = "/data/adb/modules/coreflow_autonomous/system/etc/coreflow/thermal_predictor.onnx";
    return p;
}

} // namespace coreflow