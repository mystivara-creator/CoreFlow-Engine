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
  label: string;
  cores: string;
  minFreqKhz: number;
  maxFreqKhz: number;
  stockGovernor: string;
  availableGovernors: string[];
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
  gpu: string;
  abi: string;
  kernelRelease: string;
  androidRelease: string;
  sdkLevel: string;
  memTotalKb: number;
  stockReadAheadKb: number;
  stockNrRequests: number;
  thermalZonesCount: number;
  hasPrimaryThermalSensor: boolean;
  discoveredResources: number;
  mutationReadyResources: number;
  cpuPolicies: CpuClusterSpec[];
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

export type SysfsSubsystem = 'CPU & WALT' | 'Block I/O' | 'VM / Sysctl' | 'GPU & DSP' | 'Charging';

export type SysfsNodeStatus = 'BASELINE' | 'MUTATED' | 'CLAMPED' | 'SKIPPED';

export interface SysfsNodeEntry {
  path: string;
  subsystem: SysfsSubsystem;
  baselineValue: string;
  currentValue: string;
  status: SysfsNodeStatus;
  rationale: string;
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
