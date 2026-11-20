/**
 * CoreFlow Engine v5.0.1 — Lean 4-Pillar WebUI Data Contract, Validators & Preview Simulator.
 */

export type PerformanceProfile = 'balanced' | 'smoothness' | 'battery' | 'gaming';

export type ContextModeName =
  | 'LAUNCH_BURST'
  | 'INTERACTIVE'
  | 'GAMING'
  | 'EFFICIENCY'
  | 'SCREEN_OFF'
  | 'EXCLUDED'
  | 'THERMAL_GUARD'
  | 'THERMAL_EMERGENCY';

export interface CoreFlowFeatures {
  frameSmoothness: boolean;
  storageBurst: boolean;
  memoryGuard: boolean;
  thermalPacing: boolean;
  batteryStepCharging: boolean;
}

export interface CoreFlowAppRules {
  gaming: string[];
  excluded: string[];
}

export interface CoreFlowActuatorStats {
  perThreadUclampSupported: boolean;
  boostedTids: number;
  threadUclampMin: number;
  threadUclampMax: number;
  storageBurstActive: boolean;
  readAheadKb: number;
  backgroundIsolated: boolean;
  prereadFiles: number;
  prereadBytes: number;
  managedEnvelopeNodes: number;
  contendedNodes: number;
}

export interface CoreFlowHardwareInfo {
  socModel: string;
  kernelVersion: string;
  storageType: string;
  gpuModel: string;
  ramBytes: number;
  zramBytes: number;
  refreshHz: number;
  cpuClusters: number;
  mglruAvailable: boolean;
  cgroupV2Available: boolean;
}

export interface BatteryPowerTelemetry {
  available: boolean;
  charging: boolean;
  fastCharging: boolean;
  thermalStepActive: boolean;
  temperatureCelsius: number | null;
  currentMa: number;
  voltageMv: number;
  powerMw: number;
  chargeLimitUa: number;
  oemBaselineUa: number;
  status: string;
}

export interface HistorySample {
  ts: number;
  temp: number | null;
  pressure: number;
  cpu: number;
  scale: number;
  mode: string;
}

export interface CoreFlowV5Status {
  version: string;
  running: boolean;
  pid: number;
  profile: PerformanceProfile;
  inspectionMode: boolean;
  mode: ContextModeName;
  foregroundPackage: string;
  foregroundPid: number;
  capacityScale: number;
  thermalGuard: boolean;
  socTempCelsius: number | null;
  batteryTempCelsius: number | null;
  thermalPressure: number;
  thermalSource: string;
  cpuUtilPct: number;
  gpuBusyPct: number | null;
  cpuPsi: number | null;
  memPsi: number | null;
  ioPsi: number | null;
  aiPredicted10s: number;
  aiConfidence: number;
  baselineNodesSnapshotted: number;
  recoveryPending: boolean;
  features: CoreFlowFeatures;
  apps: CoreFlowAppRules;
  actuators: CoreFlowActuatorStats;
  hardware: CoreFlowHardwareInfo;
  batteryPower: BatteryPowerTelemetry;
  history: HistorySample[];
}

export interface ReactionLogEvent {
  id: string;
  timestamp: string;
  type: 'INFO' | 'THERMAL' | 'BOOST' | 'BASELINE';
  tag: string;
  message: string;
}

export const PROFILE_META: Record<
  PerformanceProfile,
  { label: string; tagline: string }
> = {
  balanced: {
    label: 'Balanced',
    tagline: 'Daily equilibrium: smooth 120Hz touch, fast launch burst, cool battery',
  },
  smoothness: {
    label: 'Smoothness',
    tagline: 'Higher UI & SurfaceFlinger floor for ultra-consistent frame pacing',
  },
  battery: {
    label: 'Battery',
    tagline: 'Lean launch burst and rapid core sleep for maximum screen-on time',
  },
  gaming: {
    label: 'Gaming',
    tagline: 'Sustained GPU/CPU pacing, strict background isolation, PID thermal guard',
  },
};

