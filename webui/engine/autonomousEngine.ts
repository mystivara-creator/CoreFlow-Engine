/**
 * CoreFlow Engine v6.2.0 — Closed-Loop Intelligence WebUI Data Contract, Validators & Preview Simulator.
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
  idleCompaction: boolean;
  predictivePreload: boolean;
  cameraBoost: boolean;
}

export interface HyperFluidTelemetry {
  idleCompactions: number;
  predictivePreloads: number;
  lastPredictedPackage: string;
  cameraChainActive: boolean;
  mediaPipelineActive: boolean;
  memoryHealthPct: number;
  framePacingAvailable: boolean;
  measuredFps: number;
  jankRatio: number | null;
  appDnaLearned: number;
  activeAppBias: number;
  chargingDamped: boolean;
}

export interface CoreFlowPowerConfig {
  bypassCharging: boolean;
  bypassOnGaming: boolean;
  chargeLimitPct: number;
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
  semanticKnobsDiscovered: number;
  semanticKnobsActive: number;
  semanticKnobLeaves: string[];
  contendedNodes: number;
  contendedPaths: string[];
  oemYieldedNodes: number;
  oemYieldedPaths: string[];
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

export interface CpuClusterSnapshot {
  role: string;
  governor: string;
  curKhz: number;
  minKhz: number;
  maxKhz: number;
  capacity: number;
}

export interface ThermalZoneSnapshot {
  id: string;
  type: string;
  category: string;
  celsius: number;
}

export interface MemorySnapshot {
  totalBytes: number;
  availableBytes: number;
  swapTotalBytes: number;
  swapFreeBytes: number;
}

export interface DecisionEpisode {
  id: number;
  tsMs: number;
  domain: string;
  trigger: string;
  action: string;
  outcome: string;
  socTempC: number | null;
  renderPressure: number;
  capacityScale: number;
}

export interface BatteryPowerTelemetry {
  available: boolean;
  charging: boolean;
  fastCharging: boolean;
  thermalStepActive: boolean;
  bypassSupported: boolean;
  bypassActive: boolean;
  bypassReason: string;
  capacityPct: number | null;
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
  thermalVelocityCpm: number;
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
  hyperFluid: HyperFluidTelemetry;
  powerConfig: CoreFlowPowerConfig;
  apps: CoreFlowAppRules;
  actuators: CoreFlowActuatorStats;
  hardware: CoreFlowHardwareInfo;
  memory: MemorySnapshot;
  cpuClusters: CpuClusterSnapshot[];
  thermalZones: ThermalZoneSnapshot[];
  episodes: DecisionEpisode[];
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
  { label: string; tagline: string; taglineId: string }
> = {
  balanced: {
    label: 'Balanced',
    tagline: 'Smart daily speed, 120Hz fluidity & cool battery',
    taglineId: 'Seimbang: mulus 120Hz, buka cepat & baterai adem',
  },
  smoothness: {
    label: 'Smoothness',
    tagline: 'Higher UI & SurfaceFlinger floor for locked scrolling',
    taglineId: 'Floor UI & SurfaceFlinger lebih tinggi untuk scroll mulus',
  },
  battery: {
    label: 'Battery',
    tagline: 'Lean launch burst & rapid core sleep for max SOT',
    taglineId: 'Efisiensi maksimum & suhu dingin untuk daya tahan harian',
  },
  gaming: {
    label: 'Gaming',
    tagline: 'Sustained frame pacing, I/O isolation & PID thermal guard',
    taglineId: 'Frame stabil, isolasi background & penjaga termal PID',
  },
};

export const MODE_LABELS: Record<
  string,
  { label: string; labelId: string; tone: 'success' | 'warning' | 'error' | 'neutral' }
> = {
  LAUNCH_BURST: { label: 'Launch Burst', labelId: 'Lonjakan Buka', tone: 'success' },
  INTERACTIVE: { label: 'Interactive', labelId: 'Interaktif', tone: 'success' },
  GAMING: { label: 'Gaming', labelId: 'Gaming', tone: 'success' },
  EFFICIENCY: { label: 'Efficiency', labelId: 'Efisiensi', tone: 'neutral' },
  SCREEN_OFF: { label: 'Screen Off', labelId: 'Layar Mati', tone: 'neutral' },
  EXCLUDED: { label: 'OEM Stock', labelId: 'OEM Standar', tone: 'neutral' },
  THERMAL_GUARD: { label: 'Thermal Guard', labelId: 'Jaga Suhu', tone: 'warning' },
  THERMAL_EMERGENCY: { label: 'Thermal Cutoff', labelId: 'Darurat Suhu', tone: 'error' },
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
      detail: `${Math.round(status.capacityScale * 100)}% headroom`,
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
  'enable_idle_compaction',
  'enable_predictive_preload',
  'enable_camera_boost',
  'bypass_charging',
  'bypass_on_gaming',
  'charge_limit_pct',
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
  } else if (key === 'charge_limit_pct') {
    if (!/^\d{1,3}$/.test(value)) return null;
    const num = Number(value);
    if (!(num === 0 || (num >= 60 && num <= 95))) return null;
  } else if (APP_LIST_KEYS.has(key)) {
    if (value.length > 0) {
      const items = value.split(',').map((s) => s.trim()).filter(Boolean);
      if (items.some((item) => !isValidPackageName(item))) return null;
      value = sanitizePackageList(items).join(',');
    }
  } else {
    if (value !== 'true' && value !== 'false') return null;
  }
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
    throw new Error('Unexpected CoreFlow status schema');
  }
  const profile = parseProfile(raw.profile);
  const effectiveProfile = typeof raw.effectiveProfile === 'string' ? parseProfile(raw.effectiveProfile) : profile;

  const features = raw.features;
  const hf = isRecord(raw.hyperFluid) ? raw.hyperFluid : {};
  const powerCfg = isRecord(raw.powerConfig) ? raw.powerConfig : {};
  const apps = raw.apps;
  const actuators = raw.actuators;
  const hw = raw.hardware;
  const mem = isRecord(raw.memory) ? raw.memory : {};
  const batt = raw.batteryPower;
  const sess = isRecord(raw.session) ? raw.session : {};
  const historyRaw = Array.isArray(raw.history) ? raw.history : [];
  const clustersRaw = Array.isArray(raw.cpuClusters) ? raw.cpuClusters : [];
  const zonesRaw = Array.isArray(raw.thermalZones) ? raw.thermalZones : [];
  const episodesRaw = Array.isArray(raw.episodes) ? raw.episodes : [];

  return {
    version: typeof raw.version === 'string' ? raw.version : '6.2.0',
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
    thermalVelocityCpm: isFiniteNum(raw.thermalVelocityCpm) ? raw.thermalVelocityCpm : 0,
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
      idleCompaction: features.idleCompaction !== false,
      predictivePreload: features.predictivePreload !== false,
      cameraBoost: features.cameraBoost !== false,
    },
    hyperFluid: {
      idleCompactions: isFiniteNum(hf.idleCompactions) ? hf.idleCompactions : 0,
      predictivePreloads: isFiniteNum(hf.predictivePreloads) ? hf.predictivePreloads : 0,
      lastPredictedPackage: typeof hf.lastPredictedPackage === 'string' ? hf.lastPredictedPackage : '',
      cameraChainActive: hf.cameraChainActive === true,
      mediaPipelineActive: hf.mediaPipelineActive === true,
      memoryHealthPct: isFiniteNum(hf.memoryHealthPct) ? hf.memoryHealthPct : 100,
      framePacingAvailable: hf.framePacingAvailable === true,
      measuredFps: isFiniteNum(hf.measuredFps) ? hf.measuredFps : 0,
      jankRatio: isFiniteNum(hf.jankRatio) ? hf.jankRatio : null,
      appDnaLearned: isFiniteNum(hf.appDnaLearned) ? hf.appDnaLearned : 0,
      activeAppBias: isFiniteNum(hf.activeAppBias) ? hf.activeAppBias : 1.0,
      chargingDamped: hf.chargingDamped === true,
    },
    powerConfig: {
      bypassCharging: powerCfg.bypassCharging === true,
      bypassOnGaming: powerCfg.bypassOnGaming === true,
      chargeLimitPct: isFiniteNum(powerCfg.chargeLimitPct) ? powerCfg.chargeLimitPct : 0,
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
      semanticKnobsDiscovered: isFiniteNum(actuators.semanticKnobsDiscovered) ? actuators.semanticKnobsDiscovered : 0,
      semanticKnobsActive: isFiniteNum(actuators.semanticKnobsActive) ? actuators.semanticKnobsActive : 0,
      semanticKnobLeaves: Array.isArray(actuators.semanticKnobLeaves)
        ? actuators.semanticKnobLeaves.filter((x): x is string => typeof x === 'string')
        : [],
      contendedNodes: isFiniteNum(actuators.contendedNodes) ? actuators.contendedNodes : 0,
      contendedPaths: Array.isArray(actuators.contendedPaths)
        ? actuators.contendedPaths.filter((x): x is string => typeof x === 'string')
        : [],
      oemYieldedNodes: isFiniteNum(actuators.oemYieldedNodes) ? actuators.oemYieldedNodes : 0,
      oemYieldedPaths: Array.isArray(actuators.oemYieldedPaths)
        ? actuators.oemYieldedPaths.filter((x): x is string => typeof x === 'string')
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
    memory: {
      totalBytes: isFiniteNum(mem.totalBytes) ? mem.totalBytes : (isFiniteNum(hw.ramBytes) ? hw.ramBytes : 0),
      availableBytes: isFiniteNum(mem.availableBytes) ? mem.availableBytes : 0,
      swapTotalBytes: isFiniteNum(mem.swapTotalBytes) ? mem.swapTotalBytes : (isFiniteNum(hw.zramBytes) ? hw.zramBytes : 0),
      swapFreeBytes: isFiniteNum(mem.swapFreeBytes) ? mem.swapFreeBytes : 0,
    },
    cpuClusters: clustersRaw
      .filter((c): c is Record<string, unknown> => isRecord(c))
      .map((c) => ({
        role: typeof c.role === 'string' ? c.role : 'cpu',
        governor: typeof c.governor === 'string' ? c.governor : 'schedutil',
        curKhz: isFiniteNum(c.curKhz) ? c.curKhz : 0,
        minKhz: isFiniteNum(c.minKhz) ? c.minKhz : 0,
        maxKhz: isFiniteNum(c.maxKhz) ? c.maxKhz : 0,
        capacity: isFiniteNum(c.capacity) ? c.capacity : 0,
      })),
    thermalZones: zonesRaw
      .filter((z): z is Record<string, unknown> => isRecord(z) && isFiniteNum(z.celsius))
      .map((z) => ({
        id: typeof z.id === 'string' ? z.id : '',
        type: typeof z.type === 'string' ? z.type : 'sensor',
        category: typeof z.category === 'string' ? z.category : 'sensor',
        celsius: z.celsius as number,
      })),
    episodes: episodesRaw
      .filter((e): e is Record<string, unknown> => isRecord(e) && isFiniteNum(e.id))
      .map((e) => ({
        id: e.id as number,
        tsMs: isFiniteNum(e.tsMs) ? e.tsMs : 0,
        domain: typeof e.domain === 'string' ? e.domain : 'context',
        trigger: typeof e.trigger === 'string' ? e.trigger : 'Event',
        action: typeof e.action === 'string' ? e.action : '',
        outcome: typeof e.outcome === 'string' ? e.outcome : 'verified',
        socTempC: isFiniteNum(e.socTempC) ? e.socTempC : null,
        renderPressure: isFiniteNum(e.renderPressure) ? e.renderPressure : 0,
        capacityScale: isFiniteNum(e.capacityScale) ? e.capacityScale : 1,
      })),
    batteryPower: {
      available: batt.available === true,
      charging: batt.charging === true,
      fastCharging: batt.fastCharging === true,
      thermalStepActive: batt.thermalStepActive === true,
      bypassSupported: batt.bypassSupported === true,
      bypassActive: batt.bypassActive === true,
      bypassReason: typeof batt.bypassReason === 'string' ? batt.bypassReason : 'off',
      capacityPct: isFiniteNum(batt.capacityPct) ? batt.capacityPct : null,
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
    version: '6.2.0',
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
    thermalVelocityCpm: 0.1,
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
      idleCompaction: true,
      predictivePreload: true,
      cameraBoost: true,
    },
    hyperFluid: {
      idleCompactions: 2,
      predictivePreloads: 5,
      lastPredictedPackage: 'com.whatsapp',
      cameraChainActive: false,
      mediaPipelineActive: false,
      memoryHealthPct: 94,
      framePacingAvailable: true,
      measuredFps: 119.4,
      jankRatio: 0.008,
      appDnaLearned: 12,
      activeAppBias: 0.94,
      chargingDamped: false,
    },
    powerConfig: {
      bypassCharging: false,
      bypassOnGaming: true,
      chargeLimitPct: 85,
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
      semanticKnobsDiscovered: 25,
      semanticKnobsActive: 6,
      semanticKnobLeaves: ['boost', 'pl', 'bus_split', 'adrenoboost', 'iostats', 'vfs_cache_pressure'],
      contendedNodes: 0,
      contendedPaths: [],
      oemYieldedNodes: 2,
      oemYieldedPaths: ['/proc/sys/walt/sched_min_task_util_for_boost', '/sys/devices/system/cpu/cpufreq/policy0/walt/pl'],
    },
    hardware: {
      socModel: 'Preview ARM64 SoC',
      kernelVersion: '6.1.75-android15-preview',
      storageType: 'UFS',
      gpuModel: 'Adreno / Mali Preview',
      ramBytes: 12 * 1024 * 1024 * 1024,
      zramBytes: 4 * 1024 * 1024 * 1024,
      refreshHz: 120,
      cpuClusters: 3,
      mglruAvailable: true,
      cgroupV2Available: true,
    },
    memory: {
      totalBytes: 12 * 1024 * 1024 * 1024,
      availableBytes: 4.8 * 1024 * 1024 * 1024,
      swapTotalBytes: 4 * 1024 * 1024 * 1024,
      swapFreeBytes: 2.6 * 1024 * 1024 * 1024,
    },
    cpuClusters: [
      { role: 'little', governor: 'walt', curKhz: 1459200, minKhz: 595200, maxKhz: 2016000, capacity: 384 },
      { role: 'mid', governor: 'walt', curKhz: 1958400, minKhz: 710400, maxKhz: 2803200, capacity: 768 },
      { role: 'big', governor: 'walt', curKhz: 2304000, minKhz: 844800, maxKhz: 3187200, capacity: 1024 },
    ],
    thermalZones: [
      { id: 'tz0', type: 'cpu-0-0', category: 'compute', celsius: 38.4 },
      { id: 'tz1', type: 'gpuss-0', category: 'compute', celsius: 36.8 },
      { id: 'tz2', type: 'skin-msm', category: 'skin', celsius: 34.2 },
      { id: 'tz3', type: 'battery', category: 'battery', celsius: 33.4 },
    ],
    episodes: [
      {
        id: 3,
        tsMs: now - 4000,
        domain: 'storage',
        trigger: 'LaunchBurst',
        action: 'Boosted com.android.chrome',
        outcome: 'verified',
        socTempC: 37.8,
        renderPressure: 0.34,
        capacityScale: 0.96,
      },
      {
        id: 2,
        tsMs: now - 14000,
        domain: 'power',
        trigger: 'BypassReady',
        action: 'CFJ4 verified input_suspend charging node',
        outcome: 'verified',
        socTempC: 37.2,
        renderPressure: 0.18,
        capacityScale: 0.98,
      },
      {
        id: 1,
        tsMs: now - 26000,
        domain: 'context',
        trigger: 'ModeShift',
        action: 'EFFICIENCY -> INTERACTIVE (com.android.chrome)',
        outcome: 'verified',
        socTempC: 36.9,
        renderPressure: 0.22,
        capacityScale: 0.98,
      },
    ],
    batteryPower: {
      available: true,
      charging: false,
      fastCharging: false,
      thermalStepActive: false,
      bypassSupported: true,
      bypassActive: false,
      bypassReason: 'off',
      capacityPct: 78,
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
    thermalVelocityCpm: nextMode === 'GAMING' ? 0.9 : nextMode === 'EFFICIENCY' ? -0.7 : 0.1,
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
