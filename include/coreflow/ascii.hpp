#pragma once
#include <string_view>

namespace coreflow::ascii {
constexpr char lower(char c) noexcept {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
}
constexpr bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (lower(a[i]) != lower(b[i])) return false;
    return true;
}
constexpr bool containsIgnoreCase(std::string_view text, std::string_view token) noexcept {
    if (token.size() > text.size()) return false;
    for (std::size_t i = 0; i <= text.size() - token.size(); ++i)
        if (equalsIgnoreCase(text.substr(i, token.size()), token)) return true;
    return false;
}
} // namespace coreflow::ascii
