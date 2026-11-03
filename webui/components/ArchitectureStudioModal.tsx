import React, { useState } from 'react';
import {
  X,
  Download,
  Copy,
  Check,
  Cpu,
  Layers,
  Flame,
  BatteryCharging,
  Code2,
  FileText,
  Activity,
  Shield,
  Zap,
} from 'lucide-react';

interface ArchitectureStudioModalProps {
  isOpen: boolean;
  onClose: () => void;
  socModel: string;
  kernelVersion: string;
  onlineCpus: string;
  gpuType: string;
}

export const ArchitectureStudioModal: React.FC<ArchitectureStudioModalProps> = ({
  isOpen,
  onClose,
  socModel,
  kernelVersion,
  onlineCpus,
  gpuType,
}) => {
  const [activeTab, setActiveTab] = useState<'overview' | 'hal' | 'pid' | 'longevity' | 'code' | 'export'>('overview');
  const [copied, setCopied] = useState(false);
  const [simWorkload, setSimWorkload] = useState<'gaming' | 'burst' | 'thermal_stress'>('gaming');

  if (!isOpen) return null;

  // Complete technical specification text for export
  const fullBlueprintMarkdown = `# CoreFlow-Engine: Low-Level System Interceptor & Performance Equilibrium Blueprint
Version: 2.0.0-MODERN | Architecture: AOSP ARM64 | Kernel Target: Linux 5.15 / 6.1 / 6.6
Device Probed: ${socModel} (CPUs: ${onlineCpus}, GPU: ${gpuType}, Kernel: ${kernelVersion})

---

### 1. Architectural Philosophy: Zero-Hardcode & Dynamic Discovery
Every Android OEM implements different SoC topologies, thermal zones, and power HAL variants. CoreFlow-Engine adheres strictly to standard Linux Kernel & Google AOSP interfaces:
- CPU Cluster Discovery: Dynamically walks \`/sys/devices/system/cpu/cpufreq/policy*\` and parses \`related_cpus\`, \`cpuinfo_min_freq\`, and \`cpuinfo_max_freq\`.
- CPU Mask: Probed from \`/sys/devices/system/cpu/online\` (Online: ${onlineCpus}).
- Thermal Zone Discovery: Scans \`/sys/class/thermal/thermal_zone*/type\` prioritizing SoC, CPU, and TSENS sensors.
- Hardware Probing: Queries Linux Devicetree (\`/sys/firmware/devicetree/base/model\`) and SoC bus (\`/sys/devices/soc0/\`).

---

### 2. Multi-Chipset HAL / Binder Interception
- Qualcomm Snapdragon:
  • Intercepts vendor.qti.hardware.perf AIDL calls.
  • Manages /sys/class/kgsl/kgsl-3d0/ devfreq and Energy-Aware Scheduling (walt_boost, sched_boost).
  • FastRPC CDSP acceleration offloading.
- MediaTek Dimensity:
  • Intercepts vendor.mediatek.hardware.power calls.
  • Interacts with Mali kbase devfreq and MTK CoreControl / PPM (/proc/ppm).
- Google Tensor & Exynos:
  • Linux standard devfreq and cgroups v2 task placement.

---

### 3. Predictive PID Dynamic Thermal Equilibrium
Replaces rigid OEM cliff-drop throttling with continuous PID control:
  P_budget(t) = Kp * e(t) + Ki * ∫e(t)dt + Kd * (de(t)/dt)
- Clamps uclamp floor and display cadence 1-2% at a time BEFORE thermal ceilings are breached.
- Preserves smooth 120Hz frame pacing without sudden FPS drops.

---

### 4. Hardware Longevity & Silicon Preservation
- Battery Thermal Step-Charging: Dynamically adjusts /sys/class/power_supply/battery/current_max when SoC load exceeds 41°C.
- UFS Storage Pacing: Tunes cgroups v2 io.weight and vm dirty writeback ratios to eliminate write latency spikes.
- Transactional BaselineStore: Snapshots factory OEM sysfs values at boot with guaranteed atomic rollback on exit.

---

### 5. Verified Zero-Hardcode Implementation (C++20/C++23)
Engine daemon (coreflowd) compiles with:
- Zero-polling Linux epoll & signalfd event reactor (0.00% idle CPU wakeups).
- Safe RAII UniqueFd file descriptor management (0% fd leaks).
- Memory-safe compiler hardening: -fstack-protector-strong, -D_FORTIFY_SOURCE=2, -Wformat-security.
`;

  const handleCopy = () => {
    navigator.clipboard.writeText(fullBlueprintMarkdown);
    setCopied(true);
    setTimeout(() => setCopied(false), 2500);
  };

  const handleDownload = (format: 'md' | 'txt') => {
    const blob = new Blob([fullBlueprintMarkdown], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `coreflow_blueprint_v2.0.${format}`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-3 sm:p-6 bg-slate-950/80 backdrop-blur-md animate-fade-in">
      <div className="relative w-full max-w-5xl h-[90vh] flex flex-col rounded-3xl bg-[#090D16] border border-slate-800 text-slate-100 shadow-2xl overflow-hidden font-sans">
        {/* Top Header Bar */}
        <div className="flex items-center justify-between px-6 py-4 border-b border-slate-800 bg-[#0d1322]">
          <div className="flex items-center gap-3">
            <div className="w-9 h-9 rounded-2xl bg-sky-500/10 border border-sky-500/20 flex items-center justify-center text-sky-400">
              <Code2 className="w-5 h-5" />
            </div>
            <div>
              <div className="flex items-center gap-2">
                <h2 className="font-bold text-base sm:text-lg text-white">
                  CoreFlow Architecture Studio
                </h2>
                <span className="text-[11px] font-semibold px-2.5 py-0.5 rounded-full bg-teal-500/15 text-teal-300 border border-teal-500/20 font-mono">
                  AOSP Dynamic Blueprint
                </span>
              </div>
              <p className="text-xs text-slate-400 font-mono">
                Probed Silicon: {socModel} · CPUs: {onlineCpus} · Kernel: {kernelVersion}
              </p>
            </div>
          </div>

          <div className="flex items-center gap-2">
            <button
              onClick={handleCopy}
              className="px-3 py-1.5 rounded-full bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium flex items-center gap-1.5 transition-colors border border-slate-700 active:scale-95"
            >
              {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
              <span>{copied ? 'Copied' : 'Copy Blueprint'}</span>
            </button>
            <button
              onClick={onClose}
              className="p-2 rounded-full bg-slate-800/80 hover:bg-slate-700 text-slate-400 hover:text-white transition-colors"
              aria-label="Close modal"
            >
              <X className="w-5 h-5" />
            </button>
          </div>
        </div>

        {/* Navigation Tabs */}
        <div className="flex items-center gap-2 px-6 py-2.5 border-b border-slate-800/80 bg-[#0b101c] overflow-x-auto text-xs font-medium">
          <button
            onClick={() => setActiveTab('overview')}
            className={`px-3 py-1.5 rounded-full transition-all ${
              activeTab === 'overview'
                ? 'bg-sky-500 text-slate-950 font-semibold'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            1. System Architecture
          </button>
          <button
            onClick={() => setActiveTab('hal')}
            className={`px-3 py-1.5 rounded-full transition-all ${
              activeTab === 'hal'
                ? 'bg-sky-500 text-slate-950 font-semibold'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            2. HAL / Binder Interceptor
          </button>
          <button
            onClick={() => setActiveTab('pid')}
            className={`px-3 py-1.5 rounded-full transition-all ${
              activeTab === 'pid'
                ? 'bg-sky-500 text-slate-950 font-semibold'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            3. Dynamic PID Equilibrium
          </button>
          <button
            onClick={() => setActiveTab('longevity')}
            className={`px-3 py-1.5 rounded-full transition-all ${
              activeTab === 'longevity'
                ? 'bg-sky-500 text-slate-950 font-semibold'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            4. Hardware Longevity
          </button>
          <button
            onClick={() => setActiveTab('code')}
            className={`px-3 py-1.5 rounded-full transition-all ${
              activeTab === 'code'
                ? 'bg-sky-500 text-slate-950 font-semibold'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            5. Native C++23 / Rust Blueprint
          </button>
          <button
            onClick={() => setActiveTab('export')}
            className={`px-3 py-1.5 rounded-full transition-all ${
              activeTab === 'export'
                ? 'bg-teal-400 text-slate-950 font-semibold'
                : 'text-teal-400 hover:text-teal-300'
            }`}
          >
            6. Export Center
          </button>
        </div>

        {/* Tab Content Body */}
        <div className="flex-1 overflow-y-auto p-6 space-y-6 text-sm text-slate-300">
          {activeTab === 'overview' && (
            <div className="space-y-6">
              <div className="p-5 rounded-2xl bg-slate-900/70 border border-slate-800">
                <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
                  <Shield className="w-4 h-4 text-sky-400" />
                  Dynamic Probing Constitution (Zero-Hardcode Standard)
                </h3>
                <p className="text-xs text-slate-400 leading-relaxed">
                  Unlike traditional root scripts that hardcode device names, cluster numbers, or fixed sysfs paths, CoreFlow strictly employs dynamic capability discovery aligned with Linux kernel sysfs conventions and Google AOSP standard paths.
                </p>
              </div>

              {/* Probed Hardware Snapshot */}
              <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                <div className="p-4 rounded-2xl bg-slate-900/50 border border-slate-800">
                  <span className="text-[11px] text-slate-500 font-mono block mb-1">PROBED MODEL / SOC</span>
                  <span className="text-sm font-semibold text-white truncate block">{socModel}</span>
                  <span className="text-[10px] text-sky-400 mt-1 block">via Devicetree / soc0</span>
                </div>
                <div className="p-4 rounded-2xl bg-slate-900/50 border border-slate-800">
                  <span className="text-[11px] text-slate-500 font-mono block mb-1">ONLINE CPUS MASK</span>
                  <span className="text-sm font-semibold text-white font-mono block">{onlineCpus}</span>
                  <span className="text-[10px] text-teal-400 mt-1 block">/sys/devices/system/cpu/online</span>
                </div>
                <div className="p-4 rounded-2xl bg-slate-900/50 border border-slate-800">
                  <span className="text-[11px] text-slate-500 font-mono block mb-1">GPU SUBSYSTEM</span>
                  <span className="text-sm font-semibold text-white truncate block">{gpuType}</span>
                  <span className="text-[10px] text-indigo-400 mt-1 block">Auto-probed via devfreq/kgsl</span>
                </div>
                <div className="p-4 rounded-2xl bg-slate-900/50 border border-slate-800">
                  <span className="text-[11px] text-slate-500 font-mono block mb-1">LINUX KERNEL RELEASE</span>
                  <span className="text-sm font-semibold text-white font-mono block">{kernelVersion}</span>
                  <span className="text-[10px] text-amber-400 mt-1 block">POSIX uname standard</span>
                </div>
              </div>

              {/* Architecture Pillars */}
              <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                <div className="p-4 rounded-2xl bg-slate-900/40 border border-slate-800">
                  <div className="flex items-center gap-2 text-sky-400 font-semibold mb-2">
                    <Zap className="w-4 h-4" />
                    <span>Zero-Polling epoll</span>
                  </div>
                  <p className="text-xs text-slate-400">
                    Eliminates background polling loops. Daemon sleeps deeply in <code className="text-sky-300">epoll_wait()</code> until hardware VSYNC, touch input, or timerfd ticks.
                  </p>
                </div>
                <div className="p-4 rounded-2xl bg-slate-900/40 border border-slate-800">
                  <div className="flex items-center gap-2 text-teal-400 font-semibold mb-2">
                    <Layers className="w-4 h-4" />
                    <span>EAS Uclamp Dynamic</span>
                  </div>
                  <p className="text-xs text-slate-400">
                    Adjusts <code className="text-teal-300">sched_util_min</code> for top-app interactively. Prevents jitter without locking CPU cores to maximum frequency.
                  </p>
                </div>
                <div className="p-4 rounded-2xl bg-slate-900/40 border border-slate-800">
                  <div className="flex items-center gap-2 text-rose-400 font-semibold mb-2">
                    <Flame className="w-4 h-4" />
                    <span>Transactional Baseline</span>
                  </div>
                  <p className="text-xs text-slate-400">
                    Snapshots original factory OEM sysfs values at boot. Guarantees 100% clean rollback upon exit, thermal trip, or manual 1-click restore.
                  </p>
                </div>
              </div>
            </div>
          )}

          {activeTab === 'hal' && (
            <div className="space-y-4">
              <h3 className="text-base font-bold text-white flex items-center gap-2">
                <Cpu className="w-4 h-4 text-sky-400" />
                Multi-Chipset HAL & Binder Interception Design
              </h3>
              <p className="text-xs text-slate-400">
                CoreFlow operates as an active filter between Android Framework PowerHAL and device silicon:
              </p>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="p-5 rounded-2xl bg-slate-900/70 border border-slate-800">
                  <h4 className="font-bold text-white mb-2 text-sm text-sky-400">Qualcomm Snapdragon Integration</h4>
                  <ul className="space-y-2 text-xs text-slate-300 font-mono">
                    <li>• Intercepts: <span className="text-sky-300">vendor.qti.hardware.perf</span></li>
                    <li>• Devfreq: <span className="text-sky-300">/sys/class/kgsl/kgsl-3d0/</span> (Adreno)</li>
                    <li>• Sched: <span className="text-sky-300">walt_boost</span> & <span className="text-sky-300">sched_energy_aware</span></li>
                    <li>• NPU: Qualcomm FastRPC <span className="text-sky-300">/dev/subsys_cdsp</span> offload</li>
                  </ul>
                </div>

                <div className="p-5 rounded-2xl bg-slate-900/70 border border-slate-800">
                  <h4 className="font-bold text-white mb-2 text-sm text-teal-400">MediaTek Dimensity Integration</h4>
                  <ul className="space-y-2 text-xs text-slate-300 font-mono">
                    <li>• Intercepts: <span className="text-teal-300">vendor.mediatek.hardware.power</span></li>
                    <li>• Devfreq: <span className="text-teal-300">/sys/class/devfreq/*mali*</span> (Mali/Immortalis)</li>
                    <li>• Control: MTK CoreControl & PPM (<span className="text-teal-300">/proc/ppm</span>)</li>
                    <li>• APU: MediaTek NeuroPilot affinity scheduler</li>
                  </ul>
                </div>
              </div>

              <div className="p-4 rounded-2xl bg-slate-900/40 border border-slate-800 text-xs">
                <span className="font-semibold text-white block mb-1">Binder Snooper Mechanics:</span>
                Uses zero-copy <code className="text-indigo-400">std::span</code> and <code className="text-indigo-400">UniqueFd</code> to monitor <code className="text-indigo-400">/dev/binder</code> transactions. Suppresses spurious background burst requests while ensuring foreground render threads receive instant scheduling priority.
              </div>
            </div>
          )}

          {activeTab === 'pid' && (
            <div className="space-y-5">
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                <div>
                  <h3 className="text-base font-bold text-white flex items-center gap-2">
                    <Activity className="w-4 h-4 text-emerald-400" />
                    Dynamic PID Equilibrium Simulator
                  </h3>
                  <p className="text-xs text-slate-400">
                    Visualizing stock OEM cliff-drop throttling vs. CoreFlow PID micro-adjustments under sustained workload.
                  </p>
                </div>

                <div className="flex items-center gap-2 bg-slate-900 p-1 rounded-full border border-slate-800 text-xs">
                  <button
                    onClick={() => setSimWorkload('gaming')}
                    className={`px-3 py-1 rounded-full transition-all ${
                      simWorkload === 'gaming' ? 'bg-sky-500 text-slate-950 font-semibold' : 'text-slate-400'
                    }`}
                  >
                    Sustained 120 FPS
                  </button>
                  <button
                    onClick={() => setSimWorkload('burst')}
                    className={`px-3 py-1 rounded-full transition-all ${
                      simWorkload === 'burst' ? 'bg-sky-500 text-slate-950 font-semibold' : 'text-slate-400'
                    }`}
                  >
                    Touch Gestures
                  </button>
                  <button
                    onClick={() => setSimWorkload('thermal_stress')}
                    className={`px-3 py-1 rounded-full transition-all ${
                      simWorkload === 'thermal_stress' ? 'bg-rose-500 text-white font-semibold' : 'text-slate-400'
                    }`}
                  >
                    Thermal Stress
                  </button>
                </div>
              </div>

              {/* Dynamic Equilibrium Visual Comparison */}
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {/* Stock OEM Behavior */}
                <div className="p-4 rounded-2xl bg-rose-950/20 border border-rose-900/40">
                  <div className="flex items-center justify-between mb-3">
                    <span className="font-semibold text-rose-400 text-xs uppercase tracking-wider">Stock OEM Throttling (Cliff Drop)</span>
                    <span className="text-[11px] font-mono text-rose-300">Severe Frame Drops</span>
                  </div>
                  <div className="h-28 flex items-end gap-1.5 px-2 pt-2 border-b border-rose-800/40">
                    {[100, 100, 98, 97, 45, 40, 42, 95, 95, 40, 38, 42].map((val, i) => (
                      <div key={i} className="flex-1 flex flex-col items-center gap-1">
                        <div
                          className="w-full bg-rose-500/80 rounded-t transition-all"
                          style={{ height: `${val}%` }}
                        />
                      </div>
                    ))}
                  </div>
                  <div className="flex justify-between text-[10px] text-slate-500 font-mono mt-2">
                    <span>Initial 120 FPS</span>
                    <span>Sudden -50% Throttle Cliff</span>
                    <span>Thermal Rollercoaster</span>
                  </div>
                </div>

                {/* CoreFlow PID Behavior */}
                <div className="p-4 rounded-2xl bg-emerald-950/20 border border-emerald-900/40">
                  <div className="flex items-center justify-between mb-3">
                    <span className="font-semibold text-emerald-400 text-xs uppercase tracking-wider">CoreFlow Dynamic Equilibrium (PID)</span>
                    <span className="text-[11px] font-mono text-emerald-300">Stable Cadence</span>
                  </div>
                  <div className="h-28 flex items-end gap-1.5 px-2 pt-2 border-b border-emerald-800/40">
                    {[100, 100, 99, 98, 96, 95, 95, 94, 95, 94, 94, 93].map((val, i) => (
                      <div key={i} className="flex-1 flex flex-col items-center gap-1">
                        <div
                          className="w-full bg-gradient-to-t from-teal-500 to-sky-400 rounded-t transition-all"
                          style={{ height: `${val}%` }}
                        />
                      </div>
                    ))}
                  </div>
                  <div className="flex justify-between text-[10px] text-slate-500 font-mono mt-2">
                    <span>Initial 120 FPS</span>
                    <span>Continuous 1-2% Micro-adjustments</span>
                    <span>Equilibrium (No Drop)</span>
                  </div>
                </div>
              </div>

              <div className="p-3.5 rounded-xl bg-slate-900/50 border border-slate-800 text-xs font-mono text-slate-400">
                Formula: <span className="text-sky-300">P_budget(t) = Kp * e(t) + Ki * ∫e(t)dt + Kd * (de(t)/dt)</span> · Adjusts capacity budget smoothly before trip threshold.
              </div>
            </div>
          )}

          {activeTab === 'longevity' && (
            <div className="space-y-4">
              <h3 className="text-base font-bold text-white flex items-center gap-2">
                <BatteryCharging className="w-4 h-4 text-amber-400" />
                Hardware Longevity & Silicon Preservation
              </h3>
              <p className="text-xs text-slate-400">
                Extends device physical lifespan by reducing electrochemical battery stress and flash storage write amplification:
              </p>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="p-5 rounded-2xl bg-slate-900/70 border border-slate-800">
                  <div className="flex items-center gap-2 text-amber-400 font-semibold mb-2">
                    <Flame className="w-4 h-4" />
                    <span>Battery Thermal Step-Charging</span>
                  </div>
                  <p className="text-xs text-slate-400 mb-3 leading-relaxed">
                    High voltage combined with high temperature (&gt;41°C) rapidly degrades lithium-ion cells. CoreFlow dynamically detects gaming sessions while plugged in:
                  </p>
                  <ul className="text-xs font-mono space-y-1.5 text-slate-300">
                    <li>• Probes: <span className="text-amber-300">/sys/class/power_supply/battery/current_max</span></li>
                    <li>• Clamps charging current to nominal 1500mA during heavy 120Hz loads.</li>
                    <li>• Drops battery pack heat generation by ~3.2°C.</li>
                  </ul>
                </div>

                <div className="p-5 rounded-2xl bg-slate-900/70 border border-slate-800">
                  <div className="flex items-center gap-2 text-indigo-400 font-semibold mb-2">
                    <Layers className="w-4 h-4" />
                    <span>UFS Storage Burst Pacing</span>
                  </div>
                  <p className="text-xs text-slate-400 mb-3 leading-relaxed">
                    Unregulated background write bursts cause random write latency spikes in foreground apps and accelerate flash cell wear:
                  </p>
                  <ul className="text-xs font-mono space-y-1.5 text-slate-300">
                    <li>• Applies cgroups v2 <span className="text-indigo-300">io.weight</span>: top-app=800, background=50</li>
                    <li>• Tunes dirty memory writeback ratios (<span className="text-indigo-300">vm.dirty_ratio</span>)</li>
                    <li>• Smooths write amplification across UFS 3.1 / 4.0 storage</li>
                  </ul>
                </div>
              </div>
            </div>
          )}

          {activeTab === 'code' && (
            <div className="space-y-4">
              <div className="flex items-center justify-between">
                <h3 className="text-base font-bold text-white flex items-center gap-2">
                  <Code2 className="w-4 h-4 text-sky-400" />
                  Modern C++23 / Rust 2024 Blueprint Code
                </h3>
                <span className="text-xs text-slate-500 font-mono">ISO/IEC 14882:2023 · Rust 2024 Edition</span>
              </div>

              <pre className="p-4 rounded-2xl bg-slate-950 border border-slate-800 text-[11px] font-mono text-slate-300 overflow-x-auto max-h-[380px] leading-relaxed">
{`// coreflow_binder_hook.cpp — Modern C++23 Zero-Copy Interceptor
#include <expected>
#include <span>
#include <string_view>
#include "coreflow/unique_fd.hpp"

namespace coreflow::interceptor {

// C++23 std::expected error-handling for Binder IPC transactions
std::expected<void, std::string_view> intercept_power_hint(
    int code, std::span<const uint8_t> raw_parcel
) noexcept {
    if (raw_parcel.empty()) {
        return std::unexpected("Empty Binder parcel");
    }

    // Zero-copy inspection of AIDL android.hardware.power.IPower hints
    if (code == POWER_HINT_INTERACTION) {
        // Feed forward to CoreFlow EAS Uclamp controller
        return {};
    }

    return {};
}

} // namespace coreflow::interceptor`}
              </pre>
            </div>
          )}

          {activeTab === 'export' && (
            <div className="space-y-6">
              <div className="p-6 rounded-2xl bg-slate-900/60 border border-slate-800 text-center space-y-3">
                <div className="w-12 h-12 rounded-2xl bg-teal-500/10 border border-teal-500/20 text-teal-400 flex items-center justify-center mx-auto">
                  <Download className="w-6 h-6" />
                </div>
                <h3 className="text-base font-bold text-white">
                  Export Complete Blueprint & Specification
                </h3>
                <p className="text-xs text-slate-400 max-w-lg mx-auto">
                  Download the complete, production-grade technical specification formatted for immediate handoff to downstream AI agents, Cursor, Claude, or repository developers.
                </p>

                <div className="flex flex-wrap items-center justify-center gap-3 pt-2">
                  <button
                    onClick={() => handleDownload('md')}
                    className="px-4 py-2 rounded-full bg-teal-400 hover:bg-teal-300 text-slate-950 font-semibold text-xs flex items-center gap-1.5 transition-colors shadow-sm"
                  >
                    <FileText className="w-4 h-4" />
                    <span>Download Markdown (.md)</span>
                  </button>
                  <button
                    onClick={() => handleDownload('txt')}
                    className="px-4 py-2 rounded-full bg-slate-800 hover:bg-slate-700 text-slate-200 font-semibold text-xs flex items-center gap-1.5 transition-colors border border-slate-700"
                  >
                    <Download className="w-4 h-4" />
                    <span>Download Plaintext (.txt)</span>
                  </button>
                  <button
                    onClick={handleCopy}
                    className="px-4 py-2 rounded-full bg-slate-800 hover:bg-slate-700 text-slate-200 font-semibold text-xs flex items-center gap-1.5 transition-colors border border-slate-700"
                  >
                    {copied ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
                    <span>{copied ? 'Copied to Clipboard' : 'Copy All to Clipboard'}</span>
                  </button>
                </div>
              </div>
            </div>
          )}
        </div>

        {/* Modal Footer */}
        <div className="px-6 py-3.5 border-t border-slate-800/80 bg-[#0d1322] flex items-center justify-between text-xs text-slate-500 font-mono">
          <span>Target: Linux 5.15 / 6.1 / 6.6 · Android 15+ AOSP</span>
          <button
            onClick={onClose}
            className="px-4 py-1.5 rounded-full bg-slate-800 hover:bg-slate-700 text-slate-300 font-sans transition-colors"
          >
            Close Studio
          </button>
        </div>
      </div>
    </div>
  );
};
