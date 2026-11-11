import React, { useCallback, useEffect, useRef, useState } from 'react';
import { Activity, Flame, Layers, ShieldCheck, Smartphone, Terminal } from 'lucide-react';
import { applyAppearance, readAppearance, saveAppearance, Theme } from './bridge/appearance';
import { execNativeKsuShell, hasNativeKernelSuBridge } from './bridge/daemonBridge';
import { AutonomousStateHero } from './components/AutonomousStateHero';
import { AutonomousTuningCard } from './components/AutonomousTuningCard';
import { BaselineGuardianCard } from './components/BaselineGuardianCard';
import { LiveReactionStream } from './components/LiveReactionStream';
import { PixelHeader } from './components/PixelHeader';
import { ArchitectureStudioModal } from './components/ArchitectureStudioModal';
import { ThermalMatrixCard } from './components/ThermalMatrixCard';
import { HardwareLongevityCard } from './components/HardwareLongevityCard';
import { AndroidQuickSettingsCard } from './components/AndroidQuickSettingsCard';
import { DeveloperConsoleCard } from './components/DeveloperConsoleCard';
import { DeviceValidationSuiteCard } from './components/DeviceValidationSuiteCard';
import { VulkanCapabilitiesCard } from './components/VulkanCapabilitiesCard';
import { SemanticEvidencePanel } from './components/SemanticEvidencePanel';
import { isSemanticEvidence, SemanticEvidence } from './semanticEvidence';
import {
  AutonomousConfig,
  AutonomousTelemetry,
  BlockQueueTelemetry,
  computeVulkanTelemetry,
  createInitialAutonomousTelemetry,
  createInitialLogs,
  DEFAULT_AUTONOMOUS_CONFIG,
  DEFAULT_LONGEVITY_CONFIG,
  DEFAULT_VULKAN_CONFIG,
  HardwareLongevityConfig,
  mergeConfigText,
  ReactionLogEvent,
  readSectionNumberKey,
  readSectionStringKey,
  stepAutonomousSimulation,
  ThermalSensorDetail,
  TunerCandidateTelemetry,
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
  '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"; ' +
  'chmod 755 "$BIN" 2>/dev/null';
const CONFIG = '/data/adb/coreflow/coreflow.conf';
const LOG = '/data/adb/coreflow/logs/coreflowd.log';

interface NativeStatus {
  running: boolean;
  pid: number;
  uclampMin: number;
  uclampMax: number;
  uclampSupported: boolean;
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

const delay = (ms: number) => new Promise<void>((resolve) => window.setTimeout(resolve, ms));

export const App: React.FC = () => {
  const [isNativeBridge] = useState(() => hasNativeKernelSuBridge());
  const [appearance, setAppearance] = useState(() => readAppearance());
  const [nativeError, setNativeError] = useState<string | null>(null);
  const [semanticEvidence, setSemanticEvidence] = useState<SemanticEvidence | null>(null);
  const [nativeStatusAvailable, setNativeStatusAvailable] = useState(false);
  const nativeConnected = isNativeBridge && nativeStatusAvailable;
  const [isSwitching, setIsSwitching] = useState(false);
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
  const [config, setConfig] = useState<AutonomousConfig>(() => ({ ...DEFAULT_AUTONOMOUS_CONFIG }));
  type NavTab = 'dashboard' | 'vulkan' | 'thermal' | 'longevity' | 'android' | 'console' | 'validation';
  const [activeTab, setActiveTab] = useState<NavTab>('dashboard');
  const [longevityConfig, setLongevityConfig] = useState<HardwareLongevityConfig>(() => ({
    ...DEFAULT_LONGEVITY_CONFIG,
  }));
  const [longevitySaveMessage, setLongevitySaveMessage] = useState<string | null>(null);
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
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const [isRestoring, setIsRestoring] = useState(false);
  const [restoreFeedback, setRestoreFeedback] = useState<string | null>(null);

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
        ? effectiveTemp >= config.thermalTripCelsius
        : (status.thermalSeverity ?? 0) >= 3);
    setTelemetry((old) => ({
      ...old,
      engineActive: status.running,
      uclampMin: status.uclampMin,
      uclampMax: status.uclampMax,
      uclampSupported: status.uclampSupported,
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
  }, [config.thermalTripCelsius]);

  useEffect(() => {
    if (!isNativeBridge) return;
    let disposed = false;

    const loadInitialConfig = async () => {
      const configResult = await execNativeKsuShell(`cat ${CONFIG} 2>/dev/null`);
      if (disposed || configResult.errno !== 0) return;

      const rawBias = readSectionStringKey(configResult.stdout, 'ui_adaptive_bias');
      const adaptiveBias: AutonomousConfig['adaptiveBias'] =
        rawBias === 'battery' || rawBias === 'balanced' || rawBias === 'responsiveness'
          ? rawBias
          : 'balanced';
      const uiFloor = readSectionNumberKey(configResult.stdout, 'ui_balanced_uclamp_min');
      const daemonFloor = readSectionNumberKey(configResult.stdout, 'top_app_interactive_min');
      const baseFloor = uiFloor ?? daemonFloor;
      const trip = readSectionNumberKey(configResult.stdout, 'thermal_trip_celsius');

      const rawStep = readSectionStringKey(configResult.stdout, 'enable_battery_step_charging');
      const stepTemp = readSectionNumberKey(configResult.stdout, 'battery_step_temp_threshold');
      const stepCurrent = readSectionNumberKey(configResult.stdout, 'stepped_charge_current_ma');
      const rawUfs = readSectionStringKey(configResult.stdout, 'enable_ufs_burst_pacing');
      const topWeight = readSectionNumberKey(configResult.stdout, 'top_app_io_weight');
      const bgWeight = readSectionNumberKey(configResult.stdout, 'background_io_weight');

      setConfig((old) => ({
        ...old,
        adaptiveBias,
        balancedUclampMin:
          baseFloor === null ? old.balancedUclampMin : Math.max(0, Math.min(640, Math.round(baseFloor))),
        thermalTripCelsius:
          trip === null ? old.thermalTripCelsius : Math.max(30, Math.min(80, trip)),
      }));

      setLongevityConfig((old) => ({
        ...old,
        enableBatteryStepCharging: rawStep
          ? rawStep === 'true' || rawStep === '1'
          : old.enableBatteryStepCharging,
        batteryStepTempThreshold: stepTemp ?? old.batteryStepTempThreshold,
        steppedChargeCurrentMa: stepCurrent ?? old.steppedChargeCurrentMa,
        enableUfsBurstPacing: rawUfs
          ? rawUfs === 'true' || rawUfs === '1'
          : old.enableUfsBurstPacing,
        topAppIoWeight: topWeight ?? old.topAppIoWeight,
        backgroundIoWeight: bgWeight ?? old.backgroundIoWeight,
      }));

      const rawVkMode = readSectionStringKey(configResult.stdout, 'vulkan_layer_mode');
      const vkMode: VulkanLayerMode =
        rawVkMode === 'safe_hardware' || rawVkMode === 'full_capability_override'
          ? 'safe_hardware'
          : rawVkMode === 'disabled'
          ? 'disabled'
          : 'disabled';
      const rawSkiavk = readSectionStringKey(configResult.stdout, 'enable_skiavk_threaded');
      const rawVrs = readSectionStringKey(configResult.stdout, 'enable_adaptive_vrs');
      const rawPrewarm = readSectionStringKey(configResult.stdout, 'enable_pipeline_cache_prewarm');

      setVulkanConfig((old) => ({
        ...old,
        mode: vkMode,
        enableSkiavkThreaded: rawSkiavk
          ? rawSkiavk === 'true' || rawSkiavk === '1'
          : old.enableSkiavkThreaded,
        enableAdaptiveVrs: rawVrs ? rawVrs === 'true' || rawVrs === '1' : old.enableAdaptiveVrs,
        enablePipelineCachePrewarm: rawPrewarm
          ? rawPrewarm === 'true' || rawPrewarm === '1'
          : old.enablePipelineCachePrewarm,
      }));
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
      const { next, event } = stepAutonomousSimulation(telemetryRef.current, config, forced);
      telemetryRef.current = next;
      setTelemetry(next);
      if (event) {
        setLogs((previous) => [event, ...previous].slice(0, 50));
      }
    },
    [config]
  );

  useEffect(() => {
    if (isNativeBridge) return;
    const timer = window.setInterval(() => {
      advancePreviewSimulation();
    }, 2500);
    return () => window.clearInterval(timer);
  }, [isNativeBridge, advancePreviewSimulation]);

  const handleToggleEngine = async () => {
    if (!isNativeBridge) {
      setTelemetry((old) => ({ ...old, engineActive: !old.engineActive }));
      return;
    }
    if (isSwitching) return;
    setIsSwitching(true);
    setNativeError(null);
    try {
      const before = await refreshNative();
      if (before.running) {
        await execChecked(`${RESOLVE_BIN}; "$BIN" --stop`);
      } else {
        await execChecked(
          `rm -f /data/adb/coreflow/SAFE_MODE /data/adb/coreflow/DISABLE /data/adb/modules/cfae_framework/disable /data/adb/modules/coreflow_engine/disable 2>/dev/null; ` +
            `printf '0\\n' > /data/adb/coreflow/boot_attempts 2>/dev/null; ` +
            `mkdir -p /data/adb/coreflow/logs 2>/dev/null; ` +
            `${RESOLVE_BIN}; ` +
            `"$BIN" --daemon >> "${LOG}" 2>&1 </dev/null || (setsid "$BIN" >> "${LOG}" 2>&1 </dev/null &) 2>/dev/null || (nohup "$BIN" >> "${LOG}" 2>&1 </dev/null &) 2>/dev/null`
        );
      }

      let after = before;
      for (let attempt = 0; attempt < 10; attempt++) {
        await delay(350);
        after = await refreshNative();
        if (after.running !== before.running) break;
      }

      if (after.running === before.running) {
        const errorLog = await execNativeKsuShell(
          `tail -n 6 ${LOG} 2>/dev/null || cat ${LOG} 2>/dev/null`
        );
        const logSnippet = errorLog.stdout?.trim();
        throw new Error(`Daemon state did not change${logSnippet ? ': ' + logSnippet : ''}`);
      }

      addLog(
        'INFO',
        'NativeControl',
        after.running
          ? 'Daemon started with coordinated mutation policy'
          : 'Daemon stopped and baseline recovery verified'
      );
    } catch (error) {
      setNativeError(String(error));
      addLog('INFO', 'NativeControl', 'Failed: ' + String(error));
    } finally {
      setIsSwitching(false);
    }
  };

  const handleSimulateCpuLoad = () => {
    if (isNativeBridge || !telemetry.engineActive) return;
    advancePreviewSimulation('cpu');
  };

  const handleSimulateMemoryPressure = () => {
    if (isNativeBridge || !telemetry.engineActive) return;
    advancePreviewSimulation('memory');
  };

  const handleSimulateIoTrial = () => {
    if (isNativeBridge || !telemetry.engineActive) return;
    advancePreviewSimulation('io_trial');
  };

  const writeSurgicalConfigUpdates = async (
    updates: Record<string, string | number | boolean>,
    ackToken: string
  ) => {
    const existingResult = await execNativeKsuShell(
      `cat "${CONFIG}" 2>/dev/null || cat /data/adb/modules/cfae_framework/system/etc/coreflow/default.conf 2>/dev/null || cat /system/etc/coreflow/default.conf 2>/dev/null || true`
    );
    const mergedText = mergeConfigText(existingResult.stdout ?? '', updates);
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
      ` && echo ${ackToken}`;
    const output = await execChecked(command);
    if (!output.includes(ackToken)) {
      throw new Error('Configuration write not acknowledged');
    }
  };

  const handleSaveConfig = async (overrideConfig?: AutonomousConfig) => {
    const activeConfig = overrideConfig ?? config;
    setIsSaving(true);
    setSaveMessage(null);
    try {
      if (!isNativeBridge) {
        setSaveMessage('Preview transaction policy applied');
        return;
      }

      const safeBias: AutonomousConfig['adaptiveBias'] =
        activeConfig.adaptiveBias === 'battery' || activeConfig.adaptiveBias === 'responsiveness'
          ? activeConfig.adaptiveBias
          : 'balanced';
      const baseFloor = Math.max(0, Math.min(640, Math.round(activeConfig.balancedUclampMin)));
      const multiplier =
        safeBias === 'battery' ? 0.75 : safeBias === 'responsiveness' ? 1.25 : 1;
      const balancedMin = Math.max(0, Math.min(1024, Math.round(baseFloor * multiplier)));
      const trip = Math.max(30, Math.min(80, activeConfig.thermalTripCelsius));
      const recover = Math.max(25, Math.min(trip - 1, trip - 4));

      await writeSurgicalConfigUpdates(
        {
          top_app_interactive_min: balancedMin,
          ui_adaptive_bias: safeBias,
          ui_balanced_uclamp_min: baseFloor,
          thermal_trip_celsius: trip.toFixed(1),
          thermal_recover_celsius: recover.toFixed(1),
        },
        'COREFLOW_CONFIG_WRITTEN'
      );

      const before = await refreshNative();
      if (before.running && before.pid > 0) {
        await execChecked(`kill -HUP ${Math.trunc(before.pid)}`);
        await delay(250);
        const after = await refreshNative();
        if (!after.running) throw new Error('Config saved but daemon reload failed');
      }

      setSaveMessage(before.running ? 'Committed and reloaded live' : 'Saved for next start');
      addLog('INFO', 'Config', `Balanced floor=${balancedMin}, fallback trip=${trip.toFixed(1)}°C`);
    } catch (error) {
      setSaveMessage('Save failed');
      setNativeError(String(error));
    } finally {
      setIsSaving(false);
    }
  };

  const handleSaveLongevity = async (newConfig: HardwareLongevityConfig) => {
    setIsSaving(true);
    setLongevitySaveMessage(null);
    try {
      if (!isNativeBridge) {
        setLongevityConfig(newConfig);
        setLongevitySaveMessage('Saved in Preview');
        addLog(
          'INFO',
          'Longevity',
          `Preview: Step charge=${newConfig.batteryStepTempThreshold}°C / ${newConfig.steppedChargeCurrentMa}mA`
        );
        return;
      }
      const stepTemp = Math.max(30, Math.min(45, newConfig.batteryStepTempThreshold));
      const stepCurr = Math.max(100, Math.min(5000, Math.round(newConfig.steppedChargeCurrentMa)));
      const topWeight = Math.max(1, Math.min(10000, Math.round(newConfig.topAppIoWeight)));
      const bgWeight = Math.max(1, Math.min(10000, Math.round(newConfig.backgroundIoWeight)));

      await writeSurgicalConfigUpdates(
        {
          enable_battery_step_charging: newConfig.enableBatteryStepCharging ? 'true' : 'false',
          enable_ufs_burst_pacing: newConfig.enableUfsBurstPacing ? 'true' : 'false',
          battery_step_temp_threshold: stepTemp.toFixed(1),
          stepped_charge_current_ma: stepCurr,
          top_app_io_weight: topWeight,
          background_io_weight: bgWeight,
        },
        'COREFLOW_LONGEVITY_WRITTEN'
      );
      await execChecked('killall -HUP coreflowd 2>/dev/null || true');
      setLongevityConfig(newConfig);
      setLongevitySaveMessage('Armed live in kernel');
      addLog(
        'INFO',
        'Longevity',
        `Hardware longevity armed: Step charge=${newConfig.batteryStepTempThreshold}°C / ${newConfig.steppedChargeCurrentMa}mA`
      );
    } catch (err) {
      setLongevitySaveMessage('Save failed');
      setNativeError(String(err));
    } finally {
      setIsSaving(false);
    }
  };

  const handleSelectBias = async (bias: AutonomousConfig['adaptiveBias']) => {
    const nextConfig = { ...config, adaptiveBias: bias };
    setConfig(nextConfig);
    await handleSaveConfig(nextConfig);
  };

  const handleSaveVulkanConfig = async (nextVulkan: VulkanEngineConfig) => {
    setIsSaving(true);
    setVulkanSaveMessage(null);
    try {
      const normalizedMode: VulkanLayerMode =
        nextVulkan.mode === 'disabled' ? 'disabled' : 'safe_hardware';
      const sanitizedVulkan: VulkanEngineConfig = { ...nextVulkan, mode: normalizedMode };
      setVulkanConfig(sanitizedVulkan);
      setTelemetry((old) => ({
        ...old,
        vulkan: computeVulkanTelemetry(sanitizedVulkan, old.vulkan),
      }));

      if (!isNativeBridge) {
        setVulkanSaveMessage(
          normalizedMode === 'safe_hardware'
            ? 'Truthful Hardware Pass-Through active in preview'
            : 'OEM Passthrough (Disabled) active in preview'
        );
        addLog(
          'BOOST',
          'VulkanLayer',
          `Mode=${normalizedMode} | SkiaVK=${sanitizedVulkan.enableSkiavkThreaded} | AdaptiveVRS=${sanitizedVulkan.enableAdaptiveVrs}`
        );
        return;
      }

      await writeSurgicalConfigUpdates(
        {
          vulkan_layer_mode: normalizedMode,
          enable_skiavk_threaded: sanitizedVulkan.enableSkiavkThreaded ? 'true' : 'false',
          enable_adaptive_vrs: sanitizedVulkan.enableAdaptiveVrs ? 'true' : 'false',
          enable_pipeline_cache_prewarm: sanitizedVulkan.enablePipelineCachePrewarm
            ? 'true'
            : 'false',
        },
        'COREFLOW_VULKAN_WRITTEN'
      );
      await execChecked('killall -HUP coreflowd 2>/dev/null || true');
      await refreshNative();
      setVulkanSaveMessage('Armed in libVkLayer_coreflow.so');
      addLog(
        'BOOST',
        'VulkanLayer',
        `Committed Vulkan mode=${normalizedMode} to /data/adb/coreflow/vulkan_layer.state`
      );
    } catch (err) {
      setVulkanSaveMessage('Save failed');
      setNativeError(String(err));
    } finally {
      setIsSaving(false);
    }
  };

  const handleRestoreBaseline = async () => {
    setIsRestoring(true);
    setRestoreFeedback(null);
    try {
      if (!isNativeBridge) {
        setTelemetry((old) => ({
          ...old,
          baselineStatus: 'restored',
          transactionStage: 'PLANNED',
        }));
        setRestoreFeedback('Preview: simulated ownership-verified baseline rollback completed.');
        return;
      }

      await execChecked(`${RESOLVE_BIN}; "$BIN" --rollback-baseline`);
      const status = await refreshNative();
      if (status.running || status.recoveryPending) throw new Error('Rollback not verified');

      setRestoreFeedback('Tracked OEM baseline values restored and daemon stopped.');
      addLog('BASELINE', 'NativeRecovery', 'Recovery journal cleared after verified rollback.');
    } catch (error) {
      setRestoreFeedback('Restoration failed: ' + String(error));
      setNativeError(String(error));
    } finally {
      setIsRestoring(false);
    }
  };

  const navItems: { id: NavTab; label: string; Icon: React.FC<{ className?: string }> }[] = [
    { id: 'dashboard', label: 'Equilibrium', Icon: Activity },
    { id: 'vulkan', label: 'Vulkan 1.4 & GPU', Icon: Layers },
    { id: 'thermal', label: 'Thermal Matrix', Icon: Flame },
    { id: 'longevity', label: 'Hardware Longevity', Icon: ShieldCheck },
    { id: 'android', label: 'Quick Settings', Icon: Smartphone },
    { id: 'console', label: 'RPC Console', Icon: Terminal },
    { id: 'validation', label: 'Device Validation', Icon: Activity },
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
        engineActive={telemetry.engineActive}
        onToggleEngine={handleToggleEngine}
        isDark={isDark}
        onToggleTheme={toggleTheme}
        modeLabel={telemetry.modeLabel}
        nativeMode={isNativeBridge}
        nativeConnected={nativeConnected}
        onOpenArchitectureStudio={() => setIsStudioOpen(true)}
      />

      <main className="flex-1 max-w-6xl w-full mx-auto px-4 sm:px-6 py-6 sm:py-8 space-y-6">
        {/* Material Design 3 Navigation Pill Bar (Unified Monochromatic Tonal Indicator) */}
        <nav
          aria-label="Primary sections"
          className="flex items-center gap-1.5 p-1.5 rounded-full bg-white/75 dark:bg-[#181c24]/75 backdrop-blur-2xl border border-slate-900/[0.06] dark:border-white/[0.07] overflow-x-auto no-scrollbar w-fit max-w-full"
        >
          {navItems.map(({ id, label, Icon }) => {
            const active = activeTab === id;
            return (
              <button
                key={id}
                onClick={() => setActiveTab(id)}
                className={
                  'px-4 py-2 rounded-full text-xs font-medium transition-all flex items-center gap-2 whitespace-nowrap select-none ' +
                  (active
                    ? 'bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold'
                    : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200 hover:bg-slate-900/[0.04] dark:hover:bg-white/[0.04]')
                }
              >
                <Icon className="w-3.5 h-3.5 shrink-0" />
                <span>{label}</span>
              </button>
            );
          })}
        </nav>

        {/* Tab 1: Equilibrium Dashboard */}
        {activeTab === 'dashboard' && (
          <div className="space-y-6 m3-stage">
            <section aria-label="Engine stability telemetry">
              <AutonomousStateHero
                telemetry={telemetry}
                nativeMode={isNativeBridge}
                thermalTripCelsius={config.thermalTripCelsius}
                onSimulateCpuLoad={handleSimulateCpuLoad}
                onSimulateMemoryPressure={handleSimulateMemoryPressure}
                onSimulateIoTrial={handleSimulateIoTrial}
              />
            </section>

            <SemanticEvidencePanel
              evidence={semanticEvidence}
              nativeConnected={nativeConnected}
              simulationMode={!isNativeBridge}
            />

            <section aria-label="Stability policy settings">
              <AutonomousTuningCard
                config={config}
                nativeMode={isNativeBridge}
                onChangeConfig={setConfig}
                onSaveConfig={handleSaveConfig}
                isSaving={isSaving}
                saveMessage={saveMessage}
              />
            </section>

            <section aria-label="Recovery journal">
              <BaselineGuardianCard
                nodesSnapshotted={telemetry.baselineNodesSnapshotted}
                nativeMode={isNativeBridge}
                baselineStatus={telemetry.baselineStatus}
                onRestoreBaseline={handleRestoreBaseline}
                isRestoring={isRestoring}
                restoreFeedback={restoreFeedback}
              />
            </section>

            <section aria-label="Engine activity">
              <LiveReactionStream
                logs={logs}
                onClearLogs={() => setLogs([])}
                engineActive={telemetry.engineActive}
                nativeMode={isNativeBridge}
              />
            </section>
          </div>
        )}

        {/* Tab 2: Universal Vulkan 1.4 & GPU Capabilities */}
        {activeTab === 'vulkan' && (
          <div className="space-y-6 m3-stage">
            <section aria-label="Vulkan capabilities and implicit layer bridge">
              <VulkanCapabilitiesCard
                vulkanTelemetry={telemetry.vulkan}
                vulkanConfig={vulkanConfig}
                nativeMode={isNativeBridge}
                onChangeVulkanConfig={(next) => {
                  setVulkanConfig(next);
                  setTelemetry((old) => ({
                    ...old,
                    vulkan: computeVulkanTelemetry(next, old.vulkan),
                  }));
                }}
                onSaveVulkanConfig={handleSaveVulkanConfig}
                isSaving={isSaving}
                saveMessage={vulkanSaveMessage}
              />
            </section>

            <section aria-label="Engine activity">
              <LiveReactionStream
                logs={logs}
                onClearLogs={() => setLogs([])}
                engineActive={telemetry.engineActive}
                nativeMode={isNativeBridge}
              />
            </section>
          </div>
        )}

        {/* Tab 3: Thermal Matrix */}
        {activeTab === 'thermal' && (
          <div className="space-y-6 m3-stage">
            <section aria-label="Multi-sensor thermal matrix">
              <ThermalMatrixCard
                telemetry={telemetry}
                nativeMode={isNativeBridge}
                thermalTripCelsius={config.thermalTripCelsius}
              />
            </section>

            <section aria-label="Engine activity">
              <LiveReactionStream
                logs={logs}
                onClearLogs={() => setLogs([])}
                engineActive={telemetry.engineActive}
                nativeMode={isNativeBridge}
              />
            </section>
          </div>
        )}

        {/* Tab 3: Hardware Longevity & Storage */}
        {activeTab === 'longevity' && (
          <div className="space-y-6 m3-stage">
            <section aria-label="Hardware longevity controls">
              <HardwareLongevityCard
                longevityConfig={longevityConfig}
                nativeMode={isNativeBridge}
                onSaveLongevity={handleSaveLongevity}
                isSaving={isSaving}
                saveMessage={longevitySaveMessage}
              />
            </section>

            <section aria-label="Recovery journal">
              <BaselineGuardianCard
                nodesSnapshotted={telemetry.baselineNodesSnapshotted}
                nativeMode={isNativeBridge}
                baselineStatus={telemetry.baselineStatus}
                onRestoreBaseline={handleRestoreBaseline}
                isRestoring={isRestoring}
                restoreFeedback={restoreFeedback}
              />
            </section>
          </div>
        )}

        {/* Tab 4: Android Kotlin Quick Settings */}
        {activeTab === 'android' && (
          <div className="space-y-6 m3-stage">
            <section aria-label="Android Kotlin integration">
              <AndroidQuickSettingsCard
                currentBias={config.adaptiveBias}
                onSelectBias={handleSelectBias}
                nativeMode={isNativeBridge}
                socTemp={telemetry.socTempCelsius}
              />
            </section>

            <section aria-label="Scheduler configuration">
              <AutonomousTuningCard
                config={config}
                nativeMode={isNativeBridge}
                onChangeConfig={setConfig}
                onSaveConfig={handleSaveConfig}
                isSaving={isSaving}
                saveMessage={saveMessage}
              />
            </section>
          </div>
        )}

        {/* Tab 6: CoreFlow Device Validation Suite */}
        {activeTab === 'validation' && (
          <div className="space-y-6 m3-stage">
            <section aria-label="Device validation and CDDP evidence export">
              <DeviceValidationSuiteCard nativeMode={isNativeBridge} />
            </section>
          </div>
        )}

        {/* Tab 5: Developer Console */}
        {activeTab === 'console' && (
          <div className="space-y-6 m3-stage">
            <section aria-label="Developer console">
              <DeveloperConsoleCard
                nativeMode={isNativeBridge}
                onRefreshTelemetry={async () => {
                  await refreshNative();
                }}
              />
            </section>

            <section aria-label="Engine activity">
              <LiveReactionStream
                logs={logs}
                onClearLogs={() => setLogs([])}
                engineActive={telemetry.engineActive}
                nativeMode={isNativeBridge}
              />
            </section>
          </div>
        )}
      </main>

      <ArchitectureStudioModal
        isOpen={isStudioOpen}
        onClose={() => setIsStudioOpen(false)}
        {...hardwareInfo}
      />

      <footer className="border-t border-slate-900/[0.06] dark:border-white/[0.06] bg-white/50 dark:bg-[#0e1117]/60 backdrop-blur-xl py-6 text-xs text-slate-500 dark:text-slate-400">
        <div className="max-w-6xl mx-auto px-4 sm:px-6 flex flex-col sm:flex-row items-center justify-between gap-2">
          <div>
            <span className="font-semibold text-slate-700 dark:text-slate-300">
              CFAE System Framework v3.1.0
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
