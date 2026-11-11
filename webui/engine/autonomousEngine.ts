// CFAE System Framework v3.1.0 — Coordinated Mutation Transaction & Preview Engine.
// Native mode reads real daemon telemetry; this simulator models the 5-layer
// Mutation Safety architecture + Multi-Domain Autonomous Scheduler & Tunable Evaluator for browser preview.

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
  | 'RETAINED'
  | 'ROLLED_BACK';

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
  velocityValid?: boolean;
  velocityCelsiusPerSec?: number | null;
  velocitySamples?: number;
  velocityDurationSec?: number;
}

export interface BlockQueueTelemetry {
  device: string;
  activeScheduler: string;
  baselineScheduler: string;
  availableSchedulers: string[];
  readAheadKb: number;
  nrRequests: number;
  rqAffinity: number;
  readExpireMs: number;
  writesStarved: number;
  fifoBatch: number;
}

export interface TunerCandidateTelemetry {
  id: string;
  label: string;
  domain: string;
  trials: number;
  successes: number;
  rollbacks: number;
  meanDelta: number;
  ucbScore: number;
  cooldownTicks: number;
}

export type VulkanLayerMode = 'safe_hardware' | 'full_capability_override' | 'disabled';

export interface VulkanEngineConfig {
  mode: VulkanLayerMode;
  enableSkiavkThreaded: boolean;
  enableAdaptiveVrs: boolean;
  enablePipelineCachePrewarm: boolean;
}

export const DEFAULT_VULKAN_CONFIG: VulkanEngineConfig = {
  mode: 'disabled',
  enableSkiavkThreaded: true,
  enableAdaptiveVrs: true,
  enablePipelineCachePrewarm: true,
};

export interface VulkanTelemetry {
  available: boolean;
  layerInstalled: boolean;
  layerActive: boolean;
  mode: VulkanLayerMode;
  vendor: string;
  deviceName: string;
  apiVersion: string;
  effectiveApiVersion: string;
  driverVersion?: string;
  deviceExtensions: number;
  effectiveDeviceExtensions: number;
  instanceExtensions: number;
  deviceUuid: string;
  driverUuid: string;
  pipelineCacheUuid: string;
  // Section 1: Capabilities
  rayTracing: boolean;
  meshShaders: boolean;
  variableRateShading: boolean;
  cooperativeMatrix: boolean;
  rtPipeline: boolean;
  rayQuery: boolean;
  accelerationStructure: boolean;
  portabilitySubset: boolean;
  // Section 2: Core Features
  robustBufferAccess: boolean;
  samplerAnisotropy: boolean;
  shaderInt16: boolean;
  shaderFloat64: boolean;
  // Section 3: Vulkan 1.1
  protectedMemory: boolean;
  // Section 4: Vulkan 1.2
  timelineSemaphore: boolean;
  descriptorIndexing: boolean;
  bufferDeviceAddress: boolean;
  // Section 5: Vulkan 1.3
  dynamicRendering: boolean;
  synchronization2: boolean;
  // Section 6: Vulkan 1.4
  indexTypeUint8: boolean;
  lineRasterization: boolean;
  pushDescriptors: boolean;
  globalPriorityQuery: boolean;
  shaderFloatControls2: boolean;
  maxPushDescriptors: number;
  // Section 7: Highlights
  maintenance5: boolean;
  maintenance6: boolean;
  maintenance7: boolean;
  descriptorBuffer: boolean;
  graphicsPipelineLibrary: boolean;
  // Hardware baseline flags (discovered from physical GPU ICD)
  hwRayTracing?: boolean;
  hwMeshShaders?: boolean;
  hwVariableRateShading?: boolean;
  hwCooperativeMatrix?: boolean;
  hwRtPipeline?: boolean;
  hwRayQuery?: boolean;
  hwAccelerationStructure?: boolean;
  hwShaderFloat64?: boolean;
  hwIndexTypeUint8?: boolean;
  hwLineRasterization?: boolean;
  hwPushDescriptors?: boolean;
  hwGlobalPriorityQuery?: boolean;
  hwShaderFloatControls2?: boolean;
  hwMaintenance5?: boolean;
  hwMaintenance6?: boolean;
  hwMaintenance7?: boolean;
  hwDescriptorBuffer?: boolean;
  hwGraphicsPipelineLibrary?: boolean;
  // Runtime Pipeline
  skiavkActive: boolean;
  adaptiveVrsActive: boolean;
  pipelineCachePrewarmActive: boolean;
  // Universal Multi-Vendor GPU Kernel Surface
  gpuKernelAvailable: boolean;
  gpuKernelLabel: string;
  gpuGovernor: string;
  gpuCurrentFreqMhz: number;
  gpuMaxFreqMhz: number;
  adrenoboostLevel: number;
}

