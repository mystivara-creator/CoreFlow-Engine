#include "coreflow/vendor_knowledge.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <sys/stat.h>

namespace coreflow {
namespace {

std::string_view trimView(std::string_view v) noexcept {
    while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) v.remove_prefix(1);
    while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) v.remove_suffix(1);
    return v;
}

std::string upper(std::string_view v) {
    std::string out(v);
    for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

bool endsWith(std::string_view v, std::string_view s) noexcept {
    return v.size() >= s.size() && v.compare(v.size() - s.size(), s.size(), s) == 0;
}

// Splits a shell/rc line into tokens, honouring simple single/double quotes.
std::vector<std::string> tokenize(std::string_view line) {
    std::vector<std::string> out;
    std::string cur;
    char quote = 0;
    bool have = false;
    for (const char c : line) {
        if (quote != 0) {
            if (c == quote) { quote = 0; continue; }
            cur.push_back(c);
            continue;
        }
        if (c == '\'' || c == '"') { quote = c; have = true; continue; }
        if (c == '#') break;
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (have || !cur.empty()) { out.push_back(cur); cur.clear(); have = false; }
            continue;
        }
        cur.push_back(c);
    }
    if (have || !cur.empty()) out.push_back(cur);
    return out;
}

// Literal values only: anything that depends on a variable, property or glob
// cannot be attributed to a concrete device value.
bool literal(std::string_view v) noexcept {
    if (v.empty()) return false;
    for (const char c : v) {
        if (c == '$' || c == '`' || c == '*' || c == '?' || c == '{' || c == '}') return false;
    }
    return true;
}

bool kernelPath(std::string_view p) noexcept {
#ifdef COREFLOW_HOST_TEST_FIXTURES
    if (p.rfind("/tmp/", 0) == 0 ||
        p.rfind("/data/data/com.termux/files/usr/tmp/", 0) == 0)
        return true;  // host fixture trees
#endif
    return p.rfind("/proc/sys/", 0) == 0 || p.rfind("/sys/", 0) == 0 ||
           p.rfind("/dev/cpuctl/", 0) == 0 || p.rfind("/dev/cpuset/", 0) == 0 ||
           p.rfind("/dev/stune/", 0) == 0 || p.rfind("/dev/blkio/", 0) == 0;
}

// ---------------- minimal JSON (objects, arrays, strings, scalars) ----------
struct Json {
    enum class Type { Null, Bool, Number, String, Array, Object } type{Type::Null};
    std::string text;  // string value or number/bool literal
    std::vector<Json> items;
    std::vector<std::pair<std::string, Json>> fields;

    const Json* get(std::string_view key) const noexcept {
        for (const auto& [k, v] : fields) if (k == key) return &v;
        return nullptr;
    }
    std::string scalar() const { return text; }
};

