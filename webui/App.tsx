import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  Activity,
  Flame,
  Terminal,
  ClipboardCheck,
  HardDrive,
  Thermometer,
  Cpu,
  Gauge,
  LockKeyhole,
  Layers,
  ShieldCheck,
  Fingerprint,
} from 'lucide-react';
import {
  AccentSeed,
  applyAppearance,
  readAppearance,
  resolveIsDark,
  saveAppearance,
  Theme,
} from './bridge/appearance';
import { execNativeKsuShell, hasNativeKernelSuBridge } from './bridge/daemonBridge';
import { AutonomousStateHero } from './components/AutonomousStateHero';
import { BaselineGuardianCard } from './components/BaselineGuardianCard';
import { DirectAdaptiveCard } from './components/DirectAdaptiveCard';
import { LiveReactionStream } from './components/LiveReactionStream';
import { PixelHeader } from './components/PixelHeader';
import { ArchitectureStudioModal } from './components/ArchitectureStudioModal';
import { ThermalMatrixCard } from './components/ThermalMatrixCard';
import { DeveloperConsoleCard } from './components/DeveloperConsoleCard';
import { DeviceValidationSuiteCard } from './components/DeviceValidationSuiteCard';
import { VulkanCapabilitiesCard } from './components/VulkanCapabilitiesCard';
import { SemanticEvidencePanel } from './components/SemanticEvidencePanel';
import { isSemanticEvidence, SemanticEvidence } from './semanticEvidence';
import {
  AutonomousTelemetry,
  DirectAdaptiveStatus,
  isDirectAdaptiveStatus,
  BlockQueueTelemetry,
  computeSystemHealthSummary,
  computeVulkanTelemetry,
  createInitialAutonomousTelemetry,
  createInitialLogs,
  DEFAULT_AUTONOMOUS_CONFIG,
  DEFAULT_VULKAN_CONFIG,
  mergeConfigText,
  ReactionLogEvent,
  readSectionNumberKey,
  readSectionStringKey,
  stepAutonomousSimulation,
  ThermalSensorDetail,
  TunerActiveInterventionTelemetry,
  TunerCandidateTelemetry,
  TunerEvaluatedCandidateTelemetry,
  VulkanEngineConfig,
  VulkanLayerMode,
  VulkanTelemetry,
} from './engine/autonomousEngine';

const RESOLVE_BIN =
  'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"';
const CONFIG = '/data/adb/coreflow/coreflow.conf';
const LOG = '/data/adb/coreflow/logs/coreflowd.log';

interface NativeStatus {
  running: boolean;
  pid: number;
  platformVendor?: string;
  osFamily?: string;
  osMajor?: number;
  executionTier?: string;
  policyFamily?: string;
  identityConfidence?: string;
  tier1Eligible?: boolean;
  inspectionMode?: boolean;
  mutationRuntimeStarted?: boolean;
  uclampMin: number;
  uclampMax: number;
  uclampSupported: boolean;
  uclampPolicyMin?: number;
  uclampPolicyMax?: number;
  uclampTargetPath?: string;
  uclampEncoding?: string;
  cpuPressurePct?: number | null;
  memoryPressurePct?: number | null;
  ioPressurePct?: number | null;
  cpuPressureValid?: boolean;
  memoryPressureValid?: boolean;
  ioPressureValid?: boolean;
  baselineNodesSnapshotted?: number;
  socTempCelsius: number | null;
  socCompositeCelsius?: number | null;
  cpuAverageCelsius?: number | null;
  cpuPeakCelsius?: number | null;
  gpuTempCelsius?: number | null;
  batteryTempCelsius?: number | null;
  skinTempCelsius?: number | null;
  thermalSensors?: ThermalSensorDetail[];
  recoveryPending: boolean;
  thermalPolicyAvailable?: boolean;
  thermalGuardActive?: boolean;
  thermalDataDegraded?: boolean;
  thermalFallbackActive?: boolean;
  thermalHeadroomValid?: boolean;
  thermalHeadroom?: number | null;
  thermalPressure?: number;
  thermalSeverity?: number;
  thermalSource?: string;
  thermalSensor?: string;
  aiBackend?: string;
  aiOnnxActive?: boolean;
  aiPredictedPressure10s?: number;
  aiPredictedVelocity?: number;
  aiRecommendedScale?: number;
  aiConfidence?: number;
  aiSamples?: number;
  tunerRuntimeStage?: string;
  tunerStage?: string;
  tunerWorkload?: string;
  tunerBottleneck?: string;
  tunerObjective?: string;
  tunerIntent?: string;
  tunerCandidateId?: string;
  tunerCandidateLabel?: string;
  tunerCapabilityFingerprint?: string;
  tunerDecisionPlanRationale?: string;
  tunerRationale?: string;
  tunerVerdict?: string;
  tunerUtilityDelta?: number;
  tunerTrials?: number;
  tunerImproved?: number;
  tunerRolledBack?: number;
  blockQueues?: BlockQueueTelemetry[];
  tunerCandidates?: TunerCandidateTelemetry[];
  tunerEvaluatedCandidates?: TunerEvaluatedCandidateTelemetry[];
  tunerActiveInterventions?: TunerActiveInterventionTelemetry[];
  vulkan?: VulkanTelemetry;
  semanticEvidence?: SemanticEvidence;
  directAdaptive?: DirectAdaptiveStatus;
}

const extractJsonObject = (raw: string): unknown => {
  const trimmed = raw.trim();
  const lines = trimmed
    .split('\n')
    .map((line) => line.trim())
    .filter(Boolean);
  for (let i = lines.length - 1; i >= 0; i--) {
    const line = lines[i];
    if (line.startsWith('{') && line.endsWith('}')) {
      try {
        return JSON.parse(line);
      } catch {
        // continue scanning
      }
    }
  }
  const firstBrace = trimmed.indexOf('{"running":');
  const start = firstBrace >= 0 ? firstBrace : trimmed.indexOf('{');
  const end = trimmed.lastIndexOf('}');
  if (start >= 0 && end > start) {
    const candidate = trimmed.slice(start, end + 1).replace(/\r?\n/g, '');
    return JSON.parse(candidate);
  }
  return JSON.parse(lines[lines.length - 1] ?? '{}');
};

const isRecord = (value: unknown): value is Record<string, unknown> =>
  typeof value === 'object' && value !== null && !Array.isArray(value);
const isFiniteNum = (value: unknown): value is number =>
  typeof value === 'number' && Number.isFinite(value);

const OPTIONAL_NUMBER_KEYS = [
  'osMajor',
  'baselineNodesSnapshotted',
  'thermalPressure',
  'thermalSeverity',
  'aiPredictedPressure10s',
  'aiPredictedVelocity',
  'aiRecommendedScale',
  'aiConfidence',
  'aiSamples',
  'tunerUtilityDelta',
  'tunerTrials',
  'tunerImproved',
  'tunerRolledBack',
] as const;

const OPTIONAL_NULLABLE_NUMBER_KEYS = [
  'cpuPressurePct',
  'memoryPressurePct',
  'ioPressurePct',
  'socCompositeCelsius',
  'cpuAverageCelsius',
  'cpuPeakCelsius',
  'gpuTempCelsius',
  'batteryTempCelsius',
  'skinTempCelsius',
  'thermalHeadroom',
] as const;

const OPTIONAL_BOOLEAN_KEYS = [
  'tier1Eligible',
  'inspectionMode',
  'mutationRuntimeStarted',
  'cpuPressureValid',
  'memoryPressureValid',
  'ioPressureValid',
  'thermalPolicyAvailable',
  'thermalGuardActive',
  'thermalDataDegraded',
  'thermalFallbackActive',
  'thermalHeadroomValid',
  'aiOnnxActive',
] as const;

