# CoreFlow Engine

Native C++ Runtime Tuning Engine for Android

![Android](https://img.shields.io/badge/Android-14%2B-green.svg)
![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

CoreFlow Engine is a C++17-based Android system tuning engine designed to dynamically adjust kernel parameters based on device conditions.

CoreFlow uses a runtime state management approach rather than applying a single permanent performance configuration. The engine monitors system conditions, determines the appropriate state, then applies tuning parameters available on the device.

**Project Status:** Active development  
**Target:** Android 14+ / ARM64

---

## Features

### Dynamic Runtime State Machine

CoreFlow classifies device conditions based on runtime metrics such as CPU activity, screen state, GPU utilization on devices providing KGSL interface, and temperature.

Primary states used:

| State | Purpose |
|-------|---------|
| Daily Efficiency | Normal usage profile with focus on system efficiency |
| App Launch / Burst | Handles short workload spikes such as app launching |
| Gaming Unleashed | Performance profile for heavier GPU/CPU workloads |
| Ultra Deep Sleep | Conservative profile when device is idle/screen-off |
| Thermal Guardian | Protection profile when device temperature reaches defined conditions |

The state machine is designed to prevent continuous parameter rewrites when the state has not changed.

### Runtime Tuning

CoreFlow can interact with available kernel interfaces through the Android filesystem, particularly:

- CPU frequency/governor interfaces
- CPU scheduler-related parameters
- Storage I/O parameters
- ZRAM / VM parameters
- GPU interfaces on devices exposing KGSL
- Thermal information
- Relevant Android system properties

Not all devices provide the same kernel nodes. Therefore, CoreFlow uses a capability-based tuning approach: parameters are only applied if the required interface is available and usable.

### Native Architecture

CoreFlow is built as a native C++ daemon and does not depend on shell commands for each tuning operation.

This approach allows the engine to interact directly with system interfaces through native APIs such as:

- `std::ifstream`
- `std::ofstream`
- Android/Bionic system property API
- filesystem and other system interfaces

The goal is to keep the implementation simple, controlled, and avoid dependency on shell script chains for each parameter change.

**Note:** CoreFlow still has runtime overhead from monitoring and evaluating system conditions. The design focus is to keep this overhead low, not to claim zero overhead.

### Device Adaptation

CoreFlow does not assume all devices have identical kernel structures.

When a node is available:

```
Node available
    ↓
Validate
    ↓
Capture current value
    ↓
Apply tuning
    ↓
Verify
```

If a node is unavailable or cannot be used:

```
Node unavailable
    ↓
Skip parameter
    ↓
Continue with available capabilities
```

This approach allows the engine to adapt to kernel variations and device configurations.

### GPU Detection

On devices with KGSL interface, CoreFlow can use GPU busy information to help determine GPU workload.

On devices that do not provide this interface, the engine can use other available metrics, such as CPU activity.

---

## Configuration Philosophy

CoreFlow maintains the following principle:

**Detect → Validate → Snapshot → Tune → Monitor → Restore**

1. **Detect**
   Identify available hardware and kernel interfaces.

2. **Validate**
   Ensure target parameters and nodes can be used before making changes.

3. **Snapshot**
   Save initial values of parameters to be modified.

4. **Tune**
   Apply configuration based on runtime state.

5. **Monitor**
   Observe workload and device condition changes.

6. **Restore**
   Return parameters to previous values when needed.

This pipeline is important because kernel configurations can differ across devices and kernel versions.

---

## Thermal Handling

CoreFlow includes Thermal Guardian to monitor temperature available from Android's thermal interface.

When temperature exceeds thresholds defined by engine configuration, CoreFlow can switch to thermal state and reduce tuning aggressiveness.

After conditions return to normal, the engine can return to the appropriate runtime state.

Thermal thresholds and behavior should be considered as engine configuration, not as a guarantee that the device will always maintain a certain temperature.

---

## Compatibility

| Component | Target |
|-----------|--------|
| Architecture | ARM64 / AArch64 |
| Android | Android 14+ |
| Language | C++17 |
| Root Framework | Magisk / KernelSU / APatch |
| GPU Telemetry | KGSL if available |

Actual compatibility depends on kernel, vendor implementation, exposed sysfs/procfs nodes, permission model, and device configuration.

Not all features are available on all devices.

---

## Installation

CoreFlow is distributed as a module for Android environments that support systemless modules.

1. Download the appropriate release from the Releases page.
2. Install the module using a compatible root manager.
3. Reboot the device if required by the release.
4. Check CoreFlow logs to ensure the daemon started successfully.

**Note:** Release file names may change with each version. Use the file available on the relevant release, not hardcoded filenames in documentation.

---

## Runtime Verification

Runtime logs can be used to check state transitions and engine activity.

Example:

```bash
su
logcat -s CoreFlowEngine
```

Observable items include:

- Runtime state changes
- Workload changes
- Thermal state
- Device capability detection
- Successfully applied parameters
- Parameters skipped due to unavailability

Log format and tags may change during development.

---

## Building

### Requirements

- Android NDK
- CMake
- C++17-compatible compiler

### Clone

```bash
git clone https://github.com/mystivara-creator/CoreFlow-Engine.git
cd CoreFlow-Engine
```

### Build

Build configuration may differ based on target release and development environment.

To build using CMake:

```bash
cmake -S . -B build
cmake --build build
```

The output binary will be in the build directory according to CMake configuration.

---

## Project Structure

```
CoreFlow-Engine/
├── include/
│   └── ...
├── src/
│   └── ...
├── module_template/
│   └── ...
├── CMakeLists.txt
├── LICENSE
├── CHANGELOG.md
├── CONTRIBUTING.md
└── README.md
```

Structure may change during development.

---

## Design Goals

CoreFlow is developed with several primary objectives:

- Runtime-aware tuning
- Device capability detection
- Graceful handling of missing kernel interfaces
- Minimal dependency on shell scripting
- State-based configuration
- Parameter snapshot and restore
- Modular C++ architecture
- Observable runtime behavior

CoreFlow does not aim to provide a single universal configuration considered optimal for every device.

Instead, the engine attempts to use parameters that are actually available on the running device.

---

## Limitations

CoreFlow interacts with kernel interfaces that may differ across:

- SoC
- Vendor
- Kernel version
- Custom kernel
- Android version
- Device configuration

Because of this, tuning results may differ across devices.

The absence of a kernel node does not mean the engine fails overall; features dependent on that node can be skipped while other features continue running.

---

## Disclaimer

CoreFlow modifies system parameters on devices with root access.

Using inappropriate kernel configurations can cause:

- Performance changes
- Increased power consumption
- Increased temperature
- System instability
- Parameters not working as expected

Use on devices you understand and always backup configurations/original state before experimenting.

**Use at your own risk.**

---

## Contributing

Bug reports, improvements, and pull requests are welcome.

When reporting issues, please include:

- Device model
- SoC
- Android version
- Kernel version
- Root framework
- Relevant runtime logs
- Problematic parameters/nodes

This information helps reproduce issues in different environments.

---

## License

CoreFlow Engine is released under the MIT License.

See [LICENSE](./LICENSE) for the complete license text.

---

## Author

**Mystivara**

GitHub: [@mystivara](https://github.com/mystivara-creator)

Project: CoreFlow Engine

---

**Note:** CoreFlow Engine is an experimental system-tuning project focused on adaptive runtime management for Android.
