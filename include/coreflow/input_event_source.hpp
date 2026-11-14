/**
 * @file input_event_source.hpp
 * @brief Native non-blocking Linux evdev touchscreen event producer.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/frame_bridge.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/unique_fd.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <linux/input.h>
#include <string>
#include <unordered_map>

namespace coreflow {

struct TouchInputState final {
    bool btn_touch{false};
    std::array<bool, 16> active_slots{};
    uint8_t current_slot{0};
    bool pending_activity{false};
    bool dropped_until_sync{false};
    uint64_t last_emitted_ns{0};
};

[[nodiscard]] bool is_touchscreen_capability(
    bool input_prop_direct,
    bool input_prop_pointer,
    bool has_abs_x,
    bool has_abs_y,
    bool has_btn_touch,
    bool has_mt_tracking_id) noexcept;

// Pure decoder used by the evdev source and deterministic host tests.
[[nodiscard]] bool consume_touch_input_event(
    TouchInputState& state,
    const input_event& event,
    uint64_t monotonic_now_ns,
    FrameEvent& output,
    uint64_t minimum_interval_ns = 50'000'000ULL) noexcept;

using TouchscreenDeviceProbe = bool (*)(int fd) noexcept;

class NativeInputEventSource final : public FrameEventSource {
public:
    explicit NativeInputEventSource(
        EpollReactor& reactor,
        std::string device_root = "/dev/input",
        TouchscreenDeviceProbe touchscreen_probe = nullptr) noexcept;
    ~NativeInputEventSource() override;

    NativeInputEventSource(const NativeInputEventSource&) = delete;
    NativeInputEventSource& operator=(const NativeInputEventSource&) = delete;

    [[nodiscard]] bool start(FrameEventListener listener) noexcept override;
    void stop() noexcept override;

    [[nodiscard]] bool active() const noexcept {
        return has_active_devices_.load(std::memory_order_acquire);
    }
    [[nodiscard]] size_t attached_devices() const noexcept { return devices_.size(); }

private:
    friend struct NativeInputEventSourceTestAccess;

    struct DeviceState final {
        UniqueFd fd;
        std::string path;
        TouchInputState touch{};
    };

    EpollReactor& reactor_;
    std::string device_root_;
    TouchscreenDeviceProbe touchscreen_probe_{nullptr};
    UniqueFd inotify_fd_;
    FrameEventListener listener_{};
    std::unordered_map<int, DeviceState> devices_;
    // Keep source lifecycle separate from instantaneous device availability.
    std::atomic<bool> running_{false};
    std::atomic<bool> has_active_devices_{false};

    [[nodiscard]] bool add_device(const std::string& path) noexcept;
    void remove_device(int fd) noexcept;
    void scan_devices() noexcept;
    void handle_device(int fd, uint32_t events) noexcept;
    void handle_inotify(uint32_t events) noexcept;
    void dispatch(const FrameEvent& event) noexcept;
};

} // namespace coreflow