export const MODE_LABELS: Record<string, { label: string; tone: 'success' | 'warning' | 'error' | 'neutral' }> = {
  LAUNCH_BURST: { label: 'Launch Burst', tone: 'success' },
  INTERACTIVE: { label: 'Interactive UI', tone: 'success' },
  GAMING: { label: 'Sustained Gaming', tone: 'success' },
  EFFICIENCY: { label: 'Efficiency', tone: 'neutral' },
  SCREEN_OFF: { label: 'Screen Off', tone: 'neutral' },
  EXCLUDED: { label: 'Excluded (OEM Stock)', tone: 'neutral' },
  THERMAL_GUARD: { label: 'Thermal Guard', tone: 'warning' },
  THERMAL_EMERGENCY: { label: 'Thermal Emergency', tone: 'error' },
};

const isRecord = (v: unknown): v is Record<string, unknown> =>
  typeof v === 'object' && v !== null && !Array.isArray(v);

const isFiniteNum = (v: unknown): v is number =>
  typeof v === 'number' && Number.isFinite(v);

const PACKAGE_RE = /^[A-Za-z0-9][A-Za-z0-9._-]{1,126}$/;

export function isValidPackageName(pkg: string): boolean {
  const trimmed = pkg.trim();
  return PACKAGE_RE.test(trimmed) && trimmed.includes('.');
}

export function sanitizePackageList(packages: string[]): string[] {
  const seen = new Set<string>();
  const out: string[] = [];
  for (const raw of packages) {
    const trimmed = raw.trim();
    if (!isValidPackageName(trimmed)) continue;
    const key = trimmed.toLowerCase();
    if (seen.has(key)) continue;
    seen.add(key);
    out.push(trimmed);
    if (out.length >= 64) break;
  }
  return out;
}

const ALLOWED_SET_CONFIG_KEYS = new Set([
  'profile',
  'inspection_mode',
  'enable_frame_smoothness',
  'enable_storage_burst',
  'enable_memory_guard',
  'enable_thermal_pacing',
  'enable_battery_step_charging',
  'gaming_apps',
  'excluded_apps',
]);

// Builds a shell-safe single-quoted argument string for `coreflowd --set-config`.
export function buildSafeSetConfigArg(key: string, value: string): string | null {
  if (!ALLOWED_SET_CONFIG_KEYS.has(key)) return null;
  if (key === 'profile') {
    if (!['balanced', 'smoothness', 'battery', 'gaming'].includes(value)) return null;
  } else if (key === 'gaming_apps' || key === 'excluded_apps') {
    if (value.length > 0) {
      const items = value.split(',').map((s) => s.trim()).filter(Boolean);
      if (items.some((item) => !isValidPackageName(item))) return null;
      value = sanitizePackageList(items).join(',');
    }
  } else {
    if (value !== 'true' && value !== 'false') return null;
  }
  // Single quotes with strict allowlisted charset make shell injection impossible.
  return `'${key}=${value}'`;
}

