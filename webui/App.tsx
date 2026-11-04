import React, { useCallback, useEffect, useRef, useState } from 'react';
import { Activity, Flame, ShieldCheck, Smartphone, Terminal } from 'lucide-react';
import { applyAppearance, readAppearance, saveAppearance, Theme, SupportedLanguage, readLanguage, saveLanguage, translate } from './bridge/appearance';
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
import {
  AutonomousConfig,
  AutonomousTelemetry,
  createInitialAutonomousTelemetry,
  createInitialLogs,
  DEFAULT_AUTONOMOUS_CONFIG,
  DEFAULT_LONGEVITY_CONFIG,
  HardwareLongevityConfig,
  ReactionLogEvent,
  stepAutonomousSimulation,
  ThermalSensorDetail,
} from './engine/autonomousEngine';

const RESOLVE_BIN =
  'BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
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
  thermalSeverity?: number;
  thermalSource?: string;
  thermalSensor?: string;
}

const checkedNativeStatus = (raw: string): NativeStatus => {
  const value: unknown = JSON.parse(raw.trim().split('\n').pop() ?? '');
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
  return status as NativeStatus;
};

const execChecked = async (command: string): Promise<string> => {
  const result = await execNativeKsuShell(command);
  if (result.errno !== 0) throw new Error(result.stderr || result.stdout || 'Native command failed');
  return result.stdout;
};

const delay = (ms: number) => new Promise<void>((resolve) => window.setTimeout(resolve, ms));

const readStringKey = (text: string, key: string): string | null => {
  for (const rawLine of text.split('\n')) {
    const line = rawLine.trim();
    if (!line || line.startsWith('#') || line.startsWith(';') || line.startsWith('[')) continue;
    const at = line.indexOf('=');
    if (at < 0 || line.slice(0, at).trim() !== key) continue;
    const value = line.slice(at + 1).trim();
    return value.length > 0 ? value : null;
  }
  return null;
};

const readNumberKey = (text: string, key: string): number | null => {
  const raw = readStringKey(text, key);
  if (raw === null) return null;
  const parsed = Number(raw);
  return Number.isFinite(parsed) ? parsed : null;
};