export function computeVulkanTelemetry(
  vkConfig: VulkanEngineConfig,
  base?: Partial<VulkanTelemetry>
): VulkanTelemetry {
  // Clamp legacy full_capability_override to safe_hardware pass-through.
  // Never fabricate API versions, extension counts, or unsupported hardware feature flags.
  const mode: VulkanLayerMode =
    vkConfig.mode === 'full_capability_override' ? 'safe_hardware' : vkConfig.mode;
  const isActive = mode !== 'disabled';
  const baseExts = base?.deviceExtensions ?? 0;

  const hwRayTracing = base?.hwRayTracing ?? base?.rayTracing ?? false;
  const hwMeshShaders = base?.hwMeshShaders ?? base?.meshShaders ?? false;
  const hwVariableRateShading = base?.hwVariableRateShading ?? base?.variableRateShading ?? false;
  const hwCooperativeMatrix = base?.hwCooperativeMatrix ?? base?.cooperativeMatrix ?? false;
  const hwRtPipeline = base?.hwRtPipeline ?? base?.rtPipeline ?? false;
  const hwRayQuery = base?.hwRayQuery ?? base?.rayQuery ?? false;
  const hwAccelerationStructure = base?.hwAccelerationStructure ?? base?.accelerationStructure ?? false;
  const hwShaderFloat64 = base?.hwShaderFloat64 ?? base?.shaderFloat64 ?? false;

  const hwIndexTypeUint8 = base?.hwIndexTypeUint8 ?? base?.indexTypeUint8 ?? false;
  const hwLineRasterization = base?.hwLineRasterization ?? base?.lineRasterization ?? false;
  const hwPushDescriptors = base?.hwPushDescriptors ?? base?.pushDescriptors ?? false;
  const hwGlobalPriorityQuery = base?.hwGlobalPriorityQuery ?? base?.globalPriorityQuery ?? false;
  const hwShaderFloatControls2 = base?.hwShaderFloatControls2 ?? base?.shaderFloatControls2 ?? false;
  const hwMaintenance5 = base?.hwMaintenance5 ?? base?.maintenance5 ?? false;
  const hwMaintenance6 = base?.hwMaintenance6 ?? base?.maintenance6 ?? false;
  const hwMaintenance7 = base?.hwMaintenance7 ?? base?.maintenance7 ?? false;
  const hwDescriptorBuffer = base?.hwDescriptorBuffer ?? base?.descriptorBuffer ?? false;
  const hwGraphicsPipelineLibrary = base?.hwGraphicsPipelineLibrary ?? base?.graphicsPipelineLibrary ?? false;

  const hwApiVersion = base?.apiVersion ?? 'Probing ICD...';

  return {
    available: base?.available ?? false,
    layerInstalled: base?.layerInstalled ?? true,
    layerActive: isActive,
    mode,
    vendor: base?.vendor ?? 'Auto-Detecting GPU Vendor',
    deviceName: base?.deviceName ?? 'Probing Physical GPU ICD...',
    apiVersion: hwApiVersion,
    effectiveApiVersion: hwApiVersion,
    driverVersion: base?.driverVersion ?? '0.0.0',
    deviceExtensions: baseExts,
    effectiveDeviceExtensions: baseExts,
    instanceExtensions: base?.instanceExtensions ?? 0,
    deviceUuid: base?.deviceUuid ?? '00000000-0000-0000-0000-000000000000',
    driverUuid: base?.driverUuid ?? '00000000-0000-0000-0000-000000000000',
    pipelineCacheUuid: base?.pipelineCacheUuid ?? '00000000-0000-0000-0000-000000000000',
    // Strictly report hardware-verified support (never falsify unsupported features)
    rayTracing: hwRayTracing,
    meshShaders: hwMeshShaders,
    variableRateShading: hwVariableRateShading,
    cooperativeMatrix: hwCooperativeMatrix,
    rtPipeline: hwRtPipeline,
    rayQuery: hwRayQuery,
    accelerationStructure: hwAccelerationStructure,
    portabilitySubset: base?.portabilitySubset ?? false,
    // Core Features
    robustBufferAccess: base?.robustBufferAccess ?? true,
    samplerAnisotropy: base?.samplerAnisotropy ?? true,
    shaderInt16: base?.shaderInt16 ?? true,
    shaderFloat64: hwShaderFloat64,
    // Vulkan 1.1
    protectedMemory: base?.protectedMemory ?? true,
    // Vulkan 1.2
    timelineSemaphore: base?.timelineSemaphore ?? true,
    descriptorIndexing: base?.descriptorIndexing ?? true,
    bufferDeviceAddress: base?.bufferDeviceAddress ?? true,
    // Vulkan 1.3
    dynamicRendering: base?.dynamicRendering ?? true,
    synchronization2: base?.synchronization2 ?? true,
    // Vulkan 1.4
    indexTypeUint8: hwIndexTypeUint8,
    lineRasterization: hwLineRasterization,
    pushDescriptors: hwPushDescriptors,
    globalPriorityQuery: hwGlobalPriorityQuery,
    shaderFloatControls2: hwShaderFloatControls2,
    maxPushDescriptors: base?.maxPushDescriptors ?? (hwPushDescriptors ? 32 : 0),
    // Highlights
    maintenance5: hwMaintenance5,
    maintenance6: hwMaintenance6,
    maintenance7: hwMaintenance7,
    descriptorBuffer: hwDescriptorBuffer,
    graphicsPipelineLibrary: hwGraphicsPipelineLibrary,
    // Hardware baselines
    hwRayTracing,
    hwMeshShaders,
    hwVariableRateShading,
    hwCooperativeMatrix,
    hwRtPipeline,
    hwRayQuery,
    hwAccelerationStructure,
    hwShaderFloat64,
    hwIndexTypeUint8,
    hwLineRasterization,
    hwPushDescriptors,
    hwGlobalPriorityQuery,
    hwShaderFloatControls2,
    hwMaintenance5,
    hwMaintenance6,
    hwMaintenance7,
    hwDescriptorBuffer,
    hwGraphicsPipelineLibrary,
    // Runtime Pipeline
    skiavkActive: isActive && vkConfig.enableSkiavkThreaded,
    adaptiveVrsActive: isActive && vkConfig.enableAdaptiveVrs && hwVariableRateShading,
    pipelineCachePrewarmActive: isActive && vkConfig.enablePipelineCachePrewarm,
    // GPU Kernel Surface
    gpuKernelAvailable: base?.gpuKernelAvailable ?? false,
    gpuKernelLabel: base?.gpuKernelLabel ?? 'Universal Multi-Vendor GPU Devfreq',
    gpuGovernor: base?.gpuGovernor ?? 'auto',
    gpuCurrentFreqMhz: base?.gpuCurrentFreqMhz ?? 0,
    gpuMaxFreqMhz: base?.gpuMaxFreqMhz ?? 0,
    adrenoboostLevel: base?.adrenoboostLevel ?? 0,
  };
}