class JsonParser {
public:
    explicit JsonParser(std::string_view text) : s_(text) {}
    bool parse(Json& out) { ws(); return value(out, 0) && (ws(), true); }

private:
    bool value(Json& out, int depth) {
        if (depth > 32 || i_ >= s_.size()) return false;
        const char c = s_[i_];
        if (c == '{') return object(out, depth);
        if (c == '[') return array(out, depth);
        if (c == '"') { out.type = Json::Type::String; return str(out.text); }
        std::size_t start = i_;
        while (i_ < s_.size() && (std::isalnum(static_cast<unsigned char>(s_[i_])) ||
                                  s_[i_] == '-' || s_[i_] == '+' || s_[i_] == '.')) ++i_;
        if (start == i_) return false;
        out.text = std::string(s_.substr(start, i_ - start));
        out.type = (out.text == "true" || out.text == "false") ? Json::Type::Bool
                 : out.text == "null" ? Json::Type::Null : Json::Type::Number;
        return true;
    }
    bool object(Json& out, int depth) {
        out.type = Json::Type::Object;
        ++i_; ws();
        if (i_ < s_.size() && s_[i_] == '}') { ++i_; return true; }
        while (i_ < s_.size()) {
            std::string key;
            ws();
            if (i_ >= s_.size() || s_[i_] != '"' || !str(key)) return false;
            ws();
            if (i_ >= s_.size() || s_[i_] != ':') return false;
            ++i_; ws();
            Json v;
            if (!value(v, depth + 1)) return false;
            out.fields.emplace_back(std::move(key), std::move(v));
            ws();
            if (i_ < s_.size() && s_[i_] == ',') { ++i_; continue; }
            if (i_ < s_.size() && s_[i_] == '}') { ++i_; return true; }
            return false;
        }
        return false;
    }
    bool array(Json& out, int depth) {
        out.type = Json::Type::Array;
        ++i_; ws();
        if (i_ < s_.size() && s_[i_] == ']') { ++i_; return true; }
        while (i_ < s_.size()) {
            Json v;
            ws();
            if (!value(v, depth + 1)) return false;
            out.items.push_back(std::move(v));
            ws();
            if (i_ < s_.size() && s_[i_] == ',') { ++i_; continue; }
            if (i_ < s_.size() && s_[i_] == ']') { ++i_; return true; }
            return false;
        }
        return false;
    }
    bool str(std::string& out) {
        ++i_;
        while (i_ < s_.size()) {
            const char c = s_[i_++];
            if (c == '"') return true;
            if (c == '\\' && i_ < s_.size()) {
                const char e = s_[i_++];
                switch (e) {
                    case 'n': out.push_back('\n'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': i_ = std::min(s_.size(), i_ + 4); break;  // not needed for paths
                    default: out.push_back(e); break;
                }
                continue;
            }
            out.push_back(c);
        }
        return false;
    }
    void ws() {
        while (i_ < s_.size()) {
            if (std::isspace(static_cast<unsigned char>(s_[i_]))) { ++i_; continue; }
            if (s_[i_] == '/' && i_ + 1 < s_.size() && s_[i_ + 1] == '/') {  // tolerate comments
                while (i_ < s_.size() && s_[i_] != '\n') ++i_;
                continue;
            }
            break;
        }
    }
    std::string_view s_;
    std::size_t i_{0};
};

} // namespace

bool VendorKnob::declares(std::string_view value) const noexcept {
    for (const auto& v : values) if (v.value == value) return true;
    return false;
}

bool VendorKnob::declaresWithProfile(std::string_view value, VendorProfile profile) const noexcept {
    for (const auto& v : values) if (v.value == value && v.profile == profile) return true;
    return false;
}

const VendorKnob* VendorKnowledge::find(std::string_view path) const noexcept {
    try {
        const auto it = index_.find(std::string(path));
        return it == index_.end() ? nullptr : &knobs_[it->second];
    } catch (...) {
        return nullptr;
    }
}

void VendorKnowledge::clear() noexcept {
    knobs_.clear();
    index_.clear();
    visited_.clear();
    files_parsed_ = 0;
}

std::string VendorKnowledge::canonicalPath(std::string_view path) noexcept {
    try {
        std::string raw(path);
        char resolved[PATH_MAX];
        if (::realpath(raw.c_str(), resolved) != nullptr) return std::string(resolved);
        return raw;
    } catch (...) {
        return {};
    }
}

VendorProfile VendorKnowledge::profileFromLabel(std::string_view label) noexcept {
    try {
        const std::string u = upper(label);
        for (const char* k : {"LOW_POWER", "LOWPOWER", "SUSTAINED", "THERMAL", "BATTERY",
                              "POWER_SAVE", "POWERSAVE", "EFFICIEN", "SAVER", "ECO"}) {
            if (u.find(k) != std::string::npos) return VendorProfile::Efficiency;
        }
        for (const char* k : {"LAUNCH", "INTERACTION", "INTERACTIVE", "BOOST", "PERFORMANCE",
                              "GAME", "RENDERING", "CAMERA", "AUDIO_LOW_LATENCY", "TOUCH",
                              "FLING", "SCROLL"}) {
            if (u.find(k) != std::string::npos) return VendorProfile::Performance;
        }
    } catch (...) {
    }
    return VendorProfile::Boot;
}

const char* VendorKnowledge::profileName(VendorProfile profile) noexcept {
    switch (profile) {
        case VendorProfile::Boot:        return "boot";
        case VendorProfile::Default:     return "default";
        case VendorProfile::Efficiency:  return "efficiency";
        case VendorProfile::Performance: return "performance";
    }
    return "boot";
}

