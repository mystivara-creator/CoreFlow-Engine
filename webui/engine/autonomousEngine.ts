// CFAE System Framework v3.0.R — Coordinated Mutation Transaction & Preview Engine.
// Native mode reads real daemon telemetry; this simulator models the 5-layer
// Mutation Safety architecture strictly for browser preview.

export type AutonomousMode =
  | 'balanced'
  | 'sustained_load'
  | 'pressure_guard'
  | 'thermal_guard';

export type TransactionStage =
  | 'PLANNED'
  | 'APPLIED'
  | 'VERIFIED'
  | 'OBSERVING'
  | 'RETAINED';

export type RiskClass = 'Low' | 'Medium' | 'High' | 'Forbidden';

export type TransactionOutcome =
  | 'IMPROVED'
  | 'NEUTRAL'
  | 'REGRESSED'
  | 'INCONCLUSIVE'
  | 'ABORTED'
  | 'OWNERSHIP_LOST'
  | 'RECOVERY_REQUIRED';

export interface ThermalSensorDetail {
  id: string;
  type: string;
  category: number; // 1 compute, 2 battery, 3 skin, 4 board
  celsius: number | null;
  valid: boolean;
  policyAvailable: boolean;
}

export interface AutonomousTelemetry {
  mode: AutonomousMode;
  modeLabel: string;
  modeDescription: string;
  engineActive: boolean;
  uclampMin: number;
  uclampMax: number;
  uclampSupported: boolean;
  socTempCelsius: number;
  socCompositeCelsius?: number;
  cpuAverageCelsius?: number;
  cpuPeakCelsius?: number;
  gpuTempCelsius?: number;
  batteryTempCelsius?: number;
  skinTempCelsius?: number;
  thermalSensors?: ThermalSensorDetail[];
  cpuPressurePct: number;
  memoryPressurePct: number;
  ioPressurePct: number;
  baselineNodesSnapshotted: number;
  baselineStatus: 'protected' | 'restored';
  transactionStage?: TransactionStage;
  riskClass?: RiskClass;
  lastOutcome?: TransactionOutcome;
  ownershipIntact?: boolean;
  timestamp: number;
}

export interface AutonomousConfig {
  adaptiveBias: 'balanced' | 'responsiveness' | 'battery';
  thermalTripCelsius: number;
  balancedUclampMin: number;
}

export interface HardwareLongevityConfig {
  enableBatteryStepCharging: boolean;
  batteryStepTempThreshold: number;
  steppedChargeCurrentMa: number;
  enableUfsBurstPacing: boolean;
  topAppIoWeight: number;
  backgroundIoWeight: number;
}

export const DEFAULT_LONGEVITY_CONFIG: HardwareLongevityConfig = {
  enableBatteryStepCharging: true,
  batteryStepTempThreshold: 41.5,
  steppedChargeCurrentMa: 1500,
  enableUfsBurstPacing: true,
  topAppIoWeight: 800,
  backgroundIoWeight: 50,
};

export interface ReactionLogEvent {
  id: string;
  timestamp: string;
  type: 'BOOST' | 'THERMAL' | 'BASELINE' | 'INFO';
  tag: string;
  message: string;
}

export const DEFAULT_AUTONOMOUS_CONFIG: AutonomousConfig = {
  adaptiveBias: 'balanced',
  thermalTripCelsius: 44.0,
  balancedUclampMin: 320,
};

export function getModeMeta(mode: AutonomousMode) {
  switch (mode) {
    case 'sustained_load':
      return {
        label: 'Sustained Load',
        description: 'Coordinated CPU responsiveness transaction active with verified thermal headroom.',
        stage: 'OBSERVING' as TransactionStage,
        risk: 'Medium' as RiskClass,
        outcome: 'IMPROVED' as TransactionOutcome,
      };
    case 'pressure_guard':
      return {
        label: 'Pressure Guard',
        description: 'Memory or I/O contention elevated; non-essential boost candidates deferred.',
        stage: 'VERIFIED' as TransactionStage,
        risk: 'Low' as RiskClass,
        outcome: 'NEUTRAL' as TransactionOutcome,
      };
    case 'thermal_guard':
      return {
        label: 'Thermal Guard',
        description: 'OEM/Android thermal safety gate engaged; active mitigation pacing SoC demand.',
        stage: 'VERIFIED' as TransactionStage,
        risk: 'Low' as RiskClass,
        outcome: 'ABORTED' as TransactionOutcome,
      };
    case 'balanced':
    default:
      return {
        label: 'Balanced Equilibrium',
        description: 'Coordinated daily baseline retained with readback-verified top-app UClamp floor.',
        stage: 'RETAINED' as TransactionStage,
        risk: 'Low' as RiskClass,
        outcome: 'IMPROVED' as TransactionOutcome,
      };
  }
}

