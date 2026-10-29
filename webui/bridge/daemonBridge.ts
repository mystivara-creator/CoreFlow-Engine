import { DeviceHardwareProfile, RuntimeSample, RuntimeState, SysfsNodeEntry } from '../engine/types';

export interface ShellResult {
  errno: number;
  stdout: string;
  stderr: string;
}

export interface NumericFieldSpec {
  key: string;
  label: string;
  unit: string;
  kind: 'int' | 'real';
  min: number;
  max?: number;
  group: 'runtime' | 'interactivity';
  help: string;
}

export interface PolicyFormValues {
  mutation_mode: 'observe' | 'adaptive';
  runtime_refresh: boolean;
  interactivity_mode: 'observe' | 'adaptive';
  monitor_interval: string;
  min_confidence: string;
  launch_window_ms: string;
  jank_miss_rate: string;
  boost_readahead_kb: string;
  boost_uclamp_launch: string;
  boost_uclamp_jank: string;
  boost_gpu_steps: string;
  boost_window_ms: string;
  [key: string]: string | boolean;
}

export interface DaemonProbe {
  pids: string[];
  supervisor: string;
  supervisorAlive: boolean;
  running: boolean;
  safe: boolean;
  disable: boolean;
  pending: boolean;
  bootAttempts: string;
}

export interface ConscienceTraceRow {
  time: string;
  event: string;
  scope: string;
  result: string;
  reason: string;
  state: string;
  decision: string;
  kind: 'error' | 'block' | 'change' | 'safety' | 'event';
  target: string;
  requested: string;
  currentValue: string;
  baselineValue: string;
  actuatorStatus: string;
  writesAttempted: number | null;
  writesVerified: number | null;
}

export interface PipelineStage {
  id: string;
  name: string;
  state: string;
  note: string;
  tone: 'ok' | 'info' | 'warn' | 'bad';
}

export interface ParsedEngineStatus {
  ok: boolean;
  error?: string;
  age?: number;
  stale?: boolean;
  data?: {
    version: string;
    pid: number;
    sample: number;
    updated: number;
    interval: number;
    running: boolean;
    blocker: string;
    device: {
      present: boolean;
      manufacturer: string;
      model: string;
      deviceName: string;
      product: string;
      board: string;
      hardware: string;
      socManufacturer: string;
      socModel: string;
      androidRelease: string;
      sdkLevel: string;
      kernelRelease: string;
      abi: string;
      procAvailable: boolean;
      sysAvailable: boolean;
      cgroupV2: boolean;
      cpuset: boolean;
      uclamp: boolean;
      schedulerControls: boolean;
      devfreq: boolean;
      discoveredResources: number;
      mutationReadyResources: number;
      expandedDomains: Array<{
        id: number;
        evidence: 'unavailable' | 'detected' | 'observable';
        matched: number;
        readable: number;
        mutationAuthorized: boolean;
      }>;
      blockQueueMutationsQuarantined: boolean;
    };
    eyes: {
      thermal: number | null;
      hottest: number | null;
      thermalTrend: string;
      mem: number | null;
      memTrend: string;
      cpu: number | null;
      load: number | null;
      loadTrend: string;
      ioRead: number | null;
      ioWrite: number | null;
      battery: number | null;
      charging: boolean;
      confidence: number | null;
    };
    brain: {
      state: string;
      previous: string;
      decision: string;
      workload: string;
      thermalHeadroom: boolean;
      memoryHeadroom: boolean;
      powerHeadroom: boolean;
      proactive: boolean;
      stabilizing: boolean;
      planAction: string;
      intervention: string;
      eligible: boolean;
      stabilizingOnly: boolean;
      candidates: number;
      planReason: string;
      agentMode: string;
      agentReason: string;
      holdActive: boolean;
      hold: string;
      mode: string;
      armed: boolean;
      allowCpu: boolean;
      permitResource: boolean;
      permitCpu: boolean;
      baselineReady: boolean;
      baselineSamples: number;
      baselineTarget: number;
      observationSamples: number;
      observationTarget: number;
      skipReason: string;
      observing: boolean;
      cooldown: number;
    };
    hands: {
      cpuMutated: boolean;
      resourceMutated: boolean;
      stabilizingEpoch: boolean;
      cpuApplied: Array<{ key: string; value: string }>;
      resourceApplied: Array<{ key: string; value: string }>;
      lastCpu: string;
      lastResource: string;
      lastKind: string;
      lastResult: string;
      lastSample: number;
      verified: number;
      rolledBack: number;
      failed: number;
    };
  };
}