bool VendorKnowledge::add(std::string_view path, std::string_view value,
                          VendorProfile profile, std::string_view source) noexcept {
    try {
        const std::string_view v = trimView(value);
        if (!kernelPath(path) || !literal(path) || !literal(v) || v.size() > 256U) return false;
        const std::string canonical = canonicalPath(path);
        if (canonical.empty()) return false;

        auto it = index_.find(canonical);
        if (it == index_.end()) {
            VendorKnob knob;
            knob.path = canonical;
            knobs_.push_back(std::move(knob));
            it = index_.emplace(canonical, knobs_.size() - 1U).first;
        }
        VendorKnob& knob = knobs_[it->second];
        for (auto& existing : knob.values) {
            if (existing.value == v) {
                // A more specific profile (Efficiency/Performance/Default)
                // refines a plain Boot attribution of the same value.
                if (existing.profile == VendorProfile::Boot && profile != VendorProfile::Boot)
                    existing.profile = profile;
                return true;
            }
        }
        if (knob.values.size() >= kMaxValuesPerKnob) return false;
        knob.values.push_back(VendorValue{std::string(v), profile, std::string(source)});
        return true;
    } catch (...) {
        return false;
    }
}

std::size_t VendorKnowledge::parseInitRc(std::string_view text, std::string_view source) noexcept {
    std::size_t added = 0;
    try {
        VendorProfile section = VendorProfile::Boot;
        std::istringstream in{std::string(text)};
        std::string line;
        while (std::getline(in, line)) {
            const auto tokens = tokenize(line);
            if (tokens.empty()) continue;
            if (tokens[0] == "on" || tokens[0] == "service" || tokens[0] == "import") {
                // The trigger text (e.g. "on property:vendor.power.lowpower=1")
                // tells which operating profile the following writes belong to.
                section = tokens[0] == "on" ? profileFromLabel(line) : VendorProfile::Boot;
                continue;
            }
            if (tokens[0] == "write" && tokens.size() >= 3U) {
                std::string value = tokens[2];
                for (std::size_t i = 3; i < tokens.size(); ++i) value += " " + tokens[i];
                if (add(tokens[1], value, section, source)) ++added;
            }
        }
    } catch (...) {
    }
    return added;
}

std::size_t VendorKnowledge::parseShell(std::string_view text, std::string_view source) noexcept {
    std::size_t added = 0;
    try {
        VendorProfile section = VendorProfile::Boot;
        std::istringstream in{std::string(text)};
        std::string line;
        while (std::getline(in, line)) {
            // Function/case labels such as `configure_low_power()` or
            // `"sustained")` carry the vendor's own profile vocabulary.
            const std::string_view trimmed = trimView(line);
            if (!trimmed.empty() && (endsWith(trimmed, "()") || endsWith(trimmed, "() {") ||
                                     endsWith(trimmed, ")"))) {
                const VendorProfile labelled = profileFromLabel(trimmed);
                if (labelled != VendorProfile::Boot) section = labelled;
            }
            if (trimmed == "}" || trimmed == ";;" || trimmed == "esac") section = VendorProfile::Boot;

            // Split on ';' and '&&' so one-liners are handled.
            std::string flat(line);
            for (auto& c : flat) if (c == ';') c = '\n';
            std::istringstream parts(flat);
            std::string part;
            while (std::getline(parts, part)) {
                const auto tokens = tokenize(part);
                // echo [-n|-e] <value...> > <path>
                for (std::size_t i = 0; i < tokens.size(); ++i) {
                    if (tokens[i] != "echo") continue;
                    std::size_t first = i + 1;
                    while (first < tokens.size() && (tokens[first] == "-n" || tokens[first] == "-e"))
                        ++first;
                    std::size_t redirect = tokens.size();
                    for (std::size_t j = first; j < tokens.size(); ++j) {
                        if (tokens[j] == ">" || tokens[j] == ">>") { redirect = j; break; }
                    }
                    if (redirect == tokens.size() || redirect + 1 >= tokens.size() ||
                        redirect == first) break;
                    std::string value = tokens[first];
                    for (std::size_t j = first + 1; j < redirect; ++j) value += " " + tokens[j];
                    if (add(tokens[redirect + 1], value, section, source)) ++added;
                    break;
                }
            }
        }
    } catch (...) {
    }
    return added;
}