export function createInitialAutonomousTelemetry(): AutonomousTelemetry {
  const meta = getModeMeta('balanced');
  return {
    mode: 'balanced',
    modeLabel: meta.label,
    modeDescription: meta.description,
    engineActive: true,
    uclampMin: 320,
    uclampMax: 1024,
    uclampSupported: true,
    socTempCelsius: 38.6,
    socCompositeCelsius: 38.6,
    cpuAverageCelsius: 38.2,
    cpuPeakCelsius: 41.5,
    gpuTempCelsius: 36.8,
    batteryTempCelsius: 33.4,
    skinTempCelsius: 35.1,
    thermalSensors: [
      { id: 'tz0', type: 'cpu-0-0', category: 1, celsius: 37.8, valid: true, policyAvailable: true },
      { id: 'tz1', type: 'cpu-0-1', category: 1, celsius: 38.0, valid: true, policyAvailable: true },
      { id: 'tz2', type: 'cpu-1-0', category: 1, celsius: 39.4, valid: true, policyAvailable: true },
      { id: 'tz3', type: 'cpu-1-2', category: 1, celsius: 41.5, valid: true, policyAvailable: true },
      { id: 'tz4', type: 'gpuss-0', category: 1, celsius: 36.8, valid: true, policyAvailable: true },
      { id: 'tz5', type: 'quiet_therm', category: 3, celsius: 35.1, valid: true, policyAvailable: true },
      { id: 'tz6', type: 'battery', category: 2, celsius: 33.4, valid: true, policyAvailable: true },
      { id: 'tz7', type: 'ap_therm', category: 4, celsius: 37.0, valid: true, policyAvailable: true },
    ],
    cpuPressurePct: 1.2,
    memoryPressurePct: 0.3,
    ioPressurePct: 0.4,
    baselineNodesSnapshotted: 4,
    baselineStatus: 'protected',
    transactionStage: 'RETAINED',
    riskClass: 'Low',
    lastOutcome: 'IMPROVED',
    ownershipIntact: true,
    timestamp: Date.now(),
  };
}

export function createInitialLogs(): ReactionLogEvent[] {
  const now = new Date();
  const stamp = (seconds: number) =>
    new Date(now.getTime() - seconds * 1000).toTimeString().slice(0, 8);
  return [
    {
      id: 'preview-1',
      timestamp: stamp(9),
      type: 'INFO',
      tag: 'AdmissionGate',
      message: 'Preview: capability semantics, range bounds, and OEM ownership verified.',
    },
    {
      id: 'preview-2',
      timestamp: stamp(6),
      type: 'BASELINE',
      tag: 'Coordinator',
      message: 'Preview: write-ahead transaction journal committed prior to mutation.',
    },
    {
      id: 'preview-3',
      timestamp: stamp(3),
      type: 'INFO',
      tag: 'OutcomeEval',
      message: 'Preview: readback verified (uclamp.min=320, uclamp.max=1024) -> RETAINED.',
    },
  ];
}

export function stepAutonomousSimulation(
  current: AutonomousTelemetry,
  config: AutonomousConfig,
  forced?: 'cpu' | 'memory'
): { next: AutonomousTelemetry; event?: ReactionLogEvent } {
  if (!current.engineActive) {
    return {
      next: {
        ...current,
        uclampMin: 0,
        uclampMax: 1024,
        mode: 'balanced',
        modeLabel: 'Engine Standby',
        modeDescription: 'Coordinated mutations paused; OEM baseline values intact.',
        transactionStage: 'PLANNED',
        timestamp: Date.now(),
      },
    };
  }

  const biasFloor =
    config.adaptiveBias === 'battery'
      ? Math.round(config.balancedUclampMin * 0.75)
      : config.adaptiveBias === 'responsiveness'
      ? Math.round(config.balancedUclampMin * 1.25)
      : config.balancedUclampMin;
  const base = Math.max(0, Math.min(1024, biasFloor));

  const nextTemp = Math.max(
    32,
    Math.min(49, current.socTempCelsius + (Math.random() - 0.46) * 0.35)
  );
  const cpu = forced === 'cpu' ? 14 : Math.max(0, Math.random() * 10);
  const memory = forced === 'memory' ? 6 : Math.max(0, Math.random() * 3.5);
  const io = forced === 'memory' ? 6 : Math.max(0, Math.random() * 5.5);

  let mode: AutonomousMode = 'balanced';
  let min = base;
  let max = 1024;

  if (nextTemp >= config.thermalTripCelsius) {
    mode = 'thermal_guard';
    min = 0;
    max = 896;
  } else if (memory >= 3 || io >= 5) {
    mode = 'pressure_guard';
    min = Math.floor(base / 4);
  } else if (cpu >= 8 && nextTemp <= config.thermalTripCelsius - 3) {
    mode = 'sustained_load';
    min = Math.min(1024, base + Math.floor(base / 2));
  }

  const meta = getModeMeta(mode);
  const next: AutonomousTelemetry = {
    ...current,
    mode,
    modeLabel: meta.label,
    modeDescription: meta.description,
    uclampMin: min,
    uclampMax: max,
    socTempCelsius: Number(nextTemp.toFixed(1)),
    cpuPressurePct: Number(cpu.toFixed(1)),
    memoryPressurePct: Number(memory.toFixed(1)),
    ioPressurePct: Number(io.toFixed(1)),
    transactionStage: meta.stage,
    riskClass: meta.risk,
    lastOutcome: meta.outcome,
    ownershipIntact: true,
    timestamp: Date.now(),
  };

  if (mode === current.mode) return { next };

  return { next, event: createPreviewEvent(mode, meta.label, next) };
}

let previewEventSeq = 0;

function createPreviewEvent(
  mode: AutonomousMode,
  label: string,
  next: AutonomousTelemetry
): ReactionLogEvent {
  previewEventSeq += 1;
  return {
    id: `preview-${Date.now()}-${previewEventSeq}`,
    timestamp: new Date().toTimeString().slice(0, 8),
    type: mode === 'thermal_guard' ? 'THERMAL' : mode === 'sustained_load' ? 'BOOST' : 'INFO',
    tag: 'Transaction',
    message:
      `Stage=${next.transactionStage} -> ${label} | uclamp=${next.uclampMin}/${next.uclampMax} | ` +
      `cpu=${next.cpuPressurePct}% mem=${next.memoryPressurePct}% io=${next.ioPressurePct}% temp=${next.socTempCelsius}C`,
  };
}
