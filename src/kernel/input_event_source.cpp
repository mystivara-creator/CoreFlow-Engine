/**
 * @file input_event_source.cpp
 * @brief Non-blocking evdev touchscreen discovery and event normalization.
 * @license Apache-2.0
 */
#include "coreflow/input_event_source.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {

constexpr size_t kWordBits = sizeof(unsigned long) * CHAR_BIT;
constexpr size_t kEventBits = (EV_MAX + kWordBits) / kWordBits;
constexpr size_t kPropertyBits = (INPUT_PROP_MAX + kWordBits) / kWordBits;
constexpr size_t kKeyBits = (KEY_MAX + kWordBits) / kWordBits;
constexpr size_t kAbsBits = (ABS_MAX + kWordBits) / kWordBits;
constexpr size_t kReadEvents = 32U;

bool bit_is_set(const unsigned long* bits, size_t code) noexcept {
    return (bits[code / kWordBits] & (1UL << (code % kWordBits))) != 0UL;
}

uint64_t monotonic_now_ns() noexcept {
    timespec value{};
    if (::clock_gettime(CLOCK_MONOTONIC, &value) != 0 ||
        value.tv_sec < 0 || value.tv_nsec < 0) return 0;
    constexpr uint64_t kBillion = 1'000'000'000ULL;
    const uint64_t seconds = static_cast<uint64_t>(value.tv_sec);
    const uint64_t nanos = static_cast<uint64_t>(value.tv_nsec);
    if (seconds > (UINT64_MAX - nanos) / kBillion) return 0;
    return seconds * kBillion + nanos;
}

bool is_event_node(const char* name) noexcept {
    if (name == nullptr || std::strncmp(name, "event", 5) != 0 || name[5] == '\0') return false;
    for (const char* cursor = name + 5; *cursor != '\0'; ++cursor) {
        if (*cursor < '0' || *cursor > '9') return false;
    }
    return true;
}

bool query_touchscreen(int fd) noexcept {
    std::array<unsigned long, kEventBits> event_bits{};
    std::array<unsigned long, kPropertyBits> property_bits{};
    std::array<unsigned long, kKeyBits> key_bits{};
    std::array<unsigned long, kAbsBits> abs_bits{};

    if (::ioctl(fd, EVIOCGBIT(0, sizeof(event_bits)), event_bits.data()) < 0) return false;
    if (!bit_is_set(event_bits.data(), EV_ABS)) return false;
    if (::ioctl(fd, EVIOCGPROP(sizeof(property_bits)), property_bits.data()) < 0) {
        property_bits.fill(0UL);
    }
    if (bit_is_set(event_bits.data(), EV_KEY)) {
        (void)::ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits.data());
    }
    (void)::ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits.data());

    const bool direct = bit_is_set(property_bits.data(), INPUT_PROP_DIRECT);
    const bool pointer = bit_is_set(property_bits.data(), INPUT_PROP_POINTER);
    const bool has_btn_touch = bit_is_set(key_bits.data(), BTN_TOUCH);
    const bool has_mt_tracking = bit_is_set(abs_bits.data(), ABS_MT_TRACKING_ID);
    const bool has_abs_x = bit_is_set(abs_bits.data(), ABS_X) ||
        bit_is_set(abs_bits.data(), ABS_MT_POSITION_X);
    const bool has_abs_y = bit_is_set(abs_bits.data(), ABS_Y) ||
        bit_is_set(abs_bits.data(), ABS_MT_POSITION_Y);
    return is_touchscreen_capability(
        direct, pointer, has_abs_x, has_abs_y, has_btn_touch, has_mt_tracking);
}

} // namespace

bool is_touchscreen_capability(
    bool direct,
    bool pointer,
    bool has_abs_x,
    bool has_abs_y,
    bool has_btn_touch,
    bool has_mt_tracking_id) noexcept {
    if (!has_abs_x || !has_abs_y || (!has_btn_touch && !has_mt_tracking_id)) return false;
    // Pointer-only devices such as trackpads/mice are not touchscreen producers.
    // Older panels without INPUT_PROP_DIRECT are admitted only with the full
    // absolute-coordinate and contact-reporting contract.
    if (pointer && !direct) return false;
    return direct || has_btn_touch || has_mt_tracking_id;
}