export interface CoreFlowControlGlobal {
  PATHS: Record<string, string>;
  DEFAULTS: Record<string, string>;
  NUMERIC: NumericFieldSpec[];
  parseIni: (text: string) => Record<string, string>;
  formFromConfig: (cfg: Record<string, string>) => PolicyFormValues;
  validate: (form: PolicyFormValues) => {
    ok: boolean;
    errors: Record<string, string>;
    values: Record<string, string>;
  };
  gate: (
    values: Record<string, string>,
    ctx: { currentMode: string; safe: boolean; disable: boolean }
  ) => {
    ok: boolean;
    reason?: string;
    confirm: 'resume' | 'arm' | 'observe' | null;
    clearDisable: boolean;
  };
  applyValues: (raw: string, values: Record<string, string>) => string;
  snapshotCmd: () => string;
  splitSnapshot: (stdout: string) => { probe: string; status: string };
  parseProbe: (probe: string) => DaemonProbe;
  readConfigCmd: () => string;
  readTraceCmd: () => string;
  readLogCmd: () => string;
  startCmd: () => string;
  stopCmd: () => string;
  restartCmd: () => string;
  refreshCmd: () => string;
  writeConfigCmd: (
    newRaw: string,
    values: Record<string, string>,
    opts?: { clearDisable?: boolean }
  ) => string;
  parseTrace: (text: string, limit?: number) => ConscienceTraceRow[];
  filterTrace: (rows: ConscienceTraceRow[], filter: string) => ConscienceTraceRow[];
  describeEvent: (name: string) => string;
}

export interface CoreFlowStatusGlobal {
  BLOCKERS: Record<string, { tone: string; title: string; detail: string }>;
  parse: (raw: string) => ParsedEngineStatus;
  summary: (parsed: ParsedEngineStatus) => {
    code: string;
    tone: 'ok' | 'info' | 'warn' | 'bad';
    title: string;
    detail: string;
  };
  pipeline: (data: NonNullable<ParsedEngineStatus['data']>) => PipelineStage[];
  eyesRows: (data: NonNullable<ParsedEngineStatus['data']>) => Array<[string, string, string?]>;
  deviceRows: (data: NonNullable<ParsedEngineStatus['data']>) => Array<[string, string, string?]>;
  brainRows: (data: NonNullable<ParsedEngineStatus['data']>) => Array<[string, string, string?]>;
  handsRows: (data: NonNullable<ParsedEngineStatus['data']>) => Array<[string, string, string?]>;
}

declare global {
  interface Window {
    CoreFlowControl?: CoreFlowControlGlobal;
    CoreFlowStatus?: CoreFlowStatusGlobal;
    ksu?: {
      exec: (cmd: string, options: string, callbackName: string) => void;
    };
  }
}

export const EXPANDED_DOMAIN_NAMES: Record<number, string> = {
  0: 'CPUFreq Governor & Limits',
  1: 'Scheduler / WALT Controls',
  2: 'Cgroup v2 & UClamp',
  3: 'VM / Memory Sysctl',
  4: 'Block Queue I/O',
  5: 'ZRAM & Swap Backing',
  6: 'GPU Devfreq / KGSL',
  7: 'Thermal Zones & Cooling',
  8: 'Power Supply & Charging',
  9: 'PSI Stall Monitors',
  10: 'Cpuset Topology',
  11: 'Interconnect / Bus',
  12: 'Display Frame Pacing',
  13: 'Foreground Process',
  14: 'Kernel General Sysctl',
};

const DEFAULT_CONFIG_INI = `# CoreFlow System Framework: automatic operation.
monitor_interval=5
min_confidence=0.60
mutation_mode=adaptive
mutation_armed=true
runtime_refresh=true
allow_cpu_governor=yes
grant_memory=yes
grant_block_queue=yes
grant_scheduler=yes
grant_gpu=yes
grant_cgroup=yes
grant_cpufreq=yes
grant_kernel=yes
interactivity_mode=adaptive
launch_window_ms=2500
jank_miss_rate=0.05
boost_window_ms=3000
boost_uclamp_launch=20
boost_uclamp_jank=10
boost_readahead_kb=512
boost_gpu_steps=1
`;

