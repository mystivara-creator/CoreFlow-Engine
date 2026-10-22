#include "coreflow/profile/device_profile.hpp"
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace coreflow {
namespace {
std::string unescape(const std::string& in) {
    std::string out;
    bool escaped = false;
    for (char c : in) {
        if (escaped) {
            if (c == 't') out.push_back('\t');
            else if (c == 'n') out.push_back('\n');
            else out.push_back(c);
            escaped = false;
        } else if (c == '\\') escaped = true;
        else out.push_back(c);
    }
    if (escaped) out.push_back('\\');
    return out;
}
std::vector<std::string> splitTsv(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool escaped = false;
    for (char c : line) {
        if (escaped) {
            field.push_back('\\'); field.push_back(c); escaped = false;
        } else if (c == '\\') escaped = true;
        else if (c == '\t') { fields.push_back(unescape(field)); field.clear(); }
        else field.push_back(c);
    }
    if (escaped) field.push_back('\\');
    fields.push_back(unescape(field));
    return fields;
}
}
bool DeviceProfileReader::load(const std::string& file, DeviceProfile& out,
                               std::string& error) const {
    std::ifstream in(file);
    if (!in) { error = "PROFILE_OPEN_FAILED"; return false; }
    DeviceProfile parsed;
    std::unordered_set<std::string> ids;
    std::string line;
    std::size_t line_no = 0;
    bool header_seen = false;
    while (std::getline(in, line)) {
        ++line_no;
        if (line.empty() || line[0] == '#') continue;
        auto f = splitTsv(line);
        if (!header_seen) {
            if (f.size() != 2 || f[0] != "schema_version" || f[1] != "1") {
                error = "PROFILE_SCHEMA_UNSUPPORTED at line " + std::to_string(line_no);
                return false;
            }
            parsed.schema_version = f[1];
            header_seen = true;
            continue;
        }
        if (f[0] == "device") {
            if (f.size() != 3) { error = "INVALID_DEVICE_ROW at line " + std::to_string(line_no); return false; }
            if (f[1] == "manufacturer") parsed.manufacturer = f[2];
            else if (f[1] == "model") parsed.model = f[2];
            else if (f[1] == "codename") parsed.device_codename = f[2];
            else if (f[1] == "soc") parsed.soc_model = f[2];
            else if (f[1] == "android_release") parsed.android_release = f[2];
            else if (f[1] == "android_api") parsed.android_api = f[2];
            else if (f[1] == "kernel") parsed.kernel_release = f[2];
            else if (f[1] == "inventory_timestamp") parsed.inventory_timestamp = f[2];
            else if (f[1] == "complete") parsed.complete = (f[2] == "true");
            else { error = "UNKNOWN_DEVICE_FIELD at line " + std::to_string(line_no); return false; }
        } else if (f[0] == "capability") {
            if (f.size() != 9) { error = "INVALID_CAPABILITY_ROW at line " + std::to_string(line_no); return false; }
            ProfileCapability c{f[1],f[2],f[3],f[4],f[5],f[6],f[7],f[8]};
            if (c.id.empty() || c.path.empty() || c.domain.empty()) {
                error = "EMPTY_CAPABILITY_ID_DOMAIN_OR_PATH at line " + std::to_string(line_no); return false;
            }
            if (!ids.insert(c.id).second) {
                error = "DUPLICATE_CAPABILITY_ID at line " + std::to_string(line_no); return false;
            }
            parsed.capabilities.push_back(std::move(c));
        } else {
            error = "UNKNOWN_PROFILE_ROW at line " + std::to_string(line_no);
            return false;
        }
    }
    if (!header_seen) { error = "PROFILE_HEADER_MISSING"; return false; }
    if (parsed.device_codename.empty() || parsed.soc_model.empty()) {
        error = "DEVICE_IDENTITY_INCOMPLETE"; return false;
    }
    out = std::move(parsed);
    error.clear();
    return true;
}
} // namespace coreflow
