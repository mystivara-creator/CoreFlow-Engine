export enum RuntimeState {
  Idle = 'IDLE',
  Normal = 'NORMAL',
  Warming = 'WARMING',
  Elevated = 'ELEVATED',
  Pressure = 'PRESSURE',
  ThermalGuard = 'THERMAL_GUARD',
}

export enum Decision {
  NoAction = 'NO_ACTION',
  Observe = 'OBSERVE',
  Notify = 'NOTIFY',
  ReduceIntervention = 'REDUCE_INTERVENTION',
}

export enum Trend {
  Rising = 'RISING',
  Stable = 'STABLE',
  Falling = 'FALLING',
  Unknown = 'UNKNOWN',
}

export enum NotificationEvent {
  None = 'NONE',
  ThermalWarming = 'THERMAL_WARMING',
  ThermalWarning = 'THERMAL_WARNING',
  ThermalGuard = 'THERMAL_GUARD',
  ChargingProtection = 'CHARGING_PROTECTION',
}

export enum EngineMode {
  ModeIdle = 'MODE_IDLE',
  ModeBalanced = 'MODE_BALANCED',
  ModeBurst = 'MODE_BURST',
  ModeGaming = 'MODE_GAMING',
  ModeUltraIdle = 'MODE_ULTRA_IDLE',
}

export enum AppClass {
  Default = 'APP_DEFAULT',
  Game = 'APP_GAME',
  Launcher = 'APP_LAUNCHER',
}

export enum ThermalSource {
  Primary = 'PRIMARY',
  Fallback = 'FALLBACK',
  Unknown = 'UNKNOWN',
}

export enum BaselineOutcome {
  Pending = 'PENDING',
  Beneficial = 'BENEFICIAL',
  Neutral = 'NEUTRAL',
  Regression = 'REGRESSION',
  Inconclusive = 'INCONCLUSIVE',
}

export enum BaselineAction {
  CollectingBaseline = 'COLLECTING_BASELINE',
  ObservingMutation = 'OBSERVING_MUTATION',
  Keep = 'KEEP',
  Hold = 'HOLD',
  Restore = 'RESTORE',
}

export type DevicePresetId = 'kunzite_parrot' | 'universal_kalama' | 'restricted_mtk';

export interface CpuClusterSpec {
  id: number;
  policyPath: string;
  label: string;
  cores: string;
  coresRange: string;
  role: 'efficiency' | 'performance' | 'prime';
  minFreqKhz: number;
  maxFreqKhz: number;
  siliconMaxFreqKhz: number;
  oppTableKhz: number[];
  stockGovernor: string;
  availableGovernors: string[];
  governorSubdir: string;
  oemUpRateLimitUs: number;
  siliconMinUpRateLimitUs: number;
  oemDownRateLimitUs: number;
  siliconMaxDownRateLimitUs: number;
  loadTunableLeaf?: string;
  oemTargetLoad: number;
  siliconOptimalLoad: number;
  thermalClampLoad: number;
}

export interface StorageTopologySpec {
  queuePath: string;
  interfaceType: string;
  minReadAheadKb: number;
  stockReadAheadKb: number;
  maxReadAheadKb: number;
  stockNrRequests: number;
  maxQueueDepth: number;
  stockRqAffinity: number;
  stockScheduler: string;
  availableSchedulers: string[];
}

export interface GpuTopologySpec {
  governorPath: string;
  stockGovernor: string;
  availableGovernors: string[];
  minPwrlevelPath?: string;
  oemDefaultPwrlevel?: number;
  numPwrlevels?: number;
  oppTableMhz: number[];
  siliconPeakClockMhz: number;
  oemSustainedCapMhz: number;
}

export interface CgroupTopologySpec {
  topAppUclampMinPath: string;
  oemTopAppUclampMin: number;
  bgUclampMaxPath: string;
  oemBgUclampMax: string;
  bgCpusetPath: string;
  oemBgCpus: string;
}

export interface NetworkTopologySpec {
  congestionPath: string;
  stockCongestion: string;
  availableCongestion: string[];
  fastOpenPath: string;
  stockFastOpen: number;
}

export interface OemCapabilityAuditItem {
  id: string;
  domain: 'Silicon Envelope' | 'Encore Dynamic' | 'OEM Tier Audit';
  title: string;
  oemStockPosture: string;
  rawSiliconCapability: string;
  coreflowResolution: string;
  reversibleMethod: string;
}

export interface ChargingValidationSpec {
  batteryAvailable: boolean;
  statusReadable: boolean;
  telemetryReadable: boolean;
  hasInputCurrentLimit: boolean;
  hasChargeCurrentLimit: boolean;
  hasChargeControlLimit: boolean;
  chargeControlLimitPath: string;
  chargeControlLimitReadable: boolean;
  chargeControlLimitWritable: boolean;
  chargeControlLimitNumeric: boolean;
  chargeControlLimitValue: number;
  chargeControlLimitMin: number;
  chargeControlLimitMax: number;
  chargeControlLimitRangeValid: boolean;
  semanticsValidated: boolean;
  mutationReady: boolean;
  hasChargingEnabled: boolean;
  hasChargeDisable: boolean;
}

