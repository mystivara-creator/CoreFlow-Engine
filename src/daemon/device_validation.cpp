#include "coreflow/device_validation.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>
#include <unistd.h>
#include <utility>
#include <vector>
#ifdef __linux__
#include <sys/random.h>
#endif
#ifdef __ANDROID__
#include <sys/system_properties.h>
#endif

namespace coreflow {
namespace {

constexpr uint64_t kMaxSessionNs = 30ULL * 60ULL * 1000000000ULL;
constexpr uint64_t kMaxEventBytes = 8ULL * 1024ULL * 1024ULL;
constexpr uint64_t kMaxEvents = 20000ULL;
constexpr uint32_t kFsyncEverySamples = 5;
constexpr size_t kMaxQueuedWrites = 64;
constexpr size_t kMaxQueuedBytes = 256 * 1024;

bool write_all(int fd, std::string_view data) noexcept {
    size_t offset = 0;
    while (offset < data.size()) {
        const ssize_t n = ::write(fd, data.data() + offset, data.size() - offset);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        offset += static_cast<size_t>(n);
    }
    return true;
}

std::string json_escape(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    constexpr char hex[] = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(ch >> 4) & 0x0f];
                    out += hex[ch & 0x0f];
                } else {
                    out += static_cast<char>(ch);
                }
        }
    }
    return out;
}

std::string utc_now() {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
    if (::gmtime_r(&now, &tm) == nullptr) return "1970-01-01T00:00:00Z";
    char buf[32]{};
    if (::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm) == 0) {
        return "1970-01-01T00:00:00Z";
    }
    return buf;
}

uint64_t monotonic_ns() noexcept {
    timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<uint64_t>(ts.tv_nsec);
}

bool random_bytes(uint8_t* out, size_t size) noexcept {
#ifdef __linux__
    size_t offset = 0;
    while (offset < size) {
        const ssize_t n = ::getrandom(out + offset, size - offset, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        offset += static_cast<size_t>(n);
    }
    if (offset == size) return true;
#endif
    const int fd = ::open("/dev/urandom", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    size_t offset2 = 0;
    while (offset2 < size) {
        const ssize_t n = ::read(fd, out + offset2, size - offset2);
        if (n < 0) {
            if (errno == EINTR) continue;
            (void)::close(fd);
            return false;
        }
        if (n == 0) {
            (void)::close(fd);
            return false;
        }
        offset2 += static_cast<size_t>(n);
    }
    (void)::close(fd);
    return true;
}

std::string make_uuid() {
    std::array<uint8_t, 16> bytes{};
    if (!random_bytes(bytes.data(), bytes.size())) return {};
    bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3f) | 0x80);
    constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(36);
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) out += '-';
        out += hex[bytes[i] >> 4];
        out += hex[bytes[i] & 0x0f];
    }
    return out;
}

bool valid_profile(std::string_view profile) noexcept {
    constexpr std::array<std::string_view, 8> profiles{
        "BOOT_STABILITY", "IDLE_EQUILIBRIUM", "INTERACTIVE_CPU", "GAMING_LOAD",
        "MULTITASKING", "THERMAL_RECOVERY", "OWNERSHIP_CONFLICT", "ROLLBACK_RECOVERY"};
    return std::find(profiles.begin(), profiles.end(), profile) != profiles.end();
}

std::string number_or_null(const std::optional<double>& value) {
    if (!value || !std::isfinite(*value)) return "null";
    char buf[48]{};
    const int n = std::snprintf(buf, sizeof(buf), "%.8g", *value);
    return n > 0 ? std::string(buf, static_cast<size_t>(n)) : "null";
}

std::string metric_json(const std::optional<double>& value, std::string_view unit,
                        std::string_view source, std::string_view timestamp,
                        std::string_view missing_reason) {
    const bool valid = value.has_value() && std::isfinite(*value);
    return "{\"value\":" + number_or_null(value) +
           ",\"unit\":" + (unit.empty() ? "null" : "\"" + json_escape(unit) + "\"") +
           ",\"validity\":\"" + (valid ? "valid" : "unavailable") +
           "\",\"source\":\"" + json_escape(source) +
           "\",\"observed_at\":\"" + json_escape(timestamp) + "\"" +
           (valid ? "" : ",\"reason\":\"" + json_escape(missing_reason) + "\"") + "}";
}