export const CANONICAL_KEY_SECTIONS: Record<string, string> = {
  version: 'general',
  log_level: 'general',
  zero_polling_mode: 'general',
  enable_cgroups_v2: 'general',
  enable_uclamp: 'general',
  tier_override: 'device_adaptation',
  unlock_vendor_clamping: 'device_adaptation',
  force_120hz_rendering: 'device_adaptation',
  enable_cpu_frequency_control: 'device_adaptation',
  enable_gpu_frequency_control: 'device_adaptation',
  top_app_interactive_min: 'uclamp_tuning',
  top_app_interactive_max: 'uclamp_tuning',
  ui_adaptive_bias: 'uclamp_tuning',
  ui_balanced_uclamp_min: 'uclamp_tuning',
  foreground_boost_min: 'uclamp_tuning',
  background_clamp_max: 'uclamp_tuning',
  system_background_max: 'uclamp_tuning',
  temp_monitor_interval_ms: 'thermal_guardian',
  thermal_trip_celsius: 'thermal_guardian',
  thermal_recover_celsius: 'thermal_guardian',
  emergency_rollback_on_overheat: 'thermal_guardian',
  cpu_pressure_boost_pct: 'stability_policy',
  memory_pressure_guard_pct: 'stability_policy',
  io_pressure_guard_pct: 'stability_policy',
  thermal_headroom_celsius: 'stability_policy',
  thermal_emergency_margin_celsius: 'stability_policy',
  enable_pid_equilibrium: 'pid_equilibrium',
  pid_target_celsius: 'pid_equilibrium',
  pid_kp: 'pid_equilibrium',
  pid_ki: 'pid_equilibrium',
  pid_kd: 'pid_equilibrium',
  enable_battery_step_charging: 'hardware_longevity',
  enable_ufs_burst_pacing: 'hardware_longevity',
  battery_step_temp_threshold: 'hardware_longevity',
  stepped_charge_current_ma: 'hardware_longevity',
  top_app_io_weight: 'hardware_longevity',
  background_io_weight: 'hardware_longevity',
  vulkan_layer_mode: 'vulkan_engine',
  enable_skiavk_threaded: 'vulkan_engine',
  enable_adaptive_vrs: 'vulkan_engine',
  enable_pipeline_cache_prewarm: 'vulkan_engine',
  target_fps: 'render_booster',
  pre_boost_on_touch: 'render_booster',
  boost_duration_ms: 'render_booster',
};

export function normalizeSectionName(sec: string): string {
  const trimmed = sec.trim();
  return trimmed === 'stability' ? 'stability_policy' : trimmed;
}

export function parseConfigIni(raw: string): Record<string, Record<string, string>> {
  const result: Record<string, Record<string, string>> = {};
  let currentSection = '';
  for (const rawLine of raw.split(/\r?\n/)) {
    const trimmed = rawLine.trim();
    if (!trimmed || trimmed.startsWith('#') || trimmed.startsWith(';')) continue;
    if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
      currentSection = normalizeSectionName(trimmed.slice(1, -1));
      if (!result[currentSection]) result[currentSection] = {};
      continue;
    }
    const eq = trimmed.indexOf('=');
    if (eq < 0) continue;
    const key = trimmed.slice(0, eq).trim();
    const val = trimmed.slice(eq + 1).trim();
    if (!key) continue;
    const expectedSection = CANONICAL_KEY_SECTIONS[key];
    if (expectedSection && currentSection !== expectedSection) {
      continue;
    }
    const sec = currentSection || expectedSection || 'general';
    if (!result[sec]) result[sec] = {};
    result[sec][key] = val;
  }
  return result;
}

export function readSectionStringKey(raw: string, key: string): string | null {
  const parsed = parseConfigIni(raw);
  const expectedSection = CANONICAL_KEY_SECTIONS[key];
  if (expectedSection) {
    const v = parsed[expectedSection]?.[key];
    return v && v.length > 0 ? v : null;
  }
  for (const sec of Object.values(parsed)) {
    if (sec[key]) return sec[key];
  }
  return null;
}

export function readSectionNumberKey(raw: string, key: string): number | null {
  const str = readSectionStringKey(raw, key);
  if (str === null) return null;
  const num = Number(str);
  return Number.isFinite(num) ? num : null;
}

