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

## Cross-device thermal policy

Raw CPU/GPU junction, battery and skin temperatures are separate diagnostics.
They never share a universal 44/48-degree cutoff. Linux thermal-zone values are
converted from the documented millidegree ABI, without a magnitude heuristic.
BCL/SoC-droop channels and invalid negative/sentinel values are excluded.

Production decisions prefer the public Android NDK thermal severity API. Optional
headroom is sampled no faster than once per ten seconds. Moderate/severe thermal
pressure reduces demand; emergency/shutdown severity yields control with rollback.
If the API is unavailable, each classified zone is evaluated against its own
read-only OEM passive/hot/critical trip points. Only normalized policy-relative
pressure is compared across sensors; no manufacturer/model table is used.

Missing OEM thermal policy retains the stock baseline and reports unavailable;
it does not invent a cool measurement or reuse legacy Celsius settings. Old
thermal_trip_celsius/recover/emergency-margin and pid_target_celsius values no
longer control production sensors. WebUI shows OEM/Auto in native mode; Celsius
sliders remain browser-preview controls only. No OEM trip or thermal service is
modified. This supports capability-based adaptation, not a claim that every
vendor exposes usable policy interfaces.

## Predictive controller

Rust consumes a dimensionless thermal-policy pressure index, not raw Celsius.
Its PID includes a bounded integral and pressure velocity. Its output is bounded
to 0.4–1.0 and changes by at most two percentage points per elapsed second (with a
ten-point upper bound per delayed sample). CPU/GPU values are quantized to exposed
operating points: a frequency step can exceed 2%. This is not an FPS controller.
Pressure guard and thermal hysteresis remain active. Per-zone OEM hot/critical
trips or Android emergency/shutdown status trigger verified rollback and exit.

## Hardware introspection and preservation

CPU policies are discovered from sysfs; GPU discovery accepts only GPU/Mali/KGSL
identities under devfreq. Missing tables disable that domain. No invented clock,
cluster, or nominal temperature is used. Existing OEM maximums are upper bounds.

Optional charging reduction uses the standard battery constant_charge_current_max
microamp interface, uses only battery temperature for charging decisions and validates charging state, and never
raises the observed charging limit. A changed OEM value retires ownership.

Optional cgroup-v2 io.weight uses the documented `default N` format. It is relative
I/O scheduling priority, not a bandwidth limiter, UFS wear measurement or guarantee
of lower write amplification. Global VM writeback ratios are not modified.

## Power HAL, Binder and NPU integration

The Android binary performs a nonblocking lookup of
android.hardware.power.IPower/default through optional runtime symbols from libbinder_ndk. Public NDK builds do not
depend on private service-manager headers; missing symbols leave the capability
unavailable. Device access and
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