const OPTIONAL_STRING_KEYS = [
  'platformVendor',
  'osFamily',
  'executionTier',
  'policyFamily',
  'identityConfidence',
  'thermalSource',
  'thermalSensor',
  'aiBackend',
  'tunerRuntimeStage',
  'tunerStage',
  'tunerWorkload',
  'tunerBottleneck',
  'tunerObjective',
  'tunerIntent',
  'tunerCandidateId',
  'tunerCandidateLabel',
  'tunerCapabilityFingerprint',
  'tunerDecisionPlanRationale',
  'tunerRationale',
  'tunerVerdict',
] as const;

const hasStrings = (item: Record<string, unknown>, keys: string[]) =>
  keys.every((key) => typeof item[key] === 'string');
const hasNumbers = (item: Record<string, unknown>, keys: string[]) =>
  keys.every((key) => isFiniteNum(item[key]));
const hasBooleans = (item: Record<string, unknown>, keys: string[]) =>
  keys.every((key) => typeof item[key] === 'boolean');

const OPTIONAL_ARRAY_GUARDS: Record<string, (item: unknown) => boolean> = {
  thermalSensors: (item) =>
    isRecord(item) &&
    hasStrings(item, ['id', 'type']) &&
    isFiniteNum(item.category) &&
    (item.celsius === null || isFiniteNum(item.celsius)) &&
    hasBooleans(item, ['valid', 'policyAvailable']) &&
    (item.velocityCelsiusPerSec === undefined ||
      item.velocityCelsiusPerSec === null ||
      isFiniteNum(item.velocityCelsiusPerSec)),
  blockQueues: (item) =>
    isRecord(item) &&
    hasStrings(item, ['device', 'activeScheduler', 'baselineScheduler']) &&
    Array.isArray(item.availableSchedulers) &&
    item.availableSchedulers.every((name) => typeof name === 'string') &&
    hasNumbers(item, [
      'readAheadKb',
      'nrRequests',
      'rqAffinity',
      'readExpireMs',
      'writesStarved',
      'fifoBatch',
    ]),
  tunerCandidates: (item) =>
    isRecord(item) &&
    hasStrings(item, ['id', 'label', 'domain']) &&
    hasNumbers(item, [
      'trials',
      'successes',
      'rollbacks',
      'meanDelta',
      'ucbScore',
      'cooldownTicks',
    ]),
  tunerEvaluatedCandidates: (item) =>
    isRecord(item) &&
    hasStrings(item, ['id', 'domain', 'intent', 'rejectionReason']) &&
    hasNumbers(item, [
      'utilityScore',
      'empiricalScore',
      'confidence',
      'riskPenalty',
      'thermalPenalty',
    ]) &&
    hasBooleans(item, ['eligible', 'selected']),
  tunerActiveInterventions: (item) =>
    isRecord(item) &&
    hasStrings(item, ['id', 'domain', 'intent', 'stage', 'rationale']) &&
    hasNumbers(item, ['remainingTicks']),
};

// Required fields are enforced by checkedNativeStatus. Optional telemetry that is
// mistyped is dropped (treated as absent) so one malformed field cannot reach a
// `.toFixed()` in a component and blank the whole dashboard.
const dropMalformedOptionalTelemetry = (status: Record<string, unknown>): void => {
  const drop = (key: string, valid: boolean) => {
    if (status[key] !== undefined && !valid) delete status[key];
  };
  for (const key of OPTIONAL_NUMBER_KEYS) drop(key, isFiniteNum(status[key]));
  for (const key of OPTIONAL_NULLABLE_NUMBER_KEYS)
    drop(key, status[key] === null || isFiniteNum(status[key]));
  for (const key of OPTIONAL_BOOLEAN_KEYS) drop(key, typeof status[key] === 'boolean');
  for (const key of OPTIONAL_STRING_KEYS) drop(key, typeof status[key] === 'string');
  for (const [key, guard] of Object.entries(OPTIONAL_ARRAY_GUARDS)) {
    const value = status[key];
    drop(key, Array.isArray(value) && value.every(guard));
  }
  drop('vulkan', isRecord(status.vulkan));
};

const checkedNativeStatus = (raw: string): NativeStatus => {
  const value: unknown = extractJsonObject(raw);
  if (typeof value !== 'object' || value === null) throw new Error('Invalid native status');
  const status = value as Partial<NativeStatus>;
  if (
    typeof status.running !== 'boolean' ||
    !isFiniteNum(status.pid) ||
    !isFiniteNum(status.uclampMin) ||
    !isFiniteNum(status.uclampMax) ||
    typeof status.uclampSupported !== 'boolean' ||
    (status.uclampPolicyMin !== undefined && !isFiniteNum(status.uclampPolicyMin)) ||
    (status.uclampPolicyMax !== undefined && !isFiniteNum(status.uclampPolicyMax)) ||
    (status.uclampTargetPath !== undefined && typeof status.uclampTargetPath !== 'string') ||
    (status.uclampEncoding !== undefined && typeof status.uclampEncoding !== 'string') ||
    (!isFiniteNum(status.socTempCelsius) && status.socTempCelsius !== null) ||
    typeof status.recoveryPending !== 'boolean'
  ) {
    throw new Error('Unexpected daemon status schema');
  }
  if (status.semanticEvidence !== undefined && !isSemanticEvidence(status.semanticEvidence)) {
    throw new Error('Unexpected semanticEvidence schema; expected native schemaVersion 1');
  }
  if (status.directAdaptive !== undefined && !isDirectAdaptiveStatus(status.directAdaptive)) {
    throw new Error('Unexpected directAdaptive status schema');
  }
  dropMalformedOptionalTelemetry(value as Record<string, unknown>);
  return status as NativeStatus;
};

const execChecked = async (command: string): Promise<string> => {
  const result = await execNativeKsuShell(command);
  if (result.errno !== 0) throw new Error(result.stderr || result.stdout || 'Native command failed');
  return result.stdout;
};

