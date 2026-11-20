import React, { useCallback, useEffect, useMemo, useState } from 'react';
import {
  Activity,
  AppWindow,
  BatteryCharging,
  Cpu,
  Flame,
  Gamepad2,
  HardDrive,
  MemoryStick,
  Plus,
  RotateCcw,
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
  buildSafeSetConfigArg,
  computeHealthScore,
  CoreFlowFeatures,
  CoreFlowV5Status,
  createPreviewStatus,
  HistorySample,
  isValidPackageName,
  MODE_LABELS,
  parseV5Status,
  PerformanceProfile,
  PROFILE_META,
  ReactionLogEvent,
  sanitizePackageList,
  stepPreviewStatus,
} from './engine/autonomousEngine';

const RESOLVE_BIN =
  'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"';
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
  if (!Number.isFinite(bytes) || bytes <= 0) return 'N/A';
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
  const [restoring, setRestoring] = useState(false);
  const [logs, setLogs] = useState<ReactionLogEvent[]>([]);
  const [newGamingPkg, setNewGamingPkg] = useState('');
  const [newExcludedPkg, setNewExcludedPkg] = useState('');

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

  useEffect(() => {
    if (!isNativeBridge) {
      let tick = 1;
      const timer = window.setInterval(() => {
        setStatus((prev) => stepPreviewStatus(prev, tick++));
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

  const applyConfigUpdate = async (
    key: string,
    value: string,
    optimisticUpdate: (prev: CoreFlowV5Status) => CoreFlowV5Status,
    successNote: string
  ) => {
    const safeArg = buildSafeSetConfigArg(key, value);
    if (!safeArg) {
      setErrorMsg(`Invalid configuration value for ${key}`);
      return;
    }
    setSavingKey(key);
    setFeedback(null);
    setErrorMsg(null);
    try {
      if (isNativeBridge) {
        await execChecked(`${RESOLVE_BIN}; "$BIN" --set-config ${safeArg}`);
      }
      setStatus(optimisticUpdate);
      setFeedback(successNote);
      addLog('BOOST', 'Config', successNote);
      if (isNativeBridge) {
        window.setTimeout(() => void refreshNative().catch(() => {}), 450);
      }
    } catch (err) {
      setErrorMsg(`Failed to apply ${key}: ${String(err)}`);
    } finally {
      setSavingKey(null);
    }
  };

  const handleSelectProfile = (profile: PerformanceProfile) => {
    void applyConfigUpdate(
      'profile',
      profile,
      (prev) => ({ ...prev, profile }),
      `Profile switched to ${PROFILE_META[profile].label}`
    );
  };

  const handleToggleFeature = (
    configKey: string,
    featureField: keyof CoreFlowFeatures,
    label: string
  ) => {
    const nextVal = !status.features[featureField];
    void applyConfigUpdate(
      configKey,
      nextVal ? 'true' : 'false',
      (prev) => ({
        ...prev,
        features: { ...prev.features, [featureField]: nextVal },
      }),
      `${label} ${nextVal ? 'enabled' : 'disabled'}`
    );
  };

  const updateAppList = (listKind: 'gaming' | 'excluded', nextList: string[], note: string) => {
    const sanitized = sanitizePackageList(nextList);
    const configKey = listKind === 'gaming' ? 'gaming_apps' : 'excluded_apps';
    void applyConfigUpdate(
      configKey,
      sanitized.join(','),
      (prev) => ({
        ...prev,
        apps: { ...prev.apps, [listKind]: sanitized },
      }),
      note
    );
  };

  const handleAddPackage = (listKind: 'gaming' | 'excluded', rawPkg: string) => {
    const pkg = rawPkg.trim();
    if (!isValidPackageName(pkg)) {
      setErrorMsg('Enter a valid Android package name (e.g. com.miHoYo.GenshinImpact)');
      return;
    }
    const existing = status.apps[listKind];
    updateAppList(listKind, [...existing, pkg], `Added ${pkg} to ${listKind} list`);
    if (listKind === 'gaming') setNewGamingPkg('');
    else setNewExcludedPkg('');
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
      setFeedback('OEM baseline restored.');
    } catch (err) {
      setErrorMsg(`Restore failed: ${String(err)}`);
    } finally {
      setRestoring(false);
    }
  };

  const healthScore = useMemo(() => computeHealthScore(status), [status]);
  const modeMeta = MODE_LABELS[status.mode] ?? { label: status.mode, tone: 'neutral' as const };
  const modeBadgeClass =
    modeMeta.tone === 'error'
      ? 'md3-badge md3-badge-error'
      : modeMeta.tone === 'warning'
        ? 'md3-badge md3-badge-warning'
        : modeMeta.tone === 'success'
          ? 'md3-badge md3-badge-success'
          : 'md3-badge md3-badge-neutral';

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
      desc: 'Boosts SurfaceFlinger, RenderThread & UI threads via sched_setattr without fighting PowerHAL.',
      Icon: Sparkles,
    },
    {
      configKey: 'enable_storage_burst',
      field: 'storageBurst',
      title: 'UFS / eMMC Launch Burst & QoS',
      desc: 'Dynamic read_ahead_kb burst on app launch + isolates background I/O & CPU contention.',
      Icon: HardDrive,
    },
    {
      configKey: 'enable_memory_guard',
      field: 'memoryGuard',
      title: 'Anti-Stutter Memory & Pre-Read',
      desc: 'MGLRU working-set guard, zero swap thrash, and bounded posix_fadvise app asset prefetch.',
      Icon: MemoryStick,
    },
    {
      configKey: 'enable_thermal_pacing',
      field: 'thermalPacing',
      title: 'Thermal PID Smooth Pacing',
      desc: 'Rust PID + 10s thermal lookahead scales CPU/GPU envelopes smoothly before vendor cliff-throttling.',
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
            Browser Preview Mode — interactive demo. On a rooted Android device (KernelSU / APatch / Magisk), changes apply live to <code>coreflowd</code>.
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
            {/* Live Context Hero */}
            <section className="md3-card-elevated space-y-4 p-5 sm:p-6">
              <div className="flex flex-wrap items-start justify-between gap-3">
                <div>
                  <div className="flex flex-wrap items-center gap-2">
                    <span className={modeBadgeClass}>{modeMeta.label}</span>
                    <span className="md3-badge md3-badge-neutral font-mono">
                      {PROFILE_META[status.profile].label.toUpperCase()}
                    </span>
                    {status.actuators.storageBurstActive && (
                      <span className="md3-badge md3-badge-success">UFS BURST</span>
                    )}
                  </div>
                  <h1 className="mt-2 text-lg font-bold tracking-tight sm:text-xl">
                    {status.foregroundPackage || 'System Idle / No Foreground App'}
                  </h1>
                  <p className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    {status.running
                      ? `PID ${status.pid} · ${status.hardware.storageType} storage · ${status.hardware.refreshHz} Hz display`
                      : 'Daemon stopped — OEM factory baseline active'}
                  </p>
                </div>
                <div className="text-right">
                  <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    PID Capacity Budget
                  </div>
                  <div className="font-mono text-2xl font-bold tabular-nums">
                    {Math.round(status.capacityScale * 100)}%
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
                <div className="flex flex-wrap gap-2">
                  <button
                    type="button"
                    className="md3-btn-filled"
                    onClick={() => handleAddPackage('gaming', status.foregroundPackage)}
                  >
                    <Gamepad2 className="h-4 w-4" />
                    <span>Mark Gaming</span>
                  </button>
                  <button
                    type="button"
                    className="md3-btn-tonal"
                    onClick={() => handleAddPackage('excluded', status.foregroundPackage)}
                  >
                    <ShieldAlert className="h-4 w-4" />
                    <span>Exclude (OEM Stock)</span>
                  </button>
                </div>
              </section>
            )}

            {/* Gaming Apps */}
            <section className="md3-card-elevated space-y-3 p-5 sm:p-6">
              <div className="flex items-center gap-2">
                <Gamepad2 className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
                <h2 className="text-base font-bold">Gaming Apps (Forced Gaming Mode)</h2>
              </div>
              <p className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                Apps listed here immediately enter sustained Gaming pacing with strict background isolation and PID thermal protection.
              </p>
              <div className="flex gap-2">
                <input
                  type="text"
                  value={newGamingPkg}
                  onChange={(e) => setNewGamingPkg(e.target.value)}
                  placeholder="com.miHoYo.GenshinImpact"
                  className="flex-1 rounded-xl px-3 py-2 font-mono text-xs"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                />
                <button
                  type="button"
                  className="md3-btn-filled"
                  onClick={() => handleAddPackage('gaming', newGamingPkg)}
                >
                  <Plus className="h-4 w-4" />
                  <span>Add</span>
                </button>
              </div>
              <div className="flex flex-wrap gap-2">
                {status.apps.gaming.length === 0 ? (
                  <span className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    No custom gaming overrides yet (auto-detected via GPU load).
                  </span>
                ) : (
                  status.apps.gaming.map((pkg) => (
                    <span key={pkg} className="md3-badge md3-badge-success gap-1.5 font-mono">
                      {pkg}
                      <button
                        type="button"
                        aria-label={`Remove ${pkg}`}
                        onClick={() =>
                          updateAppList(
                            'gaming',
                            status.apps.gaming.filter((x) => x !== pkg),
                            `Removed ${pkg} from gaming list`
                          )
                        }
                      >
                        <Trash2 className="h-3 w-3" />
                      </button>
                    </span>
                  ))
                )}
              </div>
            </section>

            {/* Excluded Apps */}
            <section className="md3-card-elevated space-y-3 p-5 sm:p-6">
              <div className="flex items-center gap-2">
                <ShieldAlert className="h-4 w-4" style={{ color: 'var(--md-sys-color-warning)' }} />
                <h2 className="text-base font-bold">Excluded Apps (Pure OEM Stock)</h2>
              </div>
              <p className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                CoreFlow yields completely to stock OEM defaults when these apps are in the foreground (ideal for Camera or Banking apps).
              </p>
              <div className="flex gap-2">
                <input
                  type="text"
                  value={newExcludedPkg}
                  onChange={(e) => setNewExcludedPkg(e.target.value)}
                  placeholder="com.android.camera"
                  className="flex-1 rounded-xl px-3 py-2 font-mono text-xs"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                />
                <button
                  type="button"
                  className="md3-btn-tonal"
                  onClick={() => handleAddPackage('excluded', newExcludedPkg)}
                >
                  <Plus className="h-4 w-4" />
                  <span>Add</span>
                </button>
              </div>
              <div className="flex flex-wrap gap-2">
                {status.apps.excluded.length === 0 ? (
                  <span className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    No excluded apps configured.
                  </span>
                ) : (
                  status.apps.excluded.map((pkg) => (
                    <span key={pkg} className="md3-badge md3-badge-neutral gap-1.5 font-mono">
                      {pkg}
                      <button
                        type="button"
                        aria-label={`Remove ${pkg}`}
                        onClick={() =>
                          updateAppList(
                            'excluded',
                            status.apps.excluded.filter((x) => x !== pkg),
                            `Removed ${pkg} from excluded list`
                          )
                        }
                      >
                        <Trash2 className="h-3 w-3" />
                      </button>
                    </span>
                  ))
                )}
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
            </section>

            {/* Safety & CFJ4 Rollback */}
            <BaselineGuardianCard
              nodesSnapshotted={status.baselineNodesSnapshotted}
              nativeMode={isNativeBridge}
              baselineStatus={status.baselineNodesSnapshotted > 0 ? 'protected' : 'restored'}
            />

            <section className="md3-card-elevated space-y-3 p-5 sm:p-6">
              <div className="flex flex-wrap items-center justify-between gap-2">
                <h2 className="text-base font-bold">CFJ4 Baseline Audit & Emergency Restore</h2>
                <div className="flex flex-wrap gap-2">
                  <button type="button" className="md3-btn-tonal" onClick={() => void handleViewJournal()}>
                    Inspect CFJ4 Journal
                  </button>
                  <button
                    type="button"
                    disabled={restoring}
                    className="md3-btn-filled"
                    onClick={() => void handleRestoreBaseline()}
                  >
                    <RotateCcw className="h-4 w-4" />
                    <span>{restoring ? 'Restoring...' : 'Restore OEM Baseline'}</span>
                  </button>
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
        CoreFlow Engine v5.0.0 · Lean 4-Pillar Adaptive Android Framework · CFJ4 OEM Safe
      </footer>
    </div>
  );
};

export default App;