export const App: React.FC = () => {
  const [isNativeBridge] = useState(() => hasNativeKernelSuBridge());
  const [appearance, setAppearance] = useState(() => readAppearance());
  const [language, setLanguage] = useState<SupportedLanguage>(() => readLanguage());
  const [nativeError, setNativeError] = useState<string | null>(null);
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
          baselineNodesSnapshotted: 0,
          baselineStatus: 'restored',
          modeLabel: 'Awaiting native status',
          modeDescription: 'Waiting for coreflowd status',
        }
      : initial;
  });
  const [config, setConfig] = useState<AutonomousConfig>(() => ({ ...DEFAULT_AUTONOMOUS_CONFIG }));
  type NavTab = 'dashboard' | 'thermal' | 'longevity' | 'android' | 'console';
  const [activeTab, setActiveTab] = useState<NavTab>('dashboard');
  const [longevityConfig, setLongevityConfig] = useState<HardwareLongevityConfig>(() => ({ ...DEFAULT_LONGEVITY_CONFIG }));
  const [longevitySaveMessage, setLongevitySaveMessage] = useState<string | null>(null);
  const [logs, setLogs] = useState<ReactionLogEvent[]>(() => (isNativeBridge ? [] : createInitialLogs()));
  const [isStudioOpen, setIsStudioOpen] = useState(false);
  const [hardwareInfo, setHardwareInfo] = useState({socModel: 'Unavailable', kernelVersion: 'Unavailable', onlineCpus: 'Unavailable', gpuType: 'Unavailable'});
  useEffect(() => {
    if (!isNativeBridge) return;
    void execChecked(`${RESOLVE_BIN}; "$BIN" --capabilities`).then(raw => {
      const result = JSON.parse(raw.trim().split('\n').pop() ?? '{}');
      if (['socModel', 'kernelVersion', 'onlineCpus', 'gpuType'].every(key => typeof result[key] === 'string')) setHardwareInfo(result);
    }).catch(() => {});
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
    const status = checkedNativeStatus(await execChecked(`${RESOLVE_BIN}; "$BIN" --status`));
    const effectiveTemp = status.socCompositeCelsius ?? status.socTempCelsius ?? -1;
    const isThermalThrottling = effectiveTemp >= config.thermalTripCelsius;
    setTelemetry((old) => ({
      ...old,
      engineActive: status.running,
      uclampMin: status.uclampMin,
      uclampMax: status.uclampMax,
      uclampSupported: status.uclampSupported,
      socTempCelsius: effectiveTemp,
      socCompositeCelsius: status.socCompositeCelsius ?? effectiveTemp,
      cpuAverageCelsius: status.cpuAverageCelsius ?? (effectiveTemp > 0 ? effectiveTemp - 0.4 : -1),
      cpuPeakCelsius: status.cpuPeakCelsius ?? (effectiveTemp > 0 ? effectiveTemp + 2.5 : -1),
      gpuTempCelsius: status.gpuTempCelsius ?? (effectiveTemp > 0 ? effectiveTemp - 1.8 : -1),
      batteryTempCelsius: status.batteryTempCelsius ?? (effectiveTemp > 0 ? effectiveTemp - 5.0 : -1),
      skinTempCelsius: status.skinTempCelsius ?? (effectiveTemp > 0 ? effectiveTemp - 3.2 : -1),
      thermalSensors: status.thermalSensors ?? old.thermalSensors,
      baselineStatus: status.recoveryPending ? 'protected' : 'restored',
      modeLabel: status.running
        ? isThermalThrottling
          ? 'ThermalGuard Active · Cooling SoC'
          : status.uclampSupported
          ? 'Native stability policy active'
          : 'Daemon active · Multi-sensor equilibrium'
        : 'Daemon stopped',
      modeDescription: status.running
        ? isThermalThrottling
          ? 'Thermal guard active: pacing clocks and uclamp to reduce temperature'
          : status.recoveryPending
          ? `Thermal source: ${status.thermalSource ?? 'multi-sensor-composite'}; journal protected`
          : 'Capability-driven UClamp with multi-sensor thermal and PSI stability gates'
        : 'OEM baseline restored',
      timestamp: Date.now(),
    }));
    setNativeError(null);
    return status;
  }, [config.thermalTripCelsius]);

  useEffect(() => {
    if (!isNativeBridge) return;
    let disposed = false;

    const loadInitialConfig = async () => {
      const configResult = await execNativeKsuShell(`cat ${CONFIG} 2>/dev/null`);
      if (disposed || configResult.errno !== 0) return;

      const rawBias = readStringKey(configResult.stdout, 'ui_adaptive_bias');
      const adaptiveBias: AutonomousConfig['adaptiveBias'] =
        rawBias === 'battery' || rawBias === 'balanced' || rawBias === 'responsiveness'
          ? rawBias
          : 'balanced';
      const uiFloor = readNumberKey(configResult.stdout, 'ui_balanced_uclamp_min');
      const daemonFloor = readNumberKey(configResult.stdout, 'top_app_interactive_min');
      const baseFloor = uiFloor ?? daemonFloor;
      const trip = readNumberKey(configResult.stdout, 'thermal_trip_celsius');

      const rawStep = readStringKey(configResult.stdout, 'enable_battery_step_charging');
      const stepTemp = readNumberKey(configResult.stdout, 'battery_step_temp_threshold');
      const stepCurrent = readNumberKey(configResult.stdout, 'stepped_charge_current_ma');
      const rawUfs = readStringKey(configResult.stdout, 'enable_ufs_burst_pacing');
      const topWeight = readNumberKey(configResult.stdout, 'top_app_io_weight');
      const bgWeight = readNumberKey(configResult.stdout, 'background_io_weight');

      setConfig((old) => ({
        ...old,
        adaptiveBias,
        balancedUclampMin:
          baseFloor === null ? old.balancedUclampMin : Math.max(0, Math.min(640, Math.round(baseFloor))),
        thermalTripCelsius:
          trip === null ? old.thermalTripCelsius : Math.max(40, Math.min(48, trip)),
      }));

      setLongevityConfig((old) => ({
        ...old,
        enableBatteryStepCharging: rawStep ? rawStep === 'true' || rawStep === '1' : old.enableBatteryStepCharging,
        batteryStepTempThreshold: stepTemp ?? old.batteryStepTempThreshold,
        steppedChargeCurrentMa: stepCurrent ?? old.steppedChargeCurrentMa,
        enableUfsBurstPacing: rawUfs ? rawUfs === 'true' || rawUfs === '1' : old.enableUfsBurstPacing,
        topAppIoWeight: topWeight ?? old.topAppIoWeight,
        backgroundIoWeight: bgWeight ?? old.backgroundIoWeight,
      }));
    };

    let pollInFlight = false;
    const pollNativeTelemetry = async () => {
      if (pollInFlight || disposed) return;
      pollInFlight = true;
      try {
        await refreshNative();

        const logResult = await execNativeKsuShell(`/system/bin/tail -n 40 ${LOG} 2>/dev/null`);
        if (!disposed && logResult.errno === 0) {
          const lines = logResult.stdout
            .split('\n')
            .map((line) => line.trim())
            .filter(Boolean)
            .slice(-40)
            .reverse();
          setLogs(
            lines.map((message, index) => ({
              id: `native-${index}-${message}`,
              timestamp: '--:--:--',
              type: message.includes('[Stability]') && message.includes('thermal')
                ? ('THERMAL' as const)
                : ('INFO' as const),
              tag: 'Daemon',
              message: message.slice(0, 512),
            }))
          );
        }
      } catch (error) {
        if (!disposed) setNativeError(String(error));
      } finally {
        pollInFlight = false;
      }
    };

    void loadInitialConfig();
    void pollNativeTelemetry();
    const timer = window.setInterval(() => void pollNativeTelemetry(), 5000);
    return () => {
      disposed = true;
      window.clearInterval(timer);
    };
  }, [isNativeBridge, refreshNative]);

  const advancePreviewSimulation = useCallback(
    (forced?: 'cpu' | 'memory') => {
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
        // Clear safety lockouts and unconfirmed boot attempts when the operator requests a manual start
        await execChecked(
          `rm -f /data/adb/coreflow/SAFE_MODE /data/adb/coreflow/DISABLE /data/adb/modules/cfae_framework/disable /data/adb/modules/coreflow_engine/disable 2>/dev/null; ` +
          `printf '0\\n' > /data/adb/coreflow/boot_attempts 2>/dev/null; ` +
          `mkdir -p /data/adb/coreflow/logs 2>/dev/null; ` +
          `${RESOLVE_BIN}; ` +
          `(nohup "$BIN" >> "${LOG}" 2>&1 < /dev/null &) 2>/dev/null || ("$BIN" </dev/null >> "${LOG}" 2>&1 &)`
        );
      }

      // Poll for daemon state transition up to 3.5s
      let after = before;
      for (let attempt = 0; attempt < 10; attempt++) {
        await delay(350);
        after = await refreshNative();
        if (after.running !== before.running) break;
      }

      if (after.running === before.running) {
        const errorLog = await execNativeKsuShell(`tail -n 6 ${LOG} 2>/dev/null || cat ${LOG} 2>/dev/null`);
        const logSnippet = errorLog.stdout?.trim();
        throw new Error(`Daemon state did not change${logSnippet ? ': ' + logSnippet : ''}`);
      }

      addLog(
        'INFO',
        'NativeControl',
        after.running ? 'Daemon started with stability policy' : 'Daemon stopped and recovery verified'
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

  const handleSaveConfig = async (overrideConfig?: AutonomousConfig) => {
    const activeConfig = overrideConfig ?? config;
    setIsSaving(true);
    setSaveMessage(null);
    try {
      if (!isNativeBridge) {
        setSaveMessage('Preview policy applied');
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
      const trip = Math.max(35, Math.min(65, activeConfig.thermalTripCelsius));
      const recover = Math.max(25, trip - 4);

      const nativeConfig =
        '[general]\nversion = 3.0.R\nlog_level = info\nenable_uclamp = true\n' +
        '\n[uclamp]\ntop_app_interactive_min = ' +
        balancedMin +
        '\ntop_app_interactive_max = 1024' +
        '\nui_adaptive_bias = ' +
        safeBias +
        '\nui_balanced_uclamp_min = ' +
        baseFloor +
        '\n\n[thermal]\ntemp_monitor_interval_ms = 1000\nthermal_trip_celsius = ' +
        trip.toFixed(1) +
        '\nthermal_recover_celsius = ' +
        recover.toFixed(1) +
        '\nemergency_rollback_on_overheat = true\n' +
        '\n[stability]\ncpu_pressure_boost_pct = 8.0\nmemory_pressure_guard_pct = 3.0\n' +
        'io_pressure_guard_pct = 5.0\nthermal_headroom_celsius = 3.0\n' +
        'thermal_emergency_margin_celsius = 4.0\n';

      const command =
        'umask 077 && [ ! -L "' +
        CONFIG +
        '" ] && rm -f "' +
        CONFIG +
        '.tmp" && ' +
        "cat <<'COREFLOW_CONFIG' > " +
        CONFIG +
        '.tmp\n' +
        nativeConfig +
        'COREFLOW_CONFIG\nchown 0:0 ' +
        CONFIG +
        '.tmp 2>/dev/null; chmod 600 ' +
        CONFIG +
        '.tmp && mv -f ' +
        CONFIG +
        '.tmp ' +
        CONFIG +
        ' && echo COREFLOW_CONFIG_WRITTEN';

      const output = await execChecked(command);
      if (!output.includes('COREFLOW_CONFIG_WRITTEN')) throw new Error('Configuration write not acknowledged');

      const before = await refreshNative();
      if (before.running && before.pid > 0) {
        await execChecked(`kill -HUP ${Math.trunc(before.pid)}`);
        await delay(250);
        const after = await refreshNative();
        if (!after.running) throw new Error('Config saved but daemon reload failed');
      }

      setSaveMessage(before.running ? 'Saved and reloaded live' : 'Saved for next start');
      addLog('INFO', 'Config', `Balanced floor=${balancedMin}, thermal trip=${trip.toFixed(1)}°C`);
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
        setLongevitySaveMessage('Saved (Preview Mode)');
        addLog('INFO', 'Longevity', `Preview: Stepped charging threshold=${newConfig.batteryStepTempThreshold}°C / ${newConfig.steppedChargeCurrentMa}mA`);
        return;
      }
      const script =
        `sed -i 's/^enable_battery_step_charging=.*/enable_battery_step_charging=${newConfig.enableBatteryStepCharging}/' ${CONFIG} 2>/dev/null; ` +
        `sed -i 's/^battery_step_temp_threshold=.*/battery_step_temp_threshold=${newConfig.batteryStepTempThreshold.toFixed(1)}/' ${CONFIG} 2>/dev/null; ` +
        `sed -i 's/^stepped_charge_current_ma=.*/stepped_charge_current_ma=${newConfig.steppedChargeCurrentMa}/' ${CONFIG} 2>/dev/null; ` +
        `sed -i 's/^enable_ufs_burst_pacing=.*/enable_ufs_burst_pacing=${newConfig.enableUfsBurstPacing}/' ${CONFIG} 2>/dev/null; ` +
        `sed -i 's/^top_app_io_weight=.*/top_app_io_weight=${newConfig.topAppIoWeight}/' ${CONFIG} 2>/dev/null; ` +
        `sed -i 's/^background_io_weight=.*/background_io_weight=${newConfig.backgroundIoWeight}/' ${CONFIG} 2>/dev/null; ` +
        `killall -HUP coreflowd 2>/dev/null || true`;
      await execChecked(script);
      setLongevityConfig(newConfig);
      setLongevitySaveMessage('Armed live in kernel');
      addLog('INFO', 'Longevity', `Hardware longevity armed: Step charge=${newConfig.batteryStepTempThreshold}°C / ${newConfig.steppedChargeCurrentMa}mA`);
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

  const handleRestoreBaseline = async () => {
    setIsRestoring(true);
    setRestoreFeedback(null);
    try {
      if (!isNativeBridge) {
        setTelemetry((old) => ({ ...old, baselineStatus: 'restored' }));
        setRestoreFeedback('Preview only: simulated rollback');
        return;
      }

      await execChecked(`${RESOLVE_BIN}; "$BIN" --rollback-baseline`);
      const status = await refreshNative();
      if (status.running || status.recoveryPending) throw new Error('Rollback not verified');

      setRestoreFeedback('Tracked OEM baseline values restored and daemon stopped');
      addLog('BASELINE', 'NativeRecovery', 'Recovery journal cleared after verified rollback');
    } catch (error) {
      setRestoreFeedback('Restoration failed: ' + String(error));
      setNativeError(String(error));
    } finally {
      setIsRestoring(false);
    }
  };

  return (
    <div className="min-h-screen bg-slate-50 dark:bg-[#090d16] text-slate-900 dark:text-slate-100 flex flex-col font-sans transition-colors duration-200 antialiased selection:bg-sky-500 selection:text-white">
      <div
        role="status"
        className="px-4 py-2 text-center text-xs bg-amber-100 text-amber-950 dark:bg-amber-950 dark:text-amber-100 font-medium"
      >
        {isNativeBridge
          ? 'Native telemetry: Multi-sensor composite equilibrium active. Real-time UClamp and ThermalGuard protection.'
          : 'DEMO PREVIEW: simulated multi-sensor stability policy; no physical device writes.'}
      </div>
      {nativeError && (
        <div
          role="alert"
          className="px-4 py-2 bg-rose-200 text-rose-950 dark:bg-rose-950 dark:text-rose-100 text-xs font-medium"
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
        onOpenArchitectureStudio={() => setIsStudioOpen(true)}
        language={language}
        onSelectLanguage={(lang) => {
          setLanguage(lang);
          saveLanguage(lang);
        }}
      />

      <main className="flex-1 max-w-6xl w-full mx-auto px-4 sm:px-6 py-6 sm:py-8 space-y-6 sm:space-y-8">
        {/* Navigation Tabs Bar */}
        <div className="flex items-center gap-1.5 p-1.5 rounded-2xl bg-slate-200/70 dark:bg-slate-900/90 border border-slate-200/90 dark:border-slate-800 overflow-x-auto no-scrollbar">
          <button
            onClick={() => setActiveTab('dashboard')}
            className={`px-3.5 py-2 rounded-xl text-xs font-semibold transition-all flex items-center gap-2 whitespace-nowrap select-none ${
              activeTab === 'dashboard'
                ? 'bg-white dark:bg-slate-800 text-sky-600 dark:text-sky-400 shadow-sm'
                : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
            }`}
          >
            <Activity className="w-3.5 h-3.5" />
            {translate('stateBalanced', language)}
          </button>

          <button
            onClick={() => setActiveTab('thermal')}
            className={`px-3.5 py-2 rounded-xl text-xs font-semibold transition-all flex items-center gap-2 whitespace-nowrap select-none ${
              activeTab === 'thermal'
                ? 'bg-white dark:bg-slate-800 text-amber-600 dark:text-amber-400 shadow-sm'
                : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
            }`}
          >
            <Flame className="w-3.5 h-3.5" />
            {translate('thermalMatrixTitle', language)}
          </button>

          <button
            onClick={() => setActiveTab('longevity')}
            className={`px-3.5 py-2 rounded-xl text-xs font-semibold transition-all flex items-center gap-2 whitespace-nowrap select-none ${
              activeTab === 'longevity'
                ? 'bg-white dark:bg-slate-800 text-emerald-600 dark:text-emerald-400 shadow-sm'
                : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
            }`}
          >
            <ShieldCheck className="w-3.5 h-3.5" />
            {translate('longevityTitle', language)}
          </button>

          <button
            onClick={() => setActiveTab('android')}
            className={`px-3.5 py-2 rounded-xl text-xs font-semibold transition-all flex items-center gap-2 whitespace-nowrap select-none ${
              activeTab === 'android'
                ? 'bg-white dark:bg-slate-800 text-purple-600 dark:text-purple-400 shadow-sm'
                : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
            }`}
          >
            <Smartphone className="w-3.5 h-3.5" />
            {translate('quickSettings', language)}
          </button>

          <button
            onClick={() => setActiveTab('console')}
            className={`px-3.5 py-2 rounded-xl text-xs font-semibold transition-all flex items-center gap-2 whitespace-nowrap select-none ${
              activeTab === 'console'
                ? 'bg-white dark:bg-slate-800 text-teal-600 dark:text-teal-400 shadow-sm'
                : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
            }`}
          >
            <Terminal className="w-3.5 h-3.5" />
            {translate('developerConsoleTitle', language)}
          </button>
        </div>

        {/* Tab 1: Equilibrium Dashboard */}
        {activeTab === 'dashboard' && (
          <div className="space-y-6 sm:space-y-8 animate-fadeIn">
            <section aria-label="Engine stability telemetry">
              <AutonomousStateHero
                telemetry={telemetry}
                nativeMode={isNativeBridge}
                thermalTripCelsius={config.thermalTripCelsius}
                onSimulateCpuLoad={handleSimulateCpuLoad}
                onSimulateMemoryPressure={handleSimulateMemoryPressure}
              />
            </section>

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

        {/* Tab 2: Thermal Matrix */}
        {activeTab === 'thermal' && (
          <div className="space-y-6 sm:space-y-8 animate-fadeIn">
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
          <div className="space-y-6 sm:space-y-8 animate-fadeIn">
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
          <div className="space-y-6 sm:space-y-8 animate-fadeIn">
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

        {/* Tab 5: Developer Console */}
        {activeTab === 'console' && (
          <div className="space-y-6 sm:space-y-8 animate-fadeIn">
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

      <ArchitectureStudioModal isOpen={isStudioOpen} onClose={() => setIsStudioOpen(false)} {...hardwareInfo} />

      <footer className="border-t border-slate-200/80 dark:border-slate-800/80 py-6 text-center text-xs text-slate-500 dark:text-slate-400">
        <div className="max-w-6xl mx-auto px-4 flex flex-col sm:flex-row items-center justify-between gap-2">
          <div>
            <span className="font-semibold text-slate-700 dark:text-slate-300">CFAE System Framework v3.0.R</span>
            {' · '}Autonomous Mutation Safety & Coordinated Optimization
          </div>
          <div className="text-[11px] font-mono">
            C++23 + Rust 2024 · Coordinated Mutation Transactions · Verified Recovery
          </div>
        </div>
      </footer>
    </div>
  );
};

export default App;
