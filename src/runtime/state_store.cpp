#include "coreflow/runtime/state_store.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {

namespace fs = std::filesystem;
constexpr std::uintmax_t kJournalRotateBytes = 256U * 1024U;
constexpr std::size_t kMaximumCommandBytes = 64;

std::string_view trimmed(std::string_view text) noexcept {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

void appendNumberField(std::ostringstream& out, const char* key, bool valid, double value,
                       int precision) {
    out << '"' << key << "\":";
    if (valid && std::isfinite(value)) {
        out << std::fixed << std::setprecision(precision) << value;
    } else {
        out << "null";
    }
}

void appendStringField(std::ostringstream& out, const char* key, std::string_view value) {
    out << '"' << key << "\":\"" << jsonEscape(value) << '"';
}

} // namespace

Command parseCommand(std::string_view text) noexcept {
    const auto token = trimmed(text);
    if (token == "pause") return Command::Pause;
    if (token == "resume") return Command::Resume;
    if (token == "rescan") return Command::Rescan;
    if (token == "reset_learning") return Command::ResetLearning;
    return Command::None;
}

std::string_view commandToken(Command command) noexcept {
    switch (command) {
        case Command::Pause: return "pause";
        case Command::Resume: return "resume";
        case Command::Rescan: return "rescan";
        case Command::ResetLearning: return "reset_learning";
        case Command::None: break;
    }
    return "";
}

std::string jsonEscape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (const unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(c));
                    out += buffer;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

std::string formatStatusJson(const RuntimeStatus& s) {
    std::ostringstream out;
    out << "{\"schema\":1,\"generated_unix\":" << s.generated_unix << ',';
    appendStringField(out, "engine_state", s.engine_state); out << ',';
    appendStringField(out, "stage", s.stage); out << ',';
    out << "\"uptime_s\":" << s.uptime_s << ",\"cycles\":" << s.cycles << ',';
    appendStringField(out, "workload", s.workload_signature); out << ',';
    out << "\"device\":{";
    appendStringField(out, "model", s.model); out << ',';
    appendStringField(out, "soc", s.soc); out << ',';
    appendStringField(out, "android", s.android_release); out << ',';
    appendStringField(out, "kernel", s.kernel_release); out << ',';
    appendStringField(out, "storage", s.storage_type);
    out << "},\"inventory\":{\"discovered\":" << s.discovered << ",\"eligible\":" << s.eligible
        << ',';
    appendStringField(out, "catalog", s.catalog_status);
    out << "},\"counters\":{\"retained\":" << s.retained << ",\"rolled_back\":" << s.rolled_back
        << ",\"deferred\":" << s.deferred << ",\"unresolved\":" << s.unresolved << "},";
    const auto& m = s.system;
    out << "\"system\":{";
    appendNumberField(out, "cpu_busy_percent", m.cpu_valid, m.cpu_busy_percent, 1); out << ',';
    appendNumberField(out, "memory_available_ratio", m.memory_valid, m.memory_available_ratio, 4); out << ',';
    appendNumberField(out, "max_temperature_c", m.temperature_valid, m.maximum_temperature_c, 1); out << ',';
    appendNumberField(out, "thermal_headroom_c", m.thermal_headroom_valid, m.thermal_headroom_c, 1); out << ',';
    appendNumberField(out, "cpu_psi_some_avg10", m.cpu_pressure_valid, m.cpu_pressure_some_avg10, 2); out << ',';
    appendNumberField(out, "memory_psi_some_avg10", m.memory_pressure_valid, m.memory_pressure_some_avg10, 2); out << ',';
    appendNumberField(out, "io_psi_some_avg10", m.io_pressure_valid, m.io_pressure_some_avg10, 2); out << ',';
    appendNumberField(out, "frame_average_fps", m.frame_timing_valid, m.frame_average_fps, 1); out << ',';
    appendNumberField(out, "frame_p95_ms", m.frame_timing_valid, m.frame_p95_interval_ms, 1); out << ',';
    appendNumberField(out, "frame_p99_ms", m.frame_timing_valid, m.frame_p99_interval_ms, 1); out << ',';
    appendNumberField(out, "slow_frame_ratio", m.frame_timing_valid, m.slow_frame_ratio, 4); out << ',';
    appendStringField(out, "frame_package", m.frame_timing_valid ? m.frame_package : std::string{});
    out << "},";
    appendStringField(out, "last_error", s.last_error);
    out << '}';
    return out.str();
}