export function parseV5Status(raw: unknown): CoreFlowV5Status {
  if (!isRecord(raw)) throw new Error('Invalid status JSON root');
  if (
    typeof raw.running !== 'boolean' ||
    !isFiniteNum(raw.pid) ||
    typeof raw.mode !== 'string' ||
    !isRecord(raw.features) ||
    !isRecord(raw.apps) ||
    !isRecord(raw.actuators) ||
    !isRecord(raw.hardware) ||
    !isRecord(raw.batteryPower)
  ) {
    throw new Error('Unexpected CoreFlow v5 status schema');
  }
  const profileRaw = typeof raw.profile === 'string' ? raw.profile : 'balanced';
  const profile: PerformanceProfile =
    profileRaw === 'smoothness' || profileRaw === 'battery' || profileRaw === 'gaming'
      ? profileRaw
      : 'balanced';

  const features = raw.features;
  const apps = raw.apps;
  const actuators = raw.actuators;
  const hw = raw.hardware;
  const batt = raw.batteryPower;
  const historyRaw = Array.isArray(raw.history) ? raw.history : [];

  return {
    version: typeof raw.version === 'string' ? raw.version : '5.0.1',
    running: raw.running,
    pid: raw.pid,
    profile,
    inspectionMode: raw.inspectionMode === true,
    mode: (raw.mode as ContextModeName) ?? 'EFFICIENCY',
    foregroundPackage: typeof raw.foregroundPackage === 'string' ? raw.foregroundPackage : '',
    foregroundPid: isFiniteNum(raw.foregroundPid) ? raw.foregroundPid : 0,
    capacityScale: isFiniteNum(raw.capacityScale) ? Math.max(0.2, Math.min(1, raw.capacityScale)) : 1,
    thermalGuard: raw.thermalGuard === true,
    socTempCelsius: isFiniteNum(raw.socTempCelsius) ? raw.socTempCelsius : null,
    batteryTempCelsius: isFiniteNum(raw.batteryTempCelsius) ? raw.batteryTempCelsius : null,
    thermalPressure: isFiniteNum(raw.thermalPressure) ? raw.thermalPressure : 0,
    thermalSource: typeof raw.thermalSource === 'string' ? raw.thermalSource : 'unavailable',
    cpuUtilPct: isFiniteNum(raw.cpuUtilPct) ? raw.cpuUtilPct : 0,
    gpuBusyPct: isFiniteNum(raw.gpuBusyPct) ? raw.gpuBusyPct : null,
    cpuPsi: isFiniteNum(raw.cpuPsi) ? raw.cpuPsi : null,
    memPsi: isFiniteNum(raw.memPsi) ? raw.memPsi : null,
    ioPsi: isFiniteNum(raw.ioPsi) ? raw.ioPsi : null,
    aiPredicted10s: isFiniteNum(raw.aiPredicted10s) ? raw.aiPredicted10s : 0,
    aiConfidence: isFiniteNum(raw.aiConfidence) ? raw.aiConfidence : 0,
    baselineNodesSnapshotted: isFiniteNum(raw.baselineNodesSnapshotted) ? raw.baselineNodesSnapshotted : 0,
    recoveryPending: raw.recoveryPending === true,
    features: {
      frameSmoothness: features.frameSmoothness !== false,
      storageBurst: features.storageBurst !== false,
      memoryGuard: features.memoryGuard !== false,
      thermalPacing: features.thermalPacing !== false,
      batteryStepCharging: features.batteryStepCharging !== false,
    },
    apps: {
      gaming: Array.isArray(apps.gaming) ? apps.gaming.filter((x): x is string => typeof x === 'string') : [],
      excluded: Array.isArray(apps.excluded) ? apps.excluded.filter((x): x is string => typeof x === 'string') : [],
    },
    actuators: {
      perThreadUclampSupported: actuators.perThreadUclampSupported === true,
      boostedTids: isFiniteNum(actuators.boostedTids) ? actuators.boostedTids : 0,
      threadUclampMin: isFiniteNum(actuators.threadUclampMin) ? actuators.threadUclampMin : 0,
      threadUclampMax: isFiniteNum(actuators.threadUclampMax) ? actuators.threadUclampMax : 1024,
      storageBurstActive: actuators.storageBurstActive === true,
      readAheadKb: isFiniteNum(actuators.readAheadKb) ? actuators.readAheadKb : 0,
      backgroundIsolated: actuators.backgroundIsolated === true,
      prereadFiles: isFiniteNum(actuators.prereadFiles) ? actuators.prereadFiles : 0,
      prereadBytes: isFiniteNum(actuators.prereadBytes) ? actuators.prereadBytes : 0,
      managedEnvelopeNodes: isFiniteNum(actuators.managedEnvelopeNodes) ? actuators.managedEnvelopeNodes : 0,
      contendedNodes: isFiniteNum(actuators.contendedNodes) ? actuators.contendedNodes : 0,
    },
    hardware: {
      socModel: typeof hw.socModel === 'string' ? hw.socModel : 'Unknown SoC',
      kernelVersion: typeof hw.kernelVersion === 'string' ? hw.kernelVersion : 'Unknown',
      storageType: typeof hw.storageType === 'string' ? hw.storageType : 'Unknown',
      gpuModel: typeof hw.gpuModel === 'string' ? hw.gpuModel : 'unavailable',
      ramBytes: isFiniteNum(hw.ramBytes) ? hw.ramBytes : 0,
      zramBytes: isFiniteNum(hw.zramBytes) ? hw.zramBytes : 0,
      refreshHz: isFiniteNum(hw.refreshHz) ? hw.refreshHz : 60,
      cpuClusters: isFiniteNum(hw.cpuClusters) ? hw.cpuClusters : 0,
      mglruAvailable: hw.mglruAvailable === true,
      cgroupV2Available: hw.cgroupV2Available === true,
    },
    batteryPower: {
      available: batt.available === true,
      charging: batt.charging === true,
      fastCharging: batt.fastCharging === true,
      thermalStepActive: batt.thermalStepActive === true,
      temperatureCelsius: isFiniteNum(batt.temperatureCelsius) ? batt.temperatureCelsius : null,
      currentMa: isFiniteNum(batt.currentMa) ? batt.currentMa : 0,
      voltageMv: isFiniteNum(batt.voltageMv) ? batt.voltageMv : 0,
      powerMw: isFiniteNum(batt.powerMw) ? batt.powerMw : 0,
      chargeLimitUa: isFiniteNum(batt.chargeLimitUa) ? batt.chargeLimitUa : 0,
      oemBaselineUa: isFiniteNum(batt.oemBaselineUa) ? batt.oemBaselineUa : 0,
      status: typeof batt.status === 'string' && batt.status.length > 0 ? batt.status : 'Unavailable',
    },
    history: historyRaw
      .filter((h): h is Record<string, unknown> => isRecord(h) && isFiniteNum(h.ts))
      .map((h) => ({
        ts: h.ts as number,
        temp: isFiniteNum(h.temp) ? h.temp : null,
        pressure: isFiniteNum(h.pressure) ? h.pressure : 0,
        cpu: isFiniteNum(h.cpu) ? h.cpu : 0,
        scale: isFiniteNum(h.scale) ? h.scale : 1,
        mode: typeof h.mode === 'string' ? h.mode : 'EFFICIENCY',
      })),
  };
}