// Compact SHA-256 implementation used only for local bundle integrity manifests.
class Sha256 {
public:
    void update(const uint8_t* data, size_t len) noexcept {
        for (size_t i = 0; i < len; ++i) {
            buffer_[buffer_len_++] = data[i];
            if (buffer_len_ == 64) {
                transform();
                bit_len_ += 512;
                buffer_len_ = 0;
            }
        }
    }
    std::string finish() noexcept {
        size_t i = buffer_len_;
        buffer_[i++] = 0x80;
        if (i > 56) {
            while (i < 64) buffer_[i++] = 0;
            transform();
            i = 0;
        }
        while (i < 56) buffer_[i++] = 0;
        bit_len_ += static_cast<uint64_t>(buffer_len_) * 8ULL;
        for (int j = 7; j >= 0; --j) buffer_[i++] = static_cast<uint8_t>(bit_len_ >> (j * 8));
        transform();
        constexpr char hex[] = "0123456789abcdef";
        std::string out;
        out.reserve(64);
        for (uint32_t word : state_) {
            for (int shift = 28; shift >= 0; shift -= 4) out += hex[(word >> shift) & 0x0f];
        }
        return out;
    }
private:
    static constexpr std::array<uint32_t, 64> k_{
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::array<uint32_t, 8> state_{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                                   0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::array<uint8_t, 64> buffer_{};
    size_t buffer_len_{0};
    uint64_t bit_len_{0};
    static uint32_t rotr(uint32_t x, uint32_t n) noexcept { return (x >> n) | (x << (32 - n)); }
    void transform() noexcept {
        std::array<uint32_t, 64> m{};
        for (size_t i = 0; i < 16; ++i) {
            const size_t p = i * 4;
            m[i] = (static_cast<uint32_t>(buffer_[p]) << 24) |
                   (static_cast<uint32_t>(buffer_[p + 1]) << 16) |
                   (static_cast<uint32_t>(buffer_[p + 2]) << 8) |
                   static_cast<uint32_t>(buffer_[p + 3]);
        }
        for (size_t i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(m[i - 15], 7) ^ rotr(m[i - 15], 18) ^ (m[i - 15] >> 3);
            const uint32_t s1 = rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10);
            m[i] = m[i - 16] + s0 + m[i - 7] + s1;
        }
        uint32_t a=state_[0], b=state_[1], c=state_[2], d=state_[3];
        uint32_t e=state_[4], f=state_[5], g=state_[6], h=state_[7];
        for (size_t i = 0; i < 64; ++i) {
            const uint32_t s1=rotr(e,6)^rotr(e,11)^rotr(e,25);
            const uint32_t ch=(e&f)^(~e&g);
            const uint32_t t1=h+s1+ch+k_[i]+m[i];
            const uint32_t s0=rotr(a,2)^rotr(a,13)^rotr(a,22);
            const uint32_t maj=(a&b)^(a&c)^(b&c);
            const uint32_t t2=s0+maj;
            h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        state_[0]+=a; state_[1]+=b; state_[2]+=c; state_[3]+=d;
        state_[4]+=e; state_[5]+=f; state_[6]+=g; state_[7]+=h;
    }
};

bool sha256_file(const std::string& path, uint64_t& size, std::string& digest) noexcept {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        static_cast<uint64_t>(st.st_size) > kMaxEventBytes) {
        (void)::close(fd);
        return false;
    }
    Sha256 hash;
    std::array<uint8_t, 4096> buffer{};
    size = 0;
    for (;;) {
        const ssize_t n = ::read(fd, buffer.data(), buffer.size());
        if (n < 0) {
            if (errno == EINTR) continue;
            (void)::close(fd);
            return false;
        }
        if (n == 0) break;
        hash.update(buffer.data(), static_cast<size_t>(n));
        size += static_cast<uint64_t>(n);
    }
    (void)::close(fd);
    digest = hash.finish();
    return true;
}

std::string read_property(const char* key, const char* fallback) {
#ifdef __ANDROID__
    char value[PROP_VALUE_MAX]{};
    const int n = __system_property_get(key, value);
    if (n > 0) return std::string(value, static_cast<size_t>(n));
#else
    (void)key;
#endif
    return fallback;
}


std::string json_u32_array(const std::vector<uint32_t>& values) {
    std::ostringstream out; out << '[';
    const size_t count = std::min<size_t>(values.size(), 4096U);
    for (size_t i = 0; i < count; ++i) { if (i) out << ','; out << values[i]; }
    out << ']'; return out.str();
}
std::string json_u64_array(const std::vector<uint64_t>& values) {
    std::ostringstream out; out << '[';
    const size_t count = std::min<size_t>(values.size(), 512U);
    for (size_t i = 0; i < count; ++i) { if (i) out << ','; out << values[i]; }
    out << ']'; return out.str();
}
std::string profile_path_token(const std::string& path, const std::string& root, std::string_view label) {
    if (root.empty() || path.size() <= root.size() || path.compare(0, root.size(), root) != 0 ||
        (path[root.size()] != '/' && path[root.size()] != '\0')) return "unmapped";
    return std::string(label) + path.substr(root.size());
}
const char* capability_state_name(NodeCapabilityState state) noexcept {
    switch (state) {
        case NodeCapabilityState::Discovered: return "DISCOVERED";
        case NodeCapabilityState::Readable: return "READABLE";
        case NodeCapabilityState::AccessChecked: return "ACCESS_CHECKED";
        case NodeCapabilityState::SemanticsVerified: return "SEMANTICS_VERIFIED";
        case NodeCapabilityState::ConstraintsVerified: return "CONSTRAINTS_VERIFIED";
        case NodeCapabilityState::MutationEligible: return "MUTATION_ELIGIBLE";
    }
    return "UNKNOWN";
}
const char* rejection_code_name(NodeRejectionReason reason) noexcept {
    switch (reason) {
        case NodeRejectionReason::None: return "NONE";
        case NodeRejectionReason::EaccesSelinux: return "EACCES_SELINUX";
        case NodeRejectionReason::NodeMissing: return "ENOENT";
        case NodeRejectionReason::FormatMismatch: return "FORMAT_MISMATCH";
        case NodeRejectionReason::BaselineCaptureFailed: return "BASELINE_CAPTURE_FAILED";
        case NodeRejectionReason::BaselineNotCaptured: return "BASELINE_NOT_CAPTURED";
        case NodeRejectionReason::ReadbackMismatch: return "READBACK_MISMATCH";
        case NodeRejectionReason::PermissionDenied: return "PERMISSION_DENIED";
        case NodeRejectionReason::SemanticsUnverified: return "SEMANTICS_UNVERIFIED";
        case NodeRejectionReason::ConstraintsUnverified: return "CONSTRAINTS_UNVERIFIED";
        case NodeRejectionReason::AmbiguousTarget: return "AMBIGUOUS_TARGET";
        case NodeRejectionReason::IoError: return "IO_ERROR";
    }
    return "UNKNOWN";
}
const char* profile_source_name(ProfileSource source) noexcept {
    switch (source) {
        case ProfileSource::Sysfs: return "sysfs";
        case ProfileSource::Procfs: return "procfs";
        case ProfileSource::DeviceTree: return "device_tree";
        case ProfileSource::AndroidProperty: return "android_property";
        case ProfileSource::KernelApi: return "kernel_api";
        case ProfileSource::Unknown: return "unknown";
    }
    return "unknown";
}
const char* hardware_domain_name(HardwareDomain domain) noexcept {
    switch (domain) {
        case HardwareDomain::Cpu: return "cpu";
        case HardwareDomain::Gpu: return "gpu";
        case HardwareDomain::Thermal: return "thermal";
        case HardwareDomain::Memory: return "memory";
        case HardwareDomain::Storage: return "storage";
        case HardwareDomain::Cgroup: return "cgroup";
        case HardwareDomain::AndroidRuntime: return "android_runtime";
        case HardwareDomain::Graphics: return "graphics";
        case HardwareDomain::Power: return "power";
        case HardwareDomain::Unknown: return "unknown";
    }
    return "unknown";
}
std::string serialize_device_profile(const DeviceProfileSnapshot& p) {
    std::ostringstream out;
    out << "{\"generation\":" << p.generation
        << ",\"fingerprint\":\"" << json_escape(p.fingerprint) << '"'
        << ",\"captured_monotonic_ns\":" << p.captured_monotonic_ns
        << ",\"kernel_release\":\"" << json_escape(p.kernel_release) << '"'
        << ",\"android_release\":\"" << json_escape(p.android_release) << '"'
        << ",\"android_api_level\":";
    if (p.android_api_level) out << p.android_api_level; else out << "null";
    out << ",\"device_model\":\"" << json_escape(p.device_model) << '"'
        << ",\"soc_model\":\"" << json_escape(p.soc_model) << '"'
        << ",\"hardware_platform\":\"" << json_escape(p.hardware_platform) << '"'
        << ",\"cpu_topology\":{\"valid\":" << (p.cpu_topology_valid ? "true" : "false")
        << ",\"possible\":" << json_u32_array(p.possible_cpus)
        << ",\"present\":" << json_u32_array(p.present_cpus)
        << ",\"online\":" << json_u32_array(p.online_cpus) << "},\"cpu_policies\":[";
    const size_t cpu_count = std::min<size_t>(p.cpu_policies.size(), 64U);
    for (size_t i = 0; i < cpu_count; ++i) {
        const auto& x = p.cpu_policies[i]; if (i) out << ',';
        out << "{\"path\":\"" << json_escape(profile_path_token(x.policy_path, p.sys_root, "/sys")) << '"'
            << ",\"related_cpus\":" << json_u32_array(x.related_cpus)
            << ",\"frequency_table_khz\":" << json_u64_array(x.available_frequencies_khz)
            << ",\"cpuinfo_min_freq_khz\":" << x.cpuinfo_min_freq_khz
            << ",\"cpuinfo_max_freq_khz\":" << x.cpuinfo_max_freq_khz
            << ",\"scaling_min_freq_khz\":" << x.scaling_min_freq_khz
            << ",\"scaling_max_freq_khz\":" << x.scaling_max_freq_khz
            << ",\"scaling_cur_freq_khz\":" << x.scaling_cur_freq_khz
            << ",\"governor\":\"" << json_escape(x.governor) << '"'
            << ",\"related_cpus_valid\":" << (x.related_cpus_valid ? "true" : "false")
            << ",\"frequency_table_valid\":" << (x.frequency_table_valid ? "true" : "false") << '}';
    }
    out << "],\"gpu_domains\":[";
    const size_t gpu_count = std::min<size_t>(p.gpu_domains.size(), 64U);
    for (size_t i = 0; i < gpu_count; ++i) {
        const auto& x = p.gpu_domains[i]; if (i) out << ',';
        out << "{\"path\":\"" << json_escape(profile_path_token(x.device_path, p.sys_root, "/sys")) << '"'
            << ",\"driver\":\"" << json_escape(x.driver) << '"'
            << ",\"governor\":\"" << json_escape(x.governor) << '"'
            << ",\"frequency_table_hz\":" << json_u64_array(x.available_frequencies_hz)
            << ",\"min_freq_hz\":" << x.min_freq_hz
            << ",\"max_freq_hz\":" << x.max_freq_hz
            << ",\"cur_freq_hz\":" << x.cur_freq_hz
            << ",\"frequency_table_valid\":" << (x.frequency_table_valid ? "true" : "false") << '}';
    }
    out << "],\"thermal_sensors\":[";
    const size_t sensor_count = std::min<size_t>(p.thermal_sensors.size(), 256U);
    for (size_t i = 0; i < sensor_count; ++i) {
        const auto& x = p.thermal_sensors[i]; if (i) out << ',';
        out << "{\"path\":\"" << json_escape(profile_path_token(x.sensor_path, p.sys_root, "/sys")) << '"'
            << ",\"type\":\"" << json_escape(x.sensor_type) << "\",\"temperature_millicelsius\":";
        if (x.temperature_valid) out << x.temperature_millicelsius; else out << "null";
        out << ",\"temperature_valid\":" << (x.temperature_valid ? "true" : "false") << '}';
    }
    out << "],\"capabilities\":[";
    const size_t cap_count = std::min<size_t>(p.node_capabilities.size(), 512U);
    for (size_t i = 0; i < cap_count; ++i) {
        const auto& x = p.node_capabilities[i]; if (i) out << ',';
        std::string path = profile_path_token(x.path, p.sys_root, "/sys");
        if (path == "unmapped") path = profile_path_token(x.path, p.proc_root, "/proc");
        out << "{\"path\":\"" << json_escape(path) << '"'
            << ",\"domain\":\"" << hardware_domain_name(x.domain) << '"'
            << ",\"source\":\"" << profile_source_name(x.source) << '"'
            << ",\"state\":\"" << capability_state_name(x.state) << '"'
            << ",\"classification\":\"" << kernel_node_classification_name(x.classification) << '"'
            << ",\"semantic_id\":\"" << json_escape(x.semantic_id) << '"'
            << ",\"unit\":\"" << json_escape(x.unit) << '"'
            << ",\"current_value\":\"" << json_escape(x.current_value) << '"'
            << ",\"accepted_values\":\"" << json_escape(x.accepted_values) << '"'
            << ",\"constraints\":\"" << json_escape(x.constraints) << '"'
            << ",\"evidence\":\"" << json_escape(x.evidence) << '"'
            << ",\"rejection_reason\":\"" << json_escape(x.rejection_reason) << "\""
            << ",\"rejection_code\":\"" << rejection_code_name(x.rejection_code) << "\"}";
    }
    out << "],\"memory_telemetry_available\":" << (p.memory_telemetry_available ? "true" : "false")
        << ",\"cgroup_v2_available\":" << (p.cgroup_v2_available ? "true" : "false")
        << ",\"uclamp_interface_available\":" << (p.uclamp_interface_available ? "true" : "false") << '}';
    return out.str();
}

std::string build_commit() {
#ifdef COREFLOW_BUILD_COMMIT
    return COREFLOW_BUILD_COMMIT;
#else
    return "unknown";
#endif
}

} // namespace