export interface DeviceHardwareProfile {
  id: DevicePresetId;
  name: string;
  manufacturer: string;
  model: string;
  codename: string;
  board: string;
  soc: string;
  siliconArchitecture: string;
  oemRomLabel: string;
  gpu: string;
  abi: string;
  kernelRelease: string;
  androidRelease: string;
  sdkLevel: string;
  memTotalKb: number;
  zramSizeMb: number;
  stockSwappiness: number;
  stockVfsCachePressure: number;
  stockPageCluster: number;
  displayRefreshHz: number;
  stockReadAheadKb: number;
  stockNrRequests: number;
  thermalZonesCount: number;
  hasPrimaryThermalSensor: boolean;
  discoveredResources: number;
  mutationReadyResources: number;
  cpuPolicies: CpuClusterSpec[];
  storage: StorageTopologySpec;
  gpuTopology: GpuTopologySpec;
  cgroups: CgroupTopologySpec;
  network: NetworkTopologySpec;
  oemAuditItems: OemCapabilityAuditItem[];
  charging: ChargingValidationSpec;
}

export interface CapabilityOverrides {
  procAvailable: boolean;
  sysAvailable: boolean;
  kgslAvailable: boolean;
  waltAvailable: boolean;
  vmSysctlWritable: boolean;
  chargeControlNodePresent: boolean;
}

export interface WorkloadInputs {
  screenOn: boolean;
  foregroundApp: AppClass;
  foregroundPackage: string;
  cpuUsagePct: number;
  gpuBusyPct: number;
  load1: number;
  memAvailablePct: number;
  socTempC: number;
  batteryTempC: number;
  batteryPercent: number;
  charging: boolean;
  batteryCurrentMa: number;
  batteryVoltageMv: number;
  ioReadKbs: number;
  ioWriteKbs: number;
  monitorIntervalSec: number;
}

export interface RuntimeSample {
  sampleIndex: number;
  uptimeSeconds: number;
  cpuUsagePct: number;
  gpuBusyPct: number;
  load1: number;
  memTotalKb: number;
  memAvailableKb: number;
  memAvailableRatio: number;
  thermalAvailable: boolean;
  thermalMillidegrees: number;
  hottestThermalMillidegrees: number;
  thermalSource: ThermalSource;
  charging: boolean;
  chargingTelemetryAvailable: boolean;
  batteryPercent: number;
  batteryTempMillidegrees: number;
  batteryCurrentMicroamps: number;
  batteryVoltageMicrovolts: number;
  ioReadKbs: number;
  ioWriteKbs: number;
  thermalTrend: Trend;
  memoryTrend: Trend;
  loadTrend: Trend;
  confidence: number;
}

export type SysfsSubsystem =
  | 'CPU & WALT'
  | 'Block I/O'
  | 'VM / Sysctl'
  | 'GPU & DSP'
  | 'Network'
  | 'Charging';

export type SysfsNodeStatus = 'BASELINE' | 'MUTATED' | 'CLAMPED' | 'SKIPPED';

export interface SysfsNodeEntry {
  path: string;
  subsystem: SysfsSubsystem;
  baselineValue: string;
  currentValue: string;
  status: SysfsNodeStatus;
  domainEnvelope?: string;
  rationale: string;
}

export type BanditArmId =
  | 'OEM_STOCK_CONSERVE'
  | 'SILICON_RAW_UNLOCK'
  | 'ENCORE_FRAME_PACING'
  | 'HYPER_INTERACTIVE_RAMP'
  | 'THERMAL_PARETO_RELIEF';

export interface BanditArmState {
  id: BanditArmId;
  label: string;
  description: string;
  pulls: number;
  meanReward: number;
  variance: number;
  ucbScore: number;
  explorationBonus: number;
}

export interface ParetoObjectiveWeights {
  throughput: number;
  frameLatency: number;
  thermalSafety: number;
  powerEfficiency: number;
}

export interface HybridBrainEvaluation {
  selectedArm: BanditArmId;
  selectedArmLabel: string;
  paretoWeights: ParetoObjectiveWeights;
  arms: BanditArmState[];
  unlockDrive: number; // 0..1 continuous silicon envelope activation
  latencyDrive: number; // 0..1 continuous interactive ramp activation
  containmentDrive: number; // 0..1 background / thermal containment activation
  siliconGapPct: number; // % of OEM-throttled silicon headroom unlocked safely
  reasoningSummary: string;
}

export interface PredictorFeatureRow {
  index: number;
  name: string;
  rawValue: number;
  formattedValue: string;
  description: string;
}

export interface ThermalPrediction {
  features: PredictorFeatureRow[];
  vectorComplete: boolean;
  horizonSeconds: number;
  horizonTicks: number;
  predictedTempC: number;
  deltaHorizonC: number;
  preemptiveTrigger: boolean;
  predictorMode: 'ONNX_18F_EVALUATED' | 'HEURISTIC_FALLBACK';
}

export interface BaselineSampleMetric {
  tick: number;
  efficiencyScore: number;
  cpuUsagePct: number;
  tempC: number;
  load1: number;
}

export interface BaselineExperimentState {
  active: boolean;
  candidateGovernor: string;
  injectRegression: boolean;
  baselineSamples: BaselineSampleMetric[];
  mutatedSamples: BaselineSampleMetric[];
  baselineMeanScore: number | null;
  mutatedMeanScore: number | null;
  deltaPct: number | null;
  outcome: BaselineOutcome;
  action: BaselineAction;
  summary: string;
}

export interface LogcatEntry {
  id: string;
  sampleIndex: number;
  timeFormatted: string;
  level: 'I' | 'W' | 'E';
  tag: 'CoreFlowAutonomous' | 'CoreFlowConscience' | 'ThermalGuard';
  message: string;
}