bool consume_touch_input_event(
    TouchInputState& state,
    const input_event& event,
    uint64_t now_ns,
    FrameEvent& output,
    uint64_t minimum_interval_ns) noexcept {
    if (event.type == EV_SYN && event.code == SYN_DROPPED) {
        state.btn_touch = false;
        state.active_slots.fill(false);
        state.pending_activity = false;
        state.dropped_until_sync = true;
        return false;
    }
    if (state.dropped_until_sync) {
        if (event.type == EV_SYN && event.code == SYN_REPORT) state.dropped_until_sync = false;
        return false;
    }

    const bool was_active = state.btn_touch ||
        std::any_of(state.active_slots.begin(), state.active_slots.end(), [](bool value) { return value; });

    if (event.type == EV_KEY && event.code == BTN_TOUCH) {
        state.btn_touch = event.value != 0;
        if (!was_active && state.btn_touch) state.pending_activity = true;
    } else if (event.type == EV_ABS) {
        if (event.code == ABS_MT_SLOT) {
            state.current_slot = static_cast<uint8_t>(
                std::clamp(event.value, 0, static_cast<int>(state.active_slots.size() - 1U)));
        } else if (event.code == ABS_MT_TRACKING_ID) {
            const size_t slot = state.current_slot;
            if (event.value < 0) {
                state.active_slots[slot] = false;
            } else {
                state.active_slots[slot] = true;
                if (!was_active) state.pending_activity = true;
            }
        } else if (event.code == ABS_X || event.code == ABS_Y ||
                   event.code == ABS_MT_POSITION_X || event.code == ABS_MT_POSITION_Y) {
            const bool active = state.btn_touch ||
                std::any_of(state.active_slots.begin(), state.active_slots.end(), [](bool value) { return value; });
            if (active) state.pending_activity = true;
        }
    } else if (event.type == EV_SYN && event.code == SYN_REPORT) {
        const bool active = state.btn_touch ||
            std::any_of(state.active_slots.begin(), state.active_slots.end(), [](bool value) { return value; });
        if (!active) {
            state.pending_activity = false;
            return false;
        }
        if (!state.pending_activity || now_ns == 0) return false;
        if (state.last_emitted_ns != 0 && now_ns >= state.last_emitted_ns &&
            now_ns - state.last_emitted_ns < minimum_interval_ns) return false;

        output = FrameEvent{
            .kind = FrameEventKind::TouchBurst,
            .vsync_timestamp_ns = now_ns,
            .frame_duration_ns = 0,
            .jank_threshold_ns = 0,
            .burst_duration_ms = 0,
        };
        state.last_emitted_ns = now_ns;
        state.pending_activity = false;
        return true;
    }
    return false;
}

NativeInputEventSource::NativeInputEventSource(
    EpollReactor& reactor,
    std::string device_root,
    TouchscreenDeviceProbe touchscreen_probe) noexcept
    : reactor_(reactor),
      device_root_(std::move(device_root)),
      touchscreen_probe_(touchscreen_probe) {}

NativeInputEventSource::~NativeInputEventSource() {
    stop();
}

bool NativeInputEventSource::start(FrameEventListener listener) noexcept {
    if (!listener) return false;
    if (running_.load(std::memory_order_acquire)) return true;
    listener_ = std::move(listener);

    inotify_fd_.reset(::inotify_init1(IN_NONBLOCK | IN_CLOEXEC));
    if (inotify_fd_.valid()) {
        const int watch = ::inotify_add_watch(
            inotify_fd_.get(), device_root_.c_str(),
            IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB);
        if (watch >= 0) {
            if (!reactor_.register_fd(inotify_fd_.get(), EPOLLIN | EPOLLERR | EPOLLHUP,
                    [this](uint32_t events) { handle_inotify(events); })) {
                inotify_fd_.reset();
            }
        } else {
            inotify_fd_.reset();
        }
    }

    // An empty startup directory is not a terminal condition: retain the
    // inotify watch so late-probed or re-created touchscreen nodes are seen.
    scan_devices();
    const bool can_run = inotify_fd_.valid() ||
        has_active_devices_.load(std::memory_order_acquire);
    running_.store(can_run, std::memory_order_release);
    if (!can_run) {
        listener_ = {};
        return false;
    }
    return true;
}

void NativeInputEventSource::stop() noexcept {
    running_.store(false, std::memory_order_release);
    if (inotify_fd_.valid()) {
        (void)reactor_.unregister_fd(inotify_fd_.get());
        inotify_fd_.reset();
    }
    while (!devices_.empty()) remove_device(devices_.begin()->first);
    listener_ = {};
    has_active_devices_.store(false, std::memory_order_release);
}

