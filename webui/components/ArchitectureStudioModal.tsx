import React, { useEffect, useMemo, useState } from 'react';
import { X, Copy, Download, Activity, Cpu, Code2, ShieldCheck, Layers } from 'lucide-react';
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

  const { isOpen, onClose } = props;
  useEffect(() => {
    if (!isOpen) return;
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') onClose();
    };
    window.addEventListener('keydown', onKeyDown);
    return () => window.removeEventListener('keydown', onKeyDown);
  }, [isOpen, onClose]);

  useEffect(() => {
    if (!copied) return;
    const timer = window.setTimeout(() => setCopied(false), 2000);
    return () => window.clearTimeout(timer);
  }, [copied]);

  if (!isOpen) return null;

  const fullText =
    blueprint +
    `\n\n## CFAE System Framework v4.0.0\n## Current Hardware\nSoC: ${props.socModel}\nKernel: ${props.kernelVersion}\nCPUs: ${props.onlineCpus}\nGPU: ${props.gpuType}\n`;

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
    <div
      className="fixed inset-0 z-50 flex items-center justify-center p-3 sm:p-6 backdrop-blur-xl md3-page-stage"
      style={{ background: 'color-mix(in srgb, var(--md-sys-color-scrim) 55%, transparent)' }}
    >
      <div
        role="dialog"
        aria-modal="true"
        aria-label="CFAE System Framework Architecture Studio"
        className="relative w-full max-w-5xl h-[88vh] flex flex-col md3-card-elevated overflow-hidden"
        style={{
          background: 'var(--md-sys-color-surface-container-high)',
          border: '1px solid var(--md-sys-color-outline-variant)',
        }}
      >
        <header
          className="px-6 py-4 flex justify-between items-center gap-3"
          style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
        >
          <div className="flex items-center gap-3">
            <div
              className="w-10 h-10 rounded-2xl flex items-center justify-center shrink-0"
              style={{
                background: 'var(--md-sys-color-primary-container)',
                color: 'var(--md-sys-color-on-primary-container)',
              }}
            >
              <Layers size={19} />
            </div>
            <div>
              <h2
                className="font-semibold text-base sm:text-lg tracking-tight"
                style={{ color: 'var(--md-sys-color-on-surface)' }}
              >
                CFAE Architecture & Mutation Safety Studio
              </h2>
              <p
                className="text-xs font-mono"
                style={{ color: 'var(--md-sys-color-primary)' }}
              >
                v4.0.0 · C++23 + Rust 2024 + Kotlin · Coordinated Mutation Transactions
              </p>
            </div>
          </div>
          <button
            onClick={props.onClose}
            aria-label="Close Studio"
            className="md3-icon-btn"
          >
            <X size={18} />
          </button>
        </header>

        <nav
          className="flex flex-wrap gap-1.5 px-6 py-3"
          style={{
            borderBottom: '1px solid var(--md-sys-color-outline-variant)',
            background: 'var(--md-sys-color-surface-container)',
          }}
          aria-label="Studio sections"
        >
          {tabs.map((item) => (
            <button
              key={item}
              onClick={() => setTab(item)}
              className={'md3-chip ' + (item === tab ? 'md3-chip-active' : '')}
            >
              {item}
            </button>
          ))}
        </nav>

        <div className="flex-1 min-h-0 overflow-auto p-6 sm:p-8 space-y-6 text-sm">
          {tab === 'Architecture' && (
            <>
              <div className="space-y-2">
                <h3
                  className="font-semibold text-base flex items-center gap-2"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  <Cpu size={18} style={{ color: 'var(--md-sys-color-primary)' }} />
                  Multi-Language Native Architecture
                </h3>
                <p
                  className="text-xs sm:text-sm leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
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
                    'Material 3 Expressive WebUI',
                    'Offline KernelSU/APatch workbench displaying live runtime status or clearly labeled preview telemetry with dynamic seed palettes.',
                  ],
                ].map(([title, text]) => (
                  <div key={title} className="md3-card-filled p-5 space-y-1.5">
                    <h4
                      className="text-xs sm:text-sm font-semibold"
                      style={{ color: 'var(--md-sys-color-primary)' }}
                    >
                      {title}
                    </h4>
                    <p
                      className="text-xs leading-relaxed"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      {text}
                    </p>
                  </div>
                ))}
              </div>

              <div
                className="md3-card-outlined p-5 font-mono text-xs space-y-1.5"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
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
                <h3
                  className="font-semibold text-base flex items-center gap-2"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  <ShieldCheck size={18} style={{ color: 'var(--md-sys-color-primary)' }} />
                  Coordinated Optimization & 5-Layer Mutation Safety
                </h3>
                <p
                  className="text-xs sm:text-sm leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
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
                  <div key={t} className="md3-card-filled p-4 space-y-1.5">
                    <h4
                      className="text-xs font-semibold"
                      style={{ color: 'var(--md-sys-color-primary)' }}
                    >
                      {t}
                    </h4>
                    <p
                      className="text-xs leading-relaxed"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      {d}
                    </p>
                  </div>
                ))}
              </div>

              <div className="md3-card-outlined p-5 text-xs space-y-2">
                <div
                  className="font-semibold"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  12 Non-Negotiable Safety Invariants:
                </div>
                <p
                  className="leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  1. Zero write without valid baseline · 2. Zero mutation without known semantics & range · 3. Zero grouped mutation without verified relationship · 4. No concurrent conflicting transactions · 5. Mandatory readback after every write · 6. Rollback never overwrites external OEM/Android changes · 7. No blind retries without new evidence · 8. Retention requires valid outcome & ownership · 9. No unverified metric claims · 10. OEM/Android thermal policy remains supreme · 11. Write-ahead journal & idempotent recovery · 12. Fail-safe default is no write.
                </p>
              </div>
            </>
          )}

          {tab === 'Risk & Provenance' && (
            <>
              <div className="space-y-2">
                <h3
                  className="font-semibold text-base"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  Multi-Dimensional Risk Model & Dependency Provenance
                </h3>
                <p
                  className="text-xs sm:text-sm leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
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
                  <div key={cls} className="md3-card-filled p-4 space-y-1">
                    <div
                      className="text-xs font-semibold"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      {cls}
                    </div>
                    <p
                      className="text-xs leading-relaxed"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      {policy}
                    </p>
                  </div>
                ))}
              </div>

              <div className="md3-card-outlined p-5 space-y-2 text-xs">
                <div
                  className="font-semibold"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  Transaction State Machine Protocol
                </div>
                <div
                  className="font-mono leading-relaxed"
                  style={{ color: 'var(--md-sys-color-primary)' }}
                >
                  PROPOSED → ADMISSION_CHECK → PREPARED → APPLYING → READBACK_VERIFY → OBSERVING → COMMITTED → RETAINED
                </div>
                <p
                  className="leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Failed admission transitions to DEFERRED / REJECTED. Readback mismatch or observed regression transitions immediately to ROLLING_BACK under Ownership Guard verification.
                </p>
              </div>
            </>
          )}

          {tab === 'HAL / Binder' && (
            <>
              <h3
                className="font-semibold text-base"
                style={{ color: 'var(--md-sys-color-on-surface)' }}
              >
                Platform Integration & OEM Boundaries
              </h3>
              <div className="md3-card-filled overflow-hidden">
                <table className="w-full text-xs text-left">
                  <thead>
                    <tr style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}>
                      <th className="p-4 font-semibold">Capability</th>
                      <th className="p-4 font-semibold">Implementation Status</th>
                    </tr>
                  </thead>
                  <tbody>
                    {[
                      ['AIDL Power HAL discovery', 'Nonblocking libbinder_ndk lookup; capability detection only.'],
                      ['Qualcomm / MediaTek interception', 'Unavailable: requires a registered, versioned OEM/AOSP proxy backend.'],
                      ['Binder device access', 'Does not expose or reroute other processes’ transactions.'],
                      ['NPU / APU offload', 'Unavailable: requires a compatible compute workload and runtime.'],
                    ].map(([a, b]) => (
                      <tr
                        key={a}
                        style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
                      >
                        <td
                          className="p-4 font-medium"
                          style={{ color: 'var(--md-sys-color-primary)' }}
                        >
                          {a}
                        </td>
                        <td
                          className="p-4"
                          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                        >
                          {b}
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
              <p
                className="text-xs leading-relaxed"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                OEM thermal services remain authoritative. The daemon never replaces Power HAL services, patches Binder, or injects undocumented vendor transactions.
              </p>
            </>
          )}

          {tab === 'Thermal Simulator' && (
            <>
              <h3
                className="font-semibold text-base flex items-center gap-2"
                style={{ color: 'var(--md-sys-color-on-surface)' }}
              >
                <Activity size={18} style={{ color: 'var(--md-sys-color-primary)' }} />
                Illustrative Thermal Equilibrium Plant
              </h3>
              <p
                className="text-xs"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                Synthetic first-order thermal plant for previewing PID slew bounding. Separate from native device sensor telemetry.
              </p>
              <div className="grid sm:grid-cols-2 gap-4">
                <label className="md3-card-filled p-4 block text-xs">
                  <span
                    className="font-medium"
                    style={{ color: 'var(--md-sys-color-on-surface)' }}
                  >
                    Workload Intensity: {load}%
                  </span>
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
                <label className="md3-card-filled p-4 block text-xs">
                  <span
                    className="font-medium"
                    style={{ color: 'var(--md-sys-color-on-surface)' }}
                  >
                    Ambient Temperature: {ambient} °C
                  </span>
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
                className="w-full md3-card-filled p-2"
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
                  stroke="var(--md-sys-color-primary)"
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
                <span style={{ color: 'var(--md-sys-color-on-surface)' }}>
                  Final: {simulation[120].temperature.toFixed(1)} °C ·{' '}
                  {(simulation[120].scale * 100).toFixed(0)}% capacity budget
                </span>
                <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                  Amber: Temperature (20–70 °C) · Accent: Capacity Scale (40–100%)
                </span>
              </div>
            </>
          )}

          {tab === 'C++23 / Rust 2024' && (
            <>
              <div className="flex items-center justify-between gap-3 flex-wrap">
                <h3
                  className="font-semibold text-base flex items-center gap-2"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  <Code2 size={18} style={{ color: 'var(--md-sys-color-primary)' }} />
                  Production Source Inspector
                </h3>
                <select
                  aria-label="Source file"
                  value={source}
                  onChange={(e) => setSource(e.target.value)}
                  className="md3-btn-tonal text-xs"
                >
                  {Object.keys(sources).map((name) => (
                    <option key={name}>{name}</option>
                  ))}
                </select>
              </div>
              <pre
                className="overflow-auto text-xs font-mono p-5 md3-card-filled whitespace-pre max-h-96 leading-relaxed"
                style={{ color: 'var(--md-sys-color-on-surface)' }}
              >
                {sources[source]}
              </pre>
            </>
          )}

          {tab === 'Export' && (
            <>
              <h3
                className="font-semibold text-base"
                style={{ color: 'var(--md-sys-color-on-surface)' }}
              >
                Specification & Blueprint Export
              </h3>
              <div className="flex gap-2.5 flex-wrap">
                <button
                  className="md3-btn-filled"
                  onClick={() => void copy()}
                >
                  <Copy size={14} />
                  {copied ? 'Copied' : 'Copy Blueprint'}
                </button>
                {['md', 'txt'].map((ext) => (
                  <button
                    key={ext}
                    className="md3-btn-outlined"
                    onClick={() => download(ext)}
                  >
                    <Download size={14} />
                    Download .{ext}
                  </button>
                ))}
              </div>
              {copyError && (
                <p
                  role="alert"
                  className="text-xs"
                  style={{ color: 'var(--md-sys-color-warning)' }}
                >
                  {copyError}
                </p>
              )}
              <pre
                className="whitespace-pre-wrap font-mono text-xs md3-card-filled p-5 max-h-96 overflow-auto leading-relaxed"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                {fullText}
              </pre>
            </>
          )}
        </div>
      </div>
    </div>
  );
};