export const App: React.FC = () => {
  const [isNativeBridge] = useState(() => hasNativeKernelSuBridge());
  const [appearance, setAppearance] = useState(() => readAppearance());
  const [nativeError, setNativeError] = useState<string | null>(null);
  const [semanticEvidence, setSemanticEvidence] = useState<SemanticEvidence | null>(null);
  const [journalOutput, setJournalOutput] = useState<string | null>(null);
  const [restoringBaseline, setRestoringBaseline] = useState(false);
  const [nativeStatusAvailable, setNativeStatusAvailable] = useState(false);
  const nativeConnected = isNativeBridge && nativeStatusAvailable;
  const [telemetry, setTelemetry] = useState<AutonomousTelemetry>(() => {
    const initial = createInitialAutonomousTelemetry();
    return isNativeBridge
      ? {
          ...initial,
          engineActive: false,
          uclampMin: 0,
          uclampMax: 0,
          uclampSupported: false,
          socTempCelsius: -1,
          cpuPressurePct: 0,
          memoryPressurePct: 0,
          ioPressurePct: 0,
          cpuPressureValid: false,
          memoryPressureValid: false,
          ioPressureValid: false,
          baselineNodesSnapshotted: 0,
          baselineStatus: 'restored',
          transactionStage: 'PLANNED',
          modeLabel: 'Awaiting native status',
          modeDescription: 'Waiting for coreflowd runtime status',
          vulkan: computeVulkanTelemetry(DEFAULT_VULKAN_CONFIG),
        }
      : initial;
  });
  const [thermalTripCelsius, setThermalTripCelsius] = useState(
    DEFAULT_AUTONOMOUS_CONFIG.thermalTripCelsius
  );
  type NavTab = 'dashboard' | 'thermal' | 'vulkan' | 'hardware' | 'validation' | 'console';
  const [activeTab, setActiveTab] = useState<NavTab>('dashboard');
  const [vulkanConfig, setVulkanConfig] = useState<VulkanEngineConfig>(() => ({
    ...DEFAULT_VULKAN_CONFIG,
  }));
  const [vulkanSaveMessage, setVulkanSaveMessage] = useState<string | null>(null);
  const [logs, setLogs] = useState<ReactionLogEvent[]>(() =>
    isNativeBridge ? [] : createInitialLogs()
  );
  const [isStudioOpen, setIsStudioOpen] = useState(false);
  const [hardwareInfo, setHardwareInfo] = useState({
    socModel: 'Unavailable',
    kernelVersion: 'Unavailable',
    onlineCpus: 'Unavailable',
    gpuType: 'Unavailable',
  });

  useEffect(() => {
    if (!isNativeBridge) return;
    void execChecked(`${RESOLVE_BIN}; "$BIN" --capabilities`)
      .then((raw) => {
        const result = extractJsonObject(raw) as Record<string, unknown>;
        if (
          result &&
          ['socModel', 'kernelVersion', 'onlineCpus', 'gpuType'].every(
            (key) => typeof result[key] === 'string'
          )
        ) {
          setHardwareInfo(result as typeof hardwareInfo);
        }
      })
      .catch(() => {});
  }, [isNativeBridge]);

  const [isSaving, setIsSaving] = useState(false);

  const isDark = resolveIsDark(appearance.theme);

  useEffect(() => {
    applyAppearance(appearance);
  }, [appearance]);

  const toggleTheme = () => {
    const theme: Theme = isDark ? 'light' : 'dark';
    const next = { ...appearance, theme };
    setAppearance(next);
    saveAppearance(next);
  };

  const handleSelectAccent = (accent: AccentSeed) => {
    const next = { ...appearance, accent };
    setAppearance(next);
    saveAppearance(next);
  };

  const healthSummary = useMemo(
    () => computeSystemHealthSummary(telemetry, thermalTripCelsius),
    [telemetry, thermalTripCelsius]
  );

  const telemetryRef = useRef(telemetry);
  telemetryRef.current = telemetry;
  const logSeqRef = useRef(0);

  const addLog = useCallback((type: ReactionLogEvent['type'], tag: string, message: string) => {
    logSeqRef.current += 1;
    const event: ReactionLogEvent = {
      id: `evt-${Date.now()}-${logSeqRef.current}`,
      timestamp: new Date().toTimeString().slice(0, 8),
      type,
      tag,
      message,
    };
    setLogs((old) => [event, ...old].slice(0, 50));
  }, []);

  const refreshNative = useCallback(async (): Promise<NativeStatus> => {
    try {
      const status = checkedNativeStatus(await execChecked(`${RESOLVE_BIN}; "$BIN" --status`));
      setSemanticEvidence(status.semanticEvidence ?? null);
      // A schema-valid daemon payload proves the daemon link even when the semantic
      // adapter honestly reports status "unavailable"; bindings are gated separately.
      setNativeStatusAvailable(status.semanticEvidence !== undefined);
      const effectiveTemp = status.socCompositeCelsius ?? status.socTempCelsius ?? -1;
      const isThermalThrottling =
        status.directAdaptive?.thermalGuard ??
        status.thermalGuardActive ??
        (status.thermalFallbackActive
          ? effectiveTemp >= thermalTripCelsius
          : (status.thermalSeverity ?? 0) >= 3);
      const adaptiveMode = status.directAdaptive?.mode;
      const adaptiveModeLabel = adaptiveMode
        ? ({
            SCREEN_OFF: 'Screen Off / Deep Idle',
            BALANCED_SWEET_SPOT: 'Balanced Sweet-Spot',
            ACTIVE_LOAD: 'Active Load / Gaming',
            THERMAL_GUARD: 'Thermal Guard',
            DISCOVERY: 'Capability Discovery',
          } as Record<string, string>)[adaptiveMode] ?? adaptiveMode
        : undefined;
      setTelemetry((old) => ({
        ...old,
        engineActive: status.running && (status.directAdaptive?.active ?? true),
        directAdaptive: status.directAdaptive,
        platformVendor: status.platformVendor ?? old.platformVendor,
        osFamily: status.osFamily ?? old.osFamily,
        osMajor: status.osMajor ?? old.osMajor,
        executionTier: status.executionTier ?? old.executionTier,
        policyFamily: status.policyFamily ?? old.policyFamily,
        identityConfidence: status.identityConfidence ?? old.identityConfidence,
        tier1Eligible: status.tier1Eligible ?? old.tier1Eligible,
        inspectionMode: status.inspectionMode ?? old.inspectionMode,
        mutationRuntimeStarted: status.mutationRuntimeStarted ?? old.mutationRuntimeStarted,
        uclampMin: status.uclampMin,
        uclampMax: status.uclampMax,
        uclampSupported: status.uclampSupported,
        uclampPolicyMin: status.uclampPolicyMin ?? old.uclampPolicyMin,
        uclampPolicyMax: status.uclampPolicyMax ?? old.uclampPolicyMax,
        uclampTargetPath: status.uclampTargetPath ?? old.uclampTargetPath,
        uclampEncoding: status.uclampEncoding ?? old.uclampEncoding,
        // Null pressure is unavailable telemetry, not a request to retain stale values.
        cpuPressurePct: status.cpuPressurePct ?? 0,
        memoryPressurePct: status.memoryPressurePct ?? 0,
        ioPressurePct: status.ioPressurePct ?? 0,
        cpuPressureValid: status.cpuPressureValid ?? status.cpuPressurePct != null,
        memoryPressureValid: status.memoryPressureValid ?? status.memoryPressurePct != null,
        ioPressureValid: status.ioPressureValid ?? status.ioPressurePct != null,
        baselineNodesSnapshotted: status.baselineNodesSnapshotted ?? old.baselineNodesSnapshotted,
        socTempCelsius: effectiveTemp,
        socCompositeCelsius: status.socCompositeCelsius ?? effectiveTemp,
        cpuAverageCelsius: status.cpuAverageCelsius ?? -1,
        cpuPeakCelsius: status.cpuPeakCelsius ?? -1,
        gpuTempCelsius: status.gpuTempCelsius ?? -1,
        batteryTempCelsius: status.batteryTempCelsius ?? -1,
        skinTempCelsius: status.skinTempCelsius ?? -1,
        thermalGuardActive: isThermalThrottling,
        thermalDataDegraded: status.thermalDataDegraded ?? false,
        thermalFallbackActive: status.thermalFallbackActive ?? false,
        thermalHeadroomValid: status.thermalHeadroomValid ?? false,
        thermalHeadroom: status.thermalHeadroom ?? null,
        thermalPressure: status.thermalPressure ?? old.thermalPressure,
        thermalSource: status.thermalSource ?? old.thermalSource,
        thermalSeverity: status.thermalSeverity ?? old.thermalSeverity,
        thermalSensors: status.thermalSensors ?? old.thermalSensors,
        aiBackend: status.aiBackend ?? old.aiBackend,
        aiOnnxActive: status.aiOnnxActive ?? old.aiOnnxActive,
        aiPredictedPressure10s: status.aiPredictedPressure10s ?? old.aiPredictedPressure10s,
        aiPredictedVelocity: status.aiPredictedVelocity ?? old.aiPredictedVelocity,
        aiRecommendedScale: status.aiRecommendedScale ?? old.aiRecommendedScale,
        aiConfidence: status.aiConfidence ?? old.aiConfidence,
        aiSamples: status.aiSamples ?? old.aiSamples,
        tunerRuntimeStage: status.tunerRuntimeStage ?? old.tunerRuntimeStage,
        tunerStage: status.tunerStage ?? old.tunerStage,
        tunerWorkload: status.tunerWorkload ?? old.tunerWorkload,
        tunerBottleneck: status.tunerBottleneck ?? old.tunerBottleneck,
        tunerObjective: status.tunerObjective ?? old.tunerObjective,
        tunerIntent: status.tunerIntent ?? old.tunerIntent,
        tunerCandidateId: status.tunerCandidateId ?? old.tunerCandidateId,
        tunerCandidateLabel: status.tunerCandidateLabel ?? old.tunerCandidateLabel,
        tunerCapabilityFingerprint:
          status.tunerCapabilityFingerprint ?? old.tunerCapabilityFingerprint,
        tunerDecisionPlanRationale:
          status.tunerDecisionPlanRationale ?? old.tunerDecisionPlanRationale,
        tunerRationale: status.tunerRationale ?? old.tunerRationale,
        tunerVerdict: status.tunerVerdict ?? old.tunerVerdict,
        tunerUtilityDelta: status.tunerUtilityDelta ?? old.tunerUtilityDelta,
        tunerTrials: status.tunerTrials ?? old.tunerTrials,
        tunerImproved: status.tunerImproved ?? old.tunerImproved,
        tunerRolledBack: status.tunerRolledBack ?? old.tunerRolledBack,
        blockQueues: status.blockQueues ?? old.blockQueues,
        tunerCandidates: status.tunerCandidates ?? old.tunerCandidates,
        tunerEvaluatedCandidates: status.tunerEvaluatedCandidates ?? old.tunerEvaluatedCandidates,
        tunerActiveInterventions: status.tunerActiveInterventions ?? old.tunerActiveInterventions,
        vulkan: status.vulkan ?? old.vulkan,
        baselineStatus:
          status.recoveryPending ||
          (status.directAdaptive?.active === true && status.directAdaptive.baselineNodes > 0)
            ? 'protected'
            : 'restored',
        transactionStage: status.running
          ? status.recoveryPending
            ? 'RETAINED'
            : 'VERIFIED'
          : 'PLANNED',
        modeLabel: status.running
          ? adaptiveModeLabel ??
            (isThermalThrottling ? 'Thermal Guard' : 'Direct Adaptive runtime active')
          : 'Daemon Stopped',
        modeDescription: status.running
          ? status.directAdaptive?.journalFault
            ? 'Journal fault latched; all further Direct Adaptive mutations are halted until baseline recovery.'
            : status.directAdaptive?.ownershipLosses
              ? 'OEM contention isolated at affected nodes; independent capabilities continue.'
              : status.directAdaptive?.active
                ? 'Direct Baseline with capability-derived Sweet-Spot policy and verified readback.'
                : 'Daemon is running; Direct Adaptive discovery or baseline preparation is unavailable.'
          : 'OEM baseline restored',
        timestamp: Date.now(),
      }));
      setNativeError(null);
      return status;
    } catch (error) {
      setNativeStatusAvailable(false);
      setSemanticEvidence(null);
      setTelemetry((old) => ({ ...old, directAdaptive: undefined }));
      throw error;
    }
  }, [thermalTripCelsius]);

  useEffect(() => {
    if (!isNativeBridge) return;
    let disposed = false;

    const loadInitialConfig = async () => {
      const configResult = await execNativeKsuShell(`cat ${CONFIG} 2>/dev/null`);
      if (disposed || configResult.errno !== 0) return;

      const trip = readSectionNumberKey(configResult.stdout, 'thermal_trip_celsius');
      if (trip !== null) setThermalTripCelsius(Math.max(30, Math.min(80, trip)));

      const rawVkMode = readSectionStringKey(configResult.stdout, 'vulkan_layer_mode');
      const vkMode: VulkanLayerMode =
        rawVkMode === 'safe_hardware' || rawVkMode === 'full_capability_override'
          ? 'safe_hardware'
          : rawVkMode === 'disabled'
            ? 'disabled'
            : 'disabled';
      const readFlag = (key: string): boolean | null => {
        const raw = readSectionStringKey(configResult.stdout, key);
        return raw === 'true' ? true : raw === 'false' ? false : null;
      };
      setVulkanConfig((old) => ({
        ...old,
        mode: vkMode,
        enableSkiavkThreaded: readFlag('enable_skiavk_threaded') ?? old.enableSkiavkThreaded,
        enableAdaptiveVrs: readFlag('enable_adaptive_vrs') ?? old.enableAdaptiveVrs,
        enablePipelineCachePrewarm:
          readFlag('enable_pipeline_cache_prewarm') ?? old.enablePipelineCachePrewarm,
      }));
    };

    let pollInFlight = false;
    let previousLogSnapshot = '';
    const pollNativeTelemetry = async () => {
      if (pollInFlight || disposed) return;
      pollInFlight = true;
      try {
        await refreshNative();

        const logResult = await execNativeKsuShell(`/system/bin/tail -n 12 ${LOG} 2>/dev/null`);
        if (!disposed && logResult.errno === 0) {
          const snapshot = logResult.stdout.trim();
          if (snapshot && snapshot !== previousLogSnapshot) {
            previousLogSnapshot = snapshot;
            const lines = snapshot
              .split(/\r?\n/)
              .map((line) => line.trim())
              .filter(Boolean)
              .slice(-12)
              .reverse();
            setLogs(
              lines.map((message, index) => ({
                id: `native-${index}-${message}`,
                timestamp: '--:--:--',
                type:
                  message.includes('[Stability]') && message.includes('thermal')
                    ? ('THERMAL' as const)
                    : ('INFO' as const),
                tag: 'Daemon',
                message: message.slice(0, 384),
              }))
            );
          }
        }
      } catch (error) {
        if (!disposed) setNativeError(String(error));
      } finally {
        pollInFlight = false;
      }
    };

    void loadInitialConfig();
    void pollNativeTelemetry();
    const timer = window.setInterval(() => void pollNativeTelemetry(), 10000);
    return () => {
      disposed = true;
      window.clearInterval(timer);
    };
  }, [isNativeBridge, refreshNative]);

  const advancePreviewSimulation = useCallback((forced?: 'cpu' | 'memory' | 'io_trial') => {
    const { next, event } = stepAutonomousSimulation(
      telemetryRef.current,
      DEFAULT_AUTONOMOUS_CONFIG,
      forced
    );
    telemetryRef.current = next;
    setTelemetry(next);
    if (event) {
      setLogs((previous) => [event, ...previous].slice(0, 50));
    }
  }, []);

  useEffect(() => {
    if (isNativeBridge) return;
    const timer = window.setInterval(() => {
      advancePreviewSimulation();
    }, 2500);
    return () => window.clearInterval(timer);
  }, [isNativeBridge, advancePreviewSimulation]);

  // This is intentionally the only WebUI configuration writer. It cannot
  // accept arbitrary system-policy keys; the sole mutable option is Vulkan mode.
  const writeVulkanLayerMode = async (mode: VulkanLayerMode) => {
    const existingResult = await execNativeKsuShell(
      `cat "${CONFIG}" 2>/dev/null || cat /data/adb/modules/cfae_framework/system/etc/coreflow/default.conf 2>/dev/null || cat /system/etc/coreflow/default.conf 2>/dev/null || true`
    );
    const merged = mergeConfigText(existingResult.stdout ?? '', { vulkan_layer_mode: mode });
    const mergedText = merged.endsWith('\n') ? merged : merged + '\n';
    let delimiter = 'COREFLOW_CONFIG';
    for (
      let attempt = 1;
      mergedText.split('\n').some((line) => line.trim() === delimiter);
      attempt++
    ) {
      delimiter = `COREFLOW_CONFIG_${attempt}`;
    }
    const command =
      'umask 077 && [ ! -L "' +
      CONFIG +
      '" ] && rm -f "' +
      CONFIG +
      '.tmp" && ' +
      `cat <<'${delimiter}' > ` +
      CONFIG +
      '.tmp\n' +
      mergedText +
      delimiter +
      '\nchown 0:0 ' +
      CONFIG +
      '.tmp 2>/dev/null; chmod 600 ' +
      CONFIG +
      '.tmp && mv -f ' +
      CONFIG +
      '.tmp ' +
      CONFIG +
      ' && echo COREFLOW_VULKAN_WRITTEN';
    const output = await execChecked(command);
    if (!output.includes('COREFLOW_VULKAN_WRITTEN')) {
      throw new Error('Vulkan layer mode write not acknowledged');
    }
  };

  const handleSaveVulkanConfig = async (nextVulkan: VulkanEngineConfig) => {
    setIsSaving(true);
    setVulkanSaveMessage(null);
    try {
      const normalizedMode: VulkanLayerMode =
        nextVulkan.mode === 'disabled' ? 'disabled' : 'safe_hardware';
      const sanitizedVulkan: VulkanEngineConfig = { ...nextVulkan, mode: normalizedMode };

      if (!isNativeBridge) {
        setVulkanConfig(sanitizedVulkan);
        setTelemetry((old) => ({
          ...old,
          vulkan: computeVulkanTelemetry(sanitizedVulkan, old.vulkan),
        }));
        setVulkanSaveMessage(
          normalizedMode === 'safe_hardware'
            ? 'Truthful Hardware Pass-Through active in preview'
            : 'OEM Passthrough (Disabled) active in preview'
        );
        addLog(
          'BOOST',
          'VulkanLayer',
          `Layer mode=${normalizedMode}; rendering optimizers remain unimplemented.`
        );
        return;
      }

      await writeVulkanLayerMode(normalizedMode);
      setVulkanConfig(sanitizedVulkan);
      setTelemetry((old) => ({
        ...old,
        vulkan: computeVulkanTelemetry(sanitizedVulkan, old.vulkan),
      }));
      await execChecked('killall -HUP coreflowd 2>/dev/null || true');
      setVulkanSaveMessage('Mode saved · daemon reload requested');
      void refreshNative().catch(() => {});
      addLog(
        'BOOST',
        'VulkanLayer',
        `Vulkan layer mode changed to ${normalizedMode}; no system tuning policy was changed.`
      );
    } catch (err) {
      setVulkanSaveMessage('Save failed');
      setNativeError(String(err));
    } finally {
      setIsSaving(false);
    }
  };

  const handleViewJournal = useCallback(async () => {
    try {
      const raw = await execChecked(`${RESOLVE_BIN}; "$BIN" --journal`);
      const parsed = extractJsonObject(raw);
      setJournalOutput(JSON.stringify(parsed, null, 2));
      setNativeError(null);
    } catch (error) {
      setNativeError('Unable to read the mutation journal: ' + String(error));
    }
  }, []);

  const handleRestoreBaseline = useCallback(async () => {
    if (
      !window.confirm(
        'Restore every CoreFlow-owned node to its captured OEM baseline and stop coreflowd?'
      )
    )
      return;
    setRestoringBaseline(true);
    try {
      await execChecked(`${RESOLVE_BIN}; "$BIN" --rollback-baseline`);
      setJournalOutput(null);
      await refreshNative();
      addLog(
        'BASELINE',
        'Baseline',
        'OEM baseline rollback requested from the Direct Adaptive audit panel.'
      );
    } catch (error) {
      setNativeError('OEM baseline restore failed: ' + String(error));
    } finally {
      setRestoringBaseline(false);
    }
  }, [addLog, refreshNative]);

  const navItems: {
    id: NavTab;
    label: string;
    shortLabel: string;
    Icon: React.FC<{ className?: string }>;
  }[] = [
    { id: 'dashboard', label: 'Overview', shortLabel: 'Overview', Icon: Activity },
    { id: 'thermal', label: 'Thermal Matrix', shortLabel: 'Thermal', Icon: Flame },
    { id: 'vulkan', label: 'Vulkan & GPU', shortLabel: 'Vulkan', Icon: Layers },
    { id: 'hardware', label: 'Hardware & CDDP', shortLabel: 'Hardware', Icon: HardDrive },
    { id: 'validation', label: 'DVS Validation', shortLabel: 'Validation', Icon: ClipboardCheck },
    { id: 'console', label: 'Audit & RPC', shortLabel: 'Audit', Icon: Terminal },
  ];

  return (
    <div
      className="min-h-screen relative overflow-x-hidden flex flex-col font-sans antialiased"
      style={{
        background: 'var(--md-sys-color-background)',
        color: 'var(--md-sys-color-on-surface)',
      }}
    >
      {/* Bridge and telemetry trust state banner */}
      <div
        role="status"
        className="px-4 py-2 text-center text-xs font-medium"
        style={{
          background: !isNativeBridge
            ? 'var(--md-sys-color-warning-container)'
            : nativeConnected
              ? 'var(--md-sys-color-success-container)'
              : 'var(--md-sys-color-warning-container)',
          color: !isNativeBridge
            ? 'var(--md-sys-color-on-warning-container)'
            : nativeConnected
              ? 'var(--md-sys-color-on-success-container)'
              : 'var(--md-sys-color-on-warning-container)',
          borderBottom: '1px solid var(--md-sys-color-outline-variant)',
        }}
      >
        {!isNativeBridge
          ? 'Simulation Mode · Browser-only preview; synthetic values are labelled (sim) and are not kernel audit evidence.'
          : nativeConnected
            ? 'Connected · Native bridge and official daemon status with semanticEvidence schema v1 received.'
            : 'Native bridge detected · Awaiting an official daemon status payload; semantic telemetry is unavailable.'}
      </div>

      {nativeError && (
        <div
          role="alert"
          className="px-4 py-2 text-xs font-medium text-center"
          style={{
            background: 'var(--md-sys-color-error-container)',
            color: 'var(--md-sys-color-on-error-container)',
          }}
        >
          {nativeError}
        </div>
      )}

      {/* Material Design 3 Top App Bar */}
      <PixelHeader
        isDark={isDark}
        accent={appearance.accent}
        onToggleTheme={toggleTheme}
        onSelectAccent={handleSelectAccent}
        modeLabel={telemetry.modeLabel}
        nativeMode={isNativeBridge}
        nativeConnected={nativeConnected}
        equilibriumScore={healthSummary.equilibriumScore}
        onOpenArchitectureStudio={() => setIsStudioOpen(true)}
      />

      {/* Material Design 3 Adaptive Shell: Navigation Rail (Desktop/Tablet) + Main Workspace */}
      <div className="flex-1 w-full max-w-[1440px] mx-auto px-3 sm:px-6 py-5 sm:py-7 flex flex-col lg:flex-row gap-6">
        {/* Desktop M3 Navigation Rail */}
        <aside className="hidden lg:block w-56 shrink-0">
          <nav
            aria-label="Primary sections"
            className="md3-nav-rail sticky top-24 space-y-1.5"
          >
            <div
              className="px-3 py-2 text-[10px] font-bold uppercase tracking-[0.16em]"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              System Domains
            </div>
            {navItems.map(({ id, label, Icon }) => {
              const active = activeTab === id;
              return (
                <button
                  key={id}
                  type="button"
                  aria-current={active ? 'page' : undefined}
                  onClick={() => setActiveTab(id)}
                  className={
                    'w-full flex items-center gap-3 px-3.5 py-2.5 rounded-full text-xs font-medium transition-colors text-left ' +
                    (active ? 'font-semibold' : '')
                  }
                  style={{
                    background: active
                      ? 'var(--md-sys-color-secondary-container)'
                      : 'transparent',
                    color: active
                      ? 'var(--md-sys-color-on-secondary-container)'
                      : 'var(--md-sys-color-on-surface-variant)',
                  }}
                >
                  <Icon className="w-4 h-4 shrink-0" />
                  <span className="truncate">{label}</span>
                </button>
              );
            })}

            <div
              className="mt-4 pt-3 px-3 space-y-1.5 text-[11px]"
              style={{ borderTop: '1px solid var(--md-sys-color-outline-variant)' }}
            >
              <div
                className="font-semibold"
                style={{ color: 'var(--md-sys-color-on-surface)' }}
              >
                Execution Tier
              </div>
              <div
                className="font-mono text-[11px]"
                style={{ color: 'var(--md-sys-color-primary)' }}
              >
                {telemetry.executionTier ?? 'TIER_1_FULL_COORDINATED'}
              </div>
              <div
                className="text-[10px]"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                {telemetry.policyFamily ?? 'android_linux_v4'}
              </div>
            </div>
          </nav>
        </aside>

        {/* Mobile / Compact WebView M3 Segmented Pill Bar */}
        <nav
          aria-label="Primary sections mobile"
          className="lg:hidden md3-bottom-nav flex items-center gap-1.5 overflow-x-auto no-scrollbar max-w-full"
        >
          {navItems.map(({ id, shortLabel, Icon }) => {
            const active = activeTab === id;
            return (
              <button
                key={id}
                type="button"
                aria-current={active ? 'page' : undefined}
                onClick={() => setActiveTab(id)}
                className={
                  'md3-chip flex items-center gap-1.5 whitespace-nowrap select-none ' +
                  (active ? 'md3-chip-active' : '')
                }
              >
                <Icon className="w-3.5 h-3.5 shrink-0" />
                <span>{shortLabel}</span>
              </button>
            );
          })}
        </nav>

        {/* Main Content Surface */}
        <main className="flex-1 min-w-0 space-y-6">
          {activeTab === 'dashboard' && (
            <div className="space-y-6 md3-page-stage">
              <section className="space-y-4" aria-label="Autonomous runtime state">
                <div className="flex flex-col gap-2 sm:flex-row sm:items-end sm:justify-between">
                  <div>
                    <p
                      className="text-[11px] font-bold uppercase tracking-[0.18em]"
                      style={{ color: 'var(--md-sys-color-primary)' }}
                    >
                      CFAE · Material Design 3 System Console
                    </p>
                    <h1
                      className="mt-1 text-2xl font-semibold tracking-tight sm:text-3xl"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      Autonomous Operating State
                    </h1>
                    <p
                      className="mt-1 max-w-2xl text-xs sm:text-sm leading-relaxed"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Capability discovery, evidence-led tuning, and verified actuator ownership.
                      The console reflects native daemon telemetry while selection remains
                      autonomous and safety-gated.
                    </p>
                  </div>
                  <div className="flex flex-wrap items-center gap-2">
                    <span
                      className={
                        'md3-badge ' +
                        (telemetry.engineActive ? 'md3-badge-success' : 'md3-badge-neutral')
                      }
                    >
                      <span
                        className="h-1.5 w-1.5 rounded-full"
                        style={{
                          background: telemetry.engineActive
                            ? 'var(--md-sys-color-success)'
                            : 'var(--md-sys-color-outline)',
                        }}
                      />
                      {telemetry.engineActive ? 'Autonomous Engine Active' : 'Daemon Standby'}
                    </span>
                  </div>
                </div>

                {/* Always render AutonomousStateHero so both Live & Preview modes get full transaction & bandit visibility */}
                <AutonomousStateHero
                  telemetry={telemetry}
                  nativeMode={isNativeBridge}
                  thermalTripCelsius={thermalTripCelsius}
                  onTriggerSimulation={!isNativeBridge ? advancePreviewSimulation : undefined}
                />

                {isNativeBridge && (
                  <DirectAdaptiveCard
                    status={telemetry.directAdaptive}
                    nativeMode={true}
                    journalOutput={journalOutput}
                    restoringBaseline={restoringBaseline}
                    onViewJournal={handleViewJournal}
                    onRestoreBaseline={handleRestoreBaseline}
                  />
                )}
              </section>

              {/* M3 Tonal Summary Strip */}
              <div className="grid grid-cols-1 gap-3 sm:grid-cols-3">
                <div className="md3-card-elevated flex items-start gap-3.5 p-4">
                  <div
                    className="rounded-2xl p-2.5 shrink-0"
                    style={{
                      background: 'var(--md-sys-color-primary-container)',
                      color: 'var(--md-sys-color-on-primary-container)',
                    }}
                  >
                    <Thermometer className="h-4 w-4" />
                  </div>
                  <div className="min-w-0">
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Thermal Headroom
                    </div>
                    <div
                      className="mt-1 text-lg font-semibold tabular-nums"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      {telemetry.thermalHeadroomValid && telemetry.thermalHeadroom != null
                        ? `${Math.round(telemetry.thermalHeadroom * 100)}%`
                        : 'Unavailable'}
                    </div>
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      {telemetry.thermalGuardActive
                        ? 'Thermal guard engaged'
                        : 'OEM thermal policy respected'}
                    </div>
                  </div>
                </div>

                <div className="md3-card-elevated flex items-start gap-3.5 p-4">
                  <div
                    className="rounded-2xl p-2.5 shrink-0"
                    style={{
                      background: 'var(--md-sys-color-primary-container)',
                      color: 'var(--md-sys-color-on-primary-container)',
                    }}
                  >
                    <Gauge className="h-4 w-4" />
                  </div>
                  <div className="min-w-0">
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Resource Pressure
                    </div>
                    <div
                      className="mt-1 text-lg font-semibold tabular-nums"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      {telemetry.cpuPressureValid ||
                      telemetry.memoryPressureValid ||
                      telemetry.ioPressureValid
                        ? `${Math.max(
                            telemetry.cpuPressureValid ? telemetry.cpuPressurePct : 0,
                            telemetry.memoryPressureValid ? telemetry.memoryPressurePct : 0,
                            telemetry.ioPressureValid ? telemetry.ioPressurePct : 0
                          ).toFixed(1)}%`
                        : 'Unavailable'}
                    </div>
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Highest available PSI avg10
                    </div>
                  </div>
                </div>

                <div className="md3-card-elevated flex items-start gap-3.5 p-4">
                  <div
                    className="rounded-2xl p-2.5 shrink-0"
                    style={{
                      background: 'var(--md-sys-color-primary-container)',
                      color: 'var(--md-sys-color-on-primary-container)',
                    }}
                  >
                    <LockKeyhole className="h-4 w-4" />
                  </div>
                  <div className="min-w-0">
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Mutation Ownership
                    </div>
                    <div
                      className="mt-1 text-lg font-semibold"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      {telemetry.baselineStatus === 'protected' ? 'Journal Armed' : 'Guarded'}
                    </div>
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      CFJ4 Baseline + Readback guard
                    </div>
                  </div>
                </div>
              </div>

              <SemanticEvidencePanel
                evidence={semanticEvidence}
                nativeConnected={nativeConnected}
                simulationMode={!isNativeBridge}
              />
              {!isNativeBridge && (
                <BaselineGuardianCard
                  nodesSnapshotted={telemetry.baselineNodesSnapshotted}
                  nativeMode={false}
                  baselineStatus={telemetry.baselineStatus}
                />
              )}
              <LiveReactionStream logs={logs} nativeMode={isNativeBridge} />
            </div>
          )}

          {activeTab === 'thermal' && (
            <div className="space-y-6 md3-page-stage">
              <header className="space-y-1">
                <p
                  className="text-[11px] font-bold uppercase tracking-[0.18em]"
                  style={{ color: 'var(--md-sys-color-primary)' }}
                >
                  Protection Domain
                </p>
                <h1
                  className="text-2xl font-semibold tracking-tight sm:text-3xl"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  Thermal Intelligence & Multi-Zone Matrix
                </h1>
                <p
                  className="text-xs sm:text-sm leading-relaxed max-w-2xl"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Sensor evidence, velocity tracking, and OEM thermal constraints are surfaced for
                  full operational transparency. Mitigation is selected autonomously.
                </p>
              </header>
              <ThermalMatrixCard
                telemetry={telemetry}
                nativeMode={isNativeBridge}
                thermalTripCelsius={thermalTripCelsius}
              />
              <LiveReactionStream
                logs={logs.filter(
                  (event) =>
                    event.type === 'THERMAL' || event.tag.toLowerCase().includes('thermal')
                )}
                nativeMode={isNativeBridge}
              />
            </div>
          )}

          {activeTab === 'vulkan' && (
            <div className="space-y-6 md3-page-stage">
              <header className="space-y-1">
                <p
                  className="text-[11px] font-bold uppercase tracking-[0.18em]"
                  style={{ color: 'var(--md-sys-color-primary)' }}
                >
                  User Opt-In Subsystem
                </p>
                <h1
                  className="text-2xl font-semibold tracking-tight sm:text-3xl"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  Vulkan Diagnostics & Explicit Layer
                </h1>
                <p
                  className="text-xs sm:text-sm leading-relaxed max-w-2xl"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Vulkan layer activation (`VK_LAYER_COREFLOW_optimizer`) is the sole manual feature
                  switch in CoreFlow. All kernel tuning domains remain strictly autonomous.
                </p>
              </header>
              <VulkanCapabilitiesCard
                vulkanTelemetry={telemetry.vulkan}
                vulkanConfig={vulkanConfig}
                nativeMode={isNativeBridge}
                onSaveVulkanConfig={handleSaveVulkanConfig}
                isSaving={isSaving}
                saveMessage={vulkanSaveMessage}
              />
              <LiveReactionStream
                logs={logs.filter((event) => event.tag.toLowerCase().includes('vulkan'))}
                nativeMode={isNativeBridge}
              />
            </div>
          )}

          {activeTab === 'hardware' && (
            <div className="space-y-6 md3-page-stage">
              <header className="space-y-1">
                <p
                  className="text-[11px] font-bold uppercase tracking-[0.18em]"
                  style={{ color: 'var(--md-sys-color-primary)' }}
                >
                  Capability Discovery & CDDP
                </p>
                <h1
                  className="text-2xl font-semibold tracking-tight sm:text-3xl"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  Hardware & Platform Identity Profile
                </h1>
                <p
                  className="text-xs sm:text-sm leading-relaxed max-w-2xl"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Detected hardware is evidence, not automatic mutation authority. CoreFlow adapts
                  only through validated operational contracts.
                </p>
              </header>

              {/* Device Identity */}
              <section className="md3-card-elevated space-y-4 p-5 sm:p-7">
                <div className="flex items-center gap-2.5">
                  <div
                    className="w-9 h-9 rounded-2xl flex items-center justify-center"
                    style={{
                      background: 'var(--md-sys-color-primary-container)',
                      color: 'var(--md-sys-color-on-primary-container)',
                    }}
                  >
                    <Cpu className="h-4 w-4" />
                  </div>
                  <div>
                    <h2
                      className="font-semibold text-base"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      Device Identity
                    </h2>
                    <p
                      className="text-xs"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Probed via `coreflowd --capabilities`
                    </p>
                  </div>
                </div>
                <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
                  {[
                    ['SoC Model', hardwareInfo.socModel],
                    ['GPU Subsystem', hardwareInfo.gpuType],
                    ['Kernel Release', hardwareInfo.kernelVersion],
                    ['Online CPUs', hardwareInfo.onlineCpus],
                  ].map(([label, value]) => (
                    <div key={label} className="md3-card-filled p-4">
                      <div
                        className="text-[11px]"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        {label}
                      </div>
                      <div
                        className="mt-1 break-words font-mono text-sm font-semibold"
                        style={{ color: 'var(--md-sys-color-on-surface)' }}
                      >
                        {value}
                      </div>
                    </div>
                  ))}
                </div>
              </section>

              {/* Platform Context & Execution Tier */}
              <section className="md3-card-elevated space-y-4 p-5 sm:p-7">
                <div className="flex items-center justify-between gap-2 flex-wrap">
                  <div className="flex items-center gap-2.5">
                    <div
                      className="w-9 h-9 rounded-2xl flex items-center justify-center"
                      style={{
                        background: 'var(--md-sys-color-secondary-container)',
                        color: 'var(--md-sys-color-on-secondary-container)',
                      }}
                    >
                      <Fingerprint className="h-4 w-4" />
                    </div>
                    <div>
                      <h2
                        className="font-semibold text-base"
                        style={{ color: 'var(--md-sys-color-on-surface)' }}
                      >
                        Platform Context & Execution Tier
                      </h2>
                      <p
                        className="text-xs"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        Cross-platform policy gating & capability fingerprint
                      </p>
                    </div>
                  </div>
                  <span
                    className={
                      'md3-badge font-mono ' +
                      (telemetry.tier1Eligible !== false ? 'md3-badge-success' : 'md3-badge-warning')
                    }
                  >
                    {telemetry.tier1Eligible !== false ? 'TIER-1 ELIGIBLE' : 'RESTRICTED TIER'}
                  </span>
                </div>

                <div className="grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs">
                  {[
                    ['Platform Vendor', telemetry.platformVendor ?? 'Generic / Android'],
                    [
                      'OS Family',
                      `${telemetry.osFamily ?? 'Android'}${telemetry.osMajor ? ` ${telemetry.osMajor}` : ''}`,
                    ],
                    ['Execution Tier', telemetry.executionTier ?? 'TIER_1_FULL_COORDINATED'],
                    ['Policy Family', telemetry.policyFamily ?? 'android_linux_v4'],
                    ['Identity Confidence', telemetry.identityConfidence ?? 'HIGH'],
                    [
                      'Capability Fingerprint',
                      telemetry.tunerCapabilityFingerprint ?? 'cf-cap-verified',
                    ],
                  ].map(([label, val]) => (
                    <div key={label} className="md3-card-filled p-3.5">
                      <div
                        className="text-[11px]"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        {label}
                      </div>
                      <div
                        className="mt-1 font-mono font-semibold break-all"
                        style={{ color: 'var(--md-sys-color-on-surface)' }}
                      >
                        {val}
                      </div>
                    </div>
                  ))}
                </div>
              </section>

              {/* Kernel Contract Classification Model */}
              <section className="md3-card-elevated space-y-4 p-5 sm:p-7">
                <div className="flex items-center gap-2.5">
                  <div
                    className="w-9 h-9 rounded-2xl flex items-center justify-center"
                    style={{
                      background: 'var(--md-sys-color-primary-container)',
                      color: 'var(--md-sys-color-on-primary-container)',
                    }}
                  >
                    <ShieldCheck className="h-4 w-4" />
                  </div>
                  <div>
                    <p
                      className="text-[10px] font-bold uppercase tracking-[0.16em]"
                      style={{ color: 'var(--md-sys-color-primary)' }}
                    >
                      Kernel Contract Model
                    </p>
                    <h2
                      className="text-base font-semibold"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      Discovery Does Not Grant Mutation Authority
                    </h2>
                  </div>
                </div>
                <p
                  className="text-xs leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Every discovered attribute receives an explicit contract classification. Unknown
                  nodes stay visible for hardware understanding but fail closed for automated writes.
                </p>
                <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
                  {[
                    {
                      name: 'RUNTIME_TUNABLE',
                      badge: 'md3-badge-success',
                      detail:
                        'Only class eligible for tuning after ABI, range, access and baseline verification.',
                    },
                    {
                      name: 'OBSERVATION_ONLY',
                      badge: 'md3-badge-primary',
                      detail:
                        'Hardware limits, topology, alignment and telemetry contribute to the fingerprint.',
                    },
                    {
                      name: 'TEST_ONLY',
                      badge: 'md3-badge-warning',
                      detail:
                        'Writable testing interface, excluded from daily performance optimization.',
                    },
                    {
                      name: 'PROTECTED',
                      badge: 'md3-badge-error',
                      detail:
                        'Integrity-critical or system-sensitive attribute; automatic mutation prohibited.',
                    },
                  ].map((item) => (
                    <div key={item.name} className="md3-card-outlined p-4">
                      <span className={'md3-badge font-mono ' + item.badge}>{item.name}</span>
                      <p
                        className="mt-2 text-xs leading-relaxed"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        {item.detail}
                      </p>
                    </div>
                  ))}
                </div>
              </section>

              {/* Autonomous Hardware Preservation */}
              <section className="md3-card-elevated space-y-4 p-5 sm:p-7">
                <div className="flex items-center gap-2">
                  <HardDrive
                    className="h-4 w-4"
                    style={{ color: 'var(--md-sys-color-primary)' }}
                  />
                  <h2
                    className="font-semibold text-base"
                    style={{ color: 'var(--md-sys-color-on-surface)' }}
                  >
                    Autonomous Hardware Preservation
                  </h2>
                </div>
                <p
                  className="text-xs sm:text-sm leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Battery, storage, and I/O protection are evaluated from live workload and device
                  evidence. No manual thresholds or scheduler presets are exposed here; unsupported
                  nodes remain observation-only until they earn mutation authority.
                </p>
                <div className="grid grid-cols-1 gap-3 sm:grid-cols-3">
                  <div className="md3-card-filled p-4">
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Battery Temperature
                    </div>
                    <div
                      className="mt-1 font-mono text-lg font-semibold"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      {telemetry.batteryTempCelsius != null && telemetry.batteryTempCelsius >= 0
                        ? `${telemetry.batteryTempCelsius.toFixed(1)} °C`
                        : 'N/A'}
                    </div>
                  </div>
                  <div className="md3-card-filled p-4">
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Block Queues Discovered
                    </div>
                    <div
                      className="mt-1 font-mono text-lg font-semibold"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      {telemetry.blockQueues?.length ?? 0}
                    </div>
                  </div>
                  <div className="md3-card-filled p-4">
                    <div
                      className="text-[11px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      Baseline Snapshots
                    </div>
                    <div
                      className="mt-1 font-mono text-lg font-semibold"
                      style={{ color: 'var(--md-sys-color-on-surface)' }}
                    >
                      {telemetry.baselineNodesSnapshotted}
                    </div>
                  </div>
                </div>
              </section>

              <BaselineGuardianCard
                nodesSnapshotted={telemetry.baselineNodesSnapshotted}
                nativeMode={isNativeBridge}
                baselineStatus={telemetry.baselineStatus}
              />
            </div>
          )}

          {activeTab === 'validation' && (
            <div className="space-y-6 md3-page-stage">
              <header className="space-y-1">
                <p
                  className="text-[11px] font-bold uppercase tracking-[0.18em]"
                  style={{ color: 'var(--md-sys-color-primary)' }}
                >
                  Evidence, Not Assumptions
                </p>
                <h1
                  className="text-2xl font-semibold tracking-tight sm:text-3xl"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  Device Validation Suite (CF-DVS)
                </h1>
                <p
                  className="text-xs sm:text-sm leading-relaxed max-w-2xl"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Inspect the live validation contract, 8 invariant pass criteria, and native
                  subsystem readiness.
                </p>
              </header>
              <DeviceValidationSuiteCard nativeMode={isNativeBridge} />
            </div>
          )}

          {activeTab === 'console' && (
            <div className="space-y-6 md3-page-stage">
              <header className="space-y-1">
                <p
                  className="text-[11px] font-bold uppercase tracking-[0.18em]"
                  style={{ color: 'var(--md-sys-color-primary)' }}
                >
                  Runtime Transparency
                </p>
                <h1
                  className="text-2xl font-semibold tracking-tight sm:text-3xl"
                  style={{ color: 'var(--md-sys-color-on-surface)' }}
                >
                  Read-Only Audit & Diagnostic RPC
                </h1>
                <p
                  className="text-xs sm:text-sm leading-relaxed max-w-2xl"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  Query read-only runtime evidence, CFJ4 journals, DVS validation payloads, and
                  decision logs directly from `coreflowd`.
                </p>
              </header>
              <DeveloperConsoleCard
                nativeMode={isNativeBridge}
                onRefreshTelemetry={async () => {
                  await refreshNative();
                }}
              />
              <LiveReactionStream logs={logs} nativeMode={isNativeBridge} />
            </div>
          )}
        </main>
      </div>

      <ArchitectureStudioModal
        isOpen={isStudioOpen}
        onClose={() => setIsStudioOpen(false)}
        {...hardwareInfo}
      />

      <footer
        className="py-5 text-xs mt-8"
        style={{
          borderTop: '1px solid var(--md-sys-color-outline-variant)',
          background: 'var(--md-sys-color-surface-container-low)',
          color: 'var(--md-sys-color-on-surface-variant)',
        }}
      >
        <div className="max-w-[1440px] mx-auto px-4 sm:px-6 flex flex-col sm:flex-row items-center justify-between gap-2">
          <div>
            <span
              className="font-semibold"
              style={{ color: 'var(--md-sys-color-on-surface)' }}
            >
              CFAE System Framework v4.0.0
            </span>
            {' · '}Material Design 3 Autonomous Mutation Safety Console
          </div>
          <div className="text-[11px] font-mono">
            ISO C++23 · Rust 2024 · Kotlin 2.1 · CFJ4 Readback Verified Recovery
          </div>
        </div>
      </footer>
    </div>
  );
};

export default App;
