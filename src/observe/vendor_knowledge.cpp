#include "coreflow/vendor_knowledge.hpp"
#include "coreflow/durable_file.hpp"
#include "coreflow/integer_parse.hpp"

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
        if (static_cast<unsigned char>(c) < 0x20U || c == 0x7f ||
            c == '$' || c == '`' || c == '*' || c == '?' || c == '{' || c == '}') return false;
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
    bool parse(Json& out) { ws(); return value(out, 0) && (ws(), i_ == s_.size()); }

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
        if (out.type != Json::Type::Number) return true;
        // Validate JSON number grammar, without converting or losing precision.
        const std::string_view token = out.text;
        std::size_t n = token.front() == '-' ? 1U : 0U;
        const auto digit = [&](std::size_t at) { return at < token.size() && token[at] >= '0' && token[at] <= '9'; };
        if (!digit(n)) return false;
        if (token[n] == '0') ++n;
        else while (digit(n)) ++n;
        if (n < token.size() && token[n] == '.') {
            ++n;
            if (!digit(n)) return false;
            while (digit(n)) ++n;
        }
        if (n < token.size() && (token[n] == 'e' || token[n] == 'E')) {
            ++n;
            if (n < token.size() && (token[n] == '+' || token[n] == '-')) ++n;
            if (!digit(n)) return false;
            while (digit(n)) ++n;
        }
        return n == token.size();
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
            if (out.get(key) != nullptr) return false; // ambiguous duplicate key
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
    bool hex4(std::uint32_t& code) {
        if (s_.size() - i_ < 4U) return false;
        code = 0;
        for (unsigned n = 0; n < 4U; ++n) {
            const char c = s_[i_++];
            unsigned digit = 0;
            if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(c - 'A' + 10);
            else return false;
            code = (code << 4U) | digit;
        }
        return true;
    }
    bool str(std::string& out) {
        ++i_;
        while (i_ < s_.size()) {
            const char c = s_[i_++];
            if (c == '"') return true;
            if (static_cast<unsigned char>(c) < 0x20U) return false;
            if (c != '\\') { out.push_back(c); continue; }
            if (i_ >= s_.size()) return false;
            const char e = s_[i_++];
            switch (e) {
                case '"': case '\\': case '/': out.push_back(e); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    std::uint32_t code = 0;
                    if (!hex4(code)) return false;
                    if (code >= 0xD800U && code <= 0xDBFFU) {
                        if (s_.size() - i_ < 2U || s_[i_] != '\\' || s_[i_ + 1U] != 'u') return false;
                        i_ += 2U;
                        std::uint32_t low = 0;
                        if (!hex4(low) || low < 0xDC00U || low > 0xDFFFU) return false;
                        code = 0x10000U + ((code - 0xD800U) << 10U) + low - 0xDC00U;
                    } else if (code >= 0xDC00U && code <= 0xDFFFU) return false;
                    if (code <= 0x7FU) out.push_back(static_cast<char>(code));
                    else if (code <= 0x7FFU) {
                        out.push_back(static_cast<char>(0xC0U | (code >> 6U)));
                        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
                    } else if (code <= 0xFFFFU) {
                        out.push_back(static_cast<char>(0xE0U | (code >> 12U)));
                        out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
                        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
                    } else {
                        out.push_back(static_cast<char>(0xF0U | (code >> 18U)));
                        out.push_back(static_cast<char>(0x80U | ((code >> 12U) & 0x3FU)));
                        out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
                        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
                    }
                    break;
                }
                default: return false;
            }
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

bool VendorKnob::hasPerformanceHeadroom(std::string_view live_baseline) const noexcept {
    const std::string_view base = trimView(live_baseline);
    for (const auto& v : values) {
        if (v.profile == VendorProfile::Performance && trimView(v.value) != base) {
            return true;
        }
    }
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
    scan_entries_ = 0;
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
                              "POWER_SAVE", "POWERSAVE", "EFFICIEN", "SAVER", "ECO",
                              "DISPLAY_INACTIVE", "LOAD_RESET"}) {
            if (u.find(k) != std::string::npos) return VendorProfile::Efficiency;
        }
        for (const char* k : {"LAUNCH", "INTERACTION", "INTERACTIVE", "BOOST", "PERFORMANCE",
                              "GAME", "GAMING", "RENDERING", "CAMERA", "AUDIO_LOW_LATENCY", "TOUCH",
                              "FLING", "SCROLL", "ADPF", "LOAD_UP",
                              "DISPLAY_UPDATE_IMMINENT", "EARLY_WAKEUP"}) {
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
    if (text.size() > kMaxFileBytes) return 0;
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
    if (text.size() > kMaxFileBytes) return 0;
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
    if (text.size() > kMaxFileBytes) return 0;
    std::size_t added = 0;
    try {
        Json root;
        JsonParser parser(text);
        if (!parser.parse(root) || root.type != Json::Type::Object) return 0;

        std::map<std::string, const Json*> node_paths;
        if (const Json* nodes = root.get("Nodes"); nodes && nodes->type == Json::Type::Array) {
            for (const auto& node : nodes->items) {
                const Json* name = node.get("Name");
                const Json* path = node.get("Path");
                if (!name || !path || name->type != Json::Type::String ||
                    path->type != Json::Type::String) continue;
                node_paths.emplace(name->text, &node);
                const Json* values = node.get("Values");
                const Json* def = node.get("DefaultIndex");
                std::uint64_t def_index = 0;
                const bool has_default = def && def->type == Json::Type::Number &&
                    integer_parse::parseUint64(def->text, def_index);
                if (values && values->type == Json::Type::Array) {
                    for (std::size_t i = 0; i < values->items.size(); ++i) {
                        const VendorProfile p = has_default && i == def_index
                                                    ? VendorProfile::Default
                                                    : VendorProfile::Boot;
                        if (values->items[i].type == Json::Type::String &&
                            add(path->text, values->items[i].text, p, source)) ++added;
                    }
                }
            }
        }
        if (const Json* actions = root.get("Actions"); actions && actions->type == Json::Type::Array) {
            for (const auto& action : actions->items) {
                const Json* hint = action.get("PowerHint");
                const Json* node = action.get("Node");
                const Json* value = action.get("Value");
                if (!hint || !node || !value || hint->type != Json::Type::String ||
                    node->type != Json::Type::String || value->type != Json::Type::String) continue;
                const Json* type = action.get("Type");
                if (type && type->text != "Node") continue;
                const auto found = node_paths.find(node->scalar());
                if (found == node_paths.end()) continue;
                // libperfmgr Node actions name a value declared in Nodes.Values.
                // Never invent an operating point from an unrecognized action.
                const Json* values = found->second->get("Values");
                const Json* path = found->second->get("Path");
                if (!values || values->type != Json::Type::Array || !path) continue;
                const bool declared = std::any_of(values->items.begin(), values->items.end(),
                    [&](const Json& item) { return item.type == Json::Type::String && item.text == value->text; });
                if (declared && add(path->text, value->text, profileFromLabel(hint->text), source)) ++added;
            }
        }
    } catch (...) {
    }
    return added;
}