DeviceValidationRecorder::DeviceValidationRecorder(std::string root) noexcept
    : root_(std::move(root)) {}

DeviceValidationRecorder::~DeviceValidationRecorder() noexcept {
    if (active_) {
        std::string ignored;
        (void)stop(nullptr, &ignored);
    }
}

bool DeviceValidationRecorder::start(std::string_view profile, DeviceProfileHandle device_profile,
                                     std::string* error) noexcept {
    if (active_) {
        if (error) *error = "a CF-DVS session is already active";
        return false;
    }
    if (!valid_profile(profile)) {
        if (error) *error = "unsupported CF-DVS profile";
        return false;
    }
    if (!device_profile || device_profile->generation == 0 || device_profile->fingerprint.empty()) {
        if (error) *error = "a published CDDP DeviceProfileSnapshot is required";
        return false;
    }
    struct stat root_st{};
    if (::lstat(root_.c_str(), &root_st) != 0) {
        if (errno != ENOENT || ::mkdir(root_.c_str(), 0700) != 0) {
            if (error) *error = "unable to create private CF-DVS directory";
            return false;
        }
        if (::lstat(root_.c_str(), &root_st) != 0) {
            if (error) *error = "unable to inspect CF-DVS directory";
            return false;
        }
    }
    if (!S_ISDIR(root_st.st_mode) || S_ISLNK(root_st.st_mode)) {
        if (error) *error = "CF-DVS root must be a real directory, not a symlink";
        return false;
    }
    if (::chmod(root_.c_str(), 0700) != 0) {
        if (error) *error = "unable to secure CF-DVS directory permissions";
        return false;
    }
    session_id_ = make_uuid();
    if (session_id_.empty()) {
        if (error) *error = "secure random session ID generation failed";
        return false;
    }
    profile_.assign(profile);
    device_profile_ = std::move(device_profile);
    device_profile_json_ = serialize_device_profile(*device_profile_);
    session_dir_ = root_ + "/" + session_id_ + ".partial";
    if (::mkdir(session_dir_.c_str(), 0700) != 0) {
        if (error) *error = "unable to create CF-DVS session directory";
        return false;
    }
    events_fd_ = ::open((session_dir_ + "/events.jsonl").c_str(),
                        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    metrics_fd_ = ::open((session_dir_ + "/metrics.csv").c_str(),
                         O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (events_fd_ < 0 || metrics_fd_ < 0) {
        if (error) *error = "unable to create CF-DVS session payload files";
        close_files();
        return false;
    }
    const std::string csv = "sequence,observed_at,cpu_psi_pct,memory_psi_pct,io_psi_pct,thermal_pressure,temp_c,headroom,ai_prediction_10s,ai_confidence,ownership_contention_retries\n";
    if (!write_all(metrics_fd_, csv)) {
        if (error) *error = "unable to initialize CF-DVS metrics file";
        close_files();
        return false;
    }
    active_ = true;
    failed_ = false;
    writer_failed_ = false;
    writer_stop_ = false;
    writer_started_ = false;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.clear();
        queued_bytes_ = 0;
        writer_samples_ = 0;
    }
    const int thread_rc = ::pthread_create(&writer_thread_, nullptr, &DeviceValidationRecorder::writer_entry, this);
    if (thread_rc != 0) {
        active_ = false;
        close_files();
        if (error) *error = "unable to start isolated CF-DVS writer thread";
        return false;
    }
    writer_started_ = true;
    sequence_ = 0;
    started_ns_ = monotonic_ns();
    started_at_ = utc_now();
    event_bytes_ = 0;
    samples_ = 0;
    latest_ = {};
    const std::string data = "{\"profile\":\"" + json_escape(profile_) +
        "\",\"device_profile_generation\":" + std::to_string(device_profile_->generation) +
        ",\"device_profile_fingerprint\":\"" + json_escape(device_profile_->fingerprint) + "\"}";
    if (!append_event("session.started", session_id_, data)) {
        active_ = false;
        close_files();
        if (error) *error = "unable to write CF-DVS session header";
        return false;
    }
    return true;
}

void* DeviceValidationRecorder::writer_entry(void* context) noexcept {
    if (context != nullptr) static_cast<DeviceValidationRecorder*>(context)->writer_loop();
    return nullptr;
}

bool DeviceValidationRecorder::enqueue_write(std::string events, std::string metrics) noexcept {
    const size_t bytes = events.size() + metrics.size();
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (writer_stop_ || queue_.size() >= kMaxQueuedWrites || queued_bytes_ + bytes > kMaxQueuedBytes) {
            failed_ = true;
            return false;
        }
        queue_.push_back({std::move(events), std::move(metrics)});
        queued_bytes_ += bytes;
    }
    queue_cv_.notify_one();
    return true;
}