export function mergeConfigText(
  existingText: string,
  updates: Record<string, string | number | boolean>
): string {
  const pending: Record<string, string> = {};
  for (const [k, v] of Object.entries(updates)) {
    const key = k.trim();
    if (!key || !(key in CANONICAL_KEY_SECTIONS)) continue;
    pending[key] = String(v).trim();
  }
  if (Object.keys(pending).length === 0) return existingText;

  const lines = existingText ? existingText.replace(/\r\n/g, '\n').split('\n') : [];
  if (lines.length > 0 && lines[lines.length - 1] === '') {
    lines.pop();
  }

  const outLines: string[] = [];
  let currentSection = '';

  const flushRemainingForSection = (sec: string) => {
    if (!sec) return;
    for (const [k, v] of Object.entries(pending)) {
      if (CANONICAL_KEY_SECTIONS[k] === sec) {
        outLines.push(`${k} = ${v}`);
        delete pending[k];
      }
    }
  };

  for (const line of lines) {
    const trimmed = line.trim();
    if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
      flushRemainingForSection(currentSection);
      currentSection = normalizeSectionName(trimmed.slice(1, -1));
      outLines.push(line);
      continue;
    }
    if (!trimmed || trimmed.startsWith('#') || trimmed.startsWith(';')) {
      outLines.push(line);
      continue;
    }
    const eq = line.indexOf('=');
    if (eq < 0) {
      outLines.push(line);
      continue;
    }
    const key = line.slice(0, eq).trim();
    if (key in pending && CANONICAL_KEY_SECTIONS[key] === currentSection) {
      outLines.push(`${key} = ${pending[key]}`);
      delete pending[key];
    } else {
      outLines.push(line);
    }
  }
  flushRemainingForSection(currentSection);

  const sectionOrder = [
    'general',
    'device_adaptation',
    'uclamp_tuning',
    'thermal_guardian',
    'stability_policy',
    'pid_equilibrium',
    'hardware_longevity',
    'vulkan_engine',
    'render_booster',
  ];

  for (const sec of sectionOrder) {
    const keysForSec = Object.keys(pending).filter((k) => CANONICAL_KEY_SECTIONS[k] === sec);
    if (keysForSec.length === 0) continue;
    if (outLines.length > 0 && outLines[outLines.length - 1].trim() !== '') {
      outLines.push('');
    }
    outLines.push(`[${sec}]`);
    for (const k of keysForSec) {
      outLines.push(`${k} = ${pending[k]}`);
      delete pending[k];
    }
  }

  return outLines.join('\n') + '\n';
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
  thermalGuardActive?: boolean;
  thermalDataDegraded?: boolean;
  thermalFallbackActive?: boolean;
  thermalHeadroomValid?: boolean;
  thermalHeadroom?: number | null;
  thermalSource?: string;
  thermalSeverity?: number;
  thermalSensors?: ThermalSensorDetail[];
  cpuPressurePct: number;
  memoryPressurePct: number;
  ioPressurePct: number;
  cpuPressureValid?: boolean;
  memoryPressureValid?: boolean;
  ioPressureValid?: boolean;
  baselineNodesSnapshotted: number;
  baselineStatus: 'protected' | 'restored';
  transactionStage?: TransactionStage;
  riskClass?: RiskClass;
  lastOutcome?: TransactionOutcome;
  ownershipIntact?: boolean;
  aiBackend?: string;
  aiOnnxActive?: boolean;
  aiPredictedPressure10s?: number;
  aiPredictedVelocity?: number;
  aiRecommendedScale?: number;
  aiConfidence?: number;
  aiSamples?: number;
  tunerRuntimeStage?: string;
  tunerStage?: string;
  tunerIntent?: string;
  tunerCandidateId?: string;
  tunerCandidateLabel?: string;
  tunerRationale?: string;
  tunerVerdict?: string;
  tunerUtilityDelta?: number;
  tunerTrials?: number;
  tunerImproved?: number;
  tunerRolledBack?: number;
  blockQueues?: BlockQueueTelemetry[];
  tunerCandidates?: TunerCandidateTelemetry[];
  vulkan?: VulkanTelemetry;
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
        description: 'Memory or I/O contention elevated; dynamic block scheduler & VM relief engaged.',
        stage: 'OBSERVING' as TransactionStage,
        risk: 'Low' as RiskClass,
        outcome: 'IMPROVED' as TransactionOutcome,
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
    cpuPressureValid: true,
    memoryPressureValid: true,
    ioPressureValid: true,
    baselineNodesSnapshotted: 6,
    baselineStatus: 'protected',
    transactionStage: 'RETAINED',
    riskClass: 'Low',
    lastOutcome: 'IMPROVED',
    ownershipIntact: true,
    aiBackend: 'embedded-rls-mlp',
    aiOnnxActive: false,
    aiPredictedPressure10s: 46.2,
    aiPredictedVelocity: 0.08,
    aiRecommendedScale: 0.94,
    aiConfidence: 0.86,
    aiSamples: 12,
    tunerStage: 'RETAINED',
    tunerIntent: 'HoldEquilibrium',
    tunerCandidateId: 'baseline',
    tunerCandidateLabel: 'OEM Verified Baseline',
    tunerRationale: 'System in equilibrium (I/O & memory PSI low); retaining OEM block scheduler without unnecessary mutation.',
    tunerVerdict: 'IMPROVED',
    tunerUtilityDelta: 0.0,
    tunerTrials: 3,
    tunerImproved: 2,
    tunerRolledBack: 1,
    blockQueues: [
      {
        device: 'sda',
        activeScheduler: 'mq-deadline',
        baselineScheduler: 'none',
        availableSchedulers: ['none', 'mq-deadline', 'kyber', 'bfq'],
        readAheadKb: 128,
        nrRequests: 128,
        rqAffinity: 2,
        readExpireMs: 250,
        writesStarved: 3,
        fifoBatch: 8,
      },
    ],
    tunerCandidates: [
      {
        id: 'io.mq_deadline_low_latency',
        label: 'mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)',
        domain: 'io',
        trials: 2,
        successes: 2,
        rollbacks: 0,
        meanDelta: 7.4,
        ucbScore: 8.12,
        cooldownTicks: 0,
      },
      {
        id: 'io.kyber_token_bucket',
        label: 'kyber Token-Bucket Low-Latency I/O Scheduler',
        domain: 'io',
        trials: 1,
        successes: 0,
        rollbacks: 1,
        meanDelta: -1.8,
        ucbScore: 0.42,
        cooldownTicks: 0,
      },
      {
        id: 'cpufreq.schedutil_interactive_response',
        label: 'EAS Governor Rate-Limit Pacing (500us up / 10000us down)',
        domain: 'cpufreq',
        trials: 1,
        successes: 1,
        rollbacks: 0,
        meanDelta: 5.2,
        ucbScore: 6.18,
        cooldownTicks: 0,
      },
      {
        id: 'vm.zram_latency_relief',
        label: 'VM Page-Cluster Zero-Latency & VFS Dentry Preservation',
        domain: 'vm',
        trials: 1,
        successes: 1,
        rollbacks: 0,
        meanDelta: 4.1,
        ucbScore: 5.15,
        cooldownTicks: 0,
      },
    ],
    vulkan: computeVulkanTelemetry(DEFAULT_VULKAN_CONFIG, {
      available: true,
      layerInstalled: true,
      vendor: 'Multi-Vendor Preview (Adreno / Mali / Xclipse)',
      deviceName: 'Universal Mobile GPU Preview Profile',
      apiVersion: '1.3.280',
      driverVersion: '512.762.0',
      deviceExtensions: 148,
      instanceExtensions: 16,
      deviceUuid: 'cf000001-2026-4000-a000-000000000310',
      driverUuid: 'cf000002-2026-4000-a000-000000000310',
      pipelineCacheUuid: 'cf000003-2026-4000-a000-000000000310',
      hwVariableRateShading: true,
      hwPushDescriptors: true,
      hwMaintenance5: true,
      hwMaintenance6: true,
      gpuKernelAvailable: true,
      gpuKernelLabel: 'Universal GPU Devfreq Surface (Preview)',
      gpuGovernor: 'msm-adreno-tz / simple_ondemand',
      gpuCurrentFreqMhz: 680,
      gpuMaxFreqMhz: 1050,
      adrenoboostLevel: 1,
    }),
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
      tag: 'AutonomousTuner',
      message: 'Preview: sda queue/scheduler switched none -> mq-deadline (read_expire 0.5x, fifo_batch 0.5x) | delta_u=+7.4 -> IMPROVED.',
    },
  ];
}