export function hasNativeKernelSuBridge(): boolean {
  return typeof window !== 'undefined' && typeof window.ksu?.exec === 'function';
}

let callbackCounter = 0;

export function execNativeKsuShell(cmd: string, timeoutMs = 12000): Promise<ShellResult> {
  return new Promise((resolve) => {
    const ksu = window.ksu;
    if (!ksu || typeof ksu.exec !== 'function') {
      resolve({ errno: 127, stdout: '', stderr: 'KernelSU WebUI bridge unavailable' });
      return;
    }

    const cbName = `__coreflow_ksu_cb_${Date.now()}_${++callbackCounter}`;
    let settled = false;
    const win = window as unknown as Record<string, unknown>;

    const timer = window.setTimeout(() => {
      if (settled) return;
      settled = true;
      delete win[cbName];
      resolve({ errno: 124, stdout: '', stderr: 'KernelSU shell command timed out' });
    }, timeoutMs);

    win[cbName] = (errno: number, stdout: string, stderr: string) => {
      if (settled) return;
      settled = true;
      window.clearTimeout(timer);
      delete win[cbName];
      resolve({
        errno: typeof errno === 'number' ? errno : 0,
        stdout: typeof stdout === 'string' ? stdout : '',
        stderr: typeof stderr === 'string' ? stderr : '',
      });
    };

    try {
      ksu.exec(cmd, '{}', cbName);
    } catch (err) {
      if (!settled) {
        settled = true;
        window.clearTimeout(timer);
        delete win[cbName];
        resolve({ errno: 1, stdout: '', stderr: String(err) });
      }
    }
  });
}

// Virtual /data/adb/coreflow state for interactive AI Studio Mirror mode
export interface VirtualDaemonStore {
  running: boolean;
  pid: number;
  safeMode: boolean;
  disableMarker: boolean;
  configPending: boolean;
  bootAttempts: number;
  configRaw: string;
  verifiedCount: number;
  rolledBackCount: number;
  failedCount: number;
  traceLines: string[];
}

export function createInitialVirtualDaemonStore(): VirtualDaemonStore {
  return {
    running: true,
    pid: 4281,
    safeMode: false,
    disableMarker: false,
    configPending: false,
    bootAttempts: 0,
    configRaw: DEFAULT_CONFIG_INI,
    verifiedCount: 2,
    rolledBackCount: 0,
    failedCount: 0,
    traceLines: [
      JSON.stringify({
        timestamp: new Date().toISOString().slice(11, 19),
        event: 'policy_evaluated',
        scope: 'balance',
        result: 'ACTIVE',
        reason: 'Engine starts automatically with bounded optimization enabled.',
        state: 'NORMAL',
        decision: 'NO_ACTION',
      }),
      JSON.stringify({
        timestamp: new Date().toISOString().slice(11, 19),
        event: 'cycle_completed',
        scope: 'autonomous',
        result: 'OK',
        reason: 'Initial capability snapshot exported to /data/adb/coreflow/status.json.',
        state: 'NORMAL',
        decision: 'NO_ACTION',
      }),
    ],
  };
}