export function computeHealthScore(status: CoreFlowV5Status): number {
  let score = 100;
  if (status.mode === 'THERMAL_EMERGENCY') score -= 45;
  else if (status.thermalGuard) score -= 22;
  else if (status.thermalPressure >= 65) score -= 10;
  if (status.actuators.contendedNodes > 0) score -= Math.min(12, status.actuators.contendedNodes * 4);
  if (status.recoveryPending) score -= 15;
  return Math.max(15, Math.min(100, Math.round(score)));
}

export function createPreviewStatus(): CoreFlowV5Status {
  const now = Date.now();
  return {
    version: '5.0.1',
    running: true,
    pid: 1428,
    profile: 'balanced',
    inspectionMode: false,
    mode: 'INTERACTIVE',
    foregroundPackage: 'com.android.chrome',
    foregroundPid: 8421,
    capacityScale: 0.96,
    thermalGuard: false,
    socTempCelsius: 37.8,
    batteryTempCelsius: 33.4,
    thermalPressure: 41.2,
    thermalSource: 'android-thermal-preview',
    cpuUtilPct: 24.5,
    gpuBusyPct: 18.0,
    cpuPsi: 1.2,
    memPsi: 0.3,
    ioPsi: 0.4,
    aiPredicted10s: 42.5,
    aiConfidence: 0.84,
    baselineNodesSnapshotted: 14,
    recoveryPending: false,
    features: {
      frameSmoothness: true,
      storageBurst: true,
      memoryGuard: true,
      thermalPacing: true,
      batteryStepCharging: true,
    },
    apps: {
      gaming: ['com.miHoYo.GenshinImpact'],
      excluded: ['com.android.camera'],
    },
    actuators: {
      perThreadUclampSupported: true,
      boostedTids: 4,
      threadUclampMin: 288,
      threadUclampMax: 983,
      storageBurstActive: false,
      readAheadKb: 128,
      backgroundIsolated: true,
      prereadFiles: 12,
      prereadBytes: 24 * 1024 * 1024,
      managedEnvelopeNodes: 8,
      contendedNodes: 0,
    },
    hardware: {
      socModel: 'Preview ARM64 SoC (Browser Demo)',
      kernelVersion: '6.1.75-android14-preview',
      storageType: 'UFS',
      gpuModel: 'Adreno / Mali Preview',
      ramBytes: 12 * 1024 * 1024 * 1024,
      zramBytes: 4 * 1024 * 1024 * 1024,
      refreshHz: 120,
      cpuClusters: 3,
      mglruAvailable: true,
      cgroupV2Available: true,
    },
    batteryPower: {
      available: true,
      charging: false,
      fastCharging: false,
      thermalStepActive: false,
      temperatureCelsius: 33.4,
      currentMa: -460,
      voltageMv: 4120,
      powerMw: 1895,
      chargeLimitUa: 3000000,
      oemBaselineUa: 3000000,
      status: 'Discharging',
    },
    history: Array.from({ length: 20 }, (_, i) => ({
      ts: now - (19 - i) * 2000,
      temp: Number((37.0 + Math.sin(i * 0.4) * 1.2).toFixed(1)),
      pressure: Number((40 + Math.sin(i * 0.4) * 6).toFixed(1)),
      cpu: Number((22 + Math.abs(Math.cos(i * 0.5)) * 18).toFixed(1)),
      scale: 0.96,
      mode: i % 6 === 0 ? 'LAUNCH_BURST' : 'INTERACTIVE',
    })),
  };
}

