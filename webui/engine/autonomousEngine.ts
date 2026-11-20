/**
 * CoreFlow Engine v5.1.0 — Lean 4-Pillar WebUI Data Contract, Validators & Preview Simulator.
 */

export type PerformanceProfile = 'balanced' | 'smoothness' | 'battery' | 'gaming';
export type AppRuleCategory = 'default' | 'gaming' | 'smoothness' | 'battery' | 'excluded';

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
  smoothness: string[];
  battery: string[];
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
  contendedPaths: string[];
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

export interface SessionSummary {
  uptimeSec: number;
  launchBursts: number;
  thermalGuardEvents: number;
  avgSocTempCelsius: number | null;
  peakSocTempCelsius: number | null;
  avgCapacityScale: number;
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
  effectiveProfile: PerformanceProfile;
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
  renderPressure: number;
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
  session: SessionSummary;
  history: HistorySample[];
}

export interface ReactionLogEvent {
  id: string;
  timestamp: string;
  type: 'INFO' | 'THERMAL' | 'BOOST' | 'BASELINE';
  tag: string;
  message: string;
}

export interface PillarReadinessItem {
  id: string;
  name: string;
  ready: boolean;
  detail: string;
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

export function parseInstalledPackages(rawPmOutput: string): string[] {
  const pkgs: string[] = [];
  for (const line of rawPmOutput.split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed) continue;
    const pkg = trimmed.startsWith('package:') ? trimmed.slice('package:'.length).trim() : trimmed;
    if (isValidPackageName(pkg)) pkgs.push(pkg);
  }
  return Array.from(new Set(pkgs)).sort((a, b) => a.localeCompare(b));
}

export function getPackageRuleCategory(apps: CoreFlowAppRules, pkg: string): AppRuleCategory {
  const lower = pkg.trim().toLowerCase();
  if (!lower) return 'default';
  if (apps.excluded.some((x) => x.toLowerCase() === lower)) return 'excluded';
  if (apps.gaming.some((x) => x.toLowerCase() === lower)) return 'gaming';
  if (apps.smoothness.some((x) => x.toLowerCase() === lower)) return 'smoothness';
  if (apps.battery.some((x) => x.toLowerCase() === lower)) return 'battery';
  return 'default';
}

export function assignPackageRule(
  apps: CoreFlowAppRules,
  rawPkg: string,
  category: AppRuleCategory
): CoreFlowAppRules {
  const pkg = rawPkg.trim();
  if (!isValidPackageName(pkg)) return apps;
  const lower = pkg.toLowerCase();
  const next: CoreFlowAppRules = {
    gaming: apps.gaming.filter((x) => x.toLowerCase() !== lower),
    smoothness: apps.smoothness.filter((x) => x.toLowerCase() !== lower),
    battery: apps.battery.filter((x) => x.toLowerCase() !== lower),
    excluded: apps.excluded.filter((x) => x.toLowerCase() !== lower),
  };
  if (category !== 'default') {
    next[category] = sanitizePackageList([...next[category], pkg]);
  }
  return next;
}

export function formatDuration(seconds: number): string {
  if (!Number.isFinite(seconds) || seconds <= 0) return '0m';
  const total = Math.floor(seconds);
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  if (h > 0) return `${h}h ${m}m`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}

export function computePillarReadiness(status: CoreFlowV5Status): {
  readyCount: number;
  total: number;
  pillars: PillarReadinessItem[];
} {
  const p1 = status.actuators.perThreadUclampSupported && status.features.frameSmoothness;
  const p2 = status.hardware.storageType !== 'Unknown' && status.features.storageBurst;
  const p3 = status.features.memoryGuard;
  const p4 = status.features.thermalPacing && status.thermalSource !== 'unavailable';
  const pillars: PillarReadinessItem[] = [
    {
      id: 'p1',
      name: 'Frame Smoothness',
      ready: p1,
      detail: status.actuators.perThreadUclampSupported
        ? `Per-TID UClamp (${status.actuators.boostedTids} TIDs)`
        : 'Kernel lacks per-TID UClamp',
    },
    {
      id: 'p2',
      name: 'Storage Burst & QoS',
      ready: p2,
      detail:
        status.hardware.storageType !== 'Unknown'
          ? `${status.hardware.storageType} · ${status.actuators.readAheadKb || 'OEM'} KB`
          : 'Storage queue pending',
    },
    {
      id: 'p3',
      name: 'Memory & Pre-Read',
      ready: p3,
      detail: status.hardware.mglruAvailable
        ? `MGLRU Active · ${status.actuators.prereadFiles} prefetched`
        : `Adaptive VM · ${status.actuators.prereadFiles} prefetched`,
    },
    {
      id: 'p4',
      name: 'Thermal PID Guard',
      ready: p4,
      detail: `${Math.round(status.capacityScale * 100)}% budget · ${status.thermalSource}`,
    },
  ];
  return {
    readyCount: pillars.filter((p) => p.ready).length,
    total: 4,
    pillars,
  };
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
  'smoothness_apps',
  'battery_apps',
  'excluded_apps',
]);