export function stepAutonomousSimulation(
  current: AutonomousTelemetry,
  config: AutonomousConfig,
  forced?: 'cpu' | 'memory' | 'io_trial'
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
  const memory = forced === 'memory' ? 6.4 : Math.max(0, Math.random() * 3.5);
  const io =
    forced === 'io_trial'
      ? 11.8
      : forced === 'memory'
      ? 6.2
      : Math.max(0, Math.random() * 5.2);

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
  const predictedPressure = Math.min(
    130,
    Math.max(25, (nextTemp - 20) * 2.2 + cpu * 0.6 + io * 0.4)
  );
  const predictedVelocity = Number(((predictedPressure - (current.aiPredictedPressure10s ?? 45)) * 0.18).toFixed(2));
  const recScale =
    mode === 'thermal_guard'
      ? 0.68
      : mode === 'pressure_guard'
      ? 0.82
      : Number(Math.max(0.72, Math.min(1.0, 1.0 - Math.max(0, predictedPressure - 55) * 0.004)).toFixed(2));
  const samples = (current.aiSamples ?? 12) + 1;

  // Autonomous Multi-Domain Scheduler & Tunable Evaluation simulation
  const prevQueues = current.blockQueues ?? [];
  const prevCandidates = (current.tunerCandidates ?? []).map((c) => ({ ...c }));
  let tunerStage = current.tunerStage ?? 'RETAINED';
  let tunerIntent = current.tunerIntent ?? 'HoldEquilibrium';
  let tunerCandidateId = current.tunerCandidateId ?? 'baseline';
  let tunerCandidateLabel = current.tunerCandidateLabel ?? 'OEM Verified Baseline';
  let tunerRationale = current.tunerRationale ?? '';
  let tunerVerdict = current.tunerVerdict ?? 'IMPROVED';
  let tunerUtilityDelta = current.tunerUtilityDelta ?? 0;
  let tunerTrials = current.tunerTrials ?? 3;
  let tunerImproved = current.tunerImproved ?? 2;
  const tunerRolledBack = current.tunerRolledBack ?? 1;

  const updatedQueues = prevQueues.map((q) => ({ ...q }));

  if (forced === 'io_trial' || io >= 5.0) {
    tunerStage = 'RETAINED';
    tunerIntent = 'IoLatencyRelief';
    tunerCandidateId = 'io.mq_deadline_low_latency';
    tunerCandidateLabel = 'mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)';
    tunerVerdict = 'IMPROVED';
    tunerUtilityDelta = Number((5.2 + Math.random() * 4.5).toFixed(2));
    tunerTrials += 1;
    tunerImproved += 1;
    tunerRationale =
      `I/O PSI=${io.toFixed(1)}% exceeded threshold; switched sda scheduler none -> mq-deadline ` +
      `(read_expire=0.5x, writes_starved=1.5x, fifo_batch=0.5x, read_ahead_kb=2.0x) | Before/After ΔU=+${tunerUtilityDelta} -> RETAINED.`;
    if (updatedQueues.length > 0) {
      updatedQueues[0].activeScheduler = 'mq-deadline';
      updatedQueues[0].readExpireMs = 250;
      updatedQueues[0].writesStarved = 3;
      updatedQueues[0].fifoBatch = 8;
      updatedQueues[0].readAheadKb = 256;
      updatedQueues[0].rqAffinity = 2;
    }
    const cand = prevCandidates.find((c) => c.id === 'io.mq_deadline_low_latency');
    if (cand) {
      cand.trials += 1;
      cand.successes += 1;
      cand.meanDelta = Number(((cand.meanDelta * (cand.trials - 1) + tunerUtilityDelta) / cand.trials).toFixed(2));
      cand.ucbScore = Number((cand.meanDelta + 0.65).toFixed(2));
    }
  } else if (forced === 'memory' || memory >= 3.2) {
    tunerStage = 'RETAINED';
    tunerIntent = 'MemoryStallRelief';
    tunerCandidateId = 'vm.zram_latency_relief';
    tunerCandidateLabel = 'VM Page-Cluster Zero-Latency & VFS Dentry Preservation';
    tunerVerdict = 'IMPROVED';
    tunerUtilityDelta = Number((3.4 + Math.random() * 2.8).toFixed(2));
    tunerTrials += 1;
    tunerImproved += 1;
    tunerRationale =
      `Memory PSI=${memory.toFixed(1)}% elevated; applied vm.page-cluster=0 & vfs_cache_pressure=80 | Before/After ΔU=+${tunerUtilityDelta} -> RETAINED.`;
  } else if (mode === 'thermal_guard') {
    tunerStage = 'OBSERVING';
    tunerIntent = 'ThermalStabilization';
    tunerCandidateId = 'thermal.schedutil_damped_pacing';
    tunerCandidateLabel = 'Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)';
    tunerVerdict = 'IMPROVED';
    tunerUtilityDelta = Number((2.8 + Math.random() * 1.9).toFixed(2));
    tunerTrials += 1;
    tunerImproved += 1;
    tunerRationale =
      `Thermal headroom constrained (${nextTemp.toFixed(1)}°C); switched objective to ThermalStabilization and damped EAS governor ramp rate (2.0x up / 0.5x down relative to OEM baseline).`;
  } else if (forced === 'cpu' || cpu >= 8.5) {
    tunerStage = 'RETAINED';
    tunerIntent = 'InteractiveCpuPacing';
    tunerCandidateId = 'cpufreq.schedutil_interactive_response';
    tunerCandidateLabel = 'EAS Governor Rate-Limit Pacing (500us up / 10000us down)';
    tunerVerdict = 'IMPROVED';
    tunerUtilityDelta = Number((4.1 + Math.random() * 2.5).toFixed(2));
    tunerTrials += 1;
    tunerImproved += 1;
    tunerRationale =
      `CPU PSI=${cpu.toFixed(1)}% with thermal headroom; paced EAS schedutil governor rate limits (up=500us, down=10000us) | Before/After ΔU=+${tunerUtilityDelta} -> RETAINED.`;
  }

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
    aiBackend: current.aiBackend ?? 'embedded-rls-mlp',
    aiOnnxActive: current.aiOnnxActive ?? false,
    aiPredictedPressure10s: Number(predictedPressure.toFixed(1)),
    aiPredictedVelocity: predictedVelocity,
    aiRecommendedScale: recScale,
    aiConfidence: Number(Math.min(0.96, 0.78 + Math.min(samples, 50) * 0.003).toFixed(2)),
    aiSamples: samples,
    tunerStage,
    tunerIntent,
    tunerCandidateId,
    tunerCandidateLabel,
    tunerRationale,
    tunerVerdict,
    tunerUtilityDelta,
    tunerTrials,
    tunerImproved,
    tunerRolledBack,
    blockQueues: updatedQueues,
    tunerCandidates: prevCandidates,
    timestamp: Date.now(),
  };

  if (mode === current.mode && !forced) return { next };

  return { next, event: createPreviewEvent(mode, meta.label, next, forced) };
}

