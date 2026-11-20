import React, { useCallback, useEffect, useMemo, useState } from 'react';
import {
  Activity,
  AppWindow,
  Battery,
  BatteryCharging,
  CheckCircle2,
  ClipboardCopy,
  Cpu,
  FileJson,
  Flame,
  Gamepad2,
  HardDrive,
  MemoryStick,
  Pause,
  Play,
  Plus,
  RefreshCw,
  RotateCcw,
  Search,
  ShieldAlert,
  ShieldCheck,
  Sparkles,
  Trash2,
  Zap,
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
import { BaselineGuardianCard } from './components/BaselineGuardianCard';
import { LiveReactionStream } from './components/LiveReactionStream';
import { PixelHeader } from './components/PixelHeader';
import {
  AppRuleCategory,
  assignPackageRule,
  buildSafeSetConfigArg,
  computeHealthScore,
  computePillarReadiness,
  CoreFlowAppRules,
  CoreFlowFeatures,
  CoreFlowV5Status,
  createPreviewStatus,
  formatDuration,
  getPackageRuleCategory,
  HistorySample,
  isValidPackageName,
  MODE_LABELS,
  parseInstalledPackages,
  parseV5Status,
  PerformanceProfile,
  PREVIEW_INSTALLED_PACKAGES,
  PROFILE_META,
  ReactionLogEvent,
  stepPreviewStatus,
} from './engine/autonomousEngine';

const RESOLVE_BIN =
  'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"';
const RESOLVE_SERVICE =
  'SVC="/data/adb/modules/cfae_framework/service.sh"; ' +
  '[ -f "$SVC" ] || SVC="/data/adb/modules_update/cfae_framework/service.sh"';
const LOG_PATH = '/data/adb/coreflow/logs/coreflowd.log';

const extractJsonObject = (raw: string): unknown => {
  const lines = raw
    .trim()
    .split('\n')
    .map((l) => l.trim())
    .filter(Boolean);
  for (let i = lines.length - 1; i >= 0; i--) {
    if (lines[i].startsWith('{') && lines[i].endsWith('}')) {
      try {
        return JSON.parse(lines[i]);
      } catch {
        // continue
      }
    }
  }
  throw new Error('Daemon did not return valid JSON');
};

const execChecked = async (cmd: string): Promise<string> => {
  const res = await execNativeKsuShell(cmd);
  if (res.errno !== 0) throw new Error(res.stderr || res.stdout || 'Native command failed');
  return res.stdout;
};

const formatBytes = (bytes: number): string => {
  if (!Number.isFinite(bytes) || bytes <= 0) return '0 MB';
  const gb = bytes / (1024 * 1024 * 1024);
  if (gb >= 1) return `${gb.toFixed(1)} GB`;
  return `${Math.round(bytes / (1024 * 1024))} MB`;
};

const ThermalSparkline: React.FC<{ samples: HistorySample[] }> = ({ samples }) => {
  const valid = samples.filter((s) => s.temp !== null && Number.isFinite(s.temp));
  if (valid.length < 2) {
    return (
      <div
        className="flex h-20 items-center justify-center rounded-2xl text-xs"
        style={{
          background: 'var(--md-sys-color-surface-container-low)',
          color: 'var(--md-sys-color-on-surface-variant)',
        }}
      >
        Collecting live thermal telemetry...
      </div>
    );
  }
  const temps = valid.map((s) => s.temp as number);
  const minT = Math.min(25, Math.floor(Math.min(...temps) - 2));
  const maxT = Math.max(55, Math.ceil(Math.max(...temps) + 2));
  const span = Math.max(1, maxT - minT);
  const width = 320;
  const height = 72;
  const points = valid
    .map((s, idx) => {
      const x = (idx / (valid.length - 1)) * width;
      const y = height - (((s.temp as number) - minT) / span) * (height - 10) - 5;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');

  return (
    <div
      className="rounded-2xl p-3"
      style={{ background: 'var(--md-sys-color-surface-container-low)' }}
    >
      <div className="mb-1 flex items-center justify-between text-[11px]">
        <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          SoC Thermal Trend ({valid.length} samples)
        </span>
        <span className="font-mono font-semibold">
          {temps[temps.length - 1].toFixed(1)} °C
        </span>
      </div>
      <svg viewBox={`0 0 ${width} ${height}`} className="h-16 w-full overflow-visible">
        <polyline
          fill="none"
          stroke="var(--md-sys-color-primary)"
          strokeWidth="2.2"
          strokeLinecap="round"
          strokeLinejoin="round"
          points={points}
        />
      </svg>
    </div>
  );
};

type NavTab = 'home' | 'apps' | 'system';

const RULE_CATEGORY_OPTIONS: { id: AppRuleCategory; label: string }[] = [
  { id: 'default', label: 'Auto' },
  { id: 'gaming', label: 'Gaming' },
  { id: 'smoothness', label: 'Smooth' },
  { id: 'battery', label: 'Battery' },
  { id: 'excluded', label: 'Stock' },
];

export const App: React.FC = () => {
  const [isNativeBridge] = useState(() => hasNativeKernelSuBridge());
  const [appearance, setAppearance] = useState(() => readAppearance());
  const [activeTab, setActiveTab] = useState<NavTab>('home');
  const [nativeConnected, setNativeConnected] = useState(false);
  const [status, setStatus] = useState<CoreFlowV5Status>(() => createPreviewStatus());
  const [savingKey, setSavingKey] = useState<string | null>(null);
  const [feedback, setFeedback] = useState<string | null>(null);
  const [errorMsg, setErrorMsg] = useState<string | null>(null);
  const [journalJson, setJournalJson] = useState<string | null>(null);
  const [diagJson, setDiagJson] = useState<string | null>(null);
  const [restoring, setRestoring] = useState(false);
  const [starting, setStarting] = useState(false);
  const [logs, setLogs] = useState<ReactionLogEvent[]>([]);

  // Per-App Picker & manual entry state
  const [installedApps, setInstalledApps] = useState<string[]>(() =>
    hasNativeKernelSuBridge() ? [] : PREVIEW_INSTALLED_PACKAGES
  );
  const [loadingApps, setLoadingApps] = useState(false);
  const [appSearch, setAppSearch] = useState('');
  const [manualPkg, setManualPkg] = useState('');
  const [manualCategory, setManualCategory] = useState<Exclude<AppRuleCategory, 'default'>>('gaming');

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

  const addLog = useCallback((type: ReactionLogEvent['type'], tag: string, message: string) => {
    setLogs((prev) =>
      [
        {
          id: `evt-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
          timestamp: new Date().toTimeString().slice(0, 8),
          type,
          tag,
          message,
        },
        ...prev,
      ].slice(0, 40)
    );
  }, []);

  const refreshNative = useCallback(async () => {
    const raw = await execChecked(`${RESOLVE_BIN}; "$BIN" --status`);
    const parsed = parseV5Status(extractJsonObject(raw));
    setStatus(parsed);
    setNativeConnected(true);
    setErrorMsg(null);
  }, []);

  const loadInstalledApps = useCallback(async () => {
    if (!isNativeBridge) {
      setInstalledApps(PREVIEW_INSTALLED_PACKAGES);
      return;
    }
    setLoadingApps(true);
    try {
      const res = await execNativeKsuShell(
        '/system/bin/cmd package list packages -3 2>/dev/null || /system/bin/pm list packages -3 2>/dev/null',
        12000
      );
      if (res.errno === 0 && res.stdout.trim()) {
        setInstalledApps(parseInstalledPackages(res.stdout));
      }
    } catch {
      // Keep manual fallback available
    } finally {
      setLoadingApps(false);
    }
  }, [isNativeBridge]);

  useEffect(() => {
    if (activeTab === 'apps' && installedApps.length === 0 && !loadingApps) {
      void loadInstalledApps();
    }
  }, [activeTab, installedApps.length, loadingApps, loadInstalledApps]);

  useEffect(() => {
    if (!isNativeBridge) {
      let tick = 1;
      const timer = window.setInterval(() => {
        setStatus((prev) => (prev.running && !prev.inspectionMode ? stepPreviewStatus(prev, tick++) : prev));
      }, 2500);
      return () => window.clearInterval(timer);
    }

    let disposed = false;
    let inFlight = false;
    const poll = async () => {
      if (inFlight || disposed) return;
      inFlight = true;
      try {
        await refreshNative();
        const logRes = await execNativeKsuShell(`/system/bin/tail -n 12 ${LOG_PATH} 2>/dev/null`);
        if (!disposed && logRes.errno === 0 && logRes.stdout.trim()) {
          const lines = logRes.stdout
            .trim()
            .split(/\r?\n/)
            .map((l) => l.trim())
            .filter(Boolean)
            .slice(-12)
            .reverse();
          setLogs(
            lines.map((msg, i) => ({
              id: `native-${i}-${msg.slice(0, 24)}`,
              timestamp: '--:--:--',
              type: msg.includes('Safety') || msg.includes('Thermal') ? 'THERMAL' : 'INFO',
              tag: 'coreflowd',
              message: msg.slice(0, 320),
            }))
          );
        }
      } catch (err) {
        if (!disposed) {
          setNativeConnected(false);
          setErrorMsg(String(err));
        }
      } finally {
        inFlight = false;
      }
    };

    void poll();
    const timer = window.setInterval(() => void poll(), 3000);
    return () => {
      disposed = true;
      window.clearInterval(timer);
    };
  }, [isNativeBridge, refreshNative]);

  const applyConfigUpdates = async (
    pairs: [string, string][],
    optimisticUpdate: (prev: CoreFlowV5Status) => CoreFlowV5Status,
    successNote: string
  ) => {
    const args: string[] = [];
    for (const [k, v] of pairs) {
      const safeArg = buildSafeSetConfigArg(k, v);
      if (!safeArg) {
        setErrorMsg(`Invalid configuration value for ${k}`);
        return;
      }
      args.push(safeArg);
    }
    setSavingKey(pairs[0][0]);
    setFeedback(null);
    setErrorMsg(null);
    try {
      if (isNativeBridge) {
        await execChecked(`${RESOLVE_BIN}; "$BIN" --set-config ${args.join(' ')}`);
      }
      setStatus(optimisticUpdate);
      setFeedback(successNote);
      addLog('BOOST', 'Config', successNote);
      if (isNativeBridge) {
        window.setTimeout(() => void refreshNative().catch(() => {}), 450);
      }
    } catch (err) {
      setErrorMsg(`Failed to apply configuration: ${String(err)}`);
    } finally {
      setSavingKey(null);
    }
  };

  const handleSelectProfile = (profile: PerformanceProfile) => {
    void applyConfigUpdates(
      [['profile', profile]],
      (prev) => ({ ...prev, profile, effectiveProfile: profile }),
      `Profile switched to ${PROFILE_META[profile].label}`
    );
  };

  const handleTogglePause = () => {
    const nextPaused = !status.inspectionMode;
    void applyConfigUpdates(
      [['inspection_mode', nextPaused ? 'true' : 'false']],
      (prev) => ({ ...prev, inspectionMode: nextPaused }),
      nextPaused
        ? 'Paused (OEM Stock A/B compare active — all kernel nodes restored)'
        : 'CoreFlow Autonomous Engine resumed'
    );
  };

  const handleStartDaemon = async () => {
    setStarting(true);
    setErrorMsg(null);
    setFeedback(null);
    try {
      if (isNativeBridge) {
        await execChecked(
          `${RESOLVE_SERVICE}; rm -f /data/adb/coreflow/DISABLE /data/adb/coreflow/SAFE_MODE; ` +
            'COREFLOW_SKIP_BOOT_WAIT=1 sh "$SVC"'
        );
        await refreshNative();
      } else {
        setStatus((prev) => ({ ...prev, running: true, pid: 1428, inspectionMode: false }));
      }
      addLog('BOOST', 'Daemon', 'CoreFlow v5.1.0 daemon started.');
      setFeedback('CoreFlow daemon started and active.');
    } catch (err) {
      setErrorMsg(`Unable to start daemon: ${String(err)}`);
    } finally {
      setStarting(false);
    }
  };

  const handleToggleFeature = (
    configKey: string,
    featureField: keyof CoreFlowFeatures,
    label: string
  ) => {
    const nextVal = !status.features[featureField];
    void applyConfigUpdates(
      [[configKey, nextVal ? 'true' : 'false']],
      (prev) => ({
        ...prev,
        features: { ...prev.features, [featureField]: nextVal },
      }),
      `${label} ${nextVal ? 'enabled' : 'disabled'}`
    );
  };

  const handleAssignAppRule = (pkg: string, category: AppRuleCategory) => {
    const trimmed = pkg.trim();
    if (!isValidPackageName(trimmed)) {
      setErrorMsg('Enter a valid Android package name (e.g. com.miHoYo.GenshinImpact)');
      return;
    }
    const nextApps: CoreFlowAppRules = assignPackageRule(status.apps, trimmed, category);
    const labelMap: Record<AppRuleCategory, string> = {
      default: 'Auto (Global Profile)',
      gaming: 'Gaming',
      smoothness: 'Smoothness',
      battery: 'Battery',
      excluded: 'Excluded (OEM Stock)',
    };
    void applyConfigUpdates(
      [
        ['gaming_apps', nextApps.gaming.join(',')],
        ['smoothness_apps', nextApps.smoothness.join(',')],
        ['battery_apps', nextApps.battery.join(',')],
        ['excluded_apps', nextApps.excluded.join(',')],
      ],
      (prev) => ({ ...prev, apps: nextApps }),
      `${trimmed} → ${labelMap[category]}`
    );
  };

  const handleViewJournal = async () => {
    if (!isNativeBridge) {
      setJournalJson(
        JSON.stringify(
          { format: 'CFJ4', entryCount: status.baselineNodesSnapshotted, entries: [] },
          null,
          2
        )
      );
      return;
    }
    try {
      const raw = await execChecked(`${RESOLVE_BIN}; "$BIN" --journal`);
      setJournalJson(JSON.stringify(extractJsonObject(raw), null, 2));
      setErrorMsg(null);
    } catch (err) {
      setErrorMsg(`Unable to read CFJ4 journal: ${String(err)}`);
    }
  };

  const handleExportDiag = async () => {
    try {
      let pretty = '';
      if (!isNativeBridge) {
        pretty = JSON.stringify(
          {
            reportFormat: 'coreflow-diag-v1',
            version: status.version,
            generatedAtMs: Date.now(),
            status,
          },
          null,
          2
        );
      } else {
        const raw = await execChecked(`${RESOLVE_BIN}; "$BIN" --diag`);
        pretty = JSON.stringify(extractJsonObject(raw), null, 2);
      }
      setDiagJson(pretty);
      if (navigator.clipboard?.writeText) {
        await navigator.clipboard.writeText(pretty).catch(() => {});
        setFeedback('Diagnostic report exported and copied to clipboard.');
      } else {
        setFeedback('Diagnostic report generated below.');
      }
      setErrorMsg(null);
    } catch (err) {
      setErrorMsg(`Unable to export diagnostics: ${String(err)}`);
    }
  };

  const handleRestoreBaseline = async () => {
    if (
      !window.confirm(
        'Restore all CoreFlow-managed kernel nodes to their captured OEM baseline and stop coreflowd?'
      )
    ) {
      return;
    }
    setRestoring(true);
    try {
      if (isNativeBridge) {
        await execChecked(`${RESOLVE_BIN}; "$BIN" --rollback-baseline`);
        setJournalJson(null);
        await refreshNative();
      } else {
        setStatus((prev) => ({ ...prev, running: false, baselineNodesSnapshotted: 0 }));
      }
      addLog('BASELINE', 'Rollback', 'OEM factory baseline restored cleanly.');
      setFeedback('OEM baseline restored. You can restart CoreFlow anytime.');
    } catch (err) {
      setErrorMsg(`Restore failed: ${String(err)}`);
    } finally {
      setRestoring(false);
    }
  };

  const healthScore = useMemo(() => computeHealthScore(status), [status]);
  const readiness = useMemo(() => computePillarReadiness(status), [status]);
  const modeMeta = status.inspectionMode
    ? { label: 'Paused (OEM Stock)', tone: 'warning' as const }
    : MODE_LABELS[status.mode] ?? { label: status.mode, tone: 'neutral' as const };
  const modeBadgeClass =
    modeMeta.tone === 'error'
      ? 'md3-badge md3-badge-error'
      : modeMeta.tone === 'warning'
        ? 'md3-badge md3-badge-warning'
        : modeMeta.tone === 'success'
          ? 'md3-badge md3-badge-success'
          : 'md3-badge md3-badge-neutral';

  const filteredInstalledApps = useMemo(() => {
    const q = appSearch.trim().toLowerCase();
    const list = q
      ? installedApps.filter((pkg) => pkg.toLowerCase().includes(q))
      : installedApps;
    // Sort configured apps first so user immediately sees their overrides
    return [...list].sort((a, b) => {
      const ra = getPackageRuleCategory(status.apps, a) === 'default' ? 1 : 0;
      const rb = getPackageRuleCategory(status.apps, b) === 'default' ? 1 : 0;
      return ra !== rb ? ra - rb : a.localeCompare(b);
    });
  }, [installedApps, appSearch, status.apps]);

  const featureItems: {
    configKey: string;
    field: keyof CoreFlowFeatures;
    title: string;
    desc: string;
    Icon: React.FC<{ className?: string }>;
  }[] = [
    {
      configKey: 'enable_frame_smoothness',
      field: 'frameSmoothness',
      title: 'Per-Thread Frame Smoothness',
      desc: 'Closed-loop RenderThread & SurfaceFlinger UClamp via sched_setattr without fighting PowerHAL.',
      Icon: Sparkles,
    },
    {
      configKey: 'enable_storage_burst',
      field: 'storageBurst',
      title: 'UFS / eMMC Launch Burst & QoS',
      desc: '200ms foreground launch burst + isolates background I/O & CPU contention.',
      Icon: HardDrive,
    },
    {
      configKey: 'enable_memory_guard',
      field: 'memoryGuard',
      title: 'Anti-Stutter Memory & Pre-Read',
      desc: 'MGLRU working-set guard, dynamic swappiness, and bounded posix_fadvise app asset prefetch.',
      Icon: MemoryStick,
    },
    {
      configKey: 'enable_thermal_pacing',
      field: 'thermalPacing',
      title: 'Thermal PID Smooth Pacing',
      desc: 'Rust PID + Online RLS/MLP 10s lookahead scales CPU/GPU envelopes smoothly before cliff-throttling.',
      Icon: Flame,
    },
    {
      configKey: 'enable_battery_step_charging',
      field: 'batteryStepCharging',
      title: 'Battery Step-Charging Guard',
      desc: 'Protects battery longevity by stepping down charge current only when hot while charging.',
      Icon: BatteryCharging,
    },
  ];

  const ruleBuckets: {
    key: Exclude<AppRuleCategory, 'default'>;
    title: string;
    badgeClass: string;
    Icon: React.FC<{ className?: string; style?: React.CSSProperties }>;
  }[] = [
    { key: 'gaming', title: 'Gaming Mode', badgeClass: 'md3-badge md3-badge-success', Icon: Gamepad2 },
    { key: 'smoothness', title: 'Smoothness', badgeClass: 'md3-badge md3-badge-success', Icon: Sparkles },
    { key: 'battery', title: 'Battery Saver', badgeClass: 'md3-badge md3-badge-neutral', Icon: Battery },
    { key: 'excluded', title: 'Excluded (OEM Stock)', badgeClass: 'md3-badge md3-badge-warning', Icon: ShieldAlert },
  ];

  return (
    <div
      className="min-h-screen flex flex-col font-sans antialiased"
      style={{
        background: 'var(--md-sys-color-background)',
        color: 'var(--md-sys-color-on-surface)',
      }}
    >
      <PixelHeader
        isDark={isDark}
        onToggleTheme={toggleTheme}
        accent={appearance.accent}
        onSelectAccent={handleSelectAccent}
        modeLabel={modeMeta.label}
        healthScore={healthScore}
        nativeMode={isNativeBridge}
        nativeConnected={nativeConnected}
      />

      {/* Compact Tab Bar */}
      <nav
        className="sticky top-[4rem] z-20 px-4 py-2"
        style={{
          background: 'var(--md-sys-color-surface-container-low)',
          borderBottom: '1px solid var(--md-sys-color-outline-variant)',
        }}
      >
        <div className="mx-auto flex max-w-5xl items-center gap-2">
          {(
            [
              { id: 'home', label: 'Control Center', Icon: Activity },
              { id: 'apps', label: 'Per-App Rules', Icon: AppWindow },
              { id: 'system', label: 'System & Safety', Icon: ShieldCheck },
            ] as const
          ).map(({ id, label, Icon }) => {
            const active = activeTab === id;
            return (
              <button
                key={id}
                type="button"
                onClick={() => setActiveTab(id)}
                className={active ? 'md3-btn-filled flex-1 sm:flex-none' : 'md3-btn-tonal flex-1 sm:flex-none'}
              >
                <Icon className="h-4 w-4" />
                <span>{label}</span>
              </button>
            );
          })}
        </div>
      </nav>

      <main className="mx-auto w-full max-w-5xl flex-1 space-y-5 px-4 py-5 sm:px-6">
        {!isNativeBridge && (
          <div
            className="rounded-2xl px-4 py-2.5 text-xs font-medium"
            style={{
              background: 'var(--md-sys-color-warning-container)',
              color: 'var(--md-sys-color-on-warning-container)',
            }}
          >
            Browser Preview Mode — interactive demo. On a rooted Android 13+ device (KernelSU / APatch / Magisk + KsuWebUI), changes apply live to <code>coreflowd</code>.
          </div>
        )}

        {feedback && (
          <div
            className="rounded-2xl px-4 py-2.5 text-xs font-medium"
            style={{
              background: 'var(--md-sys-color-success-container)',
              color: 'var(--md-sys-color-on-success-container)',
            }}
          >
            {feedback}
          </div>
        )}

        {errorMsg && (
          <div
            className="rounded-2xl px-4 py-2.5 text-xs font-medium"
            style={{
              background: 'var(--md-sys-color-error-container)',
              color: 'var(--md-sys-color-on-error-container)',
            }}
          >
            {errorMsg}
          </div>
        )}

        {activeTab === 'home' && (
          <div className="space-y-5">
            {/* Live Context Hero + Instant A/B Pause or Start Controls */}
            <section className="md3-card-elevated space-y-4 p-5 sm:p-6">
              <div className="flex flex-wrap items-start justify-between gap-3">
                <div>
                  <div className="flex flex-wrap items-center gap-2">
                    <span className={modeBadgeClass}>{modeMeta.label}</span>
                    <span className="md3-badge md3-badge-neutral font-mono">
                      {PROFILE_META[status.effectiveProfile].label.toUpperCase()}
                    </span>
                    <span className="md3-badge md3-badge-success font-mono">
                      {readiness.readyCount}/{readiness.total} PILLARS
                    </span>
                    {status.actuators.storageBurstActive && !status.inspectionMode && (
                      <span className="md3-badge md3-badge-success">UFS BURST</span>
                    )}
                  </div>
                  <h1 className="mt-2 text-lg font-bold tracking-tight sm:text-xl">
                    {status.foregroundPackage || 'System Idle / No Foreground App'}
                  </h1>
                  <p className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    {status.running
                      ? `PID ${status.pid} · ${status.hardware.storageType} · ${status.hardware.refreshHz} Hz · Render load ${Math.round(status.renderPressure * 100)}%`
                      : 'Daemon stopped — OEM factory baseline active'}
                  </p>
                </div>

                <div className="flex items-center gap-3">
                  {status.running ? (
                    <button
                      type="button"
                      disabled={savingKey !== null}
                      onClick={handleTogglePause}
                      className={status.inspectionMode ? 'md3-btn-filled' : 'md3-btn-tonal'}
                      title="Instant A/B Compare: Pause reverts to OEM Stock immediately without stopping daemon"
                    >
                      {status.inspectionMode ? (
                        <>
                          <Play className="h-4 w-4" />
                          <span>Resume</span>
                        </>
                      ) : (
                        <>
                          <Pause className="h-4 w-4" />
                          <span>Pause (A/B)</span>
                        </>
                      )}
                    </button>
                  ) : (
                    <button
                      type="button"
                      disabled={starting}
                      onClick={() => void handleStartDaemon()}
                      className="md3-btn-filled"
                    >
                      <Play className="h-4 w-4" />
                      <span>{starting ? 'Starting...' : 'Start CoreFlow'}</span>
                    </button>
                  )}

                  <div className="text-right">
                    <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                      PID Budget
                    </div>
                    <div className="font-mono text-2xl font-bold tabular-nums">
                      {Math.round(status.capacityScale * 100)}%
                    </div>
                  </div>
                </div>
              </div>

              <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    SoC Temp
                  </div>
                  <div className="mt-0.5 font-mono text-lg font-bold">
                    {status.socTempCelsius !== null ? `${status.socTempCelsius.toFixed(1)} °C` : 'N/A'}
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Battery Temp
                  </div>
                  <div className="mt-0.5 font-mono text-lg font-bold">
                    {status.batteryTempCelsius !== null ? `${status.batteryTempCelsius.toFixed(1)} °C` : 'N/A'}
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Render TIDs Boosted
                  </div>
                  <div className="mt-0.5 font-mono text-lg font-bold">
                    {status.actuators.boostedTids}{' '}
                    <span className="text-xs font-normal" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                      ({status.actuators.threadUclampMin})
                    </span>
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Storage Read-Ahead
                  </div>
                  <div className="mt-0.5 font-mono text-lg font-bold">
                    {status.actuators.readAheadKb > 0 ? `${status.actuators.readAheadKb} KB` : 'OEM'}
                  </div>
                </div>
              </div>

              <ThermalSparkline samples={status.history} />
            </section>

            {/* Shareable Session Impact & 4-Pillar Readiness */}
            <section className="md3-card-elevated space-y-4 p-5 sm:p-6">
              <div className="flex flex-wrap items-center justify-between gap-2">
                <div className="flex items-center gap-2">
                  <CheckCircle2 className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
                  <h2 className="text-base font-bold">Session Impact & Pillar Readiness</h2>
                </div>
                <span className="font-mono text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                  Uptime {formatDuration(status.session.uptimeSec)}
                </span>
              </div>

              <div className="grid grid-cols-2 gap-2.5 sm:grid-cols-4">
                <div className="md3-card-filled p-3">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Launch Bursts
                  </div>
                  <div className="mt-0.5 font-mono text-base font-bold">{status.session.launchBursts}x</div>
                </div>
                <div className="md3-card-filled p-3">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Smart Pre-Read
                  </div>
                  <div className="mt-0.5 font-mono text-base font-bold">
                    {formatBytes(status.actuators.prereadBytes)}
                  </div>
                </div>
                <div className="md3-card-filled p-3">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Avg / Peak SoC
                  </div>
                  <div className="mt-0.5 font-mono text-base font-bold">
                    {status.session.avgSocTempCelsius !== null
                      ? `${status.session.avgSocTempCelsius.toFixed(1)}° / ${(status.session.peakSocTempCelsius ?? status.session.avgSocTempCelsius).toFixed(1)}°`
                      : 'N/A'}
                  </div>
                </div>
                <div className="md3-card-filled p-3">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Thermal Guards
                  </div>
                  <div className="mt-0.5 font-mono text-base font-bold">
                    {status.session.thermalGuardEvents}x · CFJ4 {status.baselineNodesSnapshotted}
                  </div>
                </div>
              </div>

              <div className="grid grid-cols-1 gap-2 sm:grid-cols-2">
                {readiness.pillars.map((pillar) => (
                  <div
                    key={pillar.id}
                    className="flex items-center justify-between gap-2 rounded-2xl px-3.5 py-2.5"
                    style={{ background: 'var(--md-sys-color-surface-container-low)' }}
                  >
                    <div className="min-w-0">
                      <div className="text-xs font-semibold">{pillar.name}</div>
                      <div
                        className="truncate text-[11px]"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        {pillar.detail}
                      </div>
                    </div>
                    <span className={pillar.ready ? 'md3-badge md3-badge-success' : 'md3-badge md3-badge-warning'}>
                      {pillar.ready ? 'READY' : 'LIMITED'}
                    </span>
                  </div>
                ))}
              </div>
            </section>

            {/* Profile Selector */}
            <section className="md3-card-elevated space-y-3 p-5 sm:p-6">
              <div className="flex items-center gap-2">
                <Zap className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
                <h2 className="text-base font-bold">Performance Profile</h2>
              </div>
              <div className="grid grid-cols-1 gap-2.5 sm:grid-cols-2">
                {(['balanced', 'smoothness', 'battery', 'gaming'] as PerformanceProfile[]).map((p) => {
                  const active = status.profile === p;
                  return (
                    <button
                      key={p}
                      type="button"
                      disabled={savingKey !== null}
                      onClick={() => handleSelectProfile(p)}
                      className="rounded-2xl p-3.5 text-left transition-all"
                      style={{
                        background: active
                          ? 'var(--md-sys-color-primary-container)'
                          : 'var(--md-sys-color-surface-container)',
                        color: active
                          ? 'var(--md-sys-color-on-primary-container)'
                          : 'var(--md-sys-color-on-surface)',
                        border: active
                          ? '1.5px solid var(--md-sys-color-primary)'
                          : '1px solid var(--md-sys-color-outline-variant)',
                      }}
                    >
                      <div className="flex items-center justify-between">
                        <span className="text-sm font-bold">{PROFILE_META[p].label}</span>
                        {active && <span className="md3-badge md3-badge-success">ACTIVE</span>}
                      </div>
                      <p className="mt-1 text-xs opacity-85">{PROFILE_META[p].tagline}</p>
                    </button>
                  );
                })}
              </div>
            </section>

            {/* 5 Core Feature Toggles */}
            <section className="md3-card-elevated space-y-3 p-5 sm:p-6">
              <div className="flex items-center gap-2">
                <Cpu className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
                <h2 className="text-base font-bold">Core Adaptive Features</h2>
              </div>
              <div className="divide-y" style={{ borderColor: 'var(--md-sys-color-outline-variant)' }}>
                {featureItems.map(({ configKey, field, title, desc, Icon }) => {
                  const enabled = status.features[field];
                  return (
                    <div
                      key={configKey}
                      className="flex items-center justify-between gap-4 py-3 first:pt-1 last:pb-1"
                    >
                      <div className="flex items-start gap-3">
                        <Icon className="mt-0.5 h-4 w-4 shrink-0" />
                        <div>
                          <div className="text-sm font-semibold">{title}</div>
                          <p
                            className="text-xs leading-relaxed"
                            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                          >
                            {desc}
                          </p>
                        </div>
                      </div>
                      <button
                        type="button"
                        role="switch"
                        aria-checked={enabled}
                        disabled={savingKey !== null}
                        onClick={() => handleToggleFeature(configKey, field, title)}
                        className={enabled ? 'md3-btn-filled shrink-0' : 'md3-btn-tonal shrink-0'}
                      >
                        {enabled ? 'ON' : 'OFF'}
                      </button>
                    </div>
                  );
                })}
              </div>
            </section>
          </div>
        )}

        {activeTab === 'apps' && (
          <div className="space-y-5">
            {/* Active Foreground App Quick Actions */}
            {status.foregroundPackage && (
              <section className="md3-card-elevated flex flex-wrap items-center justify-between gap-3 p-5">
                <div>
                  <div className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Detected Foreground App
                  </div>
                  <div className="font-mono text-sm font-bold">{status.foregroundPackage}</div>
                </div>
                <div className="flex flex-wrap gap-1.5">
                  {RULE_CATEGORY_OPTIONS.map((opt) => {
                    const current = getPackageRuleCategory(status.apps, status.foregroundPackage) === opt.id;
                    return (
                      <button
                        key={opt.id}
                        type="button"
                        disabled={savingKey !== null}
                        className={current ? 'md3-btn-filled' : 'md3-btn-tonal'}
                        onClick={() => handleAssignAppRule(status.foregroundPackage, opt.id)}
                      >
                        {opt.label}
                      </button>
                    );
                  })}
                </div>
              </section>
            )}

            {/* Installed App Picker (1-Tap Per-App Profile Assignment) */}
            <section className="md3-card-elevated space-y-3 p-5 sm:p-6">
              <div className="flex flex-wrap items-center justify-between gap-2">
                <div className="flex items-center gap-2">
                  <AppWindow className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
                  <h2 className="text-base font-bold">Installed App Picker ({installedApps.length})</h2>
                </div>
                <button
                  type="button"
                  disabled={loadingApps}
                  className="md3-btn-tonal"
                  onClick={() => void loadInstalledApps()}
                >
                  <RefreshCw className={`h-3.5 w-3.5 ${loadingApps ? 'animate-spin' : ''}`} />
                  <span>{loadingApps ? 'Scanning...' : 'Refresh Apps'}</span>
                </button>
              </div>
              <p className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                Assign per-app profiles without typing package names: <strong>Gaming</strong>, <strong>Smooth</strong>, <strong>Battery</strong>, or <strong>Stock</strong> (OEM untouched).
              </p>
              <div className="relative">
                <Search
                  className="pointer-events-none absolute left-3 top-1/2 h-4 w-4 -translate-y-1/2"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                />
                <input
                  type="text"
                  value={appSearch}
                  onChange={(e) => setAppSearch(e.target.value)}
                  placeholder="Search installed package (e.g. genshin, instagram, whatsapp)..."
                  className="w-full rounded-xl py-2 pl-9 pr-3 font-mono text-xs"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                />
              </div>

              <div className="max-h-80 space-y-2 overflow-y-auto pr-1">
                {filteredInstalledApps.length === 0 ? (
                  <div className="py-4 text-center text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    {loadingApps ? 'Loading installed apps...' : 'No matching packages found. Use manual entry below.'}
                  </div>
                ) : (
                  filteredInstalledApps.slice(0, 60).map((pkg) => {
                    const currentCat = getPackageRuleCategory(status.apps, pkg);
                    return (
                      <div
                        key={pkg}
                        className="flex flex-wrap items-center justify-between gap-2 rounded-2xl px-3.5 py-2.5"
                        style={{ background: 'var(--md-sys-color-surface-container-low)' }}
                      >
                        <span className="truncate font-mono text-xs font-medium">{pkg}</span>
                        <div className="flex flex-wrap gap-1">
                          {RULE_CATEGORY_OPTIONS.map((opt) => {
                            const active = currentCat === opt.id;
                            return (
                              <button
                                key={opt.id}
                                type="button"
                                disabled={savingKey !== null}
                                onClick={() => handleAssignAppRule(pkg, opt.id)}
                                className="rounded-lg px-2 py-1 text-[11px] font-semibold transition-colors"
                                style={{
                                  background: active
                                    ? 'var(--md-sys-color-primary)'
                                    : 'var(--md-sys-color-surface-container-high)',
                                  color: active
                                    ? 'var(--md-sys-color-on-primary)'
                                    : 'var(--md-sys-color-on-surface-variant)',
                                }}
                              >
                                {opt.label}
                              </button>
                            );
                          })}
                        </div>
                      </div>
                    );
                  })
                )}
              </div>
            </section>

            {/* Manual Package Entry + Active Rule Buckets */}
            <section className="md3-card-elevated space-y-4 p-5 sm:p-6">
              <h2 className="text-base font-bold">Active Overrides & Manual Entry</h2>
              <div className="flex flex-wrap gap-2">
                <input
                  type="text"
                  value={manualPkg}
                  onChange={(e) => setManualPkg(e.target.value)}
                  placeholder="com.example.package"
                  className="min-w-[180px] flex-1 rounded-xl px-3 py-2 font-mono text-xs"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                />
                <select
                  value={manualCategory}
                  onChange={(e) =>
                    setManualCategory(e.target.value as Exclude<AppRuleCategory, 'default'>)
                  }
                  className="rounded-xl px-3 py-2 text-xs font-semibold"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                >
                  <option value="gaming">Gaming</option>
                  <option value="smoothness">Smoothness</option>
                  <option value="battery">Battery</option>
                  <option value="excluded">Excluded (OEM)</option>
                </select>
                <button
                  type="button"
                  className="md3-btn-filled"
                  onClick={() => {
                    handleAssignAppRule(manualPkg, manualCategory);
                    setManualPkg('');
                  }}
                >
                  <Plus className="h-4 w-4" />
                  <span>Add</span>
                </button>
              </div>

              <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
                {ruleBuckets.map(({ key, title, badgeClass, Icon }) => {
                  const list = status.apps[key];
                  return (
                    <div
                      key={key}
                      className="rounded-2xl p-3.5 space-y-2"
                      style={{ background: 'var(--md-sys-color-surface-container-low)' }}
                    >
                      <div className="flex items-center gap-2 text-xs font-bold">
                        <Icon className="h-3.5 w-3.5" style={{ color: 'var(--md-sys-color-primary)' }} />
                        <span>{title}</span>
                        <span className="ml-auto font-mono opacity-70">{list.length}</span>
                      </div>
                      <div className="flex flex-wrap gap-1.5">
                        {list.length === 0 ? (
                          <span className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                            None configured
                          </span>
                        ) : (
                          list.map((pkg) => (
                            <span key={pkg} className={`${badgeClass} gap-1.5 font-mono`}>
                              {pkg}
                              <button
                                type="button"
                                aria-label={`Remove ${pkg}`}
                                onClick={() => handleAssignAppRule(pkg, 'default')}
                              >
                                <Trash2 className="h-3 w-3" />
                              </button>
                            </span>
                          ))
                        )}
                      </div>
                    </div>
                  );
                })}
              </div>
            </section>
          </div>
        )}

        {activeTab === 'system' && (
          <div className="space-y-5">
            {/* Hardware Probe Summary */}
            <section className="md3-card-elevated space-y-4 p-5 sm:p-6">
              <div className="flex flex-wrap items-center justify-between gap-2">
                <h2 className="text-base font-bold">Discovered Hardware & Capabilities</h2>
                <div className="flex flex-wrap gap-1.5">
                  <span
                    className={
                      status.actuators.perThreadUclampSupported
                        ? 'md3-badge md3-badge-success'
                        : 'md3-badge md3-badge-warning'
                    }
                  >
                    {status.actuators.perThreadUclampSupported
                      ? 'Per-TID UClamp Ready'
                      : 'Per-TID UClamp Unavailable'}
                  </span>
                  <span
                    className={
                      status.hardware.mglruAvailable
                        ? 'md3-badge md3-badge-success'
                        : 'md3-badge md3-badge-neutral'
                    }
                  >
                    {status.hardware.mglruAvailable ? 'MGLRU Active' : 'No MGLRU'}
                  </span>
                </div>
              </div>
              <div className="grid grid-cols-2 gap-3 sm:grid-cols-3">
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    SoC / Platform
                  </div>
                  <div className="mt-0.5 truncate font-mono text-sm font-semibold">
                    {status.hardware.socModel}
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Storage Media
                  </div>
                  <div className="mt-0.5 font-mono text-sm font-semibold">
                    {status.hardware.storageType} · {status.actuators.prereadFiles} prefetched
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    GPU Surface
                  </div>
                  <div className="mt-0.5 truncate font-mono text-sm font-semibold">
                    {status.hardware.gpuModel}
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    RAM / ZRAM
                  </div>
                  <div className="mt-0.5 font-mono text-sm font-semibold">
                    {formatBytes(status.hardware.ramBytes)} / {formatBytes(status.hardware.zramBytes)}
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    CPU Clusters / Hz
                  </div>
                  <div className="mt-0.5 font-mono text-sm font-semibold">
                    {status.hardware.cpuClusters} clusters · {status.hardware.refreshHz} Hz
                  </div>
                </div>
                <div className="md3-card-filled p-3.5">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    Battery / Power
                  </div>
                  <div className="mt-0.5 font-mono text-sm font-semibold">
                    {status.batteryPower.available
                      ? `${status.batteryPower.status} · ${Math.round(status.batteryPower.currentMa)} mA`
                      : 'Unavailable'}
                  </div>
                </div>
              </div>

              {status.actuators.contendedPaths.length > 0 && (
                <div
                  className="rounded-2xl p-3.5 text-xs space-y-1.5"
                  style={{
                    background: 'var(--md-sys-color-warning-container)',
                    color: 'var(--md-sys-color-on-warning-container)',
                  }}
                >
                  <div className="font-bold">
                    OEM Coexistence Yield ({status.actuators.contendedPaths.length} nodes in auto-cooldown)
                  </div>
                  <p className="opacity-90">
                    CoreFlow yielded cleanly to an OEM daemon on these sysfs nodes and will automatically retry after cooldown if the vendor daemon settles:
                  </p>
                  <ul className="list-disc pl-4 font-mono text-[11px]">
                    {status.actuators.contendedPaths.map((p) => (
                      <li key={p}>{p}</li>
                    ))}
                  </ul>
                </div>
              )}
            </section>

            {/* Safety & CFJ4 Rollback */}
            <BaselineGuardianCard
              nodesSnapshotted={status.baselineNodesSnapshotted}
              nativeMode={isNativeBridge}
              baselineStatus={status.baselineNodesSnapshotted > 0 ? 'protected' : 'restored'}
            />

            <section className="md3-card-elevated space-y-3 p-5 sm:p-6">
              <div className="flex flex-wrap items-center justify-between gap-2">
                <h2 className="text-base font-bold">CFJ4 Audit, Diagnostics & Emergency Restore</h2>
                <div className="flex flex-wrap gap-2">
                  <button type="button" className="md3-btn-tonal" onClick={() => void handleViewJournal()}>
                    <FileJson className="h-4 w-4" />
                    <span>Inspect CFJ4</span>
                  </button>
                  <button type="button" className="md3-btn-tonal" onClick={() => void handleExportDiag()}>
                    <ClipboardCopy className="h-4 w-4" />
                    <span>Export Diag</span>
                  </button>
                  {status.running ? (
                    <button
                      type="button"
                      disabled={restoring}
                      className="md3-btn-filled"
                      onClick={() => void handleRestoreBaseline()}
                    >
                      <RotateCcw className="h-4 w-4" />
                      <span>{restoring ? 'Restoring...' : 'Restore OEM Baseline'}</span>
                    </button>
                  ) : (
                    <button
                      type="button"
                      disabled={starting}
                      className="md3-btn-filled"
                      onClick={() => void handleStartDaemon()}
                    >
                      <Play className="h-4 w-4" />
                      <span>{starting ? 'Starting...' : 'Start CoreFlow'}</span>
                    </button>
                  )}
                </div>
              </div>
              {journalJson && (
                <pre
                  className="max-h-56 overflow-auto rounded-2xl p-3 font-mono text-[11px]"
                  style={{ background: 'var(--md-sys-color-surface-container-low)' }}
                >
                  {journalJson}
                </pre>
              )}
              {diagJson && (
                <pre
                  className="max-h-56 overflow-auto rounded-2xl p-3 font-mono text-[11px]"
                  style={{ background: 'var(--md-sys-color-surface-container-low)' }}
                >
                  {diagJson}
                </pre>
              )}
            </section>

            <LiveReactionStream logs={logs} nativeMode={isNativeBridge} />
          </div>
        )}
      </main>

      <footer
        className="mt-6 py-4 text-center text-xs"
        style={{
          borderTop: '1px solid var(--md-sys-color-outline-variant)',
          color: 'var(--md-sys-color-on-surface-variant)',
        }}
      >
        CoreFlow Engine v5.1.0 · Android 13+ Lean 4-Pillar Adaptive Framework · CFJ4 OEM Safe
      </footer>
    </div>
  );
};

export default App;
