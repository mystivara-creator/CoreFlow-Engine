import React, { useCallback, useEffect, useState } from 'react';
import { applyAppearance, readAppearance, saveAppearance, Theme } from './bridge/appearance';
import { execNativeKsuShell, hasNativeKernelSuBridge } from './bridge/daemonBridge';
import { AutonomousStateHero } from './components/AutonomousStateHero';
import { AutonomousTuningCard } from './components/AutonomousTuningCard';
import { BaselineGuardianCard } from './components/BaselineGuardianCard';
import { LiveReactionStream } from './components/LiveReactionStream';
import { PixelHeader } from './components/PixelHeader';
import {
  AutonomousConfig, AutonomousTelemetry, createInitialAutonomousTelemetry,
  createInitialLogs, DEFAULT_AUTONOMOUS_CONFIG, ReactionLogEvent,
  stepAutonomousSimulation,
} from './engine/autonomousEngine';

const BIN = '/data/adb/modules/coreflow_engine/system/bin/coreflowd';
const SERVICE = '/data/adb/modules/coreflow_engine/service.sh';

interface NativeStatus {
  running: boolean;
  pid: number;
  uclampMin: number;
  uclampMax: number;
  cgroupV2: boolean;
  socTempCelsius: number | null;
  recoveryPending: boolean;
}
const checkedNativeStatus = (result: string): NativeStatus => {
  const obj: unknown = JSON.parse(result.trim().split('\n').pop() ?? '');
  if (typeof obj !== 'object' || obj === null) throw new Error('Invalid native status');
  const status = obj as Partial<NativeStatus>;
  if (typeof status.running !== 'boolean' || typeof status.pid !== 'number' ||
      typeof status.uclampMin !== 'number' || typeof status.uclampMax !== 'number' ||
      typeof status.cgroupV2 !== 'boolean' ||
      (typeof status.socTempCelsius !== 'number' && status.socTempCelsius !== null) ||
      typeof status.recoveryPending !== 'boolean') {
    throw new Error('Unexpected daemon status schema');
  }
  return status as NativeStatus;
};
const execChecked = async (command: string) => {
  const result = await execNativeKsuShell(command);
  if (result.errno !== 0) throw new Error(result.stderr || result.stdout || 'Native command failed');
  return result.stdout;
};
const delay = (ms: number) => new Promise<void>(resolve => setTimeout(resolve, ms));

