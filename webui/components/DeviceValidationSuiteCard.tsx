import React, { useCallback, useEffect, useState } from 'react';
import {
  Activity,
  CheckCircle2,
  LockKeyhole,
  RefreshCw,
  ShieldAlert,
} from 'lucide-react';
import { execNativeKsuShell, hasNativeKernelSuBridge } from '../bridge/daemonBridge';

type SessionState = 'idle' | 'active' | 'unavailable';

const RESOLVE_BIN =
  'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || exit 127';

const VALIDATION_DOMAINS = [
  {
    code: 'DVS-01',
    title: 'Boot stability',
    detail: 'Daemon readiness, initial hardware discovery, boot_epoch tracking, and recovery state',
  },
  {
    code: 'DVS-02',
    title: 'Idle equilibrium',
    detail: 'Baseline stability, drift detection, and release of owned mutations after sustained quiet windows',
  },
  {
    code: 'DVS-03',
    title: 'Interactive workload',
    detail: 'Foreground process membership, CPU pressure, and touch/frame response pacing',
  },
  {
    code: 'DVS-04',
    title: 'Sustained workload',
    detail: 'Long-running workload admission, OPP ceiling verification, and thermal response',
  },
  {
    code: 'DVS-05',
    title: 'Multitasking',
    detail: 'Concurrent memory pressure, block I/O queue contention, and zram swap activity',
  },
  {
    code: 'DVS-06',
    title: 'Thermal recovery',
    detail: 'Cooling velocity trend, OEM thermal trip points, and post-load hysteresis stabilization',
  },
  {
    code: 'DVS-07',
    title: 'Ownership conflict',
    detail: 'External OEM writer detection, quarantine cooldown, and safe mutation relinquishment',
  },
  {
    code: 'DVS-08',
    title: 'Journal recovery',
    detail: 'CFJ4 write-ahead journal integrity, SHA-256 manifest verification, and idempotent rollback',
  },
];

