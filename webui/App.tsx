import React, { useCallback, useEffect, useRef, useState } from 'react';
import { Activity, Flame, Layers, Terminal, ClipboardCheck, HardDrive, Thermometer, Cpu, Gauge, LockKeyhole } from 'lucide-react';
import { applyAppearance, readAppearance, saveAppearance, Theme } from './bridge/appearance';
import { execNativeKsuShell, hasNativeKernelSuBridge } from './bridge/daemonBridge';
import { AutonomousStateHero } from './components/AutonomousStateHero';
import { BaselineGuardianCard } from './components/BaselineGuardianCard';
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
  BlockQueueTelemetry,
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
  tunerEvaluatedCandidates?: TunerEvaluatedCandidateTelemetry[];
  vulkan?: VulkanTelemetry;
  semanticEvidence?: SemanticEvidence;
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

const checkedNativeStatus = (raw: string): NativeStatus => {
  const value: unknown = extractJsonObject(raw);
  if (typeof value !== 'object' || value === null) throw new Error('Invalid native status');
  const status = value as Partial<NativeStatus>;
  if (
    typeof status.running !== 'boolean' ||
    typeof status.pid !== 'number' ||
    typeof status.uclampMin !== 'number' ||
    typeof status.uclampMax !== 'number' ||
    typeof status.uclampSupported !== 'boolean' ||
    (status.uclampPolicyMin !== undefined && typeof status.uclampPolicyMin !== 'number') ||
    (status.uclampPolicyMax !== undefined && typeof status.uclampPolicyMax !== 'number') ||
    (status.uclampTargetPath !== undefined && typeof status.uclampTargetPath !== 'string') ||
    (status.uclampEncoding !== undefined && typeof status.uclampEncoding !== 'string') ||
    (typeof status.socTempCelsius !== 'number' && status.socTempCelsius !== null) ||
    typeof status.recoveryPending !== 'boolean'
  ) {
    throw new Error('Unexpected daemon status schema');
  }
  if (status.semanticEvidence !== undefined && !isSemanticEvidence(status.semanticEvidence)) {
    throw new Error('Unexpected semanticEvidence schema; expected native schemaVersion 1');
  }
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
  const [thermalTripCelsius, setThermalTripCelsius] = useState(DEFAULT_AUTONOMOUS_CONFIG.thermalTripCelsius);
  type NavTab = 'dashboard' | 'vulkan' | 'thermal' | 'hardware' | 'console' | 'validation';
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

  const isDark =
    appearance.theme === 'dark' ||
    (appearance.theme === 'system' &&
      typeof window !== 'undefined' &&
      window.matchMedia?.('(prefers-color-scheme: dark)').matches);

  useEffect(() => {
    applyAppearance(appearance);
  }, [appearance]);

  const toggleTheme = () => {
    const theme: Theme = isDark ? 'light' : 'dark';
    const next = { ...appearance, theme };
    setAppearance(next);
    saveAppearance(next);
  };

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
      setNativeStatusAvailable(status.semanticEvidence?.status === 'available');
    const effectiveTemp = status.socCompositeCelsius ?? status.socTempCelsius ?? -1;
    const isThermalThrottling =
      status.thermalGuardActive ??
      (status.thermalFallbackActive
        ? effectiveTemp >= thermalTripCelsius
        : (status.thermalSeverity ?? 0) >= 3);
    setTelemetry((old) => ({
      ...old,
      engineActive: status.running,
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
      cpuPressureValid: status.cpuPressureValid ?? (status.cpuPressurePct != null),
      memoryPressureValid: status.memoryPressureValid ?? (status.memoryPressurePct != null),
      ioPressureValid: status.ioPressureValid ?? (status.ioPressurePct != null),
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
      tunerIntent: status.tunerIntent ?? old.tunerIntent,
      tunerCandidateId: status.tunerCandidateId ?? old.tunerCandidateId,
      tunerCandidateLabel: status.tunerCandidateLabel ?? old.tunerCandidateLabel,
      tunerRationale: status.tunerRationale ?? old.tunerRationale,
      tunerVerdict: status.tunerVerdict ?? old.tunerVerdict,
      tunerUtilityDelta: status.tunerUtilityDelta ?? old.tunerUtilityDelta,
      tunerTrials: status.tunerTrials ?? old.tunerTrials,
      tunerImproved: status.tunerImproved ?? old.tunerImproved,
      tunerRolledBack: status.tunerRolledBack ?? old.tunerRolledBack,
      blockQueues: status.blockQueues ?? old.blockQueues,
      tunerCandidates: status.tunerCandidates ?? old.tunerCandidates,
      tunerEvaluatedCandidates: status.tunerEvaluatedCandidates ?? old.tunerEvaluatedCandidates,
      vulkan: status.vulkan ?? old.vulkan,
      baselineStatus: status.recoveryPending ? 'protected' : 'restored',
      transactionStage: status.running
        ? status.recoveryPending
          ? 'RETAINED'
          : 'VERIFIED'
        : 'PLANNED',
      modeLabel: status.running
        ? isThermalThrottling
          ? 'ThermalGuard Active · Pacing Demand'
          : status.uclampSupported
          ? 'Coordinated Stability Policy Active'
          : 'Daemon Active · Multi-Sensor Equilibrium'
        : 'Daemon Stopped',
      modeDescription: status.running
        ? isThermalThrottling
          ? status.uclampSupported
            ? 'Thermal guard active: applying supported clock and UClamp mitigation paths'
            : 'Thermal guard active: UClamp is unavailable; mitigation is limited to other validated actuator paths'
          : status.recoveryPending
            ? `Thermal source: ${status.thermalSource ?? 'oem_policy'}; write-ahead CFJ4 journal armed`
            : status.uclampSupported
              ? 'Coordinated mutation transactions with readback-verified UClamp and OEM thermal guards'
              : 'Daemon active; UClamp capability unavailable. Other controls remain capability-gated and independently verified.'
        : 'OEM baseline restored',
      timestamp: Date.now(),
    }));
      setNativeError(null);
      return status;
    } catch (error) {
      setNativeStatusAvailable(false);
      setSemanticEvidence(null);
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
      setVulkanConfig((old) => ({ ...old, mode: vkMode }));
    };

    let pollInFlight = false;
    let previousLogSnapshot = '';
    const pollNativeTelemetry = async () => {
      if (pollInFlight || disposed) return;
      pollInFlight = true;
      try {
        // Keep native bridge work away from the interaction hot path.
        await refreshNative();

        const logResult = await execNativeKsuShell(`/system/bin/tail -n 12 ${LOG} 2>/dev/null`);
        if (!disposed && logResult.errno === 0) {
          const snapshot = logResult.stdout.trim();
          if (snapshot && snapshot !== previousLogSnapshot) {
            previousLogSnapshot = snapshot;
            const lines = snapshot
              .split('\\n')
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

  const advancePreviewSimulation = useCallback(
    (forced?: 'cpu' | 'memory' | 'io_trial') => {
      const { next, event } = stepAutonomousSimulation(telemetryRef.current, DEFAULT_AUTONOMOUS_CONFIG, forced);
      telemetryRef.current = next;
      setTelemetry(next);
      if (event) {
        setLogs((previous) => [event, ...previous].slice(0, 50));
      }
    },
    []
  );

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
    const mergedText = mergeConfigText(existingResult.stdout ?? '', { vulkan_layer_mode: mode });
    const command =
      'umask 077 && [ ! -L "' +
      CONFIG +
      '" ] && rm -f "' +
      CONFIG +
      '.tmp" && ' +
      "cat <<'COREFLOW_CONFIG' > " +
      CONFIG +
      '.tmp\n' +
      mergedText +
      'COREFLOW_CONFIG\nchown 0:0 ' +
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

      // Commit the UI state only after the narrowly-scoped config write is acknowledged.
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

  const navItems: { id: NavTab; label: string; Icon: React.FC<{ className?: string }> }[] = [
    { id: 'dashboard', label: 'Overview', Icon: Activity },
    { id: 'thermal', label: 'Thermal', Icon: Flame },
    { id: 'hardware', label: 'Hardware', Icon: HardDrive },
    { id: 'vulkan', label: 'Vulkan', Icon: Layers },
    { id: 'console', label: 'Audit & Diagnostics', Icon: Terminal },
    { id: 'validation', label: 'Validation', Icon: ClipboardCheck },
  ];

  return (
    <div className="min-h-screen relative overflow-x-hidden bg-[#f7f9fc] dark:bg-[#0e1117] text-slate-900 dark:text-slate-100 flex flex-col font-sans transition-colors duration-200 antialiased">
      {/* Ambient Material 3 Soft Diffused Background Blurs */}
      <div
        aria-hidden="true"
        className="pointer-events-none fixed top-12 left-1/4 w-[28rem] h-[28rem] rounded-full bg-[#d3e3fd]/45 dark:bg-[#0842a0]/15 blur-3xl -z-10"
      />
      <div
        aria-hidden="true"
        className="pointer-events-none fixed bottom-12 right-1/4 w-[26rem] h-[26rem] rounded-full bg-slate-200/55 dark:bg-slate-800/20 blur-3xl -z-10"
      />

      {/* Bridge and telemetry trust state: Connected requires an official daemon schema v1 response. */}
      <div
        role="status"
        className={
          'px-4 py-2 text-center text-xs backdrop-blur-md border-b border-slate-900/[0.04] dark:border-white/[0.04] ' +
          (!isNativeBridge
            ? 'bg-amber-100 text-amber-950 dark:bg-amber-950/50 dark:text-amber-200'
            : nativeConnected
              ? 'bg-emerald-100 text-emerald-900 dark:bg-emerald-950/50 dark:text-emerald-200'
              : 'bg-amber-100 text-amber-950 dark:bg-amber-950/50 dark:text-amber-200')
        }
      >
        {!isNativeBridge
          ? 'Simulation Mode · Browser-only preview; synthetic values are labelled (sim) and are not kernel audit evidence.'
          : nativeConnected
            ? 'Connected · Native bridge and official daemon semanticEvidence schema v1 are available.'
            : 'Native bridge detected · Awaiting an official daemon status payload; semantic telemetry is unavailable.'}
      </div>

      {nativeError && (
        <div
          role="alert"
          className="px-4 py-2 bg-rose-100 text-rose-900 dark:bg-rose-950/80 dark:text-rose-200 text-xs font-medium text-center"
        >
          {nativeError}
        </div>
      )}

      <PixelHeader
        isDark={isDark}
        onToggleTheme={toggleTheme}
        modeLabel={telemetry.modeLabel}
        nativeMode={isNativeBridge}
        nativeConnected={nativeConnected}
        onOpenArchitectureStudio={() => setIsStudioOpen(true)}
      />

      <main className="pixel-main flex-1 w-full mx-auto px-4 sm:px-6 py-6 sm:py-8 space-y-6">
        {/* Material Design 3 Navigation Pill Bar (Unified Monochromatic Tonal Indicator) */}
        <nav
          aria-label="Primary sections"
          className="pixel-nav flex items-center gap-1.5 overflow-x-auto no-scrollbar max-w-full"
        >
          {navItems.map(({ id, label, Icon }) => {
            const active = activeTab === id;
            return (
              <button
                key={id}
                onClick={() => setActiveTab(id)}
                className={
                  'pixel-nav-item flex items-center gap-2 whitespace-nowrap select-none ' +
                  (active ? 'is-active' : '')
                }
              >
                <Icon className="w-4 h-4 shrink-0" />
                <span>{label}</span>
              </button>
            );
          })}
        </nav>

        {activeTab === 'dashboard' && (
          <div className="pixel-page space-y-6 m3-stage">
            <section className="space-y-3" aria-label="Autonomous runtime state">
              <div className="flex flex-col gap-2 sm:flex-row sm:items-end sm:justify-between">
                <div>
                  <p className="text-[11px] font-semibold uppercase tracking-[0.18em] text-[#0b57d0] dark:text-[#a8c7fa]">CFAE · Autonomous system optimization</p>
                  <h1 className="mt-1 text-2xl font-semibold tracking-tight text-slate-900 dark:text-slate-100 sm:text-3xl">Live operating state</h1>
                  <p className="mt-1 max-w-2xl text-sm leading-relaxed text-slate-600 dark:text-slate-400">Capability discovery, evidence-led tuning, and verified actuator ownership. The dashboard reflects native daemon state; selection remains autonomous and safety-gated.</p>
                </div>
                <div className="inline-flex w-fit items-center gap-2 rounded-full border border-slate-900/[0.07] bg-white/70 px-3 py-2 text-[11px] font-semibold text-slate-600 dark:border-white/[0.08] dark:bg-white/[0.04] dark:text-slate-300">
                  <span className={'h-2 w-2 rounded-full ' + (telemetry.engineActive ? 'bg-emerald-500' : 'bg-slate-400')} />
                  {telemetry.engineActive ? 'Daemon operating autonomously' : 'Daemon standby'}
                </div>
              </div>
              <AutonomousStateHero telemetry={telemetry} nativeMode={isNativeBridge} thermalTripCelsius={thermalTripCelsius} />
            </section>

            <div className="grid grid-cols-1 gap-3 sm:grid-cols-3">
              <div className="pixel-metric m3-card flex items-start gap-3 p-4"><div className="rounded-xl bg-[#d3e3fd] p-2.5 text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]"><Thermometer className="h-4 w-4"/></div><div className="min-w-0"><div className="text-[11px] text-slate-500 dark:text-slate-400">Thermal headroom</div><div className="mt-1 text-lg font-semibold tabular-nums text-slate-900 dark:text-slate-100">{telemetry.thermalHeadroomValid && telemetry.thermalHeadroom != null ? `${Math.round(telemetry.thermalHeadroom * 100)}%` : 'Unavailable'}</div><div className="text-[11px] text-slate-500 dark:text-slate-400">{telemetry.thermalGuardActive ? 'Thermal guard engaged' : 'OEM thermal policy respected'}</div></div></div>
              <div className="pixel-metric m3-card flex items-start gap-3 p-4"><div className="rounded-xl bg-[#d3e3fd] p-2.5 text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]"><Gauge className="h-4 w-4"/></div><div className="min-w-0"><div className="text-[11px] text-slate-500 dark:text-slate-400">Resource pressure</div><div className="mt-1 text-lg font-semibold tabular-nums text-slate-900 dark:text-slate-100">{telemetry.cpuPressureValid || telemetry.memoryPressureValid || telemetry.ioPressureValid ? `${Math.max(telemetry.cpuPressureValid ? telemetry.cpuPressurePct : 0, telemetry.memoryPressureValid ? telemetry.memoryPressurePct : 0, telemetry.ioPressureValid ? telemetry.ioPressurePct : 0).toFixed(1)}%` : 'Unavailable'}</div><div className="text-[11px] text-slate-500 dark:text-slate-400">Highest available PSI avg10</div></div></div>
              <div className="pixel-metric m3-card flex items-start gap-3 p-4"><div className="rounded-xl bg-[#d3e3fd] p-2.5 text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]"><LockKeyhole className="h-4 w-4"/></div><div className="min-w-0"><div className="text-[11px] text-slate-500 dark:text-slate-400">Mutation ownership</div><div className="mt-1 text-lg font-semibold text-slate-900 dark:text-slate-100">{telemetry.baselineStatus === 'protected' ? 'Journal armed' : 'Guarded'}</div><div className="text-[11px] text-slate-500 dark:text-slate-400">Baseline + readback protection</div></div></div>
            </div>

            <SemanticEvidencePanel evidence={semanticEvidence} nativeConnected={nativeConnected} simulationMode={!isNativeBridge} />
            <BaselineGuardianCard nodesSnapshotted={telemetry.baselineNodesSnapshotted} nativeMode={isNativeBridge} baselineStatus={telemetry.baselineStatus} />
            <LiveReactionStream logs={logs} nativeMode={isNativeBridge} />
          </div>
        )}

        {activeTab === 'thermal' && (
          <div className="pixel-page space-y-6 m3-stage">
            <header className="pixel-page-heading"><p className="pixel-page-eyebrow">Protection domain</p><h1 className="pixel-page-title">Thermal intelligence</h1><p className="pixel-page-description">Sensor evidence and OEM thermal constraints are displayed for transparency. The daemon selects mitigation automatically.</p></header>
            <ThermalMatrixCard telemetry={telemetry} nativeMode={isNativeBridge} thermalTripCelsius={thermalTripCelsius} />
            <LiveReactionStream logs={logs.filter((event) => event.type === 'THERMAL' || event.tag.toLowerCase().includes('thermal'))} nativeMode={isNativeBridge} />
          </div>
        )}

        {activeTab === 'hardware' && (
          <div className="pixel-page space-y-6 m3-stage">
            <header className="pixel-page-heading"><p className="pixel-page-eyebrow">Capability discovery</p><h1 className="pixel-page-title">Hardware & resource profile</h1><p className="pixel-page-description">Detected hardware is evidence, not automatic mutation authority. CoreFlow adapts only through validated operational contracts.</p></header>
            <section className="m3-card space-y-4 p-5 sm:p-7">
              <div className="flex items-center gap-2"><Cpu className="h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"/><h2 className="font-semibold">Device identity</h2></div>
              <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
                {[['SoC', hardwareInfo.socModel], ['GPU', hardwareInfo.gpuType], ['Kernel', hardwareInfo.kernelVersion], ['Online CPUs', hardwareInfo.onlineCpus]].map(([label, value]) => <div key={label} className="m3-tonal-block p-4"><div className="text-[11px] text-slate-500 dark:text-slate-400">{label}</div><div className="mt-1 break-words font-mono text-sm font-semibold text-slate-900 dark:text-slate-100">{value}</div></div>)}
              </div>
            </section>
            <section className="m3-card space-y-4 p-5 sm:p-7">
              <div>
                <p className="text-[11px] font-semibold uppercase tracking-[0.16em] text-[#0b57d0] dark:text-[#a8c7fa]">Kernel contract model</p>
                <h2 className="mt-1 text-lg font-semibold">Discovery does not grant mutation authority</h2>
                <p className="mt-1 text-xs leading-relaxed text-slate-600 dark:text-slate-400">Every discovered attribute receives an explicit contract classification. Unknown nodes stay visible for hardware understanding but fail closed for automated writes.</p>
              </div>
              <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
                {[
                  { name: 'RUNTIME_TUNABLE', tone: 'bg-emerald-100 text-emerald-800 dark:bg-emerald-950/50 dark:text-emerald-300', detail: 'Only class eligible for tuning after ABI, range, access and baseline verification.' },
                  { name: 'OBSERVATION_ONLY', tone: 'bg-sky-100 text-sky-800 dark:bg-sky-950/50 dark:text-sky-300', detail: 'Hardware limits, topology, alignment and telemetry contribute to the fingerprint.' },
                  { name: 'TEST_ONLY', tone: 'bg-amber-100 text-amber-800 dark:bg-amber-950/50 dark:text-amber-300', detail: 'Writable testing interface, excluded from daily performance optimization.' },
                  { name: 'PROTECTED', tone: 'bg-rose-100 text-rose-800 dark:bg-rose-950/50 dark:text-rose-300', detail: 'Integrity-critical or system-sensitive attribute; automatic mutation prohibited.' },
                ].map((item) => <div key={item.name} className="rounded-2xl border border-slate-900/[0.06] p-4 dark:border-white/[0.07]"><span className={'inline-flex rounded-full px-2.5 py-1 font-mono text-[10px] font-bold ' + item.tone}>{item.name}</span><p className="mt-2 text-xs leading-relaxed text-slate-600 dark:text-slate-400">{item.detail}</p></div>)}
              </div>
            </section>
            <section className="m3-card space-y-4 p-5 sm:p-7">
              <div className="flex items-center gap-2"><HardDrive className="h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"/><h2 className="font-semibold">Autonomous hardware preservation</h2></div>
              <p className="text-sm leading-relaxed text-slate-600 dark:text-slate-400">Battery, storage, and I/O protection are evaluated from live workload and device evidence. No manual thresholds or scheduler presets are exposed here; unsupported nodes remain observation-only until they earn mutation authority.</p>
              <div className="grid grid-cols-1 gap-3 sm:grid-cols-3">
                <div className="m3-tonal-block p-4"><div className="text-[11px] text-slate-500 dark:text-slate-400">Battery temperature</div><div className="mt-1 font-mono text-lg font-semibold">{telemetry.batteryTempCelsius != null && telemetry.batteryTempCelsius >= 0 ? `${telemetry.batteryTempCelsius.toFixed(1)} °C` : 'N/A'}</div></div>
                <div className="m3-tonal-block p-4"><div className="text-[11px] text-slate-500 dark:text-slate-400">Block queues discovered</div><div className="mt-1 font-mono text-lg font-semibold">{telemetry.blockQueues?.length ?? 0}</div></div>
                <div className="m3-tonal-block p-4"><div className="text-[11px] text-slate-500 dark:text-slate-400">Baseline snapshots</div><div className="mt-1 font-mono text-lg font-semibold">{telemetry.baselineNodesSnapshotted}</div></div>
              </div>
            </section>
            <BaselineGuardianCard nodesSnapshotted={telemetry.baselineNodesSnapshotted} nativeMode={isNativeBridge} baselineStatus={telemetry.baselineStatus} />
          </div>
        )}

        {activeTab === 'vulkan' && (
          <div className="pixel-page space-y-6 m3-stage">
            <header className="pixel-page-heading"><p className="pixel-page-eyebrow">User opt-in subsystem</p><h1 className="pixel-page-title">Vulkan diagnostics & layer</h1><p className="pixel-page-description">Vulkan layer activation is the only manual feature switch. All system optimization domains remain autonomous.</p></header>
            <VulkanCapabilitiesCard vulkanTelemetry={telemetry.vulkan} vulkanConfig={vulkanConfig} nativeMode={isNativeBridge} onSaveVulkanConfig={handleSaveVulkanConfig} isSaving={isSaving} saveMessage={vulkanSaveMessage} />
            <LiveReactionStream logs={logs.filter((event) => event.tag.toLowerCase().includes('vulkan'))} nativeMode={isNativeBridge} />
          </div>
        )}

        {activeTab === 'console' && (
          <div className="pixel-page space-y-6 m3-stage">
            <header className="pixel-page-heading"><p className="pixel-page-eyebrow">Runtime transparency</p><h1 className="pixel-page-title">Audit & diagnostics</h1><p className="pixel-page-description">Read-only runtime evidence, decisions, and diagnostic output.</p></header>
            <DeveloperConsoleCard nativeMode={isNativeBridge} onRefreshTelemetry={async () => { await refreshNative(); }} />
            <LiveReactionStream logs={logs} nativeMode={isNativeBridge} />
          </div>
        )}

        {activeTab === 'validation' && (
          <div className="pixel-page space-y-6 m3-stage">
            <header className="pixel-page-heading"><p className="pixel-page-eyebrow">Evidence, not assumptions</p><h1 className="pixel-page-title">Device validation</h1><p className="pixel-page-description">Inspect the live validation contract and native subsystem readiness.</p></header>
            <DeviceValidationSuiteCard nativeMode={isNativeBridge} />
          </div>
        )}
      </main>

      <ArchitectureStudioModal
        isOpen={isStudioOpen}
        onClose={() => setIsStudioOpen(false)}
        {...hardwareInfo}
      />

      <footer className="pixel-footer text-xs text-slate-500 dark:text-slate-400">
        <div className="max-w-6xl mx-auto px-4 sm:px-6 flex flex-col sm:flex-row items-center justify-between gap-2">
          <div>
            <span className="font-semibold text-slate-700 dark:text-slate-300">
              CFAE System Framework v4.0.0
            </span>
            {' · '}Autonomous Mutation Safety & Coordinated Optimization
          </div>
          <div className="text-[11px] font-mono">
            ISO C++23 · Rust 2024 · Kotlin 2.1 · Readback Verified Recovery
          </div>
        </div>
      </footer>
    </div>
  );
};

export default App;