void DeviceValidationRecorder::writer_loop() noexcept {
    for (;;) {
        QueuedWrite item;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return writer_stop_ || !queue_.empty(); });
            if (queue_.empty() && writer_stop_) break;
            item = std::move(queue_.front());
            queued_bytes_ -= item.events.size() + item.metrics.size();
            queue_.pop_front();
        }
        bool ok = true;
        if (!item.events.empty()) ok = write_all(events_fd_, item.events) && ok;
        if (!item.metrics.empty()) {
            ok = write_all(metrics_fd_, item.metrics) && ok;
            ++writer_samples_;
            if (writer_samples_ % kFsyncEverySamples == 0) {
                ok = (::fdatasync(events_fd_) == 0) && ok;
                ok = (::fdatasync(metrics_fd_) == 0) && ok;
            }
        }
        if (!ok) {
            writer_failed_ = true;
            failed_ = true;
        }
    }
    if (::fdatasync(events_fd_) != 0 || ::fdatasync(metrics_fd_) != 0) {
        writer_failed_ = true;
        failed_ = true;
    }
}

bool DeviceValidationRecorder::append_event(std::string_view type,
                                             std::string_view correlation_id,
                                             std::string_view data_json,
                                             std::string_view metrics_line) noexcept {
    if (!active_ || events_fd_ < 0 || sequence_ >= kMaxEvents || event_bytes_ >= kMaxEventBytes) {
        failed_ = true;
        return false;
    }
    const std::string timestamp = utc_now();
    const uint64_t mono = monotonic_ns();
    const std::string line = "{\"session_id\":\"" + session_id_ +
        "\",\"sequence\":" + std::to_string(++sequence_) +
        ",\"timestamp\":\"" + timestamp +
        "\",\"monotonic_ns\":" + std::to_string(mono) +
        ",\"event_type\":\"" + json_escape(type) +
        "\",\"correlation_id\":\"" + json_escape(correlation_id) +
        "\",\"data\":" + std::string(data_json) + "}\n";
    if (event_bytes_ + line.size() > kMaxEventBytes || !enqueue_write(line, std::string(metrics_line))) {
        failed_ = true;
        return false;
    }
    event_bytes_ += line.size();
    return true;
}

