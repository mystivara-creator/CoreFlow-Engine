// CoreFlow Modern v2.0 Autonomous Engine Types and Reactive Simulation
// Models the C++20 zero-polling epoll reactor, cgroups v2, and EAS Uclamp engine.

export type AutonomousMode =
  | 'dynamic_balance'
  | 'launch_burst'
  | 'frame_pacing'
  | 'touch_boost'
  | 'thermal_guard'
  | 'deep_idle';

export interface AutonomousTelemetry {
  mode: AutonomousMode;
  modeLabel: string;
  modeDescription: string;
  engineActive: boolean;
  uclampMin: number;
  uclampMax: number;
  currentFps: number;
  targetFps: number;
  jankRatePct: number;
  socTempCelsius: number;
  cgroupsV2Status: 'unified_active' | 'hybrid' | 'disabled';
  topApp: string;
  topAppLabel: string;
  topAppPid: number;
  epollEventsPerSec: number;
  baselineNodesSnapshotted: number;
  baselineStatus: 'protected' | 'restored';
  batteryDrainEstimatedMh: number;
  timestamp: number;
}

export interface AutonomousConfig {
  adaptiveBias: 'balanced' | 'responsiveness' | 'battery';
  targetFps: 120 | 90 | 60;
  thermalTripCelsius: number;
  burstDurationMs: number;
  touchBoostMin: number;
  unlockVendorClamping: boolean;
  proactiveMemoryGuardian: boolean;
}

export interface ReactionLogEvent {
  id: string;
  timestamp: string;
  type: 'BOOST' | 'CGROUP' | 'FRAME' | 'THERMAL' | 'BASELINE' | 'INFO';
  tag: string;
  message: string;
}

export const DEFAULT_AUTONOMOUS_CONFIG: AutonomousConfig = {
  adaptiveBias: 'balanced',
  targetFps: 120,
  thermalTripCelsius: 44.0,
  burstDurationMs: 120,
  touchBoostMin: 380,
  unlockVendorClamping: true,
  proactiveMemoryGuardian: true,
};

const SAMPLE_APPS = [
  { pkg: 'com.android.chrome', label: 'Google Chrome', defaultMode: 'dynamic_balance' as AutonomousMode, baseFps: 120 },
  { pkg: 'com.miHoYo.GenshinImpact', label: 'Genshin Impact', defaultMode: 'frame_pacing' as AutonomousMode, baseFps: 120 },
  { pkg: 'com.google.android.youtube', label: 'YouTube', defaultMode: 'dynamic_balance' as AutonomousMode, baseFps: 120 },
  { pkg: 'com.instagram.android', label: 'Instagram', defaultMode: 'touch_boost' as AutonomousMode, baseFps: 120 },
  { pkg: 'com.google.android.apps.photos', label: 'Google Photos', defaultMode: 'dynamic_balance' as AutonomousMode, baseFps: 120 },
  { pkg: 'com.android.launcher3', label: 'System Launcher', defaultMode: 'touch_boost' as AutonomousMode, baseFps: 120 },
];

export function getModeMeta(mode: AutonomousMode): {
  label: string;
  description: string;
  badgeColor: string;
  glowColor: string;
} {
  switch (mode) {
    case 'launch_burst':
      return {
        label: 'Cold Launch Burst',
        description: 'Accelerating cold start via maximum top-app EAS Uclamp capacity',
        badgeColor: 'text-amber-500 bg-amber-500/10 border-amber-500/30',
        glowColor: '#f59e0b',
      };
    case 'frame_pacing':
      return {
        label: 'SurfaceFlinger 120Hz Pacing',
        description: 'Dynamically mitigating frame drops & prioritizing render thread in cgroup',
        badgeColor: 'text-sky-500 bg-sky-500/10 border-sky-500/30',
        glowColor: '#0ea5e9',
      };
    case 'touch_boost':
      return {
        label: 'Tactile Touch Response',
        description: 'Zero-latency interactive touch burst for 120Hz scrolling & gesture velocity',
        badgeColor: 'text-emerald-500 bg-emerald-500/10 border-emerald-500/30',
        glowColor: '#10b981',
      };
    case 'thermal_guard':
      return {
        label: 'Thermal Throttling Defense',
        description: 'Proactively managing clamp ceilings before hardware trip point occurs',
        badgeColor: 'text-rose-500 bg-rose-500/10 border-rose-500/30',
        glowColor: '#f43f5e',
      };
    case 'deep_idle':
      return {
        label: 'Zero-Overhead Idle Saver',
        description: 'Display low-refresh state detected; epoll reactor parked with 0 uclamp floor',
        badgeColor: 'text-indigo-400 bg-indigo-500/10 border-indigo-500/30',
        glowColor: '#818cf8',
      };
    case 'dynamic_balance':
    default:
      return {
        label: 'Dynamic Intelligent Balance',
        description: 'Autonomously evaluating task load and balancing power vs throughput',
        badgeColor: 'text-teal-400 bg-teal-500/10 border-teal-500/30',
        glowColor: '#14b8a6',
      };
  }
}

