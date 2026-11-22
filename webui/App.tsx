import React, { useCallback, useEffect, useMemo, useState } from 'react';
import {
  Activity,
  AppWindow,
  Battery,
  BatteryCharging,
  ClipboardCopy,
  ExternalLink,
  FileJson,
  Flame,
  Gamepad2,
  HardDrive,
  MemoryStick,
  Pause,
  Play,
  Plus,
  Radio,
  RefreshCw,
  RotateCcw,
  Search,
  Settings,
  ShieldAlert,
  Sliders,
  Sparkles,
  Trash2,
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
  '[ -x "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
  '[ -x "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -x "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -x "$BIN" ] || BIN="/system/bin/coreflowd"';
const RESOLVE_SERVICE =
  'SVC="/data/adb/modules_update/cfae_framework/service.sh"; ' +
  '[ -f "$SVC" ] || SVC="/data/adb/modules/cfae_framework/service.sh"; ' +
  '[ -f "$SVC" ] || SVC="/data/adb/modules/coreflow_engine/service.sh"';
const LOG_PATH = '/data/adb/coreflow/logs/coreflowd.log';
const LANG_KEY = 'coreflow_webui_lang';

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

const formatClock = (tsMs: number): string => {
  if (!tsMs || tsMs <= 0) return '--:--:--';
  const d = new Date(tsMs);
  return d.toTimeString().slice(0, 8);
};

const formatTrend = (vel: number, isId: boolean): string => {
  if (vel >= 0.6) return `${isId ? 'Naik' : 'Rising'} +${vel.toFixed(1)}°/m`;
  if (vel <= -0.6) return `${isId ? 'Turun' : 'Cooling'} ${vel.toFixed(1)}°/m`;
  return isId ? 'Stabil' : 'Stable';
};

const DualSparkline: React.FC<{
  samples: HistorySample[];
  trendLabel: string;
  isId: boolean;
}> = ({ samples, trendLabel, isId }) => {
  const valid = samples.filter((s) => s.temp !== null && Number.isFinite(s.temp));
  if (valid.length < 2) {
    return (
      <div
        className="flex h-16 items-center justify-center rounded-xl text-xs"
        style={{
          background: 'var(--md-sys-color-surface-container)',
          color: 'var(--md-sys-color-on-surface-variant)',
        }}
      >
        {isId ? 'Mengumpulkan telemetri termal live...' : 'Collecting live thermal telemetry...'}
      </div>
    );
  }
  const temps = valid.map((s) => s.temp as number);
  const minT = Math.max(20, Math.floor(Math.min(...temps) - 3));
  const maxT = Math.max(minT + 10, Math.ceil(Math.max(...temps) + 3));
  const span = Math.max(1, maxT - minT);
  const width = 320;
  const height = 64;

  const tempPoints = valid
    .map((s, idx) => {
      const x = (idx / (valid.length - 1)) * width;
      const y = height - (((s.temp as number) - minT) / span) * (height - 10) - 5;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');

  const scalePoints = valid
    .map((s, idx) => {
      const x = (idx / (valid.length - 1)) * width;
      const y = height - Math.max(0.2, Math.min(1, s.scale)) * (height - 10) - 5;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');

  return (
    <div
      className="rounded-xl p-3"
      style={{
        background: 'var(--md-sys-color-surface-container)',
        border: '1px solid var(--md-sys-color-outline-variant)',
      }}
    >
      <div className="mb-1.5 flex items-center justify-between gap-2 text-[11px]">
        <div className="flex items-center gap-3 min-w-0">
          <span className="inline-flex items-center gap-1.5 font-medium truncate">
            <span className="h-2 w-2 rounded-full shrink-0" style={{ background: '#f59e0b' }} />
            <span className="font-mono font-semibold tabular-nums">
              {temps[temps.length - 1].toFixed(1)}°C
            </span>
          </span>
          <span className="inline-flex items-center gap-1.5 font-medium truncate">
            <span className="h-2 w-2 rounded-full shrink-0" style={{ background: '#38bdf8' }} />
            <span className="font-mono font-semibold tabular-nums">
              {Math.round(valid[valid.length - 1].scale * 100)}%
            </span>
          </span>
        </div>
        <span
          className="font-mono text-[10px] font-medium shrink-0"
          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
        >
          {trendLabel} · {valid.length}s
        </span>
      </div>
      <svg viewBox={`0 0 ${width} ${height}`} className="h-14 w-full overflow-visible">
        <line x1="0" y1={height * 0.25} x2={width} y2={height * 0.25} stroke="currentColor" strokeOpacity="0.1" />
        <line x1="0" y1={height * 0.75} x2={width} y2={height * 0.75} stroke="currentColor" strokeOpacity="0.1" />
        <polyline
          fill="none"
          stroke="#38bdf8"
          strokeWidth="2"
          strokeLinecap="round"
          strokeLinejoin="round"
          points={scalePoints}
        />
        <polyline
          fill="none"
          stroke="#f59e0b"
          strokeWidth="2.4"
          strokeLinecap="round"
          strokeLinejoin="round"
          points={tempPoints}
        />
      </svg>
    </div>
  );
};

type NavTab = 'home' | 'control' | 'live' | 'apps' | 'system';

const RULE_CATEGORY_OPTIONS: { id: AppRuleCategory; label: string }[] = [
  { id: 'default', label: 'Auto' },
  { id: 'gaming', label: 'Gaming' },
  { id: 'smoothness', label: 'Smooth' },
  { id: 'battery', label: 'Battery' },
  { id: 'excluded', label: 'Stock' },
];

const AI_SPONSORS = [
  { name: 'Claude', badge: 'CL', role: 'Anthropic · Architecture', color: '#d97757', url: 'https://claude.ai' },
  { name: 'ChatGPT', badge: 'AI', role: 'OpenAI · Ideation', color: '#10a37f', url: 'https://chatgpt.com' },
  { name: 'Gemini', badge: 'GM', role: 'DeepMind · Engine & UI', color: '#4285f4', url: 'https://gemini.google.com' },
  { name: 'GitHub', badge: 'GH', role: 'CI/CD & Releases', color: '#8b5cf6', url: 'https://github.com/mystivara-creator/CoreFlow-Engine' },
];

const ACCENT_SEEDS: { id: AccentSeed; label: string; swatch: string }[] = [
  { id: 'ocean', label: 'Ocean', swatch: '#0b57d0' },
  { id: 'emerald', label: 'Emerald', swatch: '#006c4c' },
  { id: 'amber', label: 'Amber', swatch: '#b27b00' },
];

export const App: React.FC = () => {
  const [isNativeBridge] = useState(() => hasNativeKernelSuBridge());
  const [appearance, setAppearance] = useState(() => readAppearance());
  const [lang, setLang] = useState<'id' | 'en'>(() => {
    try {
      const saved = window.localStorage.getItem(LANG_KEY);
      if (saved === 'id' || saved === 'en') return saved;
      return navigator.language.toLowerCase().startsWith('id') || navigator.language.toLowerCase().startsWith('in')
        ? 'id'
        : 'en';
    } catch {
      return 'en';
    }
  });
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

  // Per-App Picker & filter state
  const [installedApps, setInstalledApps] = useState<string[]>(() =>
    hasNativeKernelSuBridge() ? [] : PREVIEW_INSTALLED_PACKAGES
  );
  const [loadingApps, setLoadingApps] = useState(false);
  const [appSearch, setAppSearch] = useState('');
  const [appFilter, setAppFilter] = useState<'all' | 'rules'>('all');
  const [manualPkg, setManualPkg] = useState('');
  const [manualCategory, setManualCategory] = useState<Exclude<AppRuleCategory, 'default'>>('gaming');

  const isId = lang === 'id';
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

  const toggleLang = () => {
    const next = lang === 'id' ? 'en' : 'id';
    setLang(next);
    try {
      window.localStorage.setItem(LANG_KEY, next);
    } catch {
      // ignore storage errors
    }
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
      // Manual fallback remains available
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
      isId
        ? `Profil diubah ke ${PROFILE_META[profile].label}`
        : `Profile switched to ${PROFILE_META[profile].label}`
    );
  };

  const handleTogglePause = () => {
    const nextPaused = !status.inspectionMode;
    void applyConfigUpdates(
      [['inspection_mode', nextPaused ? 'true' : 'false']],
      (prev) => ({ ...prev, inspectionMode: nextPaused }),
      nextPaused
        ? isId
          ? 'CoreFlow dijeda (Mode OEM A/B aktif)'
          : 'CoreFlow paused (OEM A/B compare active)'
        : isId
          ? 'CoreFlow Engine dilanjutkan'
          : 'CoreFlow Engine resumed'
    );
  };

  const handleTogglePowerFlag = (
    key: 'bypass_charging' | 'bypass_on_gaming',
    field: 'bypassCharging' | 'bypassOnGaming',
    label: string
  ) => {
    const nextVal = !status.powerConfig[field];
    void applyConfigUpdates(
      [[key, nextVal ? 'true' : 'false']],
      (prev) => ({
        ...prev,
        powerConfig: { ...prev.powerConfig, [field]: nextVal },
        batteryPower:
          field === 'bypassCharging'
            ? { ...prev.batteryPower, bypassActive: nextVal }
            : prev.batteryPower,
      }),
      `${label} ${nextVal ? 'ON' : 'OFF'}`
    );
  };

  const handleChargeLimit = (pct: number) => {
    void applyConfigUpdates(
      [['charge_limit_pct', String(pct)]],
      (prev) => ({ ...prev, powerConfig: { ...prev.powerConfig, chargeLimitPct: pct } }),
      pct === 0
        ? isId
          ? 'Batas cas dimatikan'
          : 'Charge ceiling disabled'
        : isId
          ? `Batas cas diatur ke ${pct}%`
          : `Charge ceiling set to ${pct}%`
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
      addLog('BOOST', 'Daemon', 'CoreFlow v6.2.0 daemon started.');
      setFeedback(isId ? 'Daemon CoreFlow aktif.' : 'CoreFlow daemon started.');
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
      `${label} ${nextVal ? 'ON' : 'OFF'}`
    );
  };

  const handleAssignAppRule = (pkg: string, category: AppRuleCategory) => {
    const trimmed = pkg.trim();
    if (!isValidPackageName(trimmed)) {
      setErrorMsg(
        isId
          ? 'Masukkan nama paket Android yang valid (contoh: com.miHoYo.GenshinImpact)'
          : 'Enter a valid Android package name (e.g. com.miHoYo.GenshinImpact)'
      );
      return;
    }
    const nextApps: CoreFlowAppRules = assignPackageRule(status.apps, trimmed, category);
    void applyConfigUpdates(
      [
        ['gaming_apps', nextApps.gaming.join(',')],
        ['smoothness_apps', nextApps.smoothness.join(',')],
        ['battery_apps', nextApps.battery.join(',')],
        ['excluded_apps', nextApps.excluded.join(',')],
      ],
      (prev) => ({ ...prev, apps: nextApps }),
      `${trimmed} → ${category.toUpperCase()}`
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
            reportFormat: 'coreflow-diag-v6',
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
        setFeedback(isId ? 'Diagnostik diekspor & disalin ke clipboard.' : 'Diagnostic report copied to clipboard.');
      } else {
        setFeedback(isId ? 'Diagnostik siap di bawah.' : 'Diagnostic report generated below.');
      }
      setErrorMsg(null);
    } catch (err) {
      setErrorMsg(`Unable to export diagnostics: ${String(err)}`);
    }
  };

  const handleRestoreBaseline = async () => {
    if (
      !window.confirm(
        isId
          ? 'Pulihkan semua node kernel CoreFlow ke nilai bawaan OEM dan hentikan coreflowd?'
          : 'Restore all CoreFlow-managed kernel nodes to OEM baseline and stop coreflowd?'
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
      setFeedback(isId ? 'Baseline OEM berhasil dipulihkan.' : 'OEM baseline restored cleanly.');
    } catch (err) {
      setErrorMsg(`Restore failed: ${String(err)}`);
    } finally {
      setRestoring(false);
    }
  };

  const healthScore = useMemo(() => computeHealthScore(status), [status]);
  const readiness = useMemo(() => computePillarReadiness(status), [status]);
  const rawModeMeta = MODE_LABELS[status.mode] ?? { label: status.mode, labelId: status.mode, tone: 'neutral' as const };
  const modeLabel = status.inspectionMode
    ? isId
      ? 'Dijeda · OEM'
      : 'Paused · OEM'
    : isId
      ? rawModeMeta.labelId
      : rawModeMeta.label;

  const totalOverrides =
    status.apps.gaming.length +
    status.apps.smoothness.length +
    status.apps.battery.length +
    status.apps.excluded.length;

  const filteredInstalledApps = useMemo(() => {
    const q = appSearch.trim().toLowerCase();
    const list = installedApps.filter((pkg) => {
      if (appFilter === 'rules' && getPackageRuleCategory(status.apps, pkg) === 'default') return false;
      return !q || pkg.toLowerCase().includes(q);
    });
    return [...list].sort((a, b) => {
      const ra = getPackageRuleCategory(status.apps, a) === 'default' ? 1 : 0;
      const rb = getPackageRuleCategory(status.apps, b) === 'default' ? 1 : 0;
      return ra !== rb ? ra - rb : a.localeCompare(b);
    });
  }, [installedApps, appSearch, appFilter, status.apps]);

  const navTabs: { id: NavTab; label: string; Icon: React.FC<{ className?: string }> }[] = [
    { id: 'home', label: isId ? 'Beranda' : 'Home', Icon: Activity },
    { id: 'control', label: isId ? 'Kontrol' : 'Control', Icon: Sliders },
    { id: 'live', label: 'Live AI', Icon: Radio },
    { id: 'apps', label: isId ? 'Aplikasi' : 'Apps', Icon: AppWindow },
    { id: 'system', label: isId ? 'Setelan' : 'System', Icon: Settings },
  ];

  const trendText = formatTrend(status.thermalVelocityCpm, isId);
  const ramUsedGb = Math.max(0, status.memory.totalBytes - status.memory.availableBytes) / 1024 ** 3;
  const ramTotGb = status.memory.totalBytes / 1024 ** 3;
  const ramFrac = status.memory.totalBytes > 0 ? Math.min(1, ramUsedGb / Math.max(0.1, ramTotGb)) : 0;
  const zramUsedGb = Math.max(0, status.memory.swapTotalBytes - status.memory.swapFreeBytes) / 1024 ** 3;
  const zramTotGb = status.memory.swapTotalBytes / 1024 ** 3;
  const zramFrac = status.memory.swapTotalBytes > 0 ? Math.min(1, zramUsedGb / Math.max(0.1, zramTotGb)) : 0;

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
        lang={lang}
        onToggleLang={toggleLang}
        modeLabel={modeLabel}
        version={status.version}
        healthScore={healthScore}
        nativeMode={isNativeBridge}
        nativeConnected={nativeConnected}
      />

      {/* Proportional 5-Tab Segmented Bar (zero wrap even on 360px phones) */}
      <nav
        className="sticky top-14 z-20 px-3 py-2"
        style={{
          background: 'var(--md-sys-color-surface)',
          borderBottom: '1px solid var(--md-sys-color-outline-variant)',
        }}
      >
        <div className="mx-auto max-w-4xl">
          <div className="cf-seg-bar grid-cols-5">
            {navTabs.map(({ id, label, Icon }) => {
              const active = activeTab === id;
              return (
                <button
                  key={id}
                  type="button"
                  onClick={() => setActiveTab(id)}
                  className={`cf-seg-btn flex-col sm:flex-row gap-0.5 sm:gap-1.5 py-1.5 ${active ? 'is-active' : ''}`}
                >
                  <Icon className="h-3.5 w-3.5 shrink-0" />
                  <span className="text-[10px] sm:text-xs leading-none">{label}</span>
                </button>
              );
            })}
          </div>
        </div>
      </nav>

      <main className="mx-auto w-full max-w-4xl flex-1 space-y-4 px-3.5 py-4 sm:px-5">
        {!isNativeBridge && (
          <div
            className="rounded-xl px-3.5 py-2 text-[11px] font-medium leading-snug"
            style={{
              background: 'var(--md-sys-color-warning-container)',
              color: 'var(--md-sys-color-on-warning-container)',
            }}
          >
            {isId
              ? 'Mode Pratinjau Browser — di Android root (Magisk / KSU / APatch), semua kontrol terhubung langsung ke coreflowd.'
              : 'Browser Preview Mode — on rooted Android (Magisk / KSU / APatch), changes apply live to coreflowd.'}
          </div>
        )}

        {feedback && (
          <div
            className="rounded-xl px-3.5 py-2 text-xs font-medium"
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
            className="rounded-xl px-3.5 py-2 text-xs font-medium"
            style={{
              background: 'var(--md-sys-color-error-container)',
              color: 'var(--md-sys-color-on-error-container)',
            }}
          >
            {errorMsg}
          </div>
        )}

        {/* ========================= TAB 1: HOME ========================= */}
        {activeTab === 'home' && (
          <div className="space-y-4">
            {/* Hero Status Card */}
            <section className="md3-card-elevated space-y-3.5 p-4">
              <div className="flex items-start justify-between gap-2">
                <div className="min-w-0">
                  <div className="flex items-center gap-2">
                    <span
                      className="h-2.5 w-2.5 shrink-0 rounded-full"
                      style={{
                        background: !status.running
                          ? '#ef4444'
                          : status.inspectionMode || status.thermalGuard
                            ? '#f59e0b'
                            : status.bootSettleActive || status.mode === 'BOOT_SETTLE'
                              ? '#38bdf8'
                              : '#22c55e',
                      }}
                    />
                    <h1 className="truncate text-base font-bold tracking-tight">
                      {PROFILE_META[status.effectiveProfile].label} · {modeLabel}
                    </h1>
                  </div>
                  <p
                    className="mt-0.5 truncate font-mono text-[11px]"
                    style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                  >
                    {status.foregroundPackage || status.hardware.socModel}
                  </p>
                </div>
                <span className="md3-badge md3-badge-primary font-mono shrink-0">
                  {readiness.readyCount}/4 {isId ? 'Pilar' : 'Pillars'}
                </span>
              </div>

              {(status.bootSettleActive ||
                status.mode === 'BOOT_SETTLE' ||
                (status.running && status.bootRampProgress < 0.99)) && (
                <div
                  className="flex items-center justify-between gap-2 rounded-xl px-3 py-2 text-[11px] font-semibold"
                  style={{
                    background: 'rgba(56, 189, 248, 0.13)',
                    color: '#38bdf8',
                    border: '1px solid rgba(56, 189, 248, 0.25)',
                  }}
                >
                  <span className="truncate">
                    {status.bootSettleActive || status.mode === 'BOOT_SETTLE'
                      ? isId
                        ? `BOOT_SETTLE · Menenangkan termal pasca-boot (${Math.round(status.bootSettleDurationSec)}s · 0 tulis sysfs)`
                        : `BOOT_SETTLE · Post-boot thermal calm sanctuary (${Math.round(status.bootSettleDurationSec)}s · 0 sysfs writes)`
                      : isId
                        ? `Soft-Start Aktif · Otoritas aktuasi ${Math.round(status.bootRampProgress * 100)}%`
                        : `Soft-Start Active · Actuation authority ${Math.round(status.bootRampProgress * 100)}%`}
                  </span>
                  <span className="font-mono text-[10px] shrink-0">
                    {status.bootSettleActive || status.mode === 'BOOT_SETTLE'
                      ? 'SANCTUARY'
                      : `${Math.round(status.bootRampProgress * 100)}%`}
                  </span>
                </div>
              )}

              {/* 4 Uniform Metric Tiles */}
              <div className="grid grid-cols-2 gap-2 sm:grid-cols-4">
                <div className="cf-metric-tile">
                  <div className="cf-metric-label">
                    <span className="h-1.5 w-1.5 rounded-full" style={{ background: '#22c55e' }} />
                    SoC Temp
                  </div>
                  <div className="cf-metric-value">
                    {status.socTempCelsius !== null ? `${status.socTempCelsius.toFixed(1)}°C` : '--'}
                  </div>
                </div>
                <div className="cf-metric-tile">
                  <div className="cf-metric-label">
                    <span className="h-1.5 w-1.5 rounded-full" style={{ background: '#38bdf8' }} />
                    Render
                  </div>
                  <div className="cf-metric-value">{Math.round(status.renderPressure * 100)}%</div>
                </div>
                <div className="cf-metric-tile">
                  <div className="cf-metric-label">
                    <span className="h-1.5 w-1.5 rounded-full" style={{ background: '#a78bfa' }} />
                    UClamp
                  </div>
                  <div className="cf-metric-value">{status.actuators.threadUclampMin}</div>
                </div>
                <div className="cf-metric-tile">
                  <div className="cf-metric-label">
                    <span className="h-1.5 w-1.5 rounded-full" style={{ background: '#34d399' }} />
                    Headroom
                  </div>
                  <div className="cf-metric-value">{Math.round(status.capacityScale * 100)}%</div>
                </div>
              </div>

              {/* Quick Actions Row */}
              <div className="flex items-center gap-2">
                {status.running ? (
                  <>
                    <button
                      type="button"
                      disabled={savingKey !== null}
                      onClick={handleTogglePause}
                      className={`${status.inspectionMode ? 'md3-btn-filled' : 'md3-btn-tonal'} flex-1`}
                    >
                      {status.inspectionMode ? <Play className="h-3.5 w-3.5" /> : <Pause className="h-3.5 w-3.5" />}
                      <span>
                        {status.inspectionMode
                          ? isId
                            ? 'Lanjutkan'
                            : 'Resume'
                          : isId
                            ? 'Jeda (A/B)'
                            : 'Pause (A/B)'}
                      </span>
                    </button>
                    {status.batteryPower.bypassSupported && (
                      <button
                        type="button"
                        disabled={savingKey !== null}
                        onClick={() => handleTogglePowerFlag('bypass_charging', 'bypassCharging', 'Bypass Charging')}
                        className={`${status.batteryPower.bypassActive ? 'md3-btn-filled' : 'md3-btn-tonal'} flex-1`}
                      >
                        <BatteryCharging className="h-3.5 w-3.5" />
                        <span>
                          {status.batteryPower.bypassActive
                            ? isId
                              ? 'Bypass Aktif'
                              : 'Bypass ON'
                            : isId
                              ? 'Bypass Mati'
                              : 'Bypass OFF'}
                        </span>
                      </button>
                    )}
                  </>
                ) : (
                  <button
                    type="button"
                    disabled={starting}
                    onClick={() => void handleStartDaemon()}
                    className="md3-btn-filled w-full"
                  >
                    <Play className="h-3.5 w-3.5" />
                    <span>{starting ? '...' : isId ? 'Jalankan CoreFlow Engine' : 'Start CoreFlow Engine'}</span>
                  </button>
                )}
              </div>

              <DualSparkline samples={status.history} trendLabel={trendText} isId={isId} />
            </section>

            {/* 2x2 Proportional Autonomous Profile Grid */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'Profil Otonom' : 'Autonomous Profile'}</div>
              <div className="grid grid-cols-2 gap-2.5">
                {(['balanced', 'smoothness', 'gaming', 'battery'] as PerformanceProfile[]).map((p) => {
                  const active = status.profile === p;
                  return (
                    <button
                      key={p}
                      type="button"
                      disabled={savingKey !== null}
                      onClick={() => handleSelectProfile(p)}
                      className="flex flex-col justify-between rounded-2xl p-3.5 text-left"
                      style={{
                        background: active
                          ? 'var(--md-sys-color-primary-container)'
                          : 'var(--md-sys-color-surface-container-low)',
                        color: active
                          ? 'var(--md-sys-color-on-primary-container)'
                          : 'var(--md-sys-color-on-surface)',
                        border: active
                          ? '1.5px solid var(--md-sys-color-primary)'
                          : '1px solid var(--md-sys-color-outline-variant)',
                      }}
                    >
                      <div className="flex w-full items-center justify-between gap-1">
                        <span className="text-sm font-bold truncate">{PROFILE_META[p].label}</span>
                        {active && (
                          <span
                            className="h-2 w-2 rounded-full shrink-0"
                            style={{ background: 'var(--md-sys-color-primary)' }}
                          />
                        )}
                      </div>
                      <p className="mt-1 text-[11px] leading-snug opacity-80 line-clamp-2">
                        {isId ? PROFILE_META[p].taglineId : PROFILE_META[p].tagline}
                      </p>
                    </button>
                  );
                })}
              </div>
            </div>

            {/* Session Impact 2x3 Uniform Grid */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'Dampak Sesi' : 'Session Impact'}</div>
              <section className="md3-card-elevated p-3.5">
                <div className="grid grid-cols-3 gap-2">
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Uptime</div>
                    <div className="cf-metric-value">{formatDuration(status.session.uptimeSec)}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Bursts</div>
                    <div className="cf-metric-value">{status.session.launchBursts}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Pre-Read</div>
                    <div className="cf-metric-value">{formatBytes(status.actuators.prereadBytes)}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Avg / Peak</div>
                    <div className="cf-metric-value">
                      {status.session.avgSocTempCelsius !== null
                        ? `${Math.round(status.session.avgSocTempCelsius)}/${Math.round(
                            status.session.peakSocTempCelsius ?? status.session.avgSocTempCelsius
                          )}°`
                        : '--'}
                    </div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Guards</div>
                    <div className="cf-metric-value">{status.session.thermalGuardEvents}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">CFJ4</div>
                    <div className="cf-metric-value">{status.baselineNodesSnapshotted}</div>
                  </div>
                </div>
              </section>
            </div>
          </div>
        )}

        {/* ========================= TAB 2: CONTROL HUB ========================= */}
        {activeTab === 'control' && (
          <div className="space-y-4">
            {/* CPU Clusters */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'CPU & Penjadwal EAS' : 'CPU & EAS Scheduler'}</div>
              <section className="md3-card-elevated space-y-3 p-4">
                {status.cpuClusters.length > 0 && (
                  <div className="space-y-2.5">
                    {status.cpuClusters.map((c, idx) => {
                      const frac = c.maxKhz > 0 && c.curKhz > 0 ? Math.min(1, Math.max(0.05, c.curKhz / c.maxKhz)) : 0;
                      return (
                        <div key={`${c.role}-${idx}`} className="space-y-1">
                          <div className="flex items-center justify-between text-xs">
                            <span className="font-semibold uppercase">
                              {c.role} · <span className="font-mono font-normal opacity-80">{c.governor}</span>
                            </span>
                            <span className="font-mono text-[11px] tabular-nums opacity-85">
                              {c.curKhz > 0 ? `${Math.round(c.curKhz / 1000)} MHz` : '--'}
                              {c.maxKhz > 0 ? ` / ${Math.round(c.maxKhz / 1000)}` : ''}
                            </span>
                          </div>
                          <div className="md3-progress-track">
                            <div className="md3-progress-bar" style={{ width: `${Math.round(frac * 100)}%` }} />
                          </div>
                        </div>
                      );
                    })}
                  </div>
                )}

                <FeatureRow
                  Icon={Sparkles}
                  title="Per-TID Frame Smoothness"
                  desc={`UClamp ${status.actuators.threadUclampMin}..${status.actuators.threadUclampMax} (${status.actuators.boostedTids} TIDs)`}
                  enabled={status.features.frameSmoothness}
                  disabled={savingKey !== null}
                  onToggle={() => handleToggleFeature('enable_frame_smoothness', 'frameSmoothness', 'Frame Smoothness')}
                />
              </section>
            </div>

            {/* GPU & Thermal Zones */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'GPU & Zona Termal' : 'GPU & Thermal Zones'}</div>
              <section className="md3-card-elevated space-y-3 p-4">
                <div className="grid grid-cols-2 gap-2">
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">GPU</div>
                    <div className="cf-metric-value">{status.hardware.gpuModel || 'Devfreq'}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">GPU Busy</div>
                    <div className="cf-metric-value">
                      {status.gpuBusyPct !== null ? `${Math.round(status.gpuBusyPct)}%` : '--'}
                    </div>
                  </div>
                </div>
                {status.thermalZones.length > 0 && (
                  <div className="grid grid-cols-2 gap-2 sm:grid-cols-4">
                    {status.thermalZones.slice(0, 6).map((z, idx) => (
                      <div key={`${z.id}-${idx}`} className="cf-metric-tile">
                        <div className="cf-metric-label">{z.type}</div>
                        <div className="cf-metric-value">{z.celsius.toFixed(1)}°C</div>
                      </div>
                    ))}
                  </div>
                )}
                <FeatureRow
                  Icon={Flame}
                  title="Rust PID + RLS Thermal Pacing"
                  desc={`${Math.round(status.capacityScale * 100)}% headroom · ${status.actuators.managedEnvelopeNodes} nodes`}
                  enabled={status.features.thermalPacing}
                  disabled={savingKey !== null}
                  onToggle={() => handleToggleFeature('enable_thermal_pacing', 'thermalPacing', 'Thermal Pacing')}
                />
              </section>
            </div>

            {/* Power & CFJ4 Bypass Charging */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'Daya & Bypass Charging' : 'Power & Bypass Charging'}</div>
              <section className="md3-card-elevated space-y-3 p-4">
                {status.batteryPower.available && (
                  <div className="grid grid-cols-3 gap-2">
                    <div className="cf-metric-tile">
                      <div className="cf-metric-label">Battery</div>
                      <div className="cf-metric-value">
                        {status.batteryPower.capacityPct !== null
                          ? `${status.batteryPower.capacityPct}%`
                          : status.batteryPower.status}
                      </div>
                    </div>
                    <div className="cf-metric-tile">
                      <div className="cf-metric-label">Power</div>
                      <div className="cf-metric-value">
                        {status.batteryPower.powerMw > 0
                          ? `${(status.batteryPower.powerMw / 1000).toFixed(1)} W`
                          : '--'}
                      </div>
                    </div>
                    <div className="cf-metric-tile">
                      <div className="cf-metric-label">Batt Temp</div>
                      <div className="cf-metric-value">
                        {status.batteryPower.temperatureCelsius !== null
                          ? `${status.batteryPower.temperatureCelsius.toFixed(1)}°C`
                          : '--'}
                      </div>
                    </div>
                  </div>
                )}

                {status.batteryPower.bypassSupported && (
                  <>
                    <FeatureRow
                      Icon={BatteryCharging}
                      title="Bypass Charging (CFJ4)"
                      desc={
                        status.batteryPower.bypassActive
                          ? `Active (${status.batteryPower.bypassReason})`
                          : isId
                            ? 'Alirkan daya langsung ke sistem tanpa memanaskan baterai'
                            : 'Power device directly while plugged in without heating battery'
                      }
                      enabled={status.powerConfig.bypassCharging}
                      disabled={savingKey !== null}
                      onToggle={() => handleTogglePowerFlag('bypass_charging', 'bypassCharging', 'Bypass Charging')}
                    />
                    <FeatureRow
                      Icon={Gamepad2}
                      title={isId ? 'Auto Bypass saat Gaming' : 'Auto Bypass in Gaming'}
                      desc={
                        isId
                          ? 'Aktifkan Bypass Charging otomatis saat game berjalan'
                          : 'Automatically engage bypass charging during Gaming Mode'
                      }
                      enabled={status.powerConfig.bypassOnGaming}
                      disabled={savingKey !== null}
                      onToggle={() => handleTogglePowerFlag('bypass_on_gaming', 'bypassOnGaming', 'Auto Bypass Gaming')}
                    />
                    <div className="space-y-1.5 pt-1">
                      <div className="flex items-center justify-between text-xs">
                        <span className="font-semibold">
                          {isId ? 'Batas Cas Baterai (Charge Ceiling)' : 'Battery Charge Ceiling'}
                        </span>
                        <span className="font-mono text-[11px] opacity-75">
                          {status.powerConfig.chargeLimitPct === 0 ? 'OFF' : `${status.powerConfig.chargeLimitPct}%`}
                        </span>
                      </div>
                      <div className="cf-seg-bar grid-cols-4">
                        {[0, 80, 85, 90].map((pct) => (
                          <button
                            key={pct}
                            type="button"
                            disabled={savingKey !== null}
                            onClick={() => handleChargeLimit(pct)}
                            className={`cf-seg-btn ${status.powerConfig.chargeLimitPct === pct ? 'is-active' : ''}`}
                          >
                            {pct === 0 ? 'Off' : `${pct}%`}
                          </button>
                        ))}
                      </div>
                    </div>
                  </>
                )}

                <FeatureRow
                  Icon={Battery}
                  title={isId ? 'Penjaga Step-Charging Termal' : 'Thermal Step-Charging Guard'}
                  desc={
                    status.batteryPower.thermalStepActive
                      ? 'Stepped down due to battery heat'
                      : isId
                        ? 'Menjaga umur baterai saat fast-charging panas'
                        : 'Protects battery longevity during hot fast-charging'
                  }
                  enabled={status.features.batteryStepCharging}
                  disabled={savingKey !== null}
                  onToggle={() =>
                    handleToggleFeature('enable_battery_step_charging', 'batteryStepCharging', 'Step Charging')
                  }
                />
              </section>
            </div>

            {/* Memory & Storage */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'Memori & Penyimpanan' : 'Memory & Storage'}</div>
              <section className="md3-card-elevated space-y-3 p-4">
                {ramTotGb > 0 && (
                  <div className="space-y-1">
                    <div className="flex justify-between text-xs">
                      <span className="font-medium">RAM</span>
                      <span className="font-mono text-[11px] tabular-nums opacity-80">
                        {ramUsedGb.toFixed(1)} / {ramTotGb.toFixed(1)} GB
                      </span>
                    </div>
                    <div className="md3-progress-track">
                      <div className="md3-progress-bar" style={{ width: `${Math.round(ramFrac * 100)}%`, background: '#38bdf8' }} />
                    </div>
                  </div>
                )}
                {zramTotGb > 0 && (
                  <div className="space-y-1">
                    <div className="flex justify-between text-xs">
                      <span className="font-medium">ZRAM / Swap</span>
                      <span className="font-mono text-[11px] tabular-nums opacity-80">
                        {zramUsedGb.toFixed(1)} / {zramTotGb.toFixed(1)} GB
                      </span>
                    </div>
                    <div className="md3-progress-track">
                      <div className="md3-progress-bar" style={{ width: `${Math.round(zramFrac * 100)}%`, background: '#a78bfa' }} />
                    </div>
                  </div>
                )}
                <div className="grid grid-cols-3 gap-2">
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">CPU PSI</div>
                    <div className="cf-metric-value">
                      {status.cpuPsi !== null ? `${status.cpuPsi.toFixed(1)}%` : '--'}
                    </div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Mem PSI</div>
                    <div className="cf-metric-value">
                      {status.memPsi !== null ? `${status.memPsi.toFixed(1)}%` : '--'}
                    </div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">I/O PSI</div>
                    <div className="cf-metric-value">
                      {status.ioPsi !== null ? `${status.ioPsi.toFixed(1)}%` : '--'}
                    </div>
                  </div>
                </div>
                <div className="grid grid-cols-3 gap-2">
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Mem Health</div>
                    <div className="cf-metric-value">{status.hyperFluid.memoryHealthPct}%</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Compactions</div>
                    <div className="cf-metric-value">{status.hyperFluid.idleCompactions}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Preloads</div>
                    <div className="cf-metric-value">{status.hyperFluid.predictivePreloads}</div>
                  </div>
                </div>
                <div className="grid grid-cols-3 gap-2">
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Frame Pacing</div>
                    <div className="cf-metric-value">
                      {status.hyperFluid.measuredFps >= 12 && status.hyperFluid.jankRatio !== null
                        ? `${Math.round(status.hyperFluid.measuredFps)} FPS · ${Math.round(status.hyperFluid.jankRatio * 100)}%`
                        : status.hyperFluid.framePacingAvailable
                          ? 'Ready'
                          : 'RT Proxy'}
                    </div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">App DNA</div>
                    <div className="cf-metric-value">
                      {status.hyperFluid.appDnaLearned} · {status.hyperFluid.activeAppBias.toFixed(2)}x
                    </div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Thermal Eq</div>
                    <div className="cf-metric-value">
                      {status.batteryPower.bypassActive
                        ? 'Bypass'
                        : status.hyperFluid.chargingDamped
                          ? 'Chg Damped'
                          : 'Optimal'}
                    </div>
                  </div>
                </div>
                <FeatureRow
                  Icon={HardDrive}
                  title={`${status.hardware.storageType} Launch Burst`}
                  desc={`Read-ahead: ${status.actuators.readAheadKb || 'OEM'} KB · I/O isolation`}
                  enabled={status.features.storageBurst}
                  disabled={savingKey !== null}
                  onToggle={() => handleToggleFeature('enable_storage_burst', 'storageBurst', 'Storage Burst')}
                />
                <FeatureRow
                  Icon={MemoryStick}
                  title="MGLRU & Smart Pre-Read"
                  desc={status.hardware.mglruAvailable ? 'Linux MGLRU active · anti-stutter guard' : 'Adaptive VM anti-stutter guard'}
                  enabled={status.features.memoryGuard}
                  disabled={savingKey !== null}
                  onToggle={() => handleToggleFeature('enable_memory_guard', 'memoryGuard', 'Memory Guard')}
                />
                <FeatureRow
                  Icon={MemoryStick}
                  title={isId ? 'Anti-Aging Memory Compaction' : '9-Hour Anti-Aging Compaction'}
                  desc={
                    isId
                      ? 'Defragmentasi RAM otomatis saat layar mati & dingin (< 42°C)'
                      : 'Proactive RAM defragmentation during cool ScreenOff idle (< 42°C)'
                  }
                  enabled={status.features.idleCompaction}
                  disabled={savingKey !== null}
                  onToggle={() => handleToggleFeature('enable_idle_compaction', 'idleCompaction', 'Idle Compaction')}
                />
                <FeatureRow
                  Icon={Sparkles}
                  title="Rust Predictive App Preloader"
                  desc={
                    status.hyperFluid.lastPredictedPackage
                      ? `Last pre-warmed: ${status.hyperFluid.lastPredictedPackage}`
                      : isId
                        ? 'Prediksi Markov (<1 KB RAM) hangatkan APK/SO sebelum dibuka'
                        : 'Markov predictor (<1 KB RAM) warms APK/SO pages before launch'
                  }
                  enabled={status.features.predictivePreload}
                  disabled={savingKey !== null}
                  onToggle={() =>
                    handleToggleFeature('enable_predictive_preload', 'predictivePreload', 'Predictive Preloader')
                  }
                />
                <FeatureRow
                  Icon={Activity}
                  title={isId ? 'Fast-Launch Kamera & Video Chain' : 'Fast-Launch Camera & Video Chain'}
                  desc={
                    isId
                      ? 'Boost Per-TID UClamp untuk cameraserver, Camera HAL & MediaCodec'
                      : 'Per-TID UClamp full-chain boost for cameraserver, Camera HAL & MediaCodec'
                  }
                  enabled={status.features.cameraBoost}
                  disabled={savingKey !== null}
                  onToggle={() => handleToggleFeature('enable_camera_boost', 'cameraBoost', 'Camera Full-Chain')}
                />
              </section>
            </div>
          </div>
        )}

        {/* ========================= TAB 3: LIVE AI FEED ========================= */}
        {activeTab === 'live' && (
          <div className="space-y-4">
            <div className="space-y-2">
              <div className="cf-section-title">
                {isId ? 'Prediktor AI & Konteks Live' : 'Live AI Predictor & Context'}
              </div>
              <section className="md3-card-elevated space-y-3 p-4">
                <div className="flex items-center justify-between gap-2">
                  <div className="flex items-center gap-2 min-w-0">
                    <span
                      className="h-2.5 w-2.5 shrink-0 rounded-full"
                      style={{ background: status.actuators.contendedNodes > 0 ? '#f59e0b' : '#22c55e' }}
                    />
                    <h2 className="truncate text-sm font-bold">
                      {modeLabel} · {PROFILE_META[status.effectiveProfile].label}
                    </h2>
                  </div>
                  <span
                    className={
                      status.actuators.contendedNodes > 0
                        ? 'md3-badge md3-badge-warning'
                        : 'md3-badge md3-badge-success'
                    }
                  >
                    {status.actuators.contendedNodes > 0
                      ? `OEM Yield (${status.actuators.contendedNodes})`
                      : isId
                        ? 'Adaptif'
                        : 'Adaptive'}
                  </span>
                </div>

                <div className="grid grid-cols-2 gap-2 sm:grid-cols-4">
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">{isId ? 'Prediksi 10d' : '10s Forecast'}</div>
                    <div className="cf-metric-value">{Math.round(status.aiPredicted10s)}%</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">{isId ? 'Akurasi AI' : 'Confidence'}</div>
                    <div className="cf-metric-value">{Math.round(status.aiConfidence * 100)}%</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">{isId ? 'Beban CPU' : 'CPU Load'}</div>
                    <div className="cf-metric-value">{Math.round(status.cpuUtilPct)}%</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">{isId ? 'Tren Suhu' : 'Trend'}</div>
                    <div className="cf-metric-value">{trendText}</div>
                  </div>
                </div>
              </section>
            </div>

            <div className="space-y-2">
              <div className="cf-section-title">
                {isId ? `Log Keputusan Terverifikasi (${status.episodes.length})` : `Verified Decision Episodes (${status.episodes.length})`}
              </div>
              {status.episodes.length === 0 ? (
                <div className="md3-card-elevated p-4 text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                  {isId
                    ? 'Mesin memantau secara tenang — belum ada mutasi baru.'
                    : 'Engine observing calmly — no new mutations needed yet.'}
                </div>
              ) : (
                <div className="space-y-2">
                  {status.episodes.map((ep) => {
                    const badgeClass =
                      ep.outcome === 'oem_yield'
                        ? 'md3-badge md3-badge-warning'
                        : ep.outcome === 'rolled_back'
                          ? 'md3-badge md3-badge-error'
                          : 'md3-badge md3-badge-success';
                    return (
                      <div key={ep.id} className="md3-card-elevated space-y-1.5 p-3.5">
                        <div className="flex items-center justify-between gap-2">
                          <div className="flex items-center gap-1.5 min-w-0">
                            <span className="md3-badge md3-badge-primary uppercase">{ep.domain}</span>
                            <span className="truncate text-xs font-bold">{ep.trigger}</span>
                          </div>
                          <span className={badgeClass}>
                            {ep.outcome === 'oem_yield'
                              ? 'OEM Yield'
                              : isId
                                ? 'Terverifikasi'
                                : 'Verified'}
                          </span>
                        </div>
                        <p className="text-xs leading-snug">{ep.action}</p>
                        <div
                          className="flex items-center justify-between font-mono text-[10px]"
                          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                        >
                          <span>{formatClock(ep.tsMs)}</span>
                          <span>
                            {ep.socTempC !== null ? `${ep.socTempC.toFixed(1)}°C · ` : ''}
                            RP {Math.round(ep.renderPressure * 100)}% · Scale {Math.round(ep.capacityScale * 100)}%
                          </span>
                        </div>
                      </div>
                    );
                  })}
                </div>
              )}
            </div>
          </div>
        )}

        {/* ========================= TAB 4: APPS ========================= */}
        {activeTab === 'apps' && (
          <div className="space-y-4">
            <section className="md3-card-elevated space-y-3 p-4">
              <div className="flex items-center justify-between gap-2">
                <h2 className="text-sm font-bold">
                  {isId ? `Aturan Aplikasi (${installedApps.length})` : `Installed Apps (${installedApps.length})`}
                </h2>
                <button
                  type="button"
                  disabled={loadingApps}
                  className="md3-btn-tonal"
                  onClick={() => void loadInstalledApps()}
                >
                  <RefreshCw className={`h-3.5 w-3.5 ${loadingApps ? 'animate-spin' : ''}`} />
                  <span>{loadingApps ? '...' : isId ? 'Muat Ulang' : 'Refresh'}</span>
                </button>
              </div>

              <div className="relative">
                <Search
                  className="pointer-events-none absolute left-3 top-1/2 h-3.5 w-3.5 -translate-y-1/2"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                />
                <input
                  type="text"
                  value={appSearch}
                  onChange={(e) => setAppSearch(e.target.value)}
                  placeholder={isId ? 'Cari nama paket...' : 'Search package name...'}
                  className="w-full rounded-xl py-2 pl-9 pr-3 font-mono text-xs"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                />
              </div>

              <div className="cf-seg-bar grid-cols-2">
                <button
                  type="button"
                  onClick={() => setAppFilter('all')}
                  className={`cf-seg-btn ${appFilter === 'all' ? 'is-active' : ''}`}
                >
                  {isId ? `Semua (${installedApps.length})` : `All (${installedApps.length})`}
                </button>
                <button
                  type="button"
                  onClick={() => setAppFilter('rules')}
                  className={`cf-seg-btn ${appFilter === 'rules' ? 'is-active' : ''}`}
                >
                  {isId ? `Diatur (${totalOverrides})` : `Rules (${totalOverrides})`}
                </button>
              </div>

              <div className="max-h-96 space-y-2 overflow-y-auto pr-0.5">
                {filteredInstalledApps.length === 0 ? (
                  <div className="py-4 text-center text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    {loadingApps ? '...' : isId ? 'Tidak ada paket yang cocok.' : 'No matching packages.'}
                  </div>
                ) : (
                  filteredInstalledApps.slice(0, 60).map((pkg) => {
                    const currentCat = getPackageRuleCategory(status.apps, pkg);
                    return (
                      <div
                        key={pkg}
                        className="space-y-2 rounded-xl p-3"
                        style={{
                          background: 'var(--md-sys-color-surface-container)',
                          border: '1px solid var(--md-sys-color-outline-variant)',
                        }}
                      >
                        <div className="flex items-center justify-between gap-2">
                          <span className="truncate font-mono text-xs font-semibold">{pkg}</span>
                          <span
                            className={
                              currentCat === 'default' ? 'md3-badge md3-badge-neutral' : 'md3-badge md3-badge-primary'
                            }
                          >
                            {RULE_CATEGORY_OPTIONS.find((o) => o.id === currentCat)?.label ?? 'Auto'}
                          </span>
                        </div>
                        <div className="grid grid-cols-5 gap-1">
                          {RULE_CATEGORY_OPTIONS.map((opt) => {
                            const active = currentCat === opt.id;
                            return (
                              <button
                                key={opt.id}
                                type="button"
                                disabled={savingKey !== null}
                                onClick={() => handleAssignAppRule(pkg, opt.id)}
                                className="rounded-lg py-1.5 text-[10.5px] font-semibold truncate"
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

            {/* Manual Package Rule Entry */}
            <section className="md3-card-elevated space-y-3 p-4">
              <h2 className="text-sm font-bold">{isId ? 'Tambah Paket Manual' : 'Manual Package Entry'}</h2>
              <div className="flex flex-wrap gap-2">
                <input
                  type="text"
                  value={manualPkg}
                  onChange={(e) => setManualPkg(e.target.value)}
                  placeholder="com.example.package"
                  className="min-w-[160px] flex-1 rounded-xl px-3 py-2 font-mono text-xs"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                />
                <select
                  value={manualCategory}
                  onChange={(e) => setManualCategory(e.target.value as Exclude<AppRuleCategory, 'default'>)}
                  className="rounded-xl px-3 py-2 text-xs font-semibold"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                >
                  <option value="gaming">Gaming</option>
                  <option value="smoothness">Smooth</option>
                  <option value="battery">Battery</option>
                  <option value="excluded">Stock</option>
                </select>
                <button
                  type="button"
                  className="md3-btn-filled"
                  onClick={() => {
                    handleAssignAppRule(manualPkg, manualCategory);
                    setManualPkg('');
                  }}
                >
                  <Plus className="h-3.5 w-3.5" />
                  <span>{isId ? 'Tambah' : 'Add'}</span>
                </button>
              </div>

              {totalOverrides > 0 && (
                <div className="flex flex-wrap gap-1.5 pt-1">
                  {(['gaming', 'smoothness', 'battery', 'excluded'] as const).flatMap((bucket) =>
                    status.apps[bucket].map((pkg) => (
                      <span key={`${bucket}-${pkg}`} className="md3-badge md3-badge-neutral gap-1.5 font-mono">
                        <span>{pkg}</span>
                        <span className="opacity-70">({bucket})</span>
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
              )}
            </section>
          </div>
        )}

        {/* ========================= TAB 5: SYSTEM ========================= */}
        {activeTab === 'system' && (
          <div className="space-y-4">
            {/* Appearance & Accent Palette */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'Tampilan & Bahasa' : 'Appearance & Language'}</div>
              <section className="md3-card-elevated space-y-3 p-4">
                <div className="flex items-center justify-between gap-2">
                  <span className="text-xs font-semibold">{isId ? 'Palet Aksen Material You' : 'Material You Accent'}</span>
                  <div className="flex items-center gap-2">
                    {ACCENT_SEEDS.map((opt) => (
                      <button
                        key={opt.id}
                        type="button"
                        onClick={() => handleSelectAccent(opt.id)}
                        className="h-6 w-6 rounded-full"
                        aria-label={opt.label}
                        style={{
                          background: opt.swatch,
                          boxShadow:
                            appearance.accent === opt.id
                              ? '0 0 0 2px var(--md-sys-color-surface), 0 0 0 4px var(--md-sys-color-primary)'
                              : 'none',
                        }}
                      />
                    ))}
                  </div>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <button type="button" onClick={toggleTheme} className="md3-btn-tonal w-full">
                    {isDark ? (isId ? 'Mode Terang' : 'Light Mode') : isId ? 'Mode Gelap' : 'Dark Mode'}
                  </button>
                  <button type="button" onClick={toggleLang} className="md3-btn-tonal w-full">
                    {isId ? 'Switch to English' : 'Ganti ke Indonesia'}
                  </button>
                </div>
              </section>
            </div>

            {/* Hardware Atlas */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'Atlas Perangkat Keras' : 'Hardware Atlas'}</div>
              <section className="md3-card-elevated space-y-3 p-4">
                <div className="grid grid-cols-2 gap-2 sm:grid-cols-3">
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">SoC</div>
                    <div className="cf-metric-value">{status.hardware.socModel}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Storage</div>
                    <div className="cf-metric-value">{status.hardware.storageType}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">Display</div>
                    <div className="cf-metric-value">{status.hardware.refreshHz} Hz</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">RAM / ZRAM</div>
                    <div className="cf-metric-value">
                      {formatBytes(status.hardware.ramBytes)} / {formatBytes(status.hardware.zramBytes)}
                    </div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">CPU Clusters</div>
                    <div className="cf-metric-value">{status.hardware.cpuClusters}</div>
                  </div>
                  <div className="cf-metric-tile">
                    <div className="cf-metric-label">MGLRU / UClamp</div>
                    <div className="cf-metric-value">
                      {status.hardware.mglruAvailable ? 'ON' : 'OFF'} ·{' '}
                      {status.actuators.perThreadUclampSupported ? 'TID' : 'OFF'}
                    </div>
                  </div>
                </div>

                {status.actuators.semanticKnobsDiscovered > 0 && (
                  <div className="rounded-xl p-3 text-xs space-y-2" style={{ background: 'var(--md-sys-color-surface-container)' }}>
                    <div className="flex items-center justify-between font-bold">
                      <span>
                        Co-Pilot Knobs ({status.actuators.semanticKnobsActive}/{status.actuators.semanticKnobsDiscovered})
                      </span>
                      <span className="font-mono text-[10.5px] opacity-80">
                        OEM Yield: {status.actuators.oemYieldedNodes}
                      </span>
                    </div>
                    {status.actuators.semanticKnobLeaves.length > 0 && (
                      <div className="flex flex-wrap gap-1.5">
                        {status.actuators.semanticKnobLeaves.slice(0, 12).map((leaf) => (
                          <span
                            key={leaf}
                            className="rounded-md px-2 py-0.5 font-mono text-[10px]"
                            style={{ background: 'var(--md-sys-color-secondary-container)' }}
                          >
                            {leaf}
                          </span>
                        ))}
                      </div>
                    )}
                  </div>
                )}

                {status.actuators.oemYieldedPaths.length > 0 && (
                  <div
                    className="rounded-xl p-3 text-xs space-y-1"
                    style={{ background: 'var(--md-sys-color-surface-container)' }}
                  >
                    <div className="font-bold">
                      {isId
                        ? `Kolaborasi OEM (${status.actuators.oemYieldedPaths.length} node diserahkan)`
                        : `OEM Collaboration (${status.actuators.oemYieldedPaths.length} nodes yielded)`}
                    </div>
                    <ul className="list-disc pl-4 font-mono text-[10.5px] opacity-85">
                      {status.actuators.oemYieldedPaths.map((p) => (
                        <li key={p} className="truncate">{p}</li>
                      ))}
                    </ul>
                  </div>
                )}

                {status.actuators.contendedPaths.length > 0 && (
                  <div
                    className="rounded-xl p-3 text-xs space-y-1"
                    style={{
                      background: 'var(--md-sys-color-warning-container)',
                      color: 'var(--md-sys-color-on-warning-container)',
                    }}
                  >
                    <div className="flex items-center gap-1.5 font-bold">
                      <ShieldAlert className="h-3.5 w-3.5" />
                      <span>OEM Yield ({status.actuators.contendedPaths.length} nodes)</span>
                    </div>
                    <ul className="list-disc pl-4 font-mono text-[10.5px]">
                      {status.actuators.contendedPaths.map((p) => (
                        <li key={p} className="truncate">{p}</li>
                      ))}
                    </ul>
                  </div>
                )}
              </section>
            </div>

            {/* CFJ4 Recovery & Diagnostics */}
            <BaselineGuardianCard
              nodesSnapshotted={status.baselineNodesSnapshotted}
              nativeMode={isNativeBridge}
              baselineStatus={status.baselineNodesSnapshotted > 0 ? 'protected' : 'restored'}
            />

            <section className="md3-card-elevated space-y-3 p-4">
              <h2 className="text-sm font-bold">
                {isId ? 'Diagnostik & Pemulihan CFJ4' : 'CFJ4 Diagnostics & Recovery'}
              </h2>
              <div className="grid grid-cols-2 gap-2 sm:grid-cols-3">
                <button type="button" className="md3-btn-tonal w-full" onClick={() => void handleViewJournal()}>
                  <FileJson className="h-3.5 w-3.5" />
                  <span>CFJ4</span>
                </button>
                <button type="button" className="md3-btn-tonal w-full" onClick={() => void handleExportDiag()}>
                  <ClipboardCopy className="h-3.5 w-3.5" />
                  <span>Diag</span>
                </button>
                {status.running ? (
                  <button
                    type="button"
                    disabled={restoring}
                    className="md3-btn-filled col-span-2 sm:col-span-1 w-full"
                    onClick={() => void handleRestoreBaseline()}
                  >
                    <RotateCcw className="h-3.5 w-3.5" />
                    <span>{restoring ? '...' : isId ? 'Pulihkan OEM' : 'Restore OEM'}</span>
                  </button>
                ) : (
                  <button
                    type="button"
                    disabled={starting}
                    className="md3-btn-filled col-span-2 sm:col-span-1 w-full"
                    onClick={() => void handleStartDaemon()}
                  >
                    <Play className="h-3.5 w-3.5" />
                    <span>{starting ? '...' : isId ? 'Jalankan' : 'Start'}</span>
                  </button>
                )}
              </div>
              {journalJson && (
                <pre
                  className="max-h-48 overflow-auto rounded-xl p-3 font-mono text-[10.5px]"
                  style={{ background: 'var(--md-sys-color-surface-container)' }}
                >
                  {journalJson}
                </pre>
              )}
              {diagJson && (
                <pre
                  className="max-h-48 overflow-auto rounded-xl p-3 font-mono text-[10.5px]"
                  style={{ background: 'var(--md-sys-color-surface-container)' }}
                >
                  {diagJson}
                </pre>
              )}
            </section>

            {/* Creator Identity & AI Sponsors */}
            <div className="space-y-2">
              <div className="cf-section-title">{isId ? 'Pembuat & Partner AI' : 'Creator & AI Partners'}</div>
              <section className="md3-card-elevated space-y-3 p-4">
                <a
                  href="https://github.com/mystivara-creator"
                  target="_blank"
                  rel="noreferrer"
                  className="flex items-center justify-between gap-3 rounded-xl p-3"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                  }}
                >
                  <div className="flex items-center gap-3 min-w-0">
                    <div
                      className="flex h-10 w-10 shrink-0 items-center justify-center rounded-full font-bold text-sm"
                      style={{
                        background: 'var(--md-sys-color-primary-container)',
                        color: 'var(--md-sys-color-on-primary-container)',
                      }}
                    >
                      MC
                    </div>
                    <div className="min-w-0">
                      <div className="text-sm font-bold truncate">Mystivara Creator</div>
                      <div
                        className="text-[11px] truncate"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        @mystivara-creator · CoreFlow Engine
                      </div>
                    </div>
                  </div>
                  <ExternalLink className="h-4 w-4 shrink-0 opacity-70" />
                </a>

                <div className="grid grid-cols-2 gap-2">
                  {AI_SPONSORS.map((sp) => (
                    <a
                      key={sp.name}
                      href={sp.url}
                      target="_blank"
                      rel="noreferrer"
                      className="flex flex-col justify-between rounded-xl p-3"
                      style={{
                        background: 'var(--md-sys-color-surface-container)',
                        border: '1px solid var(--md-sys-color-outline-variant)',
                      }}
                    >
                      <div className="flex items-center gap-2">
                        <span
                          className="flex h-6 w-6 shrink-0 items-center justify-center rounded-lg text-[10px] font-bold"
                          style={{ background: `${sp.color}28`, color: sp.color }}
                        >
                          {sp.badge}
                        </span>
                        <span className="text-xs font-bold truncate">{sp.name}</span>
                      </div>
                      <span
                        className="mt-1 text-[10.5px] leading-tight truncate"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        {sp.role}
                      </span>
                    </a>
                  ))}
                </div>

                <p
                  className="rounded-xl p-3 text-[11px] leading-relaxed"
                  style={{
                    background: 'var(--md-sys-color-surface-container)',
                    color: 'var(--md-sys-color-on-surface-variant)',
                  }}
                >
                  <strong style={{ color: 'var(--md-sys-color-on-surface)' }}>
                    CoreFlow Community Source-Available License v1.0
                  </strong>{' '}
                  —{' '}
                  {isId
                    ? 'Gratis untuk penggunaan pribadi, belajar, dan audit keamanan. Dibuat oleh Mystivara Creator berkolaborasi secara transparan bersama AI (Claude, ChatGPT, Gemini). Dilarang rebrand atau memperjualbelikan tanpa izin.'
                    : 'Free for personal use, learning, and security auditing. Authored by Mystivara Creator with transparent AI collaboration (Claude, ChatGPT, Gemini). No rebranding or commercial resale without permission.'}
                </p>
              </section>
            </div>

            <LiveReactionStream logs={logs} nativeMode={isNativeBridge} />
          </div>
        )}
      </main>

      <footer
        className="mt-4 py-3 text-center text-[11px]"
        style={{
          borderTop: '1px solid var(--md-sys-color-outline-variant)',
          color: 'var(--md-sys-color-on-surface-variant)',
        }}
      >
        CoreFlow Engine v{status.version} NextGen · Mystivara Creator · CFJ4 OEM Safe
      </footer>
    </div>
  );
};

const FeatureRow: React.FC<{
  Icon: React.FC<{ className?: string }>;
  title: string;
  desc: string;
  enabled: boolean;
  disabled: boolean;
  onToggle: () => void;
}> = ({ Icon, title, desc, enabled, disabled, onToggle }) => (
  <div
    className="flex items-center justify-between gap-3 pt-2.5"
    style={{ borderTop: '1px solid var(--md-sys-color-outline-variant)' }}
  >
    <div className="flex items-start gap-2.5 min-w-0">
      <Icon className="mt-0.5 h-4 w-4 shrink-0 opacity-85" />
      <div className="min-w-0">
        <div className="text-xs font-semibold leading-snug">{title}</div>
        <p
          className="text-[11px] leading-snug"
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
      disabled={disabled}
      onClick={onToggle}
      className={`${enabled ? 'md3-btn-filled' : 'md3-btn-tonal'} shrink-0 min-w-[3.2rem]`}
    >
      {enabled ? 'ON' : 'OFF'}
    </button>
  </div>
);

export default App;
