// CoreFlow Modern v2 preview model.
// Native mode never uses this simulator; it exists only for browser preview.
export type AutonomousMode =
  | 'balanced'
  | 'sustained_load'
  | 'pressure_guard'
  | 'thermal_guard';

export interface AutonomousTelemetry {
  mode: AutonomousMode;
  modeLabel: string;
  modeDescription: string;
  engineActive: boolean;
  uclampMin: number;
  uclampMax: number;
  uclampSupported: boolean;
  socTempCelsius: number;
  cpuPressurePct: number;
  memoryPressurePct: number;
  ioPressurePct: number;
  baselineNodesSnapshotted: number;
  baselineStatus: 'protected' | 'restored';
  timestamp: number;
}

export interface AutonomousConfig {
  adaptiveBias: 'balanced' | 'responsiveness' | 'battery';
  thermalTripCelsius: number;
  balancedUclampMin: number;
}

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
        description: 'CPU pressure is elevated while thermal headroom remains available.',
        badgeColor: 'text-sky-500 bg-sky-500/10 border-sky-500/30',
        glowColor: '#0ea5e9',
      };
    case 'pressure_guard':
      return {
        label: 'Pressure Guard',
        description: 'Memory or I/O contention is elevated, so artificial boosting is reduced.',
        badgeColor: 'text-amber-500 bg-amber-500/10 border-amber-500/30',
        glowColor: '#f59e0b',
      };
    case 'thermal_guard':
      return {
        label: 'Thermal Guard',
        description: 'Thermal pressure has priority; scheduler demand is reduced smoothly.',
        badgeColor: 'text-rose-500 bg-rose-500/10 border-rose-500/30',
        glowColor: '#f43f5e',
      };
    case 'balanced':
    default:
      return {
        label: 'Balanced',
        description: 'Stable daily baseline with the configured top-app UClamp floor.',
        badgeColor: 'text-teal-500 bg-teal-500/10 border-teal-500/30',
        glowColor: '#14b8a6',
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
    socTempCelsius: 36.5,
    cpuPressurePct: 1.2,
    memoryPressurePct: 0.3,
    ioPressurePct: 0.4,
    baselineNodesSnapshotted: 0,
    baselineStatus: 'protected',
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
      tag: 'Discovery',
      message: 'Preview: semantic top-app UClamp capability discovered.',
    },
    {
      id: 'preview-2',
      timestamp: stamp(6),
      type: 'BASELINE',
      tag: 'Recovery',
      message: 'Preview: pre-mutation values protected by a write-ahead journal.',
    },
    {
      id: 'preview-3',
      timestamp: stamp(3),
      type: 'INFO',
      tag: 'Stability',
      message: 'Preview: four-state thermal + PSI policy is active.',
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
        modeLabel: 'Engine Paused',
        modeDescription: 'Preview controls paused; OEM baseline assumed.',
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
    timestamp: Date.now(),
  };

  if (mode === current.mode) return { next };

  const event: ReactionLogEvent = {
    id: 'preview-' + Date.now(),
    timestamp: new Date().toTimeString().slice(0, 8),
    type: mode === 'thermal_guard' ? 'THERMAL' : mode === 'sustained_load' ? 'BOOST' : 'INFO',
    tag: 'Stability',
    message: 'Preview state -> ' + meta.label +
      ' | cpu=' + next.cpuPressurePct + '% mem=' + next.memoryPressurePct +
      '% io=' + next.ioPressurePct + '% temp=' + next.socTempCelsius + 'C',
  };
  return { next, event };
}
