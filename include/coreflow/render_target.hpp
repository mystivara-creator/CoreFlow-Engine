#pragma once

#include <string>
#include <string_view>

namespace coreflow::observe {
// Layer names are diagnostic strings, not stable Android IDs. Select only an
// unambiguous package-qualified activity surface; never guess among windows.
inline std::string selectRenderLayer(std::string_view list, std::string_view package) {
    if (package.empty() || package.size() > 255U || list.size() > 256U * 1024U) return {};
    for (char c : package)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '.')) return {};
    std::string selected;
    while (!list.empty()) {
        auto end = list.find('\n');
        auto line = list.substr(0, end);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        auto pos = line.find(package);
        const bool left = pos != std::string_view::npos &&
            (pos == 0U || line[pos - 1U] == '[' || line[pos - 1U] == ' ');
        const bool right = left && pos + package.size() < line.size() &&
            line[pos + package.size()] == '/';
        if (right && line.size() <= 1024U) {
            if (!selected.empty() && selected != line) return {}; // ambiguous
            selected.assign(line);
        }
        if (end == std::string_view::npos) break;
        list.remove_prefix(end + 1U);
    }
    return selected;
}
} // namespace coreflow::observe