bool NativeInputEventSource::add_device(const std::string& path) noexcept {
    const int access_mode = touchscreen_probe_ != nullptr ? O_RDWR : O_RDONLY;
    UniqueFd fd(::open(path.c_str(),
        access_mode | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW));
    if (!fd.valid()) return false;
    const bool touchscreen = touchscreen_probe_ != nullptr
        ? touchscreen_probe_(fd.get())
        : query_touchscreen(fd.get());
    if (!touchscreen) return false;
    const int raw_fd = fd.get();
    DeviceState device;
    device.fd = std::move(fd);
    device.path = path;
    const auto inserted = devices_.emplace(raw_fd, std::move(device));
    if (!inserted.second) return false;
    if (!reactor_.register_fd(raw_fd, EPOLLIN | EPOLLERR | EPOLLHUP,
            [this, raw_fd](uint32_t events) { handle_device(raw_fd, events); })) {
        devices_.erase(raw_fd);
        return false;
    }
    return true;
}

void NativeInputEventSource::remove_device(int fd) noexcept {
    const auto it = devices_.find(fd);
    if (it == devices_.end()) return;
    (void)reactor_.unregister_fd(fd);
    devices_.erase(it);
    has_active_devices_.store(!devices_.empty(), std::memory_order_release);
}

void NativeInputEventSource::scan_devices() noexcept {
    DIR* directory = ::opendir(device_root_.c_str());
    if (directory == nullptr) {
        has_active_devices_.store(!devices_.empty(), std::memory_order_release);
        return;
    }
    while (dirent* entry = ::readdir(directory)) {
        if (!is_event_node(entry->d_name)) continue;
        const std::string path = device_root_ + "/" + entry->d_name;
        bool known = false;
        for (const auto& pair : devices_) {
            if (pair.second.path == path) { known = true; break; }
        }
        if (!known) (void)add_device(path);
    }
    ::closedir(directory);

    for (auto it = devices_.begin(); it != devices_.end();) {
        struct stat st{};
        const bool valid_node = ::lstat(it->second.path.c_str(), &st) == 0 &&
            (S_ISCHR(st.st_mode) ||
             (touchscreen_probe_ != nullptr && S_ISFIFO(st.st_mode)));
        if (!valid_node) {
            const int fd = it->first;
            ++it;
            remove_device(fd);
        } else {
            ++it;
        }
    }
    has_active_devices_.store(!devices_.empty(), std::memory_order_release);
}

void NativeInputEventSource::handle_inotify(uint32_t events) noexcept {
    if ((events & (EPOLLERR | EPOLLHUP)) != 0U) {
        if (inotify_fd_.valid()) {
            (void)reactor_.unregister_fd(inotify_fd_.get());
            inotify_fd_.reset();
        }
        return;
    }
    if (!inotify_fd_.valid()) return;
    std::array<char, 2048> buffer{};
    for (;;) {
        const ssize_t count = ::read(inotify_fd_.get(), buffer.data(), buffer.size());
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
    }
    scan_devices();
}

void NativeInputEventSource::dispatch(const FrameEvent& event) noexcept {
    // Keep dispatch independent of device count. The inotify callback remains
    // registered and can repopulate devices_ while the directory is empty.
    if (running_.load(std::memory_order_acquire) && listener_) listener_(event);
}

void NativeInputEventSource::handle_device(int fd, uint32_t events) noexcept {
    auto it = devices_.find(fd);
    if (it == devices_.end()) return;
    if ((events & (EPOLLERR | EPOLLHUP)) != 0U) {
        remove_device(fd);
        return;
    }

    std::array<input_event, kReadEvents> input_events{};
    for (;;) {
        const ssize_t bytes = ::read(fd, input_events.data(), sizeof(input_events));
        if (bytes < 0 && errno == EINTR) continue;
        if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (bytes <= 0 || (static_cast<size_t>(bytes) % sizeof(input_event)) != 0U) {
            remove_device(fd);
            return;
        }
        const size_t count = static_cast<size_t>(bytes) / sizeof(input_event);
        for (size_t index = 0; index < count; ++index) {
            FrameEvent normalized{};
            if (consume_touch_input_event(
                    it->second.touch, input_events[index], monotonic_now_ns(), normalized)) {
                dispatch(normalized);
            }
        }
    }
}

} // namespace coreflow