export function createInitialAutonomousTelemetry(): AutonomousTelemetry {
  const meta = getModeMeta('dynamic_balance');
  return {
    mode: 'dynamic_balance',
    modeLabel: meta.label,
    modeDescription: meta.description,
    engineActive: true,
    uclampMin: 320,
    uclampMax: 1024,
    currentFps: 120,
    targetFps: 120,
    jankRatePct: 0.2,
    socTempCelsius: 36.6,
    cgroupsV2Status: 'unified_active',
    topApp: 'com.android.chrome',
    topAppLabel: 'Google Chrome',
    topAppPid: 24819,
    epollEventsPerSec: 38,
    baselineNodesSnapshotted: 18,
    baselineStatus: 'protected',
    batteryDrainEstimatedMh: 380,
    timestamp: Date.now(),
  };
}

export function createInitialLogs(): ReactionLogEvent[] {
  const now = new Date();
  const t = (deltaSec: number) => {
    const d = new Date(now.getTime() - deltaSec * 1000);
    return d.toTimeString().slice(0, 8);
  };

  return [
    {
      id: 'log-1',
      timestamp: t(14),
      type: 'BASELINE',
      tag: 'BaselineStore',
      message: 'OEM factory values snapshotted (18 sysfs nodes preserved in transactional store).',
    },
    {
      id: 'log-2',
      timestamp: t(12),
      type: 'INFO',
      tag: 'Reactor',
      message: 'Linux epoll zero-polling event reactor initialized on kernel 6.1.75-android15.',
    },
    {
      id: 'log-3',
      timestamp: t(9),
      type: 'CGROUP',
      tag: 'CgroupV2',
      message: 'Unified cgroups v2 controller attached. Task migration configured for /sys/fs/cgroup/top-app.',
    },
    {
      id: 'log-4',
      timestamp: t(6),
      type: 'BOOST',
      tag: 'Uclamp',
      message: 'EAS dynamic clamp initialized. Top-app default clamped to min: 320, max: 1024.',
    },
    {
      id: 'log-5',
      timestamp: t(3),
      type: 'FRAME',
      tag: 'RenderBooster',
      message: 'SurfaceFlinger HWUI frame pacing active: locked to 120 FPS target window.',
    },
    {
      id: 'log-6',
      timestamp: t(0),
      type: 'INFO',
      tag: 'Autonomous',
      message: 'Autonomous engine online. Self-adapting to foreground application and thermal telemetry.',
    },
  ];
}

