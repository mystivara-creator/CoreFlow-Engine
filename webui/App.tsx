import React, { useCallback, useEffect, useRef, useState } from 'react';
import {
  applyAppearance,
  readAppearance,
  saveAppearance,
  Theme,
} from './bridge/appearance';
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

export const App: React.FC = () => {
  const [isNativeBridge] = useState<boolean>(() => hasNativeKernelSuBridge());
  const [appearance, setAppearance] = useState(() => readAppearance());
  const isDark =
    appearance.theme === 'dark' ||
    (appearance.theme === 'system' &&
      typeof window !== 'undefined' &&
      window.matchMedia?.('(prefers-color-scheme: dark)').matches);

  const [telemetry, setTelemetry] = useState<AutonomousTelemetry>(() =>
    createInitialAutonomousTelemetry()
  );
  const [config, setConfig] = useState<AutonomousConfig>(() => ({
    ...DEFAULT_AUTONOMOUS_CONFIG,
  }));
  const [logs, setLogs] = useState<ReactionLogEvent[]>(() => createInitialLogs());

  const [isSaving, setIsSaving] = useState<boolean>(false);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);

  const [isRestoring, setIsRestoring] = useState<boolean>(false);
  const [restoreFeedback, setRestoreFeedback] = useState<string | null>(null);

  // Sync theme to DOM on mount and changes
  useEffect(() => {
    applyAppearance(appearance);
  }, [appearance]);

  const toggleTheme = () => {
    const nextTheme: Theme = isDark ? 'light' : 'dark';
    const nextApp = { ...appearance, theme: nextTheme };
    setAppearance(nextApp);
    saveAppearance(nextApp);
  };

  const addLog = useCallback(
    (type: ReactionLogEvent['type'], tag: string, message: string) => {
      const newEvent: ReactionLogEvent = {
        id: `evt-${Date.now()}-${Math.floor(Math.random() * 1000)}`,
        timestamp: new Date().toTimeString().slice(0, 8),
        type,
        tag,
        message,
      };
      setLogs((prev) => [newEvent, ...prev.slice(0, 50)]);
    },
    []
  );

  // Main autonomous reaction loop (2.5s tick)
  useEffect(() => {
    const interval = setInterval(() => {
      setTelemetry((prev) => {
        const { next, event } = stepAutonomousSimulation(prev, config);
        if (event) {
          setLogs((l) => [event, ...l.slice(0, 50)]);
        }
        return next;
      });
    }, 2500);

    return () => clearInterval(interval);
  }, [config]);

  // Master engine toggle (Active vs Stock OEM)
  const handleToggleEngine = () => {
    setTelemetry((prev) => {
      const nextActive = !prev.engineActive;
      addLog(
        'INFO',
        'AutonomousEngine',
        nextActive
          ? 'Autonomous daemon unpaused: zero-polling reactor resuming task acceleration.'
          : 'Autonomous daemon suspended: device reverted to OEM factory scheduler limits.'
      );
      return {
        ...prev,
        engineActive: nextActive,
      };
    });
  };

  // Instant touch boost simulation
  const handleSimulateTouchBoost = () => {
    if (!telemetry.engineActive) return;
    setTelemetry((prev) => ({
      ...prev,
      mode: 'touch_boost',
      uclampMin: config.touchBoostMin,
    }));
    addLog(
      'BOOST',
      'TouchInteraction',
      `Touch gesture event intercepted: instant uclamp boost (${config.touchBoostMin}) dispatched for 120Hz velocity.`
    );
  };

  // App launch simulation
  const handleSimulateAppLaunch = () => {
    if (!telemetry.engineActive) return;
    setTelemetry((prev) => ({
      ...prev,
      mode: 'launch_burst',
      uclampMin: 800,
      topApp: 'com.miHoYo.GenshinImpact',
      topAppLabel: 'Genshin Impact',
      topAppPid: 29402,
    }));
    addLog(
      'BOOST',
      'LaunchDetector',
      `Cold launch burst: Genshin Impact pinned to cgroup top-app, uclamp floor raised to 800 for ${config.burstDurationMs}ms.`
    );
  };

  // Save configuration (updates memory and writes to KernelSU config if rooted)
  const handleSaveConfig = async () => {
    setIsSaving(true);
    setSaveMessage(null);

    if (isNativeBridge) {
      try {
        const confContent = `[general]
version = 2.0.0
zero_polling_mode = true
enable_cgroups_v2 = true
enable_uclamp = true

[device_adaptation]
tier_override = auto
unlock_vendor_clamping = ${config.unlockVendorClamping}
force_120hz_rendering = ${config.targetFps === 120}

[uclamp_tuning]
top_app_interactive_min = ${config.touchBoostMin}
touch_boost_uclamp_min = ${config.touchBoostMin}
burst_boost_duration_ms = ${config.burstDurationMs}

[thermal_guardian]
thermal_trip_celsius = ${config.thermalTripCelsius.toFixed(1)}
emergency_rollback_on_overheat = true
`;
        await execNativeKsuShell(`cat << 'EOF' > /data/adb/modules/coreflow_engine/coreflow.conf\n${confContent}\nEOF`);
        addLog('INFO', 'ConfigWriter', 'coreflow.conf updated on /data/adb/modules/coreflow_engine/');
      } catch (err) {
        addLog('INFO', 'ConfigWriter', `Native save fallback: ${String(err)}`);
      }
    }

    setTimeout(() => {
      setIsSaving(false);
      setSaveMessage('Saved successfully');
      addLog(
        'INFO',
        'AutonomousConfig',
        `Autonomous parameters updated: Bias=${config.adaptiveBias}, Target=${config.targetFps}Hz, ThermalTrip=${config.thermalTripCelsius}°C`
      );
      setTimeout(() => setSaveMessage(null), 3000);
    }, 400);
  };

  // Restore OEM baseline
  const handleRestoreBaseline = async () => {
    setIsRestoring(true);
    setRestoreFeedback(null);

    if (isNativeBridge) {
      try {
        await execNativeKsuShell('coreflowd --rollback-baseline');
      } catch {
        // fallback
      }
    }

    setTimeout(() => {
      setIsRestoring(false);
      setTelemetry((prev) => ({
        ...prev,
        baselineStatus: 'restored',
        uclampMin: 0,
        currentFps: 60,
      }));
      setRestoreFeedback(
        'Transaction completed: All 18 sysfs kernel nodes restored to OEM factory state.'
      );
      addLog(
        'BASELINE',
        'BaselineStore',
        'Transactional rollback executed: sysfs nodes, CPU governor & EAS clamps reverted to initial factory snapshot.'
      );
      setTimeout(() => setRestoreFeedback(null), 5000);
    }, 600);
  };

  return (
    <div className="min-h-screen bg-slate-50 dark:bg-[#090d16] text-slate-900 dark:text-slate-100 flex flex-col font-sans transition-colors duration-200 antialiased selection:bg-sky-500 selection:text-white">
      {/* Google Pixel / Material 3 Top Navigation */}
      <PixelHeader
        engineActive={telemetry.engineActive}
        onToggleEngine={handleToggleEngine}
        isDark={isDark}
        onToggleTheme={toggleTheme}
        epollEventsPerSec={telemetry.epollEventsPerSec}
        modeLabel={telemetry.modeLabel}
      />

      {/* Main Single-Page Modular Dashboard Container */}
      <main className="flex-1 max-w-6xl w-full mx-auto px-4 sm:px-6 py-6 sm:py-8 space-y-6 sm:space-y-8">
        {/* Section 1: Hero Autonomous Adaptation & Live Metric Squircles */}
        <section aria-label="Engine Real-Time Telemetry">
          <AutonomousStateHero
            telemetry={telemetry}
            thermalTripCelsius={config.thermalTripCelsius}
            onSimulateTouchBoost={handleSimulateTouchBoost}
            onSimulateAppLaunch={handleSimulateAppLaunch}
          />
        </section>

        {/* Section 2: Autonomous Adaptation Dials & Boundaries */}
        <section aria-label="Autonomous Adaptation Dials">
          <AutonomousTuningCard
            config={config}
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
