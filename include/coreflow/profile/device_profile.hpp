#pragma once
#include <string>
#include <vector>

namespace coreflow {

struct ProfileCapability {
    std::string id;
    std::string domain;
    std::string path;
    std::string value_type;
    std::string access;
    std::string semantic_status;
    std::string strategy_id;
    std::string provenance;
};

struct DeviceProfile {
    std::string schema_version{"1"};
    std::string manufacturer;
    std::string model;
    std::string device_codename;
    std::string soc_model;
    std::string android_release;
    std::string android_api;
    std::string kernel_release;
    std::vector<ProfileCapability> capabilities;
    bool complete{false};
    std::string inventory_timestamp;
};

// Reads a deliberately simple, escaped TSV profile. It does not scan or mutate
// the filesystem. Installer/discovery produces the profile; runtime validates it.
class DeviceProfileReader {
public:
    bool load(const std::string& file, DeviceProfile& profile,
              std::string& error) const;
};

} // namespace coreflow
