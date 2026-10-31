#pragma once
#include <string>
namespace coreflow {
// Bounded, same-directory atomic replacement with private mode and file/directory fsync.
bool writeDurablePrivateFile(const std::string& path, const std::string& content) noexcept;
bool readBoundedPrivateFile(const std::string& path, std::string& content) noexcept;
}
