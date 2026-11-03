import React, { useCallback, useEffect, useRef, useState } from 'react';
import { applyAppearance, readAppearance, saveAppearance, Theme } from './bridge/appearance';
import { execNativeKsuShell, hasNativeKernelSuBridge } from './bridge/daemonBridge';
import { AutonomousStateHero } from './components/AutonomousStateHero';
import { AutonomousTuningCard } from './components/AutonomousTuningCard';
import { BaselineGuardianCard } from './components/BaselineGuardianCard';
import { LiveReactionStream } from './components/LiveReactionStream';
import { PixelHeader } from './components/PixelHeader';
import {
  AutonomousConfig,
  AutonomousTelemetry,
  createInitialAutonomousTelemetry,
  createInitialLogs,
  DEFAULT_AUTONOMOUS_CONFIG,
  ReactionLogEvent,
  stepAutonomousSimulation,
} from './engine/autonomousEngine';

const RESOLVE_BIN =
  'BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -x "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -x "$BIN" ] || BIN="/system/bin/coreflowd"';
const SERVICE = '/data/adb/modules/coreflow_engine/service.sh';
const CONFIG = '/data/adb/coreflow/coreflow.conf';
const LOG = '/data/adb/coreflow/logs/coreflowd.log';

interface NativeStatus {
  running: boolean;
  pid: number;
  uclampMin: number;
  uclampMax: number;
  uclampSupported: boolean;
  socTempCelsius: number | null;
  recoveryPending: boolean;
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
  const [logs, setLogs] = useState<ReactionLogEvent[]>(() => (isNativeBridge ? [] : createInitialLogs()));
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
    setTelemetry((old) => ({
      ...old,
      engineActive: status.running,
      uclampMin: status.uclampMin,
      uclampMax: status.uclampMax,
      uclampSupported: status.uclampSupported,
      socTempCelsius: status.socTempCelsius ?? -1,
      baselineStatus: status.recoveryPending ? 'protected' : 'restored',
      modeLabel: status.running
        ? status.uclampSupported
          ? 'Native stability policy active'
          : 'Daemon active · UClamp unavailable'
        : 'Daemon stopped',
      modeDescription: status.recoveryPending
        ? status.running
          ? 'Tracked UClamp changes are protected by the recovery journal'
          : 'Recovery is still pending'
        : status.running
        ? 'Capability-driven UClamp with thermal and PSI stability gates'
        : 'OEM baseline restored',
      timestamp: Date.now(),
    }));
    setNativeError(null);
    return status;
  }, []);

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

      setConfig((old) => ({
        ...old,
        adaptiveBias,
        balancedUclampMin:
          baseFloor === null ? old.balancedUclampMin : Math.max(0, Math.min(640, Math.round(baseFloor))),
        thermalTripCelsius:
          trip === null ? old.thermalTripCelsius : Math.max(40, Math.min(48, trip)),
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
    try {
      const before = await refreshNative();
      if (before.running) {
        await execChecked(`${RESOLVE_BIN}; "$BIN" --stop`);
      } else {
        await execChecked(
          `if [ -f "${SERVICE}" ]; then /system/bin/sh "${SERVICE}"; else ${RESOLVE_BIN}; nohup "$BIN" >> "${LOG}" 2>&1 < /dev/null & fi`
        );
      }
      await delay(800);
      const after = await refreshNative();
      if (after.running === before.running) throw new Error('Daemon state did not change');
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

  const handleSaveConfig = async () => {
    setIsSaving(true);
    setSaveMessage(null);
    try {
      if (!isNativeBridge) {
        setSaveMessage('Preview policy applied');
        return;
      }

      const safeBias: AutonomousConfig['adaptiveBias'] =
        config.adaptiveBias === 'battery' || config.adaptiveBias === 'responsiveness'
          ? config.adaptiveBias
          : 'balanced';
      const baseFloor = Math.max(0, Math.min(640, Math.round(config.balancedUclampMin)));
      const multiplier =
        safeBias === 'battery' ? 0.75 : safeBias === 'responsiveness' ? 1.25 : 1;
      const balancedMin = Math.max(0, Math.min(1024, Math.round(baseFloor * multiplier)));
      const trip = Math.max(35, Math.min(65, config.thermalTripCelsius));
      const recover = Math.max(25, trip - 4);

      const nativeConfig =
        '[general]\nversion = 2.0.0\nlog_level = info\nenable_uclamp = true\n' +
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

      setRestoreFeedback('Tracked pre-CoreFlow values restored and daemon stopped');
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
        className="px-4 py-2 text-center text-xs bg-amber-100 text-amber-950 dark:bg-amber-950 dark:text-amber-100"
      >
        {isNativeBridge
          ? 'Native telemetry: UClamp capability and hottest trusted thermal sensor. PSI state transitions are recorded in daemon logs.'
          : 'DEMO PREVIEW: simulated four-state stability policy; no device writes.'}
      </div>
      {nativeError && (
        <div
          role="alert"
          className="px-4 py-2 bg-rose-200 text-rose-950 dark:bg-rose-950 dark:text-rose-100 text-xs"
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
      />

      <main className="flex-1 max-w-6xl w-full mx-auto px-4 sm:px-6 py-6 sm:py-8 space-y-6 sm:space-y-8">
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
      </main>

      <footer className="border-t border-slate-200/80 dark:border-slate-800/80 py-6 text-center text-xs text-slate-500 dark:text-slate-400">
        <div className="max-w-6xl mx-auto px-4 flex flex-col sm:flex-row items-center justify-between gap-2">
          <div>
            <span className="font-semibold text-slate-700 dark:text-slate-300">CoreFlow Engine v2.0</span>
            {' · '}Stability-first Android optimizer
          </div>
          <div className="text-[11px] font-mono">
            semantic UClamp · thermal + PSI · verified recovery
          </div>
        </div>
      </footer>
    </div>
  );
};

export default App;