let previewEventSeq = 0;

function createPreviewEvent(
  mode: AutonomousMode,
  label: string,
  next: AutonomousTelemetry,
  forced?: 'cpu' | 'memory' | 'io_trial'
): ReactionLogEvent {
  previewEventSeq += 1;
  if (forced === 'io_trial') {
    return {
      id: `preview-${Date.now()}-${previewEventSeq}`,
      timestamp: new Date().toTimeString().slice(0, 8),
      type: 'BOOST',
      tag: 'AutonomousTuner',
      message: next.tunerRationale ?? 'Dynamic scheduler trial completed.',
    };
  }
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

// ============================================================================
// Pure Architecture Models (Discovery, Workload/Thermal Reasoning, Coordinator,
// Settling & Noise-Floor Feedback, Context-Aware Anti-Repeat & Adaptive Sizing)
// ============================================================================

export type ResourcePermissionClass =
  | 'ReadOnly'
  | 'Writable'
  | 'WritableButUnsafe'
  | 'UnknownSemantic'
  | 'VerifiedConstraints'
  | 'IncompleteMetadata';

export type WorkloadClass =
  | 'Idle'
  | 'GeneralInteraction'
  | 'ScrollingAndAnimation'
  | 'Gaming'
  | 'HeavyMultitasking'
  | 'CpuBound'
  | 'GpuBound'
  | 'MemoryPressure'
  | 'StorageIoPressure'
  | 'ThermalConstrained'
  | 'BackgroundWorkload';

export type BottleneckKind =
  | 'None'
  | 'CpuSchedulingLatency'
  | 'IoQueueContention'
  | 'ConcurrentCpuIoContention'
  | 'MemorySwapThrashing'
  | 'ThermalHeadroomDepletion'
  | 'ThermalEmergency';

export type OptimizationObjective =
  | 'HoldBaseline'
  | 'InteractiveLatency'
  | 'ThroughputBalance'
  | 'MemoryRelief'
  | 'IoLatencyRelief'
  | 'ThermalStabilization'
  | 'ThermalRecovery'
  | 'EmergencyMitigation';

export type MutationDomain =
  | 'CpuPolicy'
  | 'BlockIo'
  | 'VmMemory'
  | 'UclampCgroup'
  | 'GpuDevfreq'
  | 'PowerLongevity'
  | 'ThermalCore';

// Const object instead of `enum`: Node 22's --experimental-strip-types cannot
// execute TypeScript enums, which broke the WebUI build in CI. This form is
// erasable TypeScript and keeps the same names and numeric values.
export const ControllerPriority = {
  BackgroundEfficiency: 20,
  WorkloadOptimization: 40,
  PressureGuard: 60,
  ThermalStabilization: 80,
  EmergencySafety: 100,
} as const;
export type ControllerPriority = (typeof ControllerPriority)[keyof typeof ControllerPriority];

export interface ResourceProbeInput {
  path: string;
  writable: boolean;
  rawValue: string;
  companionNodesComplete?: boolean;
  activeGovernor?: string;
  targetGovernor?: string;
  zramSwapActive?: boolean;
}

export function classifyResourcePermission(probe: ResourceProbeInput): {
  permissionClass: ResourcePermissionClass;
  mutatable: boolean;
  reason: string;
} {
  if (probe.companionNodesComplete === false) {
    return {
      permissionClass: 'IncompleteMetadata',
      mutatable: false,
      reason: 'Missing required coupled companion node or constraint table',
    };
  }

  const blockMatch = probe.path.match(/\/block\/([^/]+)\/queue\//);
  if (blockMatch) {
    const dev = blockMatch[1];
    if (/^(loop|ram|zram|dm-|md|nbd)/.test(dev)) {
      return {
        permissionClass: 'WritableButUnsafe',
        mutatable: false,
        reason: `Virtual block device (${dev}) excluded from scheduler/queue mutation`,
      };
    }
    if (!/^(sd|mmcblk|nvme)/.test(dev)) {
      return {
        permissionClass: 'UnknownSemantic',
        mutatable: false,
        reason: `Unrecognized block device prefix (${dev}); semantics unverified`,
      };
    }
  }

  if (
    probe.targetGovernor &&
    probe.activeGovernor &&
    probe.activeGovernor !== probe.targetGovernor
  ) {
    return {
      permissionClass: 'WritableButUnsafe',
      mutatable: false,
      reason: `Active governor (${probe.activeGovernor}) does not match target (${probe.targetGovernor})`,
    };
  }

  if (probe.path.includes('/sys/vm/') && probe.zramSwapActive === false) {
    return {
      permissionClass: 'WritableButUnsafe',
      mutatable: false,
      reason: 'VM tunable gated because zram swap is inactive',
    };
  }

  const trimmed = probe.rawValue.trim();
  if (!trimmed) {
    return {
      permissionClass: 'IncompleteMetadata',
      mutatable: false,
      reason: 'Empty sysfs/procfs reading',
    };
  }

  const isSchedulerNode = probe.path.endsWith('/scheduler');
  if (isSchedulerNode) {
    const tokens = trimmed.split(/\s+/).filter(Boolean);
    const hasKnownScheduler = tokens.some((t) =>
      ['none', 'mq-deadline', 'kyber', 'bfq', '[none]', '[mq-deadline]', '[kyber]', '[bfq]'].includes(t)
    );
    if (!hasKnownScheduler) {
      return {
        permissionClass: 'UnknownSemantic',
        mutatable: false,
        reason: 'Scheduler node contains no recognized kernel scheduler tokens',
      };
    }
  } else {
    const num = Number(trimmed);
    if (!Number.isFinite(num) || num < 0) {
      return {
        permissionClass: 'UnknownSemantic',
        mutatable: false,
        reason: 'Non-numeric or negative value in numeric tunable node',
      };
    }
  }

  if (!probe.writable) {
    return {
      permissionClass: 'ReadOnly',
      mutatable: false,
      reason: 'Node is read-only',
    };
  }

  return {
    permissionClass: 'VerifiedConstraints',
    mutatable: true,
    reason: 'Verified constraints, baseline captured, and dependencies satisfied',
  };
}

export interface TelemetryReasoningInput {
  cpuPsi: number;
  memoryPsi: number;
  ioPsi: number;
  thermalPressure: number;
  thermalGuardActive?: boolean;
  thermalEmergency?: boolean;
  thermalHeadroom?: number; // < 0 if unavailable, [0..1] remaining headroom
  thermalTrendCelsius?: number;
  inThermalRecovery?: boolean;
}

export function classifyWorkload(t: TelemetryReasoningInput): WorkloadClass {
  if (
    t.thermalEmergency ||
    t.thermalGuardActive ||
    t.thermalPressure >= 65.0 ||
    (t.thermalHeadroom !== undefined && t.thermalHeadroom >= 0 && t.thermalHeadroom <= 0.20) ||
    (t.thermalPressure >= 56.0 && (t.thermalTrendCelsius ?? 0) >= 0.8)
  ) {
    return 'ThermalConstrained';
  }
  if (t.memoryPsi >= 3.5 && t.ioPsi >= 3.0) return 'HeavyMultitasking';
  if (t.memoryPsi >= 2.0) return 'MemoryPressure';
  if (t.ioPsi >= 5.0 && t.cpuPsi >= 10.0) return 'HeavyMultitasking';
  if (t.ioPsi >= 2.2) return 'StorageIoPressure';
  if (t.cpuPsi >= 16.0 && t.thermalPressure >= 50.0) return 'Gaming';
  if (t.cpuPsi >= 10.0) return 'CpuBound';
  if (t.cpuPsi >= 6.0) return 'ScrollingAndAnimation';
  if (t.cpuPsi >= 2.5 || t.ioPsi >= 1.0) return 'GeneralInteraction';
  if (t.cpuPsi >= 1.2) return 'BackgroundWorkload';
  return 'Idle';
}

export function diagnoseBottleneck(
  t: TelemetryReasoningInput,
  workload: WorkloadClass = classifyWorkload(t)
): BottleneckKind {
  if (t.thermalEmergency || t.thermalPressure >= 88.0) return 'ThermalEmergency';
  if (workload === 'ThermalConstrained') return 'ThermalHeadroomDepletion';
  if (t.ioPsi >= 4.5 && t.cpuPsi >= 9.0) return 'ConcurrentCpuIoContention';
  if (t.ioPsi >= 2.2) return 'IoQueueContention';
  if (t.memoryPsi >= 2.0) return 'MemorySwapThrashing';
  if (t.cpuPsi >= 8.0) return 'CpuSchedulingLatency';
  return 'None';
}

export function selectOptimizationObjective(
  t: TelemetryReasoningInput,
  workload: WorkloadClass = classifyWorkload(t),
  bottleneck: BottleneckKind = diagnoseBottleneck(t, workload)
): OptimizationObjective {
  if (bottleneck === 'ThermalEmergency') return 'EmergencyMitigation';
  if (bottleneck === 'ThermalHeadroomDepletion' || workload === 'ThermalConstrained') {
    return 'ThermalStabilization';
  }
  if (t.inThermalRecovery) return 'ThermalRecovery';
  switch (bottleneck) {
    case 'ConcurrentCpuIoContention':
      return 'ThroughputBalance';
    case 'IoQueueContention':
      return 'IoLatencyRelief';
    case 'MemorySwapThrashing':
      return 'MemoryRelief';
    case 'CpuSchedulingLatency':
      return workload === 'Gaming' ? 'ThroughputBalance' : 'InteractiveLatency';
    default:
      return 'HoldBaseline';
  }
}

export function computeRelativeTunableValue(
  factoryBaseline: number,
  targetRatio: number,
  minBound: number,
  maxBound: number,
  adaptiveScale = 1.0
): number {
  const clampedScale = Math.max(0.25, Math.min(1.0, adaptiveScale));
  const effectiveRatio = 1.0 + (targetRatio - 1.0) * clampedScale;
  const raw = Math.max(1, Math.round(factoryBaseline * effectiveRatio));
  return Math.max(minBound, Math.min(maxBound, raw));
}

export interface FeedbackEvaluationInput {
  intent: 'IoLatencyRelief' | 'MemoryStallRelief' | 'InteractiveCpuPacing' | 'ThermalStabilization';
  beforeIoPsi: number;
  afterIoPsiSamples: number[];
  beforeMemPsi: number;
  afterMemPsiSamples: number[];
  beforeCpuPsi: number;
  afterCpuPsiSamples: number[];
  beforeThermal: number;
  afterThermalSamples: number[];
  thermalTrendCelsius?: number;
  ownershipIntact: boolean;
  currentAdaptiveScale: number;
  consecutiveNonImproved: number;
}

export interface FeedbackEvaluationResult {
  verdict: 'IMPROVED' | 'NEUTRAL' | 'REGRESSED' | 'INCONCLUSIVE' | 'OWNERSHIP_LOST';
  action: 'RETAIN' | 'ROLLBACK';
  utilityDelta: number;
  nextAdaptiveScale: number;
  cooldownTicks: number;
  nextConsecutiveNonImproved: number;
}

export function evaluateAdaptiveFeedback(input: FeedbackEvaluationInput): FeedbackEvaluationResult {
  if (!input.ownershipIntact) {
    return {
      verdict: 'OWNERSHIP_LOST',
      action: 'ROLLBACK',
      utilityDelta: 0,
      nextAdaptiveScale: input.currentAdaptiveScale,
      cooldownTicks: 4,
      nextConsecutiveNonImproved: input.consecutiveNonImproved + 1,
    };
  }

  const avg = (arr: number[], fallback: number) =>
    arr.length > 0 ? arr.reduce((a, b) => a + b, 0) / arr.length : fallback;

  const afterIo = avg(input.afterIoPsiSamples, input.beforeIoPsi);
  const afterMem = avg(input.afterMemPsiSamples, input.beforeMemPsi);
  const afterCpu = avg(input.afterCpuPsiSamples, input.beforeCpuPsi);
  const afterTherm = avg(input.afterThermalSamples, input.beforeThermal);

  const ioGain = input.beforeIoPsi - afterIo;
  const memGain = input.beforeMemPsi - afterMem;
  const cpuGain = input.beforeCpuPsi - afterCpu;
  const thermGain = input.beforeThermal - afterTherm;
  const thermPenalty = Math.max(0, afterTherm - input.beforeThermal - 2.0);

  let utilityDelta = 0;
  if (input.intent === 'ThermalStabilization') {
    const trendBonus = (input.thermalTrendCelsius ?? 0) <= 0 ? 0.35 : -0.30;
    utilityDelta = 0.65 * thermGain + trendBonus + 0.20 * ioGain + 0.15 * cpuGain;
  } else if (input.intent === 'IoLatencyRelief') {
    utilityDelta = 1.40 * ioGain + 0.35 * cpuGain - 0.60 * thermPenalty;
  } else if (input.intent === 'MemoryStallRelief') {
    utilityDelta = 1.60 * memGain + 0.45 * ioGain - 0.60 * thermPenalty;
  } else {
    utilityDelta = 1.50 * cpuGain + 0.35 * ioGain - 0.65 * thermPenalty;
  }

  const ioSpread =
    input.afterIoPsiSamples.length > 1
      ? Math.max(...input.afterIoPsiSamples) - Math.min(...input.afterIoPsiSamples)
      : 0;
  if (ioSpread > Math.max(6.0, input.beforeIoPsi * 1.2) && Math.abs(utilityDelta) < 0.50) {
    const nextNonImp = input.consecutiveNonImproved + 1;
    const shift = Math.min(3, nextNonImp - 1);
    return {
      verdict: 'INCONCLUSIVE',
      action: 'ROLLBACK',
      utilityDelta: Number(utilityDelta.toFixed(3)),
      nextAdaptiveScale: input.currentAdaptiveScale,
      cooldownTicks: Math.min(32, 4 * (1 << shift)),
      nextConsecutiveNonImproved: nextNonImp,
    };
  }

  if (utilityDelta >= 0.25) {
    return {
      verdict: 'IMPROVED',
      action: 'RETAIN',
      utilityDelta: Number(utilityDelta.toFixed(3)),
      nextAdaptiveScale: Math.min(1.0, Number((input.currentAdaptiveScale + 0.15).toFixed(2))),
      cooldownTicks: 0,
      nextConsecutiveNonImproved: 0,
    };
  }

  if (utilityDelta < -0.20) {
    const nextNonImp = input.consecutiveNonImproved + 1;
    const shift = Math.min(3, nextNonImp - 1);
    return {
      verdict: 'REGRESSED',
      action: 'ROLLBACK',
      utilityDelta: Number(utilityDelta.toFixed(3)),
      nextAdaptiveScale: Math.max(0.25, Number((input.currentAdaptiveScale * 0.75).toFixed(3))),
      cooldownTicks: Math.min(64, 8 * (1 << shift)),
      nextConsecutiveNonImproved: nextNonImp,
    };
  }

  const nextNonImp = input.consecutiveNonImproved + 1;
  const shift = Math.min(3, nextNonImp - 1);
  return {
    verdict: 'NEUTRAL',
    action: 'ROLLBACK',
    utilityDelta: Number(utilityDelta.toFixed(3)),
    nextAdaptiveScale: Math.max(0.50, Number((input.currentAdaptiveScale * 0.90).toFixed(3))),
    cooldownTicks: Math.min(32, 4 * (1 << shift)),
    nextConsecutiveNonImproved: nextNonImp,
  };
}

export class CentralMutationCoordinatorModel {
  private leases = new Map<
    MutationDomain,
    { owner: string; transactionId: string; priority: ControllerPriority }
  >();
  public conflictRejections = 0;

  acquireLease(
    domain: MutationDomain,
    controller: string,
    transactionId: string,
    priority: ControllerPriority
  ): { granted: boolean; reason?: string } {
    const existing = this.leases.get(domain);
    if (existing && existing.owner !== controller) {
      if (priority <= existing.priority) {
        this.conflictRejections += 1;
        return {
          granted: false,
          reason: `Domain ${domain} locked by ${existing.owner} (${existing.transactionId}) at priority ${existing.priority}`,
        };
      }
    }
    this.leases.set(domain, { owner: controller, transactionId, priority });
    return { granted: true };
  }

  releaseLease(domain: MutationDomain, controller: string): void {
    const existing = this.leases.get(domain);
    if (existing && existing.owner === controller) {
      this.leases.delete(domain);
    }
  }

  emergencyPreemptAll(): void {
    this.leases.clear();
  }

  isLockedByOther(
    domain: MutationDomain,
    controller: string,
    priority: ControllerPriority
  ): boolean {
    const existing = this.leases.get(domain);
    if (!existing || existing.owner === controller) return false;
    return priority <= existing.priority;
  }
}