export function buildMirrorStatusSnapshot(
  store: VirtualDaemonStore,
  profile: DeviceHardwareProfile,
  sample: RuntimeSample,
  policyState: RuntimeState,
  prevPolicyState: RuntimeState,
  decision: string,
  workload: string,
  sysfsNodes: SysfsNodeEntry[],
  baselineSamplesCount: number,
  observationSamplesCount: number
): string {
  const C = window.CoreFlowControl;
  const parsedCfg = C ? C.parseIni(store.configRaw) : { mutation_mode: 'observe', mutation_armed: 'false', monitor_interval: '5' };
  const isAdaptive = parsedCfg.mutation_mode === 'adaptive';
  const isArmed = parsedCfg.mutation_armed === 'true' && !store.safeMode && !store.disableMarker;
  const nowEpoch = Math.floor(Date.now() / 1000);

  const mutatedResources = sysfsNodes
    .filter((n) => (n.status === 'MUTATED' || n.status === 'CLAMPED') && !n.path.includes('scaling_governor'))
    .map((n) => ({ path: n.path, value: n.currentValue }));

  const mutatedCpu = sysfsNodes
    .filter((n) => n.status === 'MUTATED' && n.path.includes('scaling_governor'))
    .map((n) => ({ key: n.path, value: n.currentValue }));

  let blocker = 'READY';
  if (!store.running) blocker = 'STOPPED';
  else if (store.safeMode || store.disableMarker && isAdaptive) blocker = 'SAFETY_HOLD';
  else if (!isAdaptive) blocker = 'OBSERVE_MODE';
  else if (!isArmed) blocker = 'NOT_ARMED';
  else if (sample.confidence < Number(parsedCfg.min_confidence || 0.6)) blocker = 'CONTEXT_LOW_CONFIDENCE';
  else if (policyState === RuntimeState.Idle) blocker = 'CONTEXT_IDLE';
  else if (observationSamplesCount > 0 && observationSamplesCount < 4) blocker = 'OBSERVING_OUTCOME';
  else if (mutatedResources.length > 0 || mutatedCpu.length > 0) blocker = 'CHANGE_HELD';

  const statusObj = {
    schema: 1,
    version: '3.0.1',
    pid: store.running ? store.pid : 0,
    sample: sample.sampleIndex,
    updated_epoch: nowEpoch,
    interval_s: Number(parsedCfg.monitor_interval || 5),
    running: store.running,
    blocker,
    device: {
      manufacturer: profile.manufacturer,
      model: profile.model,
      device_name: profile.codename,
      product: profile.codename,
      board: profile.board,
      hardware: profile.board,
      soc_manufacturer: profile.manufacturer,
      soc_model: profile.soc,
      android_release: profile.androidRelease,
      sdk_level: profile.sdkLevel,
      kernel_release: profile.kernelRelease,
      abi: profile.abi,
      proc_available: true,
      sys_available: true,
      cgroup_v2: true,
      cpuset_available: true,
      uclamp_available: true,
      scheduler_controls_available: profile.board !== 'mt6897',
      devfreq_available: profile.board !== 'mt6897',
      discovered_resources: profile.discoveredResources,
      mutation_ready_resources: profile.mutationReadyResources,
      expanded_domains: Array.from({ length: 10 }, (_, idx) => ({
        id: idx,
        evidence: idx === 6 && profile.board === 'mt6897' ? 'detected' : 'observable',
        matched: idx === 0 ? 6 : idx === 4 ? 8 : 4,
        readable: idx === 0 ? 6 : idx === 4 ? 8 : 4,
        mutation_authorized: isAdaptive && isArmed && idx !== 7 && idx !== 8,
      })),
      block_queue_mutations_quarantined: false,
    },
    eyes: {
      thermal_available: sample.thermalAvailable,
      thermal_c: Number((sample.thermalMillidegrees / 1000).toFixed(2)),
      hottest_c: Number((sample.hottestThermalMillidegrees / 1000).toFixed(2)),
      thermal_trend: sample.thermalTrend,
      mem_available_ratio: Number(sample.memAvailableRatio.toFixed(4)),
      memory_trend: sample.memoryTrend,
      cpu_available: true,
      cpu_utilization: Number((sample.cpuUsagePct / 100).toFixed(3)),
      load_available: true,
      load1: sample.load1,
      load_trend: sample.loadTrend,
      io_available: true,
      io_read_kbs: sample.ioReadKbs,
      io_write_kbs: sample.ioWriteKbs,
      battery_percent: sample.batteryPercent,
      charging: sample.charging,
      confidence: sample.confidence,
    },
    brain: {
      state: policyState,
      previous_state: prevPolicyState,
      decision,
      workload,
      thermal_headroom: policyState !== RuntimeState.ThermalGuard && policyState !== RuntimeState.Warming,
      memory_headroom: policyState !== RuntimeState.Pressure,
      power_headroom: sample.batteryPercent > 20 || sample.charging,
      proactive_allowed:
        isAdaptive &&
        isArmed &&
        policyState !== RuntimeState.ThermalGuard &&
        policyState !== RuntimeState.Pressure &&
        policyState !== RuntimeState.Idle,
      stabilizing_allowed: isAdaptive && isArmed,
      plan_action: !isAdaptive
        ? 'OBSERVE_ONLY'
        : policyState === RuntimeState.ThermalGuard
          ? 'STABILIZE_THERMAL'
          : mutatedResources.length > 0
            ? 'APPLY_BOUNDED'
            : 'HOLD_BASELINE',
      intervention:
        policyState === RuntimeState.ThermalGuard || policyState === RuntimeState.Pressure
          ? 'STABILIZING'
          : isAdaptive && isArmed
            ? 'BOUNDED'
            : 'NONE',
      mutation_eligible: isAdaptive && isArmed && sample.confidence >= 0.6,
      stabilizing_only:
        policyState === RuntimeState.ThermalGuard || policyState === RuntimeState.Pressure,
      candidates: isAdaptive && isArmed ? mutatedResources.length + mutatedCpu.length : 0,
      plan_reason: !isAdaptive
        ? 'mutation_mode=observe in config.ini; observing telemetry without kernel writes'
        : store.safeMode
          ? 'SAFE_MODE marker active; all mutations suppressed'
          : 'Capability-verified candidates evaluated against factory baseline',
      agent_mode: isAdaptive ? 'ADAPTIVE_GUARDED' : 'PASSIVE_OBSERVER',
      agent_reason: 'Evidence-driven control loop synchronized with 18-feature predictor',
      hold_active: store.safeMode || (isAdaptive && store.disableMarker),
      safety_hold: store.safeMode ? 'SAFE_MODE' : store.disableMarker && !isAdaptive ? 'DISABLE_MARKER' : 'NONE',
      mode: isAdaptive ? 'adaptive' : 'observe',
      armed: isArmed,
      allow_cpu_governor: true,
      permit_resource: isAdaptive && isArmed,
      permit_cpu: isAdaptive && isArmed,
      baseline_ready: baselineSamplesCount >= 4 || sample.sampleIndex >= 4,
      baseline_samples: Math.min(4, Math.max(baselineSamplesCount, Math.min(4, sample.sampleIndex))),
      baseline_target: 4,
      observation_samples: observationSamplesCount,
      observation_target: 4,
      skip_reason: !isAdaptive ? 'observe_mode_default' : '',
      observing: observationSamplesCount > 0 && observationSamplesCount < 4,
      cooldown_remaining: 0,
    },
    hands: {
      cpu_mutated: isAdaptive && isArmed && mutatedCpu.length > 0,
      resource_mutated: isAdaptive && isArmed && mutatedResources.length > 0,
      stabilizing_epoch:
        isAdaptive &&
        isArmed &&
        (policyState === RuntimeState.ThermalGuard || policyState === RuntimeState.Pressure),
      cpu_applied: isAdaptive && isArmed ? mutatedCpu : [],
      resource_applied: isAdaptive && isArmed ? mutatedResources.slice(0, 8) : [],
      last_cpu_result: mutatedCpu.length > 0 && isArmed ? 'VERIFIED' : 'IDLE',
      last_resource_result: mutatedResources.length > 0 && isArmed ? 'VERIFIED' : 'IDLE',
      last_change_kind: mutatedResources.length > 0 && isArmed ? 'resource' : mutatedCpu.length > 0 && isArmed ? 'cpufreq' : '',
      last_change_result: (mutatedResources.length > 0 || mutatedCpu.length > 0) && isArmed ? 'VERIFIED' : '',
      last_change_sample: sample.sampleIndex,
      verified: store.verifiedCount,
      rolled_back: store.rolledBackCount,
      failed: store.failedCount,
    },
  };

  const probeBlock = [
    '@@PROBE',
    `PIDS=${store.running ? store.pid : ''}`,
    `SUP=${store.running ? store.pid - 11 : ''}`,
    `SUP_ALIVE=${store.running ? '1' : '0'}`,
    `SAFE=${store.safeMode ? '1' : '0'}`,
    `DISABLE=${store.disableMarker ? '1' : '0'}`,
    `PENDING=${store.configPending ? '1' : '0'}`,
    `BOOT=${store.bootAttempts}`,
    '@@STATUS',
    `NOW=${nowEpoch}`,
    JSON.stringify(statusObj),
    '@@END',
  ].join('\n');

  return probeBlock;
}