bool DeviceValidationRecorder::record_sample(const DeviceValidationSample& sample) noexcept {
    if (!active_ || failed_ || writer_failed_) return false;
    const uint64_t elapsed = monotonic_ns();
    if (started_ns_ == 0 || elapsed < started_ns_ || elapsed - started_ns_ > kMaxSessionNs ||
        sequence_ >= kMaxEvents - 1) {
        failed_ = true;
        return false;
    }
    latest_ = sample;
    const std::string data =
        "{\"cpu_psi_pct\":" + number_or_null(sample.cpu_psi) +
        ",\"memory_psi_pct\":" + number_or_null(sample.memory_psi) +
        ",\"io_psi_pct\":" + number_or_null(sample.io_psi) +
        ",\"thermal_pressure\":" + number_or_null(sample.thermal_pressure) +
        ",\"temperature_c\":" + number_or_null(sample.temperature_celsius) +
        ",\"temperature_velocity_c_per_sec\":" + number_or_null(sample.temperature_velocity_celsius_per_sec) +
        ",\"thermal_headroom\":" + number_or_null(sample.thermal_headroom) +
        ",\"ai_prediction_10s\":" + number_or_null(sample.ai_prediction_10s) +
        ",\"ai_confidence\":" + number_or_null(sample.ai_confidence) +
        ",\"thermal_source\":\"" + json_escape(sample.thermal_source) +
        "\",\"workload\":\"" + json_escape(sample.workload) +
        "\",\"tuner_stage\":\"" + json_escape(sample.tuner_stage) +
        "\",\"tuner_verdict\":\"" + json_escape(sample.tuner_verdict) +
        "\",\"candidate\":\"" + json_escape(sample.candidate) +
        "\",\"capacity_scale\":" + (std::isfinite(sample.capacity_scale) && sample.capacity_scale >= 0.0
            ? number_or_null(sample.capacity_scale) : "null") +
        ",\"valid_thermal_sensors\":" + std::to_string(sample.valid_thermal_sensors) +
        ",\"ownership_contention_retries\":" + std::to_string(sample.ownership_contention_retries) +
        ",\"recovery_pending\":" + (sample.recovery_pending ? "true" : "false") +
        ",\"journal_clear\":" + (sample.journal_clear ? "true" : "false") +
        ",\"rollback_verified\":" + (sample.rollback_verified ? "true" : "false") + "}";
    const std::string csv = std::to_string(sequence_ + 1) + "," + utc_now() + "," +
        number_or_null(sample.cpu_psi) + "," + number_or_null(sample.memory_psi) + "," +
        number_or_null(sample.io_psi) + "," + number_or_null(sample.thermal_pressure) + "," +
        number_or_null(sample.temperature_celsius) + "," + number_or_null(sample.thermal_headroom) + "," +
        number_or_null(sample.ai_prediction_10s) + "," + number_or_null(sample.ai_confidence) + "," +
        std::to_string(sample.ownership_contention_retries) + "\n";
    if (!append_event("telemetry.sample", sample.candidate, data, csv)) return false;
    ++samples_;
    return true;
}

