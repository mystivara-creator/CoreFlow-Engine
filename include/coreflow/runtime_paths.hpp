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

    static RuntimePaths defaults();
};

} // namespace coreflow