export function stepPreviewStatus(prev: CoreFlowV5Status, tick: number): CoreFlowV5Status {
  const modes: ContextModeName[] = ['INTERACTIVE', 'LAUNCH_BURST', 'GAMING', 'EFFICIENCY'];
  const nextMode = modes[tick % modes.length];
  const temp = Number((36.5 + (nextMode === 'GAMING' ? 4.2 : nextMode === 'LAUNCH_BURST' ? 1.8 : 0.4) + Math.sin(tick * 0.6) * 0.6).toFixed(1));
  const pressure = Number((nextMode === 'GAMING' ? 62 : nextMode === 'LAUNCH_BURST' ? 48 : 34).toFixed(1));
  const scale = nextMode === 'GAMING' ? 0.91 : 0.98;
  const cpu = Number((nextMode === 'GAMING' ? 54 : nextMode === 'LAUNCH_BURST' ? 42 : 16).toFixed(1));
  const nextPoint: HistorySample = { ts: Date.now(), temp, pressure, cpu, scale, mode: nextMode };
  return {
    ...prev,
    mode: nextMode,
    foregroundPackage:
      nextMode === 'GAMING' ? 'com.miHoYo.GenshinImpact' :
      nextMode === 'LAUNCH_BURST' ? 'com.instagram.android' : 'com.android.chrome',
    socTempCelsius: temp,
    thermalPressure: pressure,
    cpuUtilPct: cpu,
    gpuBusyPct: nextMode === 'GAMING' ? 68 : nextMode === 'LAUNCH_BURST' ? 24 : 8,
    capacityScale: scale,
    actuators: {
      ...prev.actuators,
      boostedTids: nextMode === 'EFFICIENCY' ? 0 : 4,
      threadUclampMin: nextMode === 'LAUNCH_BURST' ? 512 : nextMode === 'GAMING' ? 384 : nextMode === 'INTERACTIVE' ? 288 : 0,
      threadUclampMax: Math.round(1024 * scale),
      storageBurstActive: nextMode === 'LAUNCH_BURST',
      readAheadKb: nextMode === 'LAUNCH_BURST' ? 512 : 128,
      backgroundIsolated: nextMode !== 'EFFICIENCY',
    },
    history: [...prev.history, nextPoint].slice(-60),
  };
}
