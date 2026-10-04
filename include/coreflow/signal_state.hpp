#pragma once

namespace coreflow::signal_state {

void install() noexcept;
bool stopRequested() noexcept;
bool consumeRefresh() noexcept;

} // namespace coreflow::signal_state