export const DeviceValidationSuiteCard: React.FC<{ nativeMode: boolean }> = ({
  nativeMode,
}) => {
  const [sessionState, setSessionState] = useState<SessionState>('unavailable');
  const [sessionId, setSessionId] = useState<string | null>(null);
  const [message, setMessage] = useState('Waiting for native validation status.');
  const [lastChecked, setLastChecked] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [isRefreshing, setIsRefreshing] = useState(false);

  const refresh = useCallback(async () => {
    if (!nativeMode || !hasNativeKernelSuBridge()) {
      setSessionState('unavailable');
      setSessionId(null);
      setMessage('Native validation status is unavailable in browser simulation preview.');
      return;
    }

    setIsRefreshing(true);
    try {
      const result = await execNativeKsuShell(`${RESOLVE_BIN}; "$BIN" --dvs-status`);
      if (result.errno !== 0) {
        throw new Error(result.stderr || result.stdout || 'Validation status query failed');
      }
      const response = result.stdout.trim();
      const active = response.match(/^OK ACTIVE ([0-9a-f-]{36})$/i);
      if (active) {
        setSessionState('active');
        setSessionId(active[1]);
        setMessage('A native validation capture is active. Its lifecycle remains daemon-owned.');
      } else if (response === 'OK INACTIVE') {
        setSessionState('idle');
        setSessionId(null);
        setMessage('No validation capture is currently active.');
      } else {
        throw new Error(response || 'Unexpected CF-DVS status response');
      }
      setError(null);
      setLastChecked(new Date().toLocaleTimeString());
    } catch (reason) {
      setSessionState('unavailable');
      setSessionId(null);
      setError(reason instanceof Error ? reason.message : String(reason));
      setMessage('Unable to read the native validation session state.');
    } finally {
      setIsRefreshing(false);
    }
  }, [nativeMode]);

  useEffect(() => {
    let disposed = false;
    const check = async () => {
      if (disposed) return;
      await refresh();
    };
    void check();
    const timer = window.setInterval(() => void check(), 15000);
    return () => {
      disposed = true;
      window.clearInterval(timer);
    };
  }, [refresh]);

  const statusLabel =
    sessionState === 'active'
      ? 'Capture Active'
      : sessionState === 'idle'
        ? 'Idle · Ready'
        : 'Unavailable';

  const statusBadge =
    sessionState === 'active'
      ? 'md3-badge md3-badge-success'
      : sessionState === 'idle'
        ? 'md3-badge md3-badge-primary'
        : 'md3-badge md3-badge-warning';

  return (
    <div className="md3-card-elevated space-y-6 p-6 sm:p-8">
      <div
        className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5"
        style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
      >
        <div className="flex items-start gap-3.5">
          <div
            className="flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl"
            style={{
              backgroundColor: 'var(--md-sys-color-primary-container)',
              color: 'var(--md-sys-color-on-primary-container)',
            }}
          >
            <Activity className="h-5 w-5" />
          </div>
          <div className="min-w-0 flex-1">
            <div className="flex flex-wrap items-center gap-2">
              <h2 className="text-base sm:text-lg font-bold tracking-tight">
                CoreFlow Device Validation Suite (CF-DVS)
              </h2>
              <span className="md3-badge md3-badge-neutral">READ ONLY · SCHEMA 1.1</span>
            </div>
            <p
              className="mt-1 text-xs sm:text-sm leading-relaxed"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              CoreFlow validates its own runtime invariants and exports SHA-256 manifest bundles with pinned CDDP device profiles. This surface explains coverage and reports native capture state without starting or stopping test sessions.
            </p>
          </div>
        </div>

        {nativeMode && (
          <button
            type="button"
            id="btn-refresh-dvs-status"
            disabled={isRefreshing}
            onClick={() => void refresh()}
            className="md3-btn-outlined shrink-0 self-start"
          >
            <RefreshCw className={'h-3.5 w-3.5 ' + (isRefreshing ? 'animate-spin' : '')} />
            <span>Check Status</span>
          </button>
        )}
      </div>

      <section className="md3-card-filled flex flex-col gap-3 p-4 sm:p-5 sm:flex-row sm:items-center sm:justify-between">
        <div className="space-y-1.5">
          <div
            className="text-[11px] font-bold uppercase tracking-wider"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Native CF-DVS Session State
          </div>
          <div className="flex flex-wrap items-center gap-2">
            <span className={statusBadge}>{statusLabel}</span>
            {sessionId && (
              <span className="font-mono text-xs font-semibold break-all">
                {sessionId}
              </span>
            )}
          </div>
          <p
            className="break-all text-xs"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            {message}
          </p>
          {lastChecked && (
            <p
              className="text-[11px] font-mono"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Last checked {lastChecked} · auto-refresh every 15s
            </p>
          )}
        </div>
        <div
          className="flex shrink-0 items-center gap-2 text-xs font-semibold"
          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
        >
          <LockKeyhole className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
          <span>Daemon-owned lifecycle</span>
        </div>
      </section>

      {error && (
        <div
          role="status"
          className="flex items-start gap-2.5 rounded-2xl p-3.5 text-xs"
          style={{
            backgroundColor: 'var(--md-sys-color-warning-container)',
            color: 'var(--md-sys-color-on-warning-container)',
          }}
        >
          <ShieldAlert className="mt-0.5 h-4 w-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      <section className="space-y-3">
        <div>
          <h3 className="text-sm font-bold">8-Domain Evidence &amp; Invariant Matrix</h3>
          <p
            className="mt-0.5 text-xs"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Evidence domains evaluated against the offline Python contract validator (<code className="font-mono">cf_dvs_validate.py</code>).
          </p>
        </div>
        <div className="grid grid-cols-1 gap-2.5 sm:grid-cols-2">
          {VALIDATION_DOMAINS.map((domain) => (
            <div
              key={domain.title}
              className="md3-card-outlined flex items-start gap-3 p-3.5"
            >
              <CheckCircle2
                className="mt-0.5 h-4 w-4 shrink-0"
                style={{ color: 'var(--md-sys-color-success)' }}
              />
              <div className="min-w-0">
                <div className="flex items-center gap-2">
                  <span className="font-mono text-[10px] font-bold" style={{ color: 'var(--md-sys-color-primary)' }}>
                    {domain.code}
                  </span>
                  <span className="text-xs font-bold">{domain.title}</span>
                </div>
                <p
                  className="mt-1 text-[11px] leading-relaxed"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  {domain.detail}
                </p>
              </div>
            </div>
          ))}
        </div>
      </section>

      <p
        className="pt-4 text-[11px] leading-relaxed"
        style={{
          borderTop: '1px solid var(--md-sys-color-outline-variant)',
          color: 'var(--md-sys-color-on-surface-variant)',
        }}
      >
        Diagnostic capture is not an optimization command. Zero-sample sessions fail closed (<code className="font-mono">RECOVERY_REQUIRED</code>) and never fabricate passing device acceptance.
      </p>
    </div>
  );
};