export const App: React.FC = () => {
  const [isNativeBridge] = useState(() => hasNativeKernelSuBridge());
  const [appearance, setAppearance] = useState(() => readAppearance());
  const [nativeError, setNativeError] = useState<string | null>(null);
  const [isSwitching, setIsSwitching] = useState(false);
  const isDark = appearance.theme === 'dark' ||
    (appearance.theme === 'system' && typeof window !== 'undefined' &&
      window.matchMedia?.('(prefers-color-scheme: dark)').matches);

  const [telemetry, setTelemetry] = useState<AutonomousTelemetry>(() => {
    const initial = createInitialAutonomousTelemetry();
    return isNativeBridge ? {
      ...initial, engineActive: false, currentFps: 0, targetFps: 0,
      jankRatePct: 0, socTempCelsius: -1, uclampMin: 0, uclampMax: 0,
      topApp: '', topAppLabel: 'Unavailable', topAppPid: 0, epollEventsPerSec: 0,
      cgroupsV2Status: 'disabled', baselineNodesSnapshotted: 0,
      modeLabel: 'Awaiting native status', modeDescription: 'Awaiting daemon response',
    } : initial;
  });
  const [config, setConfig] = useState<AutonomousConfig>(() => ({ ...DEFAULT_AUTONOMOUS_CONFIG }));
  const [logs, setLogs] = useState<ReactionLogEvent[]>(() => isNativeBridge ? [] : createInitialLogs());
  const [isSaving, setIsSaving] = useState(false);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const [isRestoring, setIsRestoring] = useState(false);
  const [restoreFeedback, setRestoreFeedback] = useState<string | null>(null);

  useEffect(() => { applyAppearance(appearance); }, [appearance]);
  const toggleTheme = () => {
    const theme: Theme = isDark ? 'light' : 'dark';
    const changed = { ...appearance, theme };
    setAppearance(changed);
    saveAppearance(changed);
  };
  const addLog = useCallback((type: ReactionLogEvent['type'], tag: string, message: string) => {
    const event: ReactionLogEvent = {
      id: String(Date.now()) + String(Math.random()), timestamp: new Date().toTimeString().slice(0, 8),
      type, tag, message,
    };
    setLogs(old => [event, ...old].slice(0, 50));
  }, []);

  const refreshNative = useCallback(async () => {
    const status = checkedNativeStatus(await execChecked(BIN + ' --status'));
    setTelemetry(old => ({
      ...old, engineActive: status.running, uclampMin: status.uclampMin,
      uclampMax: status.uclampMax, socTempCelsius: status.socTempCelsius ?? -1,
      cgroupsV2Status: status.cgroupV2 ? 'unified_active' : 'disabled',
      baselineStatus: status.recoveryPending ? 'protected' : 'restored',
      modeLabel: status.running ? 'Native daemon active' : 'Daemon stopped',
      modeDescription: status.recoveryPending ? 'Recovery needs attention' : 'Native kernel status',
      timestamp: Date.now(),
    }));
    setNativeError(null);
    return status;
  }, []);

  useEffect(() => {
    if (!isNativeBridge) return;
    let disposed = false;
    const poll = async () => {
      try {
        await refreshNative();
        const result = await execNativeKsuShell('/system/bin/tail -n 35 /data/adb/coreflow/logs/coreflowd.log');
        if (!disposed && result.errno === 0) {
          const lines = result.stdout.split('\n').map(v => v.trim()).filter(Boolean).slice(-30);
          setLogs(lines.reverse().map((message, i) => ({
            id: 'native-' + i + '-' + message,
            timestamp: '--:--:--', type: 'INFO' as const, tag: 'Daemon', message: message.slice(0, 512),
          })));
        }
      } catch (error) {
        if (!disposed) setNativeError(String(error));
      }
    };
    void poll();
    const timer = window.setInterval(() => { void poll(); }, 5000);
    return () => { disposed = true; window.clearInterval(timer); };
  }, [isNativeBridge, refreshNative]);

  useEffect(() => {
    if (isNativeBridge) return;
    const timer = window.setInterval(() => {
      setTelemetry(old => {
        const { next, event } = stepAutonomousSimulation(old, config);
        if (event) setLogs(previous => [event, ...previous].slice(0, 50));
        return next;
      });
    }, 2500);
    return () => window.clearInterval(timer);
  }, [isNativeBridge, config]);

  const handleToggleEngine = async () => {
    if (!isNativeBridge) {
      setTelemetry(old => ({ ...old, engineActive: !old.engineActive }));
      return;
    }
    if (isSwitching) return;
    setIsSwitching(true);
    try {
      const before = await refreshNative();
      if (before.running) {
        await execChecked(BIN + ' --stop');
      } else {
        await execChecked('/system/bin/sh ' + SERVICE);
      }
      await delay(800);
      const after = await refreshNative();
      if (after.running === before.running) throw new Error('Daemon state did not change');
      addLog('INFO', 'NativeControl', after.running ? 'Daemon started' : 'Daemon stopped; rollback verified');
    } catch (error) {
      setNativeError(String(error));
      addLog('INFO', 'NativeControl', 'Failed: ' + String(error));
    } finally {
      setIsSwitching(false);
    }
  };
  const handleSimulateTouchBoost = () => {
    if (isNativeBridge || !telemetry.engineActive) return;
    setTelemetry(old => ({ ...old, mode: 'touch_boost', uclampMin: config.touchBoostMin }));
    addLog('BOOST', 'Preview', 'Simulated touch boost');
  };
  const handleSimulateAppLaunch = () => {
    if (isNativeBridge || !telemetry.engineActive) return;
    setTelemetry(old => ({ ...old, mode: 'launch_burst', uclampMin: 800 }));
    addLog('BOOST', 'Preview', 'Simulated cold launch');
  };
  const handleSaveConfig = async () => {
    setIsSaving(true);
    setSaveMessage(null);
    try {
      if (!isNativeBridge) {
        setSaveMessage('Preview only: nothing written to the device');
        return;
      }
      const interactiveMin = config.adaptiveBias === 'battery' ? 220
        : config.adaptiveBias === 'responsiveness' ? 480 : config.touchBoostMin;
      const trip = Math.max(40, Math.min(48, config.thermalTripCelsius));
      const content = '[general]\nversion = 2.0.0\nenable_cgroups_v2 = true\nenable_uclamp = true\n'
        + '\n[device_adaptation]\nunlock_vendor_clamping = false\nforce_120hz_rendering = false\n'
        + '\n[uclamp_tuning]\ntop_app_interactive_min = ' + interactiveMin
        + '\ntop_app_interactive_max = 1024\n'
        + '\n[thermal_guardian]\ntemp_monitor_interval_ms = 1000\nthermal_trip_celsius = ' + trip.toFixed(1)
        + '\nthermal_recover_celsius = ' + (trip - 4).toFixed(1) + '\n';
      const output = await execChecked("cat <<'COREFLOW_CONFIG' > /data/adb/coreflow/coreflow.conf\n"
        + content + "COREFLOW_CONFIG\n[ -s /data/adb/coreflow/coreflow.conf ] && echo COREFLOW_CONFIG_WRITTEN");
      if (!output.includes('COREFLOW_CONFIG_WRITTEN')) throw new Error('Configuration write not acknowledged');
      const before = await refreshNative();
      if (before.running) {
        await execChecked(BIN + ' --stop');
        await execChecked('/system/bin/sh ' + SERVICE);
        await delay(800);
        const after = await refreshNative();
        if (!after.running) throw new Error('Config saved but daemon restart failed');
      }
      setSaveMessage(before.running ? 'Saved and daemon restarted' : 'Saved; takes effect on next start');
      addLog('INFO', 'Config', 'Native config written and acknowledged');
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
        setTelemetry(old => ({ ...old, baselineStatus: 'restored' }));
        setRestoreFeedback('Preview only: simulated rollback');
        return;
      }
      await execChecked(BIN + ' --rollback-baseline');
      const status = await refreshNative();
      if (status.running || status.recoveryPending) throw new Error('Rollback not verified');
      setRestoreFeedback('OEM baseline restoration verified by daemon');
      addLog('BASELINE', 'NativeRecovery', 'Daemon stopped and journal cleared');
    } catch (error) {
      setRestoreFeedback('Restoration failed: ' + String(error));
      setNativeError(String(error));
    } finally {
      setIsRestoring(false);
    }
  };
  return (
    <div className="min-h-screen bg-slate-50 dark:bg-[#090d16] text-slate-900 dark:text-slate-100 flex flex-col font-sans transition-colors duration-200 antialiased selection:bg-sky-500 selection:text-white">
      <div role="status" className="px-4 py-2 text-center text-xs bg-amber-100 text-amber-950 dark:bg-amber-950 dark:text-amber-100">{isNativeBridge ? "Native telemetry: measured UClamp and temperature. FPS, top-app and jank measurement unavailable." : "DEMO PREVIEW: simulated telemetry and controls, not connected to a device."}</div>
      {nativeError && <div role="alert" className="px-4 py-2 bg-rose-200 text-rose-950 dark:bg-rose-950 dark:text-rose-100 text-xs">{nativeError}</div>}
      {/* Google Pixel / Material 3 Top Navigation */}
      <PixelHeader
        engineActive={telemetry.engineActive}
        onToggleEngine={handleToggleEngine}
        isDark={isDark}
        onToggleTheme={toggleTheme}
        epollEventsPerSec={telemetry.epollEventsPerSec}
        modeLabel={telemetry.modeLabel}
        nativeMode={isNativeBridge}
      />

      {/* Main Single-Page Modular Dashboard Container */}
      <main className="flex-1 max-w-6xl w-full mx-auto px-4 sm:px-6 py-6 sm:py-8 space-y-6 sm:space-y-8">
        {/* Section 1: Hero Autonomous Adaptation & Live Metric Squircles */}
        <section aria-label="Engine Real-Time Telemetry">
          <AutonomousStateHero
            telemetry={telemetry}
            nativeMode={isNativeBridge}
            thermalTripCelsius={config.thermalTripCelsius}
            onSimulateTouchBoost={handleSimulateTouchBoost}
            onSimulateAppLaunch={handleSimulateAppLaunch}
          />
        </section>

        {/* Section 2: Autonomous Adaptation Dials & Boundaries */}
        <section aria-label="Autonomous Adaptation Dials">
          <AutonomousTuningCard
            config={config}
            nativeMode={isNativeBridge}
            onChangeConfig={setConfig}
            onSaveConfig={handleSaveConfig}
            isSaving={isSaving}
            saveMessage={saveMessage}
          />
        </section>

        {/* Section 3: Transactional Baseline Protection (Safety & OEM Rollback) */}
        <section aria-label="OEM Baseline Protection">
          <BaselineGuardianCard
            nodesSnapshotted={telemetry.baselineNodesSnapshotted}
            nativeMode={isNativeBridge}
            baselineStatus={telemetry.baselineStatus}
            onRestoreBaseline={handleRestoreBaseline}
            isRestoring={isRestoring}
            restoreFeedback={restoreFeedback}
          />
        </section>

        {/* Section 4: Live Activity & Reaction Stream Feed */}
        <section aria-label="Live Activity Stream">
          <LiveReactionStream
            logs={logs}
            onClearLogs={() => setLogs([])}
            engineActive={telemetry.engineActive}
          />
        </section>
      </main>

      {/* Footer */}
      <footer className="border-t border-slate-200/80 dark:border-slate-800/80 py-6 text-center text-xs text-slate-500 dark:text-slate-400">
        <div className="max-w-6xl mx-auto px-4 flex flex-col sm:flex-row items-center justify-between gap-2">
          <div>
            <span className="font-semibold text-slate-700 dark:text-slate-300">
              CoreFlow Engine v2.0
            </span>{' '}
            · Autonomous Performance Manager for Android 15+
          </div>
          <div className="text-[11px] font-mono">
            Linux Kernel 5.15 / 6.1 / 6.6 · epoll zero-polling · EAS Uclamp
          </div>
        </div>
      </footer>
    </div>
  );
};

export default App;