std::string formatCapabilitiesJson(const std::vector<CapabilityRecord>& records, std::size_t limit) {
    std::ostringstream out;
    out << "{\"schema\":1,\"total\":" << records.size() << ",\"items\":[";
    const auto count = std::min(records.size(), limit);
    for (std::size_t i = 0; i < count; ++i) {
        const auto& r = records[i];
        if (i) out << ',';
        out << '{';
        appendStringField(out, "path", r.path); out << ',';
        appendStringField(out, "domain", r.domain); out << ',';
        appendStringField(out, "attribute", r.attribute); out << ',';
        appendStringField(out, "strategy", r.strategy_id); out << ',';
        appendStringField(out, "value", r.current_value); out << ',';
        out << "\"eligible\":" << (r.eligible ? "true" : "false") << '}';
    }
    out << "]}";
    return out.str();
}

std::string formatJournalLine(const JournalRecord& r) {
    std::ostringstream out;
    out << '{';
    appendStringField(out, "kind", r.kind); out << ',';
    out << "\"time\":" << r.unix_time << ',';
    appendStringField(out, "resource", r.resource); out << ',';
    appendStringField(out, "from", r.previous_value); out << ',';
    appendStringField(out, "to", r.target_value); out << ',';
    appendStringField(out, "disposition", r.disposition); out << ',';
    appendStringField(out, "detail", r.detail);
    out << '}';
    return out.str();
}

std::string defaultStateDirectory() {
    const char* configured = std::getenv("COREFLOW_STATE_DIR");
    if (configured && *configured) return configured;
    return "/data/adb/coreflow";
}

StateStore::StateStore(std::string directory) : directory_(std::move(directory)) {
    std::error_code ec;
    fs::create_directories(directory_, ec);
    ready_ = !ec && fs::is_directory(directory_, ec) && ::access(directory_.c_str(), W_OK) == 0;
}

bool StateStore::atomicWrite(const std::string& name, const std::string& content) const {
    if (!ready_) return false;
    const auto target = fs::path(directory_) / name;
    const auto temporary = fs::path(directory_) / (name + ".tmp");
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << content;
        out.flush();
        if (!out) return false;
    }
    // rename() replaces the destination atomically on the same filesystem, so the
    // WebUI never observes a half-written JSON document.
    std::error_code ec;
    fs::rename(temporary, target, ec);
    if (ec) {
        fs::remove(temporary, ec);
        return false;
    }
    return true;
}

bool StateStore::writeStatus(const RuntimeStatus& status) const {
    return atomicWrite("status.json", formatStatusJson(status) + "\n");
}

bool StateStore::writeCapabilities(const std::vector<CapabilityRecord>& records) const {
    return atomicWrite("capabilities.json", formatCapabilitiesJson(records, 2048) + "\n");
}

bool StateStore::appendJournal(const JournalRecord& record) const {
    if (!ready_) return false;
    const auto path = fs::path(directory_) / "journal.jsonl";
    std::error_code ec;
    const auto size = fs::exists(path, ec) ? fs::file_size(path, ec) : std::uintmax_t{0};
    if (!ec && size > kJournalRotateBytes) {
        fs::rename(path, fs::path(directory_) / "journal.jsonl.1", ec);
    }
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out) return false;
    out << formatJournalLine(record) << '\n';
    return static_cast<bool>(out);
}

Command StateStore::takeCommand() const {
    if (!ready_) return Command::None;
    const auto path = fs::path(directory_) / "command";
    std::ifstream in(path, std::ios::binary);
    if (!in) return Command::None;
    std::string buffer(kMaximumCommandBytes + 1, '\0');
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto count = static_cast<std::size_t>(in.gcount());
    in.close();
    // Consume the request even when it is invalid, so a bad token cannot repeat forever.
    std::error_code ec;
    fs::remove(path, ec);
    if (count == 0 || count > kMaximumCommandBytes) return Command::None;
    return parseCommand(std::string_view(buffer.data(), count));
}

} // namespace coreflow