// Tick the autonomous simulator state
export function stepAutonomousSimulation(
  current: AutonomousTelemetry,
  config: AutonomousConfig,
  forcedAppIndex?: number
): { next: AutonomousTelemetry; event?: ReactionLogEvent } {
  if (!current.engineActive) {
    return {
      next: {
        ...current,
        uclampMin: 0,
        currentFps: 60,
        jankRatePct: 4.8,
        epollEventsPerSec: 0,
        mode: 'deep_idle',
        modeLabel: 'Engine Paused (OEM Stock)',
        modeDescription: 'All autonomous controls suspended; running on stock OEM kernel parameters',
        timestamp: Date.now(),
      },
    };
  }

  // 1. Determine autonomous mode dynamically
  const appIdx = forcedAppIndex ?? Math.floor(Math.random() * SAMPLE_APPS.length);
  const activeApp = SAMPLE_APPS[appIdx] || SAMPLE_APPS[0];

  let nextMode: AutonomousMode = 'dynamic_balance';
  let targetUclamp = 320;
  let targetFps = config.targetFps;
  let jank = 0.1;
  let deltaTemp = (Math.random() - 0.5) * 0.3;
  let nextTemp = Math.max(33.0, Math.min(46.0, current.socTempCelsius + deltaTemp));

  // Check thermal guard
  if (nextTemp >= config.thermalTripCelsius) {
    nextMode = 'thermal_guard';
    targetUclamp = 180;
    targetFps = Math.min(targetFps, 90) as (120 | 90 | 60);
    jank = 1.2;
  } else {
    // Normal autonomous state transitions
    const rand = Math.random();
    if (rand < 0.22) {
      nextMode = 'touch_boost';
      targetUclamp = config.touchBoostMin;
    } else if (rand < 0.42 && activeApp.pkg.includes('GenshinImpact')) {
      nextMode = 'frame_pacing';
      targetUclamp = 512;
    } else if (rand < 0.55 && Math.random() < 0.3) {
      nextMode = 'launch_burst';
      targetUclamp = 800;
    } else if (rand > 0.88) {
      nextMode = 'deep_idle';
      targetUclamp = 40;
      targetFps = 60;
    } else {
      nextMode = 'dynamic_balance';
      if (config.adaptiveBias === 'responsiveness') {
        targetUclamp = 420;
      } else if (config.adaptiveBias === 'battery') {
        targetUclamp = 220;
      } else {
        targetUclamp = 320;
      }
    }
  }

  // Adjust for config bias
  if (config.adaptiveBias === 'responsiveness' && nextMode !== 'thermal_guard') {
    targetUclamp = Math.min(1024, Math.round(targetUclamp * 1.15));
  } else if (config.adaptiveBias === 'battery' && nextMode !== 'launch_burst') {
    targetUclamp = Math.max(40, Math.round(targetUclamp * 0.82));
  }

  const fpsJitter = (Math.random() - 0.5) * 1.2;
  const currentFps = Math.min(targetFps, Math.max(30, Math.round(targetFps - jank + fpsJitter)));
  const meta = getModeMeta(nextMode);

  const nextTelemetry: AutonomousTelemetry = {
    ...current,
    mode: nextMode,
    modeLabel: meta.label,
    modeDescription: meta.description,
    uclampMin: targetUclamp,
    currentFps,
    targetFps,
    jankRatePct: Number((Math.max(0, jank + (Math.random() * 0.2 - 0.1))).toFixed(1)),
    socTempCelsius: Number(nextTemp.toFixed(1)),
    topApp: activeApp.pkg,
    topAppLabel: activeApp.label,
    topAppPid: 24000 + Math.floor(Math.random() * 5000),
    epollEventsPerSec: Math.floor(25 + Math.random() * 30),
    batteryDrainEstimatedMh: Math.round(targetUclamp * 0.65 + 180),
    timestamp: Date.now(),
  };

  // Generate occasional reaction event log if mode changed
  let event: ReactionLogEvent | undefined;
  if (current.mode !== nextMode) {
    const timeStr = new Date().toTimeString().slice(0, 8);
    const id = `evt-${Date.now()}-${Math.floor(Math.random() * 1000)}`;

    if (nextMode === 'launch_burst') {
      event = {
        id,
        timestamp: timeStr,
        type: 'BOOST',
        tag: 'LaunchDetector',
        message: `App launch transition detected for ${activeApp.label}: uclamp raised to ${targetUclamp} for ${config.burstDurationMs}ms`,
      };
    } else if (nextMode === 'frame_pacing') {
      event = {
        id,
        timestamp: timeStr,
        type: 'FRAME',
        tag: 'RenderBooster',
        message: `HWUI deadline variance stabilized: render thread pinned and clamped to ${targetUclamp}`,
      };
    } else if (nextMode === 'touch_boost') {
      event = {
        id,
        timestamp: timeStr,
        type: 'BOOST',
        tag: 'TouchReaction',
        message: `Input gesture detected: instantaneous ${config.touchBoostMin} uclamp touch burst applied`,
      };
    } else if (nextMode === 'thermal_guard') {
      event = {
        id,
        timestamp: timeStr,
        type: 'THERMAL',
        tag: 'ThermalGuard',
        message: `SoC reached ${nextTelemetry.socTempCelsius}°C (trip limit ${config.thermalTripCelsius}°C): clamped ceiling to prevent frame throttling`,
      };
    } else if (nextMode === 'dynamic_balance') {
      event = {
        id,
        timestamp: timeStr,
        type: 'CGROUP',
        tag: 'Autonomous',
        message: `Dynamic balance restored: ${activeApp.label} normalized with uclamp=${targetUclamp}`,
      };
    }
  }

  return { next: nextTelemetry, event };
}