std::size_t VendorKnowledge::parsePowerHintJson(std::string_view text,
                                                std::string_view source) noexcept {
    std::size_t added = 0;
    try {
        Json root;
        JsonParser parser(text);
        if (!parser.parse(root) || root.type != Json::Type::Object) return 0;

        std::map<std::string, std::string> node_paths;
        if (const Json* nodes = root.get("Nodes"); nodes && nodes->type == Json::Type::Array) {
            for (const auto& node : nodes->items) {
                const Json* name = node.get("Name");
                const Json* path = node.get("Path");
                if (!name || !path) continue;
                node_paths[name->scalar()] = path->scalar();
                const Json* values = node.get("Values");
                const Json* def = node.get("DefaultIndex");
                const long def_index = def ? std::strtol(def->text.c_str(), nullptr, 10) : -1;
                if (values && values->type == Json::Type::Array) {
                    for (std::size_t i = 0; i < values->items.size(); ++i) {
                        const VendorProfile p = static_cast<long>(i) == def_index
                                                    ? VendorProfile::Default
                                                    : VendorProfile::Boot;
                        if (add(path->scalar(), values->items[i].scalar(), p, source)) ++added;
                    }
                }
            }
        }
        if (const Json* actions = root.get("Actions"); actions && actions->type == Json::Type::Array) {
            for (const auto& action : actions->items) {
                const Json* hint = action.get("PowerHint");
                const Json* node = action.get("Node");
                const Json* value = action.get("Value");
                if (!hint || !node || !value) continue;
                const auto found = node_paths.find(node->scalar());
                if (found == node_paths.end()) continue;
                if (add(found->second, value->scalar(), profileFromLabel(hint->scalar()), source))
                    ++added;
            }
        }
    } catch (...) {
    }
    return added;
}

std::size_t VendorKnowledge::parseFile(const std::string& path) noexcept {
    try {
        if (files_parsed_ >= kMaxFiles) return 0;
        const std::string_view name = std::string_view(path).substr(path.rfind('/') + 1U);
        enum class Kind { None, Rc, Shell, PowerHint } kind = Kind::None;
        if (endsWith(name, ".rc")) kind = Kind::Rc;
        else if (endsWith(name, ".sh")) kind = Kind::Shell;
        else if (endsWith(name, ".json") && upper(name).find("POWERHINT") != std::string::npos)
            kind = Kind::PowerHint;
        if (kind == Kind::None) return 0;  // never read unrelated vendor files

        const std::string canonical = canonicalPath(path);
        if (!visited_.insert(canonical).second) return 0;

        struct stat st {};
        if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) ||
            static_cast<std::size_t>(st.st_size) > kMaxFileBytes) {
            return 0;
        }
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file) return 0;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        const std::string text = buffer.str();
        ++files_parsed_;

        switch (kind) {
            case Kind::Rc:        return parseInitRc(text, path);
            case Kind::Shell:     return parseShell(text, path);
            case Kind::PowerHint: return parsePowerHintJson(text, path);
            case Kind::None:      break;
        }
    } catch (...) {
    }
    return 0;
}

void VendorKnowledge::scan(const std::string& root, std::size_t depth) noexcept {
    try {
        struct stat st {};
        if (::stat(root.c_str(), &st) != 0) return;
        if (S_ISREG(st.st_mode)) { (void)parseFile(root); return; }
        if (!S_ISDIR(st.st_mode) || depth > 3U) return;

        std::unique_ptr<DIR, int (*)(DIR*)> dir(::opendir(root.c_str()), ::closedir);
        if (!dir) return;
        std::vector<std::string> entries;
        while (const dirent* entry = ::readdir(dir.get())) {
            const std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            entries.push_back(root + "/" + name);
        }
        std::sort(entries.begin(), entries.end());
        for (const auto& entry : entries) {
            if (files_parsed_ >= kMaxFiles) return;
            scan(entry, depth + 1U);
        }
    } catch (...) {
    }
}

std::size_t VendorKnowledge::harvest(const std::vector<std::string>& roots) noexcept {
    for (const auto& root : roots) scan(root, 0);
    return knobs_.size();
}

std::size_t VendorKnowledge::harvest() noexcept {
    try {
        return harvest({
            "/vendor/etc/init", "/vendor/etc/powerhint.json", "/vendor/etc",
            "/odm/etc/init", "/odm/etc", "/vendor/odm/etc/init",
            "/system/etc/init/hw", "/system/etc/init", "/system_ext/etc/init",
            "/product/etc/init", "/vendor/bin",
        });
    } catch (...) {
        return knobs_.size();
    }
}

} // namespace coreflow