std::size_t VendorKnowledge::parseVendorPerfXml(std::string_view text,
                                                std::string_view source) noexcept {
    // Harvests Qualcomm BoostFramework (targetresourceconfigs.xml /
    // perfboostsconfig.xml), MediaTek PerfService (perfservscntbl.xml /
    // power_app_cfg.xml), and OEM perf/thermal XML tables read-only without
    // hardcoding any device path. Only literal kernel paths and literal values
    // pass VendorKnowledge::add().
    if (text.size() > kMaxFileBytes) return 0;
    std::size_t added = 0;
    try {
        VendorProfile active_profile = VendorProfile::Boot;
        std::size_t pos = 0;
        while (pos < text.size()) {
            const auto lt = text.find('<', pos);
            if (lt == std::string_view::npos) break;
            if (lt + 3U < text.size() && text.substr(lt, 4U) == "<!--") {
                const auto end_comment = text.find("-->", lt + 4U);
                if (end_comment == std::string_view::npos) break;
                pos = end_comment + 3U;
                continue;
            }
            const auto gt = text.find('>', lt + 1U);
            if (gt == std::string_view::npos) break;
            const std::string_view tag_body = trimView(text.substr(lt + 1U, gt - lt - 1U));
            pos = gt + 1U;
            if (tag_body.empty() || tag_body.front() == '?' || tag_body.front() == '!') continue;
            if (tag_body.front() == '/') {
                active_profile = VendorProfile::Boot;
                continue;
            }

            // Parse XML attributes key="value" / key='value' inside this tag.
            std::string attr_path;
            std::string attr_val;
            std::string attr_default;
            std::string attr_boost;
            std::string attr_label;
            std::size_t i = 0;
            while (i < tag_body.size() && !std::isspace(static_cast<unsigned char>(tag_body[i])) &&
                   tag_body[i] != '/') {
                ++i;
            }
            const std::string tag_name = upper(tag_body.substr(0, i));
            while (i < tag_body.size()) {
                while (i < tag_body.size() &&
                       (std::isspace(static_cast<unsigned char>(tag_body[i])) || tag_body[i] == '/')) {
                    ++i;
                }
                const std::size_t kstart = i;
                while (i < tag_body.size() && tag_body[i] != '=' &&
                       !std::isspace(static_cast<unsigned char>(tag_body[i]))) {
                    ++i;
                }
                const std::string key = upper(tag_body.substr(kstart, i - kstart));
                while (i < tag_body.size() && std::isspace(static_cast<unsigned char>(tag_body[i]))) ++i;
                if (i >= tag_body.size() || tag_body[i] != '=') continue;
                ++i;
                while (i < tag_body.size() && std::isspace(static_cast<unsigned char>(tag_body[i]))) ++i;
                if (i >= tag_body.size() || (tag_body[i] != '"' && tag_body[i] != '\'')) continue;
                const char q = tag_body[i++];
                const std::size_t vstart = i;
                while (i < tag_body.size() && tag_body[i] != q) ++i;
                const std::string_view val = tag_body.substr(vstart, i - vstart);
                if (i < tag_body.size() && tag_body[i] == q) ++i;

                if (key == "NODE" || key == "PATH" || key == "SYSFS" || key == "SYSFSNODE" || key == "FILE") {
                    attr_path = std::string(val);
                } else if (key == "VALUE" || key == "PARAM" || key == "TARGETVALUE" ||
                           key == "MAXVALUE" || key == "PERFVALUE") {
                    attr_val = std::string(val);
                } else if (key == "BOOSTVALUE") {
                    attr_boost = std::string(val);
                } else if (key == "DEFAULTVALUE" || key == "DEFAULT" || key == "RESETVALUE") {
                    attr_default = std::string(val);
                } else if (key == "TYPE" || key == "NAME" || key == "SCENARIO" ||
                           key == "HINT" || key == "PROFILE" || key == "MODE") {
                    if (!attr_label.empty()) attr_label.push_back(' ');
                    attr_label.append(val);
                }
            }

            if (!attr_label.empty()) {
                const VendorProfile derived = profileFromLabel(attr_label);
                if (derived != VendorProfile::Boot) active_profile = derived;
            } else if (tag_name != "RESOURCECONFIGS" && tag_name != "PERFRESOURCES") {
                // Structural container names are not profile labels ("ECO" in
                // ResourceConfigs must not turn all children into efficiency hints).
                const VendorProfile tag_derived = profileFromLabel(tag_name);
                if (tag_derived != VendorProfile::Boot) active_profile = tag_derived;
            }

            if (!attr_path.empty()) {
                if (!attr_default.empty()) {
                    if (add(attr_path, attr_default, VendorProfile::Default, source)) ++added;
                }
                if (!attr_boost.empty() && add(attr_path, attr_boost, VendorProfile::Performance, source)) ++added;
                if (!attr_val.empty()) {
                    const VendorProfile p = active_profile == VendorProfile::Boot
                                                ? VendorProfile::Performance
                                                : active_profile;
                    if (add(attr_path, attr_val, p, source)) ++added;
                }
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
        enum class Kind { None, Rc, Shell, PowerHint, PerfXml } kind = Kind::None;
        if (endsWith(name, ".rc")) kind = Kind::Rc;
        else if (endsWith(name, ".sh")) kind = Kind::Shell;
        else if (endsWith(name, ".json")) {
            const std::string uname = upper(name);
            if (uname.find("POWERHINT") != std::string::npos ||
                uname.find("PERFHINT") != std::string::npos ||
                uname.find("ADPF") != std::string::npos ||
                uname.find("PERF") != std::string::npos ||
                uname.find("BOOST") != std::string::npos ||
                uname.find("GPU") != std::string::npos ||
                uname.find("POWER") != std::string::npos) {
                kind = Kind::PowerHint;
            }
        } else if (endsWith(name, ".xml")) {
            const std::string uname = upper(name);
            if (uname.find("PERF") != std::string::npos ||
                uname.find("BOOST") != std::string::npos ||
                uname.find("TARGETRESOURCE") != std::string::npos ||
                uname.find("POWER") != std::string::npos ||
                uname.find("SCN") != std::string::npos) {
                kind = Kind::PerfXml;
            }
        }
        if (kind == Kind::None) return 0;  // never read unrelated vendor files

        const std::string canonical = canonicalPath(path);
        if (!visited_.insert(canonical).second) return 0;

        struct stat st {};
        if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) ||
            static_cast<std::size_t>(st.st_size) > kMaxFileBytes) {
            return 0;
        }
        std::string text;
        // Bound the actual read too: stat size alone does not cap a growing file.
        if (!readBoundedPrivateFile(canonical, text)) return 0;
        ++files_parsed_;

        switch (kind) {
            case Kind::Rc:        return parseInitRc(text, path);
            case Kind::Shell:     return parseShell(text, path);
            case Kind::PowerHint: return parsePowerHintJson(text, path);
            case Kind::PerfXml:   return parseVendorPerfXml(text, path);
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
            if (scan_entries_ >= kMaxScanEntries) break;
            ++scan_entries_;
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
            "/vendor/etc/init", "/vendor/etc/powerhint.json", "/vendor/etc/perf",
            "/vendor/etc/pwr", "/vendor/etc",
            "/odm/etc/init", "/odm/etc/perf", "/odm/etc", "/vendor/odm/etc/init",
            "/system/etc/init/hw", "/system/etc/init", "/system_ext/etc/init",
            "/product/etc/init", "/vendor/bin",
        });
    } catch (...) {
        return knobs_.size();
    }
}

} // namespace coreflow
