#pragma once
#include <expected>
#include <string_view>

namespace coreflow {
struct PowerCapabilities {
    bool binder_device{false};
    bool hwbinder_device{false};
    bool aidl_power_service{false};
    bool interception_active{false};
};
// Introspection only. Opening Binder cannot interpose another process's transactions.
class PowerBridge {
public:
    PowerCapabilities probe() const;
    [[nodiscard]] std::expected<void, std::string_view> enable_interception() const;
};
}