const APP_LIST_KEYS = new Set(['gaming_apps', 'smoothness_apps', 'battery_apps', 'excluded_apps']);

// Builds a shell-safe single-quoted argument string for `coreflowd --set-config`.
export function buildSafeSetConfigArg(key: string, value: string): string | null {
  if (!ALLOWED_SET_CONFIG_KEYS.has(key)) return null;
  if (key === 'profile') {
    if (!['balanced', 'smoothness', 'battery', 'gaming'].includes(value)) return null;
  } else if (APP_LIST_KEYS.has(key)) {
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

const parseProfile = (raw: unknown): PerformanceProfile =>
  raw === 'smoothness' || raw === 'battery' || raw === 'gaming' ? raw : 'balanced';

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
  const profile = parseProfile(raw.profile);
  const effectiveProfile = typeof raw.effectiveProfile === 'string' ? parseProfile(raw.effectiveProfile) : profile;

  const features = raw.features;
  const apps = raw.apps;
  const actuators = raw.actuators;
  const hw = raw.hardware;
  const batt = raw.batteryPower;
  const sess = isRecord(raw.session) ? raw.session : {};
  const historyRaw = Array.isArray(raw.history) ? raw.history : [];

  return {
    version: typeof raw.version === 'string' ? raw.version : '5.1.0',
    running: raw.running,
    pid: raw.pid,
    profile,
    effectiveProfile,
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
    renderPressure: isFiniteNum(raw.renderPressure) ? Math.max(0, Math.min(1, raw.renderPressure)) : 0,
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
      smoothness: Array.isArray(apps.smoothness) ? apps.smoothness.filter((x): x is string => typeof x === 'string') : [],
      battery: Array.isArray(apps.battery) ? apps.battery.filter((x): x is string => typeof x === 'string') : [],
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
      contendedPaths: Array.isArray(actuators.contendedPaths)
        ? actuators.contendedPaths.filter((x): x is string => typeof x === 'string')
        : [],
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
    session: {
      uptimeSec: isFiniteNum(sess.uptimeSec) ? sess.uptimeSec : 0,
      launchBursts: isFiniteNum(sess.launchBursts) ? sess.launchBursts : 0,
      thermalGuardEvents: isFiniteNum(sess.thermalGuardEvents) ? sess.thermalGuardEvents : 0,
      avgSocTempCelsius: isFiniteNum(sess.avgSocTempCelsius) ? sess.avgSocTempCelsius : null,
      peakSocTempCelsius: isFiniteNum(sess.peakSocTempCelsius) ? sess.peakSocTempCelsius : null,
      avgCapacityScale: isFiniteNum(sess.avgCapacityScale) ? sess.avgCapacityScale : 1,
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

export const PREVIEW_INSTALLED_PACKAGES: string[] = [
  'com.miHoYo.GenshinImpact',
  'com.kurogame.wutheringwaves.global',
  'com.mobile.legends',
  'com.instagram.android',
  'com.android.chrome',
  'com.whatsapp',
  'com.spotify.music',
  'com.google.android.youtube',
  'com.android.camera',
];

export function createPreviewStatus(): CoreFlowV5Status {
  const now = Date.now();
  return {
    version: '5.1.0',
    running: true,
    pid: 1428,
    profile: 'balanced',
    effectiveProfile: 'balanced',
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
    renderPressure: 0.34,
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
      smoothness: ['com.instagram.android'],
      battery: ['com.whatsapp'],
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
      prereadFiles: 18,
      prereadBytes: 42 * 1024 * 1024,
      managedEnvelopeNodes: 8,
      contendedNodes: 0,
      contendedPaths: [],
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
    session: {
      uptimeSec: 2540,
      launchBursts: 14,
      thermalGuardEvents: 0,
      avgSocTempCelsius: 37.2,
      peakSocTempCelsius: 41.6,
      avgCapacityScale: 0.97,
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
    effectiveProfile: nextMode === 'GAMING' ? 'gaming' : prev.profile,
    foregroundPackage:
      nextMode === 'GAMING' ? 'com.miHoYo.GenshinImpact' :
      nextMode === 'LAUNCH_BURST' ? 'com.instagram.android' : 'com.android.chrome',
    socTempCelsius: temp,
    thermalPressure: pressure,
    cpuUtilPct: cpu,
    renderPressure: nextMode === 'GAMING' ? 0.62 : nextMode === 'LAUNCH_BURST' ? 0.48 : 0.18,
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
    session: {
      ...prev.session,
      uptimeSec: prev.session.uptimeSec + 2,
      launchBursts: prev.session.launchBursts + (nextMode === 'LAUNCH_BURST' ? 1 : 0),
      peakSocTempCelsius: Math.max(prev.session.peakSocTempCelsius ?? temp, temp),
    },
    history: [...prev.history, nextPoint].slice(-60),
  };
}
