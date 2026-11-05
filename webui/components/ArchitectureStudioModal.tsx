import React, { useMemo, useState } from 'react';
import { X, Copy, Download, Activity, Cpu, Code2, ShieldCheck } from 'lucide-react';
import blueprint from '../../docs/EQUILIBRIUM_BLUEPRINT.md?raw';
import rustController from '../../rust/src/controller.rs?raw';
import rustBridge from '../../rust/src/bridge.rs?raw';
import autonomousTuner from '../../src/kernel/autonomous_tuner.cpp?raw';
import neuralPredictor from '../../src/kernel/neural_predictor.cpp?raw';
import cppController from '../../src/kernel/frequency_controller.cpp?raw';
import powerBridge from '../../src/kernel/power_bridge.cpp?raw';

interface ArchitectureStudioModalProps {
  isOpen: boolean;
  onClose: () => void;
  socModel: string;
  kernelVersion: string;
  onlineCpus: string;
  gpuType: string;
}

const tabs = [
  'Architecture',
  'Mutation Safety',
  'Risk & Provenance',
  'HAL / Binder',
  'Thermal Simulator',
  'C++23 / Rust 2024',
  'Export',
] as const;
type Tab = (typeof tabs)[number];

export const ArchitectureStudioModal: React.FC<ArchitectureStudioModalProps> = (props) => {
  const [tab, setTab] = useState<Tab>('Architecture');
  const [load, setLoad] = useState(75);
  const [ambient, setAmbient] = useState(30);
  const [copied, setCopied] = useState(false);
  const [copyError, setCopyError] = useState('');
  const [source, setSource] = useState('Autonomous tuner');

  const sources: Record<string, string> = {
    'Autonomous tuner': autonomousTuner,
    'Neural predictor': neuralPredictor,
    'Rust controller': rustController,
    'cxx bridge': rustBridge,
    'CPU/GPU controller': cppController,
    'Power HAL probe': powerBridge,
  };

  const simulation = useMemo(() => {
    let temperature = ambient;
    let integral = 0;
    let previous = temperature;
    let scale = 1;
    const samples: { temperature: number; scale: number }[] = [];
    for (let second = 0; second <= 120; second++) {
      const error = 42.5 - temperature;
      integral = Math.max(-50, Math.min(0, integral + error));
      const velocity = second === 0 ? 0 : temperature - previous;
      const target = Math.max(
        0.4,
        Math.min(1, 1 + (2.5 * error + 0.15 * integral - 1.2 * velocity) / 50)
      );
      scale += Math.max(-0.02, Math.min(0.02, target - scale));
      previous = temperature;
      temperature += (load / 100) * scale * 0.5 - (temperature - ambient) * 0.025;
      samples.push({ temperature, scale });
    }
    return samples;
  }, [load, ambient]);

  if (!props.isOpen) return null;

  const fullText =
    blueprint +
    `\n\n## CFAE System Framework v3.1.0\n## Current Hardware\nSoC: ${props.socModel}\nKernel: ${props.kernelVersion}\nCPUs: ${props.onlineCpus}\nGPU: ${props.gpuType}\n`;

  const copy = async () => {
    try {
      await navigator.clipboard.writeText(fullText);
      setCopied(true);
      setCopyError('');
    } catch {
      setCopyError('Clipboard unavailable. Use Download.');
    }
  };

  const download = (extension: string) => {
    const url = URL.createObjectURL(new Blob([fullText], { type: 'text/plain;charset=utf-8' }));
    const anchor = document.createElement('a');
    anchor.href = url;
    anchor.download = `cfae_framework_blueprint.${extension}`;
    document.body.appendChild(anchor);
    anchor.click();
    anchor.remove();
    window.setTimeout(() => URL.revokeObjectURL(url), 1000);
  };

  const temperaturePoints = simulation
    .map((s, i) => `${i * 5},${200 - (s.temperature - 20) * 4}`)
    .join(' ');
  const capacityPoints = simulation.map((s, i) => `${i * 5},${200 - s.scale * 160}`).join(' ');

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-3 sm:p-6 bg-slate-950/55 backdrop-blur-2xl m3-stage">
      <div
        role="dialog"
        aria-modal="true"
        aria-label="CFAE System Framework Architecture Studio"
        className="relative w-full max-w-5xl h-[88vh] flex flex-col m3-card text-slate-900 dark:text-slate-100 shadow-2xl overflow-hidden"
      >
        <header className="px-6 py-5 border-b border-slate-900/[0.06] dark:border-white/[0.06] flex justify-between items-center gap-3">
          <div>
            <h2 className="font-semibold text-lg text-slate-900 dark:text-white">
              CFAE Architecture & Mutation Safety Studio
            </h2>
            <p className="text-xs text-[#0b57d0] dark:text-[#a8c7fa] font-mono">
              v3.1.0 · C++23 + Rust 2024 + Kotlin · Coordinated Mutation Transactions
            </p>
          </div>
          <button
            onClick={props.onClose}
            aria-label="Close Studio"
            className="w-9 h-9 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.1] dark:bg-white/[0.06] dark:hover:bg-white/[0.12] flex items-center justify-center text-slate-600 dark:text-slate-300"
          >
            <X size={18} />
          </button>
        </header>

        <nav
          className="flex flex-wrap gap-1.5 px-6 py-3 border-b border-slate-900/[0.06] dark:border-white/[0.06]"
          aria-label="Studio sections"
        >
          {tabs.map((item) => (
            <button
              key={item}
              onClick={() => setTab(item)}
              className={
                'px-3.5 py-1.5 rounded-full text-xs font-medium transition-all ' +
                (item === tab
                  ? 'bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold'
                  : 'bg-slate-900/[0.04] dark:bg-white/[0.05] text-slate-600 dark:text-slate-300 hover:bg-slate-900/[0.08]')
              }
            >
              {item}
            </button>
          ))}
        </nav>

        <div className="flex-1 min-h-0 overflow-auto p-6 sm:p-8 space-y-6 text-sm">
          {tab === 'Architecture' && (
            <>
              <div className="space-y-2">
                <h3 className="font-semibold text-base flex items-center gap-2">
                  <Cpu size={18} className="text-[#0b57d0] dark:text-[#a8c7fa]" />
                  Multi-Language Native Architecture
                </h3>
                <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
                  Sensor & Capability Discovery → State Normalization → Reasoning Engine → Dependency Analyzer → Mutation Planner → Safety Admission Gate → Transaction Coordinator (Apply Order + Ownership Guard) → Readback Verify → Outcome Evaluator → Retain / Rollback → Outcome Learning.
                </p>
              </div>

              <div className="grid sm:grid-cols-2 gap-4">
                {[
                  [
                    'ISO C++23 Core Runtime',
                    'Zero-polling epoll/timerfd/signalfd loop, semantic cgroup v2 & sysfs discovery, O_NOFOLLOW + fstat identity verification, and fsync write-ahead journal.',
                  ],
                  [
                    'Rust 2024 Decision Engine',
                    'Memory-safe PID thermal controller, hysteresis state machine, PSI contention arbitration, and bounded capacity scaling via cxx bridge.',
                  ],
                  [
                    'Kotlin 2.1 Android Bridge',
                    'Android 15 (API 35) Quick Settings TileService and Battery thermal broadcast integration.',
                  ],
                  [
                    'Material 3 WebUI Workbench',
                    'Lightweight offline KernelSU/APatch dashboard displaying live runtime status or labeled preview telemetry.',
                  ],
                ].map(([title, text]) => (
                  <div key={title} className="m3-tonal-block p-5 space-y-1.5">
                    <h4 className="text-xs sm:text-sm font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                      {title}
                    </h4>
                    <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">
                      {text}
                    </p>
                  </div>
                ))}
              </div>

              <div className="m3-tonal-block p-5 font-mono text-xs space-y-1.5 text-slate-700 dark:text-slate-300">
                <div>SoC: {props.socModel}</div>
                <div>Kernel: {props.kernelVersion}</div>
                <div>Online CPUs: {props.onlineCpus}</div>
                <div>GPU: {props.gpuType}</div>
              </div>
            </>
          )}

          {tab === 'Mutation Safety' && (
            <>
              <div className="space-y-2">
                <h3 className="font-semibold text-base flex items-center gap-2">
                  <ShieldCheck size={18} className="text-[#0b57d0] dark:text-[#a8c7fa]" />
                  Coordinated Optimization & 5-Layer Mutation Safety
                </h3>
                <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
                  The primary unit of control is the <strong>Mutation Transaction</strong>: one explicit optimization goal, a dependency-verified change plan within a bounded risk budget, per-write readback verification, and ownership-safe rollback.
                </p>
              </div>

              <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-4">
                {[
                  [
                    '1. Reasoning Engine',
                    'Synthesizes workload observations, CPU/Memory/I/O PSI, thermal headroom, and power state into an inspectable optimization intent.',
                  ],
                  [
                    '2. Dependency Analyzer',
                    'Builds a provenance-backed dependency graph (Required-before, Requires, Coupled-with, Conflicts-with, Unknown). Unknown edges are never grouped.',
                  ],
                  [
                    '3. Mutation Planner',
                    'Constructs a bounded transaction plan specifying baseline, target, allowed range source, execution order, risk budget, and abort conditions.',
                  ],
                  [
                    '4. Safety Admission Gate',
                    'Verifies 10 pre-write invariants: object identity, semantic units, valid baseline, zero ownership conflict, and OEM thermal compliance.',
                  ],
                  [
                    '5. Coordinator & Readback',
                    'Serializes conflicting transactions, writes the durable journal prior to mutation, applies changes in dependency order, and verifies readback.',
                  ],
                  [
                    '6. Outcome Learning',
                    'Evaluates Stability, Responsiveness, Pressure, Thermal, and Power outcomes (IMPROVED, NEUTRAL, REGRESSED, OWNERSHIP_LOST) with anti-repeat deferral.',
                  ],
                ].map(([t, d]) => (
                  <div key={t} className="m3-tonal-block p-4 space-y-1.5">
                    <h4 className="text-xs font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">{t}</h4>
                    <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">{d}</p>
                  </div>
                ))}
              </div>

              <div className="m3-tonal-block p-5 text-xs space-y-2">
                <div className="font-semibold text-slate-900 dark:text-slate-100">
                  12 Non-Negotiable Safety Invariants:
                </div>
                <p className="text-slate-600 dark:text-slate-400 leading-relaxed">
                  1. Zero write without valid baseline · 2. Zero mutation without known semantics & range · 3. Zero grouped mutation without verified relationship · 4. No concurrent conflicting transactions · 5. Mandatory readback after every write · 6. Rollback never overwrites external OEM/Android changes · 7. No blind retries without new evidence · 8. Retention requires valid outcome & ownership · 9. No unverified metric claims · 10. OEM/Android thermal policy remains supreme · 11. Write-ahead journal & idempotent recovery · 12. Fail-safe default is no write.
                </p>
              </div>
            </>
          )}

          {tab === 'Risk & Provenance' && (
            <>
              <div className="space-y-2">
                <h3 className="font-semibold text-base">
                  Multi-Dimensional Risk Model & Dependency Provenance
                </h3>
                <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
                  CoreFlow evaluates seven distinct risk dimensions rather than a single opaque score: Semantic uncertainty, Platform uncertainty, Interaction risk, Reversibility, System impact, Observability, and Ownership confidence.
                </p>
              </div>

              <div className="grid sm:grid-cols-2 gap-4">
                {[
                  ['Low Risk Class', 'Strong semantics, bounded impact, reversible, zero conflict, high observability. Eligible for coupled mutation groups within budget.'],
                  ['Medium Risk Class', 'Limited interaction or platform uncertainty with understood dependencies and verified rollback. Applied step-by-step in one transaction.'],
                  ['High Risk Class', 'Broad impact, uncertain platform semantics, or weak observability. Strictly isolated or rejected; never grouped with other changes.'],
                  ['Forbidden Class', 'Missing baseline, unsafe rollback path, ownership conflict, or violates OEM thermal/power safety policy. Never mutated.'],
                ].map(([cls, policy]) => (
                  <div key={cls} className="m3-tonal-block p-4 space-y-1">
                    <div className="text-xs font-semibold text-slate-900 dark:text-slate-100">{cls}</div>
                    <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">{policy}</p>
                  </div>
                ))}
              </div>

              <div className="m3-tonal-block p-5 space-y-2 text-xs">
                <div className="font-semibold text-slate-900 dark:text-slate-100">
                  Transaction State Machine Protocol
                </div>
                <div className="font-mono text-[#0b57d0] dark:text-[#a8c7fa] leading-relaxed">
                  PROPOSED → ADMISSION_CHECK → PREPARED → APPLYING → READBACK_VERIFY → OBSERVING → COMMITTED → RETAINED
                </div>
                <p className="text-slate-600 dark:text-slate-400 leading-relaxed">
                  Failed admission transitions to DEFERRED / REJECTED. Readback mismatch or observed regression transitions immediately to ROLLING_BACK under Ownership Guard verification.
                </p>
              </div>
            </>
          )}

          {tab === 'HAL / Binder' && (
            <>
              <h3 className="font-semibold text-base">Platform Integration & OEM Boundaries</h3>
              <div className="m3-tonal-block overflow-hidden">
                <table className="w-full text-xs text-left">
                  <thead>
                    <tr className="border-b border-slate-900/[0.06] dark:border-white/[0.06]">
                      <th className="p-4 font-semibold">Capability</th>
                      <th className="p-4 font-semibold">Implementation Status</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]">
                    {[
                      ['AIDL Power HAL discovery', 'Nonblocking libbinder_ndk lookup; capability detection only.'],
                      ['Qualcomm / MediaTek interception', 'Unavailable: requires a registered, versioned OEM/AOSP proxy backend.'],
                      ['Binder device access', 'Does not expose or reroute other processes’ transactions.'],
                      ['NPU / APU offload', 'Unavailable: requires a compatible compute workload and runtime.'],
                    ].map(([a, b]) => (
                      <tr key={a}>
                        <td className="p-4 font-medium text-[#0b57d0] dark:text-[#a8c7fa]">{a}</td>
                        <td className="p-4 text-slate-600 dark:text-slate-400">{b}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
              <p className="text-xs text-slate-500 dark:text-slate-400 leading-relaxed">
                OEM thermal services remain authoritative. The daemon never replaces Power HAL services, patches Binder, or injects undocumented vendor transactions.
              </p>
            </>
          )}

          {tab === 'Thermal Simulator' && (
            <>
              <h3 className="font-semibold text-base flex items-center gap-2">
                <Activity size={18} className="text-[#0b57d0] dark:text-[#a8c7fa]" />
                Illustrative Thermal Equilibrium Plant
              </h3>
              <p className="text-xs text-slate-600 dark:text-slate-400">
                Synthetic first-order thermal plant for previewing PID slew bounding. Separate from native device sensor telemetry.
              </p>
              <div className="grid sm:grid-cols-2 gap-4">
                <label className="m3-tonal-block p-4 block text-xs">
                  <span className="font-medium">Workload Intensity: {load}%</span>
                  <input
                    aria-label="Simulated workload"
                    className="block w-full mt-2"
                    type="range"
                    min="10"
                    max="100"
                    value={load}
                    onChange={(e) => setLoad(Number(e.target.value))}
                  />
                </label>
                <label className="m3-tonal-block p-4 block text-xs">
                  <span className="font-medium">Ambient Temperature: {ambient} °C</span>
                  <input
                    aria-label="Simulated ambient temperature"
                    className="block w-full mt-2"
                    type="range"
                    min="20"
                    max="40"
                    value={ambient}
                    onChange={(e) => setAmbient(Number(e.target.value))}
                  />
                </label>
              </div>
              <svg
                viewBox="0 0 620 235"
                role="img"
                aria-label="Temperature and capacity scale over 120 simulated seconds"
                className="w-full m3-tonal-block p-2"
              >
                <line x1="10" y1="200" x2="610" y2="200" stroke="#64748b" strokeOpacity="0.4" />
                <polyline
                  points={temperaturePoints}
                  fill="none"
                  stroke="#f59e0b"
                  strokeWidth="2.2"
                  transform="translate(10,0)"
                />
                <polyline
                  points={capacityPoints}
                  fill="none"
                  stroke="#0b57d0"
                  strokeWidth="2.2"
                  transform="translate(10,0)"
                />
                <text x="12" y="222" fill="#64748b" fontSize="11">
                  0 s
                </text>
                <text x="565" y="222" fill="#64748b" fontSize="11">
                  120 s
                </text>
              </svg>
              <div className="flex flex-wrap items-center justify-between gap-2 text-xs font-mono">
                <span>
                  Final: {simulation[120].temperature.toFixed(1)} °C ·{' '}
                  {(simulation[120].scale * 100).toFixed(0)}% capacity budget
                </span>
                <span className="text-slate-500">
                  Amber: Temperature (20–70 °C) · Blue: Capacity Scale (40–100%)
                </span>
              </div>
            </>
          )}

          {tab === 'C++23 / Rust 2024' && (
            <>
              <div className="flex items-center justify-between gap-3">
                <h3 className="font-semibold text-base flex items-center gap-2">
                  <Code2 size={18} className="text-[#0b57d0] dark:text-[#a8c7fa]" />
                  Production Source Inspector
                </h3>
                <select
                  aria-label="Source file"
                  value={source}
                  onChange={(e) => setSource(e.target.value)}
                  className="px-4 py-1.5 rounded-full text-xs font-medium bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]"
                >
                  {Object.keys(sources).map((name) => (
                    <option key={name}>{name}</option>
                  ))}
                </select>
              </div>
              <pre className="overflow-auto text-xs font-mono p-5 m3-tonal-block whitespace-pre max-h-96 leading-relaxed">
                {sources[source]}
              </pre>
            </>
          )}

          {tab === 'Export' && (
            <>
              <h3 className="font-semibold text-base">Specification & Blueprint Export</h3>
              <div className="flex gap-2.5 flex-wrap">
                <button
                  className="inline-flex items-center gap-2 bg-[#0b57d0] dark:bg-[#a8c7fa] text-white dark:text-[#062e6f] px-5 py-2 rounded-full text-xs font-medium"
                  onClick={() => void copy()}
                >
                  <Copy size={14} />
                  {copied ? 'Copied' : 'Copy Blueprint'}
                </button>
                {['md', 'txt'].map((ext) => (
                  <button
                    key={ext}
                    className="inline-flex items-center gap-2 bg-slate-900/[0.05] dark:bg-white/[0.06] px-4 py-2 rounded-full text-xs font-medium"
                    onClick={() => download(ext)}
                  >
                    <Download size={14} />
                    Download .{ext}
                  </button>
                ))}
              </div>
              {copyError && (
                <p role="alert" className="text-xs text-amber-600 dark:text-amber-400">
                  {copyError}
                </p>
              )}
              <pre className="whitespace-pre-wrap font-mono text-xs text-slate-600 dark:text-slate-300 m3-tonal-block p-5 max-h-96 overflow-auto leading-relaxed">
                {fullText}
              </pre>
            </>
          )}
        </div>
      </div>
    </div>
  );
};
