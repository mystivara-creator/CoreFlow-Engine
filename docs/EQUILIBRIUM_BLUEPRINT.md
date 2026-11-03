# CoreFlow C++23 + Rust 2024 equilibrium blueprint

## Runtime architecture

C++23 handles discovery, Linux epoll/timerfd/signalfd, journaled kernel writes,
Android service discovery, and status JSON. The Rust 2024 controller owns thermal
PID state, hysteresis, pressure decisions, invalid-input rejection and a bounded
capacity budget. The cxx bridge shares typed snapshots and decisions.

1. Discover trusted CPU/SoC temperatures and semantic top-app UClamp.
2. Discover CPUFreq policies and GPU devfreq domains with readable frequency tables.
3. Sample temperature and CPU/memory/I/O PSI at the configured timerfd cadence.
4. Rust validates samples, prioritizes thermal and resource pressure, and computes a capacity scale.
5. Apply verified UClamp and supported CPU/GPU maximum frequency changes through BaselineStore.
6. Respect external OEM writes, emergency thermal exit, operator markers, and rollback on failure.

## Predictive thermal controller

Error = target temperature - measured temperature. The controller uses proportional
error, a bounded negative integral, and temperature velocity. The first sample
has zero derivative. Its output is bounded to 0.4–1.0 and changes by at most two
percentage points per elapsed second (with a ten-point upper bound per delayed
sample). CPU/GPU values are quantized to exposed operating points: a frequency
step can exceed 2%. This is a thermal controller, not an FPS controller.

Threshold thermal guard has hysteresis. Memory/I/O pressure takes priority over
CPU boosting. Missing or invalid telemetry never becomes a low-pressure or cool
sample. Emergency overheating restores tracked changes and stops the daemon.

## Hardware introspection and preservation

CPU policies are discovered from sysfs; GPU discovery accepts only GPU/Mali/KGSL
identities under devfreq. Missing tables disable that domain. No invented clock,
cluster, or nominal temperature is used. Existing OEM maximums are upper bounds.

Optional charging reduction uses the standard battery constant_charge_current_max
microamp interface, validates battery temperature and charging state, and never
raises the observed charging limit. A changed OEM value retires ownership.

Optional cgroup-v2 io.weight uses the documented `default N` format. It is relative
I/O scheduling priority, not a bandwidth limiter, UFS wear measurement or guarantee
of lower write amplification. Global VM writeback ratios are not modified.

## Power HAL, Binder and NPU integration

The Android binary performs a nonblocking lookup of
android.hardware.power.IPower/default through libbinder_ndk. Device access and
service availability are capabilities, not proof of an installed interceptor.

Transparent interception of existing framework/vendor Binder transactions is
UNAVAILABLE in this module. A production proxy needs an explicit OEM/AOSP service
integration, generated versioned AIDL/HIDL bindings, service registration,
SELinux permissions, death handling, and pass-through validation. Merely opening
/dev/binder does not route other processes through this daemon.

Generic application work is not offloaded to Hexagon or MediaTek APU. NPU work
requires a supported compute/model runtime, compatible workload and explicit
application integration. Firmware thermal controllers remain active. No voltage,
energy-model table, raw vendor ioctl, or forced display-mode override is installed.

## Workbench and simulator

The Architecture Studio displays this specification and exports Markdown/Text.
Its thermal model is an illustrative simulation, separate from native telemetry.
Hardware identity is fetched from coreflowd --capabilities; unavailable data stays
unavailable. Browser preview uses labeled synthetic data only.

## Verification and release limits

Host coverage includes Rust input/state tests, the real C++/Rust cxx bridge,
CPU/GPU ownership and rollback fixtures, existing UClamp and journal recovery
regressions, configuration parsing and thermal discovery. CI also builds Android
ARM64 with the NDK. Passing CI is not device validation.

Measure frame-time distribution, temperature, energy use, idle wakeups and daemon
CPU time on the target firmware. There is no claim of zero overhead, guaranteed
120 Hz, eliminated stutter, extended component life or universal chipset support.
