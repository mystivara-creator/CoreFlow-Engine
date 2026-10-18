#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace coreflow::process {

// Result of a bounded external command. A command never blocks the caller for
// longer than its timeout plus a short reap, and its output is capped.
struct BoundedResult final {
    bool started{false};      // the child was created
    bool timed_out{false};    // killed because the timeout elapsed
    bool truncated{false};    // output reached the cap; the child was killed
    int exit_status{-1};      // valid when the child exited on its own; -1 otherwise
    std::string output;       // stdout, at most max_output_bytes
};

// Runs argv[0] with the remaining argv entries. Rules:
//   - no shell: the program is executed directly, so no shell syntax is interpreted;
//   - argv[0] must be an absolute path: there is no PATH lookup;
//   - stderr goes to /dev/null, stdin is /dev/null;
//   - the child is killed when the timeout elapses or the output cap is reached.
// Returns started=false for invalid arguments or when the child cannot be created.
[[nodiscard]] BoundedResult runBounded(const std::vector<std::string>& argv,
                                       std::uint32_t timeout_ms,
                                       std::size_t max_output_bytes) noexcept;

} // namespace coreflow::process