void DeviceValidationRecorder::close_files() noexcept {
    if (events_fd_ >= 0) {
        (void)::close(events_fd_);
        events_fd_ = -1;
    }
    if (metrics_fd_ >= 0) {
        (void)::close(metrics_fd_);
        metrics_fd_ = -1;
    }
}

bool DeviceValidationRecorder::stop(std::string* bundle_path, std::string* error) noexcept {
    if (!active_) {
        if (error) *error = "no active CF-DVS session";
        return false;
    }
    const std::string data = "{\"samples\":" + std::to_string(samples_) +
        ",\"capture_complete\":" + ((!failed_ && samples_ > 0) ? "true" : "false") + "}";
    if (!append_event("session.stopped", session_id_, data)) failed_ = true;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        writer_stop_ = true;
    }
    queue_cv_.notify_one();
    if (writer_started_) {
        if (::pthread_join(writer_thread_, nullptr) != 0) failed_ = true;
        writer_started_ = false;
    }
    if (writer_failed_) failed_ = true;
    if (::fdatasync(events_fd_) != 0 || ::fdatasync(metrics_fd_) != 0) failed_ = true;
    close_files();
    const bool capture_ok = !failed_ && samples_ > 0;
    const bool journal_clear = samples_ > 0 && latest_.journal_clear;
    const bool active_mutations = samples_ > 0 && latest_.active_mutations;
    const bool recovery_pending = samples_ > 0 &&
        (latest_.recovery_pending || (!journal_clear && !active_mutations));
    const bool rollback_verified = samples_ > 0 && latest_.rollback_verified &&
        journal_clear && !active_mutations;
    const std::string ended_at = utc_now();

    const std::string manufacturer = read_property("ro.product.manufacturer", "unknown");
    const std::string model = read_property("ro.product.model", "unknown");
    const std::string api = read_property("ro.build.version.sdk", "35");
    const int api_level = std::max(0, std::atoi(api.c_str()));
    const std::string started_at = started_at_;
    const std::string recovery_status = recovery_pending ? "inconclusive" :
        (rollback_verified ? "pass" : "inconclusive");
    const std::string collector_status = capture_ok ? "pass" : "inconclusive";
    const std::string device_status = "inconclusive"; // Capture is evidence, not device acceptance.
    const std::string verdict = recovery_pending ? "RECOVERY_REQUIRED" : "INCONCLUSIVE";

    const std::string report =
        "{\n"
        "\"schema_version\":\"1.1\",\n"
        "\"session\":{\"id\":\"" + session_id_ + "\",\"started_at\":\"" + started_at +
        "\",\"ended_at\":\"" + ended_at + "\",\"device\":{\"manufacturer\":\"" +
        json_escape(manufacturer) + "\",\"model\":\"" + json_escape(model) +
        "\",\"android_api\":" + std::to_string(api_level) + ",\"build_fingerprint_hash\":null}},\n"
        "\"profile\":\"" + profile_ + "\",\n"
        "\"build\":{\"version\":\"3.1.0\",\"commit\":\"" + build_commit() + "\"},\n"
        "\"device_profile\":" + device_profile_json_ + ",\n"
        "\"metrics\":{\n"
        "\"cpu_psi_avg10\":" + metric_json(latest_.cpu_psi, "%", "procfs.psi.cpu", ended_at, "CPU PSI unavailable") + ",\n"
        "\"memory_psi_avg10\":" + metric_json(latest_.memory_psi, "%", "procfs.psi.memory", ended_at, "Memory PSI unavailable") + ",\n"
        "\"io_psi_avg10\":" + metric_json(latest_.io_psi, "%", "procfs.psi.io", ended_at, "I/O PSI unavailable") + ",\n"
        "\"thermal_pressure\":" + metric_json(latest_.thermal_pressure, "index", "thermal.assessment", ended_at, "Thermal pressure unavailable") + ",\n"
        "\"temperature_celsius\":" + metric_json(latest_.temperature_celsius, "C", "thermal.soc_composite", ended_at, "Physical temperature unavailable") + ",\n"
        "\"temperature_velocity_celsius_per_sec\":" + metric_json(latest_.temperature_velocity_celsius_per_sec, "C/s", "thermal.temperature_trend", ended_at, "Thermal trend unavailable") + ",\n"
        "\"thermal_headroom\":" + metric_json(latest_.thermal_headroom, "ratio", "thermal.headroom", ended_at, "Thermal headroom unavailable") + ",\n"
        "\"ai_prediction_10s\":" + metric_json(latest_.ai_prediction_10s, "index", "neural.predictor", ended_at, "AI prediction unavailable") + ",\n"
        "\"ai_confidence\":" + metric_json(latest_.ai_confidence, "ratio", "neural.predictor", ended_at, "AI confidence unavailable") + "\n"
        "},\n"
        "\"checks\":["
        "{\"id\":\"collector.session\",\"required\":true,\"status\":\"" + collector_status + "\",\"detail\":\"Session capture and bounded file writes\"},"
        "{\"id\":\"collector.telemetry\",\"required\":true,\"status\":\"" + collector_status + "\",\"detail\":\"Samples recorded: " + std::to_string(samples_) + "\"},"
        "{\"id\":\"device.cddp_profile\",\"required\":true,\"status\":\"pass\",\"detail\":\"Pinned CDDP generation " + std::to_string(device_profile_->generation) + " fingerprint " + json_escape(device_profile_->fingerprint) + "\"},"
        "{\"id\":\"device.acceptance\",\"required\":true,\"status\":\"" + device_status + "\",\"detail\":\"Runtime capture does not certify device behavior\"},"
        "{\"id\":\"recovery.state\",\"required\":true,\"status\":\"" + recovery_status + "\",\"detail\":\"Journal and rollback evidence from last sample\"}],\n"
        "\"recovery\":{\"pending\":" + (recovery_pending ? "true" : "false") +
        ",\"journal_clear\":" + (journal_clear ? "true" : "false") +
        ",\"active_mutations\":" + (active_mutations ? "true" : "false") +
        ",\"rollback_verified\":" + (rollback_verified ? "true" : "false") + "},\n"
        "\"verdict\":\"" + verdict + "\",\n"
        "\"notes\":[\"Local device capture only; no upload performed.\",\"Device acceptance remains inconclusive until profile-specific checks are evaluated.\",\"Thermal and PSI values are sampled read-only.\"]\n"
        "}\n";

    const std::string report_path = session_dir_ + "/report.json";
    const int report_fd = ::open(report_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    bool ok = report_fd >= 0;
    if (ok) {
        ok = write_all(report_fd, report) && ::fdatasync(report_fd) == 0;
        (void)::close(report_fd);
    }
    const std::string summary =
        "CoreFlow Device Validation Suite\n"
        "Session: " + session_id_ + "\nProfile: " + profile_ +
        "\nSamples: " + std::to_string(samples_) +
        "\nCapture: " + (capture_ok ? "complete" : "incomplete") +
        "\nActive mutations: " + (active_mutations ? "yes" : "no") +
        "\nRecovery pending: " + (recovery_pending ? "yes" : "no") +
        "\nCDDP profile generation: " + std::to_string(device_profile_->generation) +
        "\nCDDP fingerprint: " + device_profile_->fingerprint +
        "\nCPU policies: " + std::to_string(device_profile_->cpu_policies.size()) +
        "\nGPU domains: " + std::to_string(device_profile_->gpu_domains.size()) +
        "\nThermal sensors: " + std::to_string(device_profile_->thermal_sensors.size()) +
        "\nVerdict: " + verdict +
        "\nDevice acceptance: INCONCLUSIVE (capture-only Phase 2)\n";
    const std::string summary_path = session_dir_ + "/summary.txt";
    const int summary_fd = ::open(summary_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (summary_fd < 0 || !write_all(summary_fd, summary) || ::fdatasync(summary_fd) != 0) ok = false;
    if (summary_fd >= 0) (void)::close(summary_fd);

    if (ok) {
        constexpr std::array<const char*, 4> names{"events.jsonl", "metrics.csv", "report.json", "summary.txt"};
        std::string manifest = "{\"manifest_version\":\"1.0\",\"files\":[";
        bool first = true;
        for (const char* name : names) {
            uint64_t size = 0;
            std::string digest;
            if (!sha256_file(session_dir_ + "/" + name, size, digest)) {
                ok = false;
                break;
            }
            if (!first) manifest += ",";
            first = false;
            manifest += "{\"path\":\"" + std::string(name) + "\",\"size_bytes\":" +
                std::to_string(size) + ",\"sha256\":\"" + digest + "\"}";
        }
        manifest += "]}\n";
        if (ok) {
            const std::string manifest_path = session_dir_ + "/manifest.json";
            const int manifest_fd = ::open(manifest_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
            if (manifest_fd < 0 || !write_all(manifest_fd, manifest) || ::fdatasync(manifest_fd) != 0) ok = false;
            if (manifest_fd >= 0) (void)::close(manifest_fd);
        }
    }

    if (ok) {
        const std::string final_dir = root_ + "/" + session_id_;
        if (::rename(session_dir_.c_str(), final_dir.c_str()) != 0) {
            ok = false;
        } else {
            session_dir_ = final_dir;
            const int root_fd = ::open(root_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
            if (root_fd < 0 || ::fsync(root_fd) != 0) ok = false;
            if (root_fd >= 0) (void)::close(root_fd);
        }
    }
    active_ = false;
    if (!ok) {
        failed_ = true;
        if (error) *error = "CF-DVS finalization failed; partial session preserved at " + session_dir_;
        return false;
    }
    if (bundle_path) *bundle_path = session_dir_;
    return true;
}

} // namespace coreflow